#include "dm.h"

#ifdef DM_DIRECTX12

#include "../platform/dm_platform_win32.h"

#include <windows.h>

#define COBJMACROS
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#undef COBJMACROS

#define DM_DX12_MAX_FRAMES 3

typedef struct dm_dx12_cpu_heap_handle_t
{
    D3D12_CPU_DESCRIPTOR_HANDLE begin;
    D3D12_CPU_DESCRIPTOR_HANDLE current;
} dm_dx12_cpu_heap_handle;

typedef struct dm_dx12_gpu_heap_handle_t
{
    D3D12_GPU_DESCRIPTOR_HANDLE begin;
    D3D12_GPU_DESCRIPTOR_HANDLE current;
} dm_dx12_gpu_heap_handle;

typedef struct dm_dx12_descriptor_heap_t
{
    ID3D12DescriptorHeap* heap;
    size_t                size;
    
    dm_dx12_cpu_heap_handle cpu_handle;
    dm_dx12_gpu_heap_handle gpu_handle;
} dm_dx12_descriptor_heap;

typedef struct dm_dx12_fence_t
{
    ID3D12Fence* fence;
    uint64_t     value;
} dm_dx12_fence;

typedef struct dm_dx12_renderer_t
{
    ID3D12Device5*   device;
    IDXGISwapChain4* swap_chain;

    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_DX12_MAX_FRAMES];
    ID3D12GraphicsCommandList7* command_list;

    dm_dx12_fence fences[DM_DX12_MAX_FRAMES];
    HANDLE        fence_event;

    ID3D12Resource*         render_targets[DM_DX12_MAX_FRAMES];
    dm_dx12_descriptor_heap rtv_heap;
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
#endif

    uint8_t current_frame;
} dm_dx12_renderer;

#define DM_DX12_GET_RENDERER dm_dx12_renderer* dx12_renderer = renderer->internal_renderer

DM_INLINE
bool dm_dx12_wait_for_previous_frame(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;


    dm_dx12_fence* fence = &dx12_renderer->fences[dx12_renderer->current_frame];

    if(ID3D12Fence_GetCompletedValue(fence->fence) < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, dx12_renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Fence_SetEventOnCompletion failed");
            return false;
        }

        WaitForSingleObject(dx12_renderer->fence_event, INFINITE);
    }

    dx12_renderer->fences[dx12_renderer->current_frame].value++;

    return true;
}

/*******************
 * RENDERER BACKEND
 * ******************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_INFO("Initializing DirectX12 backend...");

    context->renderer.internal_renderer = dm_alloc(sizeof(dm_dx12_renderer));

    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    void* temp = NULL;

    dm_internal_w32_data* w32_data = context->platform_data.internal_data;

    // create device
    {
#ifdef DM_DEBUG
        hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("D3D12GetDebugInterface failed");
            return false;
        }
        dx12_renderer->debug = temp;
        temp = NULL;

        ID3D12Debug_EnableDebugLayer(dx12_renderer->debug);
#endif

        hr = CreateDXGIFactory1(&IID_IDXGIFactory4, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("CreateDXGIFactory1 failed");
            return false;
        }
        dx12_renderer->factory = temp;
        temp = NULL;

        int adapter_index = 0;
        bool found = false;

        DXGI_ADAPTER_DESC1 adapter_desc = { 0 };
        while(IDXGIFactory4_EnumAdapters1(dx12_renderer->factory, adapter_index, &dx12_renderer->adapter) != DXGI_ERROR_NOT_FOUND)
        {
            IDXGIAdapter1_GetDesc1(dx12_renderer->adapter, &adapter_desc);

            if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter_index++;
                continue;
            }

            hr = D3D12CreateDevice((IUnknown*)dx12_renderer->adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL);
            if(SUCCEEDED(hr))
            {
                found = true;
                break;
            }

            adapter_index++;
        }

        if(!found)
        {
            DM_LOG_FATAL("No suitable DirectX12 adapter found");
            return false;
        }

        hr = D3D12CreateDevice((IUnknown*)dx12_renderer->adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device5, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("D3D12CreateDevice failed");
            return false;
        }

        DM_LOG_INFO("Device: %ls", adapter_desc.Description);

        dx12_renderer->device = temp;
        temp = NULL;

    }

    // command queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = { 0 };
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        hr = ID3D12Device5_CreateCommandQueue(dx12_renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandQueue failed");
            return false;
        }
        dx12_renderer->command_queue = temp;
        temp = NULL;
    }

    // swap chain
    {
        DXGI_SAMPLE_DESC sample_desc = { 0 };
        sample_desc.Count = 1;

        DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
        swap_desc.BufferCount  = DM_DX12_MAX_FRAMES;
        swap_desc.Width        = context->renderer.width;
        swap_desc.Height       = context->renderer.height;
        swap_desc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_desc.SampleDesc   = sample_desc;
        swap_desc.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
        
        HWND hwnd = w32_data->hwnd;

        hr = IDXGIFactory4_CreateSwapChainForHwnd(dx12_renderer->factory, (IUnknown*)dx12_renderer->command_queue, hwnd, &swap_desc, NULL,NULL, (IDXGISwapChain1**)&dx12_renderer->swap_chain);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("IDXGIFactory4_CreateSwapChainForHwnd failed");
            return false;
        }

        IDXGIFactory4_MakeWindowAssociation(dx12_renderer->factory, hwnd, DXGI_MWA_NO_ALT_ENTER); 

        dx12_renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(dx12_renderer->swap_chain);
    }

    // command allocators
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES; i++)
        {
            hr = ID3D12Device5_CreateCommandAllocator(dx12_renderer->device, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
                return false;
            }
            dx12_renderer->command_allocator[i] = temp;
            temp = NULL;
        }
    }

    // command list
    {
        hr = ID3D12Device5_CreateCommandList(dx12_renderer->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, dx12_renderer->command_allocator[0], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandList failed");
            return false;
        }
        dx12_renderer->command_list = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(dx12_renderer->command_list);
    }

    // rtv heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = DM_DX12_MAX_FRAMES;

        hr = ID3D12Device5_CreateDescriptorHeap(dx12_renderer->device, &desc, &IID_ID3D12DescriptorHeap, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device5_CreateDescriptorHeap failed");
            return false;
        }
        dx12_renderer->rtv_heap.heap = temp;
        temp = NULL;

        dx12_renderer->rtv_heap.size = ID3D12Device5_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_heap.heap, &dx12_renderer->rtv_heap.cpu_handle.begin);
        dx12_renderer->rtv_heap.cpu_handle.current = dx12_renderer->rtv_heap.cpu_handle.begin;

        //ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(dx12_renderer->rtv_heap.heap, &dx12_renderer->rtv_heap.gpu_handle.begin);
        //dx12_renderer->rtv_heap.gpu_handle.current = dx12_renderer->rtv_heap.gpu_handle.begin;
    }

    // render targets
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES; i++)
        {
            hr = IDXGISwapChain4_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("IDXGISwapChain4_GetBuffer failed");
                return false;
            }
            dx12_renderer->render_targets[i] = temp;
            temp = NULL;

            ID3D12Device5_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->render_targets[i], NULL, dx12_renderer->rtv_heap.cpu_handle.current);

            dx12_renderer->rtv_heap.cpu_handle.current.ptr += dx12_renderer->rtv_heap.size;
        }
    }

    // fence stuff
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES; i++)
        {
            hr = ID3D12Device5_CreateFence(dx12_renderer->device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateFence failed");
                return false;
            }
            dx12_renderer->fences[i].fence = temp;
            temp = NULL;
        }

        dx12_renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!dx12_renderer->fence_event)
        {
            DM_LOG_FATAL("CreateEvent failed");
            return false;
        }
    }

    return true;
}

bool dm_renderer_backend_finish_init(dm_context* context)
{
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES; i++)
    {
        dx12_renderer->current_frame = i;
        if(!dm_dx12_wait_for_previous_frame(dx12_renderer))
        {
            DM_LOG_ERROR("Waiting for previous frame failed");
            continue;
        }
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES; i++)
    {
        ID3D12Resource_Release(dx12_renderer->render_targets[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocator[i]);
        ID3D12Fence_Release(dx12_renderer->fences[i].fence);
    }

    CloseHandle(dx12_renderer->fence_event);

    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_heap.heap);
    IDXGISwapChain4_Release(dx12_renderer->swap_chain);
    ID3D12GraphicsCommandList_Release(dx12_renderer->command_list);
    ID3D12CommandQueue_Release(dx12_renderer->command_queue);

#ifdef DM_DEBUG
    IDXGIDebug1* dbg = NULL;
    void* temp = NULL;
    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_ERROR("ID3D12Debug_QueryInterface failed");
        ID3D12Debug_Release(dx12_renderer->debug);
    }
    else
    {
        dbg = temp;
        temp = NULL;
        ID3D12Debug_Release(dx12_renderer->debug);
        IDXGIDebug1_ReportLiveObjects(dbg, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        IDXGIDebug1_Release(dbg);
    }
#endif

    ID3D12Device5_Release(dx12_renderer->device);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    if(!dm_dx12_wait_for_previous_frame(dx12_renderer))
    {
        DM_LOG_FATAL("Waiting for previous frame failed");
        return false;
    }

    const uint8_t frame_index = dx12_renderer->current_frame;

    hr = ID3D12CommandAllocator_Reset(dx12_renderer->command_allocator[frame_index]);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandAllocator_Reset failed");
        return false;
    }

    hr = ID3D12GraphicsCommandList7_Reset(dx12_renderer->command_list, dx12_renderer->command_allocator[frame_index], NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Reset failed");
        return false;
    }

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->render_targets[frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ID3D12GraphicsCommandList7_ResourceBarrier(dx12_renderer->command_list, 1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx12_renderer->rtv_heap.cpu_handle.begin;
    rtv_handle.ptr += frame_index * dx12_renderer->rtv_heap.size;

    float clear_color[] = { 0.2f,0.5f,0.7f,1.f };

    ID3D12GraphicsCommandList7_OMSetRenderTargets(dx12_renderer->command_list, 1, &rtv_handle, FALSE, NULL);
    ID3D12GraphicsCommandList7_ClearRenderTargetView(dx12_renderer->command_list, rtv_handle, clear_color, 0, NULL);

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    const uint8_t frame_index = dx12_renderer->current_frame;

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->render_targets[frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    ID3D12GraphicsCommandList7_ResourceBarrier(dx12_renderer->command_list, 1, &barrier);

    hr = ID3D12GraphicsCommandList7_Close(dx12_renderer->command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Close failed");
        return false;
    }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)dx12_renderer->command_list };

    ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, _countof(command_lists), command_lists);

    const dm_dx12_fence fence = dx12_renderer->fences[frame_index];

    hr = ID3D12CommandQueue_Signal(dx12_renderer->command_queue, fence.fence, fence.value);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandQueue_Signal failed");
        return false;
    }

    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, 0,0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }

    dx12_renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(dx12_renderer->swap_chain);

    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    return true;
}

#endif
