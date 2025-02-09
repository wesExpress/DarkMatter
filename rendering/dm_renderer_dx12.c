#include "dm.h"

#ifdef DM_DIRECTX12

#include "../platform/dm_platform_win32.h"

#include <stdio.h>

#include <windows.h>

#define COBJMACROS
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#undef COBJMACROS

#define DM_DX12_MAX_FRAMES_IN_FLIGHT DM_MAX_FRAMES_IN_FLIGHT 

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

typedef struct dm_dx12_raster_pipeline_t
{
    ID3D12PipelineState* state;
    ID3D12RootSignature* root_signature;

    D3D_PRIMITIVE_TOPOLOGY topology;
    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor;

    D3D12_GPU_DESCRIPTOR_HANDLE root_param_handle[DM_MAX_DESCRIPTOR_GROUPS][DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t                     root_param_count;
} dm_dx12_raster_pipeline;

typedef struct dm_dx12_vertex_buffer_t
{
    D3D12_VERTEX_BUFFER_VIEW view[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    D3D12_INDEX_BUFFER_VIEW view[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    DXGI_FORMAT             index_format;

    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    D3D12_CPU_DESCRIPTOR_HANDLE handle[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    size_t                      size, big_size;
    void*                       mapped_addresses[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_constant_buffer;

typedef struct dm_dx12_texture_t
{
    uint8_t host_textures[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_textures[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    D3D12_CPU_DESCRIPTOR_HANDLE handle[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_texture;

#define DM_DX12_MAX_RAST_PIPES 10
#define DM_DX12_MAX_VBS        100
#define DM_DX12_MAX_IBS        100
#define DM_DX12_MAX_CBS        100
#define DM_DX12_MAX_BUFFERS    (DM_DX12_MAX_VBS + DM_DX12_MAX_IBS + DM_DX12_MAX_CBS)
#define DM_DX12_MAX_TEXTURES   100
#define DM_DX12_MAX_RESOURCES ((DM_DX12_MAX_BUFFERS + DM_DX12_MAX_TEXTURES) * 2)

#define DM_DX12_TABLE_MAX_CBV 7
#define DM_DX12_TABLE_MAX_UBV 7
#define DM_DX12_TABLE_MAX_SRV 15

typedef struct dm_dx12_renderer_t
{
    ID3D12Device5*   device;
    IDXGISwapChain4* swap_chain;

    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* command_list[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    dm_dx12_fence fences[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    HANDLE        fence_event;

    ID3D12Resource*         render_targets[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap rtv_heap;
    dm_dx12_descriptor_heap resource_heap[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap binding_heap[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

    dm_dx12_raster_pipeline rast_pipelines[DM_DX12_MAX_RAST_PIPES];
    uint32_t                rast_pipe_count;

    dm_dx12_vertex_buffer vertex_buffers[DM_DX12_MAX_VBS];
    uint32_t              vb_count;

    dm_dx12_index_buffer  index_buffers[DM_DX12_MAX_IBS];
    uint32_t              ib_count;

    dm_dx12_constant_buffer constant_buffers[DM_DX12_MAX_CBS];
    uint32_t                cb_count;

    dm_dx12_texture textures[DM_DX12_MAX_TEXTURES];
    uint32_t        texture_count;

    ID3D12Resource* resources[DM_DX12_MAX_RESOURCES];
    uint32_t        resource_count;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
    IDXGIDebug1* dxgi_debug;
    IDXGIInfoQueue* info_queue;
#endif

    uint8_t current_frame;
} dm_dx12_renderer;

#define DM_DX12_GET_RENDERER dm_dx12_renderer* dx12_renderer = renderer->internal_renderer

#ifdef DM_DEBUG
void dm_dx12_get_debug_message(dm_dx12_renderer* dx12_renderer);
#else
void dm_dx12_get_debug_message(dm_dx12_renderer* dx12_renderer) {}
#endif

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_wait_for_previous_frame(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;

    dm_dx12_fence* fence = &dx12_renderer->fences[dx12_renderer->current_frame];

    const uint64_t v = ID3D12Fence_GetCompletedValue(fence->fence);
    if(v < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, dx12_renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Fence_SetEventOnCompletion failed");
            return false;
        }

        WaitForSingleObject(dx12_renderer->fence_event, INFINITE);
    }

    fence->value++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_descriptor_heap(uint32_t descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, dm_dx12_descriptor_heap* heap, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    void* temp = NULL;

    D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
    desc.Type            = type;
    desc.NumDescriptors  = descriptor_count;
    desc.Flags          |= flags;

    hr = ID3D12Device5_CreateDescriptorHeap(dx12_renderer->device, &desc, &IID_ID3D12DescriptorHeap, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreateDescriptorHeap failed");
        return false;
    }
    heap->heap = temp;
    temp = NULL;

    heap->size = ID3D12Device5_GetDescriptorHandleIncrementSize(dx12_renderer->device, type);

    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap->heap, &heap->cpu_handle.begin);
    heap->cpu_handle.current = heap->cpu_handle.begin;

    if(flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap->heap, &heap->gpu_handle.begin);
        heap->gpu_handle.current = heap->gpu_handle.begin;
    }

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

        hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("DXGIGetDebugInterface failed");
            return false;
        }
        dx12_renderer->dxgi_debug = temp;
        temp = NULL;

        hr = IDXGIDebug1_QueryInterface(dx12_renderer->dxgi_debug, &IID_IDXGIInfoQueue, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("IDXGIDebug1_QueryInterface failed");
            return false;
        }
        dx12_renderer->info_queue = temp;
        temp = NULL;
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
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

        while(IDXGIFactory4_EnumAdapters1(dx12_renderer->factory, adapter_index, &dx12_renderer->adapter) != DXGI_ERROR_NOT_FOUND)
        {
            IDXGIAdapter1_GetDesc1(dx12_renderer->adapter, &adapter_desc);

            if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter_index++;
                continue;
            }

            hr = D3D12CreateDevice((IUnknown*)dx12_renderer->adapter, feature_level, &IID_ID3D12Device, NULL);
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

        hr = D3D12CreateDevice((IUnknown*)dx12_renderer->adapter, feature_level, &IID_ID3D12Device5, &temp);
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
        swap_desc.BufferCount  = DM_DX12_MAX_FRAMES_IN_FLIGHT;
        swap_desc.Width        = context->renderer.width;
        swap_desc.Height       = context->renderer.height;
        swap_desc.BufferUsage  = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_desc.SampleDesc   = sample_desc;
        swap_desc.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_desc.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;        

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

    // command allocators and lists
    {
        const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            hr = ID3D12Device5_CreateCommandAllocator(dx12_renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
                return false;
            }
            dx12_renderer->command_allocator[i] = temp;
            temp = NULL;

            hr = ID3D12Device5_CreateCommandList(dx12_renderer->device, 0, type, dx12_renderer->command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandList failed");
                return false;
            }
            dx12_renderer->command_list[i] = temp;
            temp = NULL;
        }
    }

    // rtv heap
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &dx12_renderer->rtv_heap, dx12_renderer)) return false;
    }

    // resource heap(s)
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            // resources are created here
            if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &dx12_renderer->resource_heap[i], dx12_renderer)) return false;
            // descriptors are copied into here for drawing
            if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &dx12_renderer->binding_heap[i], dx12_renderer)) return false;
        }
    }

    // render targets
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {

            hr = IDXGISwapChain4_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("IDXGISwapChain4_GetBuffer failed");
                return false;
            }
            dx12_renderer->render_targets[i] = temp;
            temp = NULL;
            ID3D12Resource* rt = dx12_renderer->render_targets[i];

            D3D12_RENDER_TARGET_VIEW_DESC view_desc = { 0 };
            view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
            view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            D3D12_CPU_DESCRIPTOR_HANDLE* handle = &dx12_renderer->rtv_heap.cpu_handle.current;

            ID3D12Device5_CreateRenderTargetView(dx12_renderer->device, rt, &view_desc, *handle);

            handle->ptr += dx12_renderer->rtv_heap.size;
        }
    }

    // fence stuff
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
            hr = ID3D12Device5_CreateFence(dx12_renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
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
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    ID3D12CommandQueue* command_queue = dx12_renderer->command_queue;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[dx12_renderer->current_frame];
    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12GraphicsCommandList7_Close(dx12_renderer->command_list[i]);
    }

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    ID3D12CommandQueue_Signal(dx12_renderer->command_queue, dx12_renderer->fences[dx12_renderer->current_frame].fence, dx12_renderer->fences[dx12_renderer->current_frame].value);
    if(!dm_dx12_wait_for_previous_frame(dx12_renderer)) 
    {
        DM_LOG_FATAL("Waiting for previous frame failed");
        return false;
    }

    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dx12_renderer->current_frame = i;
        ID3D12CommandQueue_Signal(dx12_renderer->command_queue, dx12_renderer->fences[i].fence, dx12_renderer->fences[i].value);
        if(!dm_dx12_wait_for_previous_frame(dx12_renderer))
        {
            DM_LOG_ERROR("Waiting for previous frame failed");
            continue;
        }
    }

    for(uint32_t i=0; i<dx12_renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_DX12_MAX_FRAMES_IN_FLIGHT; j++)
        {
            ID3D12Resource_Unmap(dx12_renderer->resources[dx12_renderer->constant_buffers[i].device_buffers[j]], 0,0);
            dx12_renderer->constant_buffers[i].mapped_addresses[j] = NULL;
        }
    }

    for(uint32_t i=0; i<dx12_renderer->resource_count; i++)
    {
        ID3D12Resource_Release(dx12_renderer->resources[i]);
    }

    for(uint32_t i=0; i<dx12_renderer->rast_pipe_count; i++)
    {
        ID3D12RootSignature_Release(dx12_renderer->rast_pipelines[i].root_signature);
        ID3D12PipelineState_Release(dx12_renderer->rast_pipelines[i].state);
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(dx12_renderer->render_targets[i]);
        ID3D12GraphicsCommandList_Release(dx12_renderer->command_list[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocator[i]);
        ID3D12Fence_Release(dx12_renderer->fences[i].fence);
        ID3D12DescriptorHeap_Release(dx12_renderer->resource_heap[i].heap);
        ID3D12DescriptorHeap_Release(dx12_renderer->binding_heap[i].heap);
    }

    CloseHandle(dx12_renderer->fence_event);

    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_heap.heap);
    IDXGISwapChain4_Release(dx12_renderer->swap_chain);
    ID3D12CommandQueue_Release(dx12_renderer->command_queue);
    ID3D12Device5_Release(dx12_renderer->device);

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
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dx12_renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(dx12_renderer->swap_chain);
    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12CommandAllocator*     command_allocator = dx12_renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    hr = ID3D12CommandAllocator_Reset(command_allocator);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandAllocator_Reset failed");
        return false;
    }

    hr = ID3D12GraphicsCommandList7_Reset(command_list, command_allocator, NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Reset failed");
        return false;
    }

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->render_targets[current_frame];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    
    // rtv heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx12_renderer->rtv_heap.cpu_handle.begin;
    rtv_handle.ptr += current_frame * dx12_renderer->rtv_heap.size;

    float clear_color[] = { 0.2f,0.5f,0.7f,1.f };

    ID3D12GraphicsCommandList7_OMSetRenderTargets(command_list, 1, &rtv_handle, FALSE, NULL);
    ID3D12GraphicsCommandList7_ClearRenderTargetView(command_list, rtv_handle, clear_color, 0, NULL);

    // binding heap (reset its ptr back to the start)
    dm_dx12_descriptor_heap* binding_heap = &dx12_renderer->binding_heap[current_frame];
    binding_heap->cpu_handle.current = binding_heap->cpu_handle.begin;
    
    ID3D12DescriptorHeap* heaps[] = { binding_heap->heap };
    ID3D12GraphicsCommandList7_SetDescriptorHeaps(command_list, _countof(heaps), heaps);

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    const uint8_t frame_index = dx12_renderer->current_frame;

    ID3D12CommandQueue*         command_queue = dx12_renderer->command_queue;
    ID3D12CommandAllocator*     command_allocator = dx12_renderer->command_allocator[frame_index];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[frame_index];

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->render_targets[frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

    hr = ID3D12GraphicsCommandList7_Close(command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Close failed");
        return false;
    }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    const dm_dx12_fence fence = dx12_renderer->fences[frame_index];

    hr = ID3D12CommandQueue_Signal(dx12_renderer->command_queue, fence.fence, fence.value);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandQueue_Signal failed");
        return false;
    }

    UINT present_flag = 0;
    present_flag = DXGI_PRESENT_ALLOW_TEARING;

    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, context->renderer.vsync, present_flag);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }

    if(!dm_dx12_wait_for_previous_frame(dx12_renderer))
    {
        DM_LOG_FATAL("Waiting for previous frame failed");
        return false;
    }
    
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dx12_renderer->current_frame = i;
        dm_dx12_fence* fence = &dx12_renderer->fences[i];
        ID3D12CommandQueue_Signal(dx12_renderer->command_queue, fence->fence, fence->value);
        dm_dx12_wait_for_previous_frame(dx12_renderer);
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(dx12_renderer->render_targets[i]);
    }

    hr = IDXGISwapChain4_ResizeBuffers(dx12_renderer->swap_chain, DM_DX12_MAX_FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_ResizeBuffers failed");
        return false;
    }

    void* temp = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE handle = dx12_renderer->rtv_heap.cpu_handle.begin;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = IDXGISwapChain4_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("IDXGISwapChain4_GetBuffer failed");
            return false;
        }
        dx12_renderer->render_targets[i] = temp;
        temp = NULL;

        D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        ID3D12Device_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->render_targets[i], &desc, handle);
        handle.ptr += dx12_renderer->rtv_heap.size;
    }

    dx12_renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(dx12_renderer->swap_chain);

    return true;
}

/******************
 * RASTER PIPELINE
 *******************/
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_load_shader_data(const char* path, ID3D10Blob** blob)
{
    HRESULT hr;
    wchar_t ws[512];
    swprintf(ws, 512, L"%hs", path);
    hr = D3DReadFileToBlob(ws, blob);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("D3DReadFileToBlob failed");
        DM_LOG_ERROR("Could not load shader: %s", path);
        return false;
    }

    return true;
}

bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    void* temp = NULL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // === descriptors ===
    // TODO: this is probably terrible
    D3D12_ROOT_DESCRIPTOR_TABLE tables[DM_MAX_DESCRIPTOR_GROUPS] = { 0 };
    D3D12_DESCRIPTOR_RANGE ranges[DM_MAX_DESCRIPTOR_GROUPS][DM_DESCRIPTOR_GROUP_MAX_RANGES] = { 0 };
    D3D12_ROOT_PARAMETER params[DM_MAX_DESCRIPTOR_GROUPS] = { 0 };

    uint32_t descriptor_count = 0;

    // for each descriptor group
    for(uint8_t i=0; i<desc.descriptor_group_count; i++)
    {
        dm_descriptor_group group = desc.descriptor_group[i];

        // for each range
        for(uint8_t j=0; j<group.range_count; j++)
        {
            dm_descriptor_range range = group.ranges[j];

            switch(range.type)
            {
                case DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER:
                ranges[i][j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;

                case DM_DESCRIPTOR_RANGE_TYPE_TEXTURE:
                ranges[i][j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;

                default:
                DM_LOG_FATAL("Unknown or unsupported descriptor range type");
                return false;
            }

            ranges[i][j].NumDescriptors                    = range.count;
            ranges[i][j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            descriptor_count += range.count;
        }

        tables[i].NumDescriptorRanges = group.range_count;
        tables[i].pDescriptorRanges   = ranges[i];

        params[i].ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[i].DescriptorTable  = tables[i];
        if(group.flags == DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER) params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        else if(group.flags == DM_DESCRIPTOR_GROUP_FLAG_PIXEL_SHADER) params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        for(uint8_t j=0; j<DM_DX12_MAX_FRAMES_IN_FLIGHT; j++)
        {
            pipeline.root_param_handle[i][j] = dx12_renderer->binding_heap[j].gpu_handle.current;
            dx12_renderer->binding_heap[j].gpu_handle.current.ptr += descriptor_count * dx12_renderer->binding_heap[j].size;
        }
    }

    pipeline.root_param_count = desc.descriptor_group_count;

    // === sampler ===
    D3D12_STATIC_SAMPLER_DESC sampler = { 0 };
    sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // === root signature ===
    D3D12_ROOT_SIGNATURE_DESC root_desc = { 0 };
    
    root_desc.NumParameters     = desc.descriptor_group_count;
    root_desc.pParameters       = params;
    root_desc.NumStaticSamplers = 1;
    root_desc.pStaticSamplers   = &sampler;

    root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ID3D10Blob* root_blob = NULL;

    hr = D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_blob, NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("D3D12SerializeRootSignature failed");
        dm_dx12_get_debug_message(dx12_renderer);
        return false;
    }

    hr = ID3D12Device5_CreateRootSignature(dx12_renderer->device, 0, ID3D10Blob_GetBufferPointer(root_blob), ID3D10Blob_GetBufferSize(root_blob), &IID_ID3D12RootSignature, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreateRootSignature failed");
        dm_dx12_get_debug_message(dx12_renderer);
        return false;
    }

    // === rasterizer ===
    switch(desc.rasterizer.cull_mode)
    {
        default:
        DM_LOG_ERROR("Unknown cull mode. Assuming D3D12_CULL_MODE_BACK");
        case DM_RASTERIZER_CULL_MODE_BACK:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        break;

        case DM_RASTERIZER_CULL_MODE_FRONT:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        break;

        case DM_RASTERIZER_CULL_MODE_NONE:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        break;
    }

    switch(desc.rasterizer.polygon_fill)
    {
        default:
        DM_LOG_ERROR("Unknown polygon fill mode. Assuming D3D12_FILL_MODE_SOLID");
        case DM_RASTERIZER_POLYGON_FILL_FILL:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        break;

        case DM_RASTERIZER_POLYGON_FILL_WIREFRAME:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    }

    switch(desc.rasterizer.front_face)
    {
        case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        break;

        default:
        DM_LOG_ERROR("Unknown front face. Assuming counter clockwise");
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = TRUE;
        break;
    }

    // TODO: needs to be configurable
    // === blending ===
    D3D12_RENDER_TARGET_BLEND_DESC blend_desc = { 0 };
    
    blend_desc.BlendEnable           = TRUE;
    blend_desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOp               = D3D12_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha         = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.LogicOpEnable         = FALSE;
    blend_desc.LogicOp               = D3D12_LOGIC_OP_NOOP;

    // === shaders ===
    ID3DBlob* vs = NULL;
    ID3DBlob* ps = NULL;
    
    const char* vertex_path = desc.rasterizer.vertex_shader_desc.path;
    const char* pixel_path  = desc.rasterizer.pixel_shader_desc.path;

    if(!dm_dx12_load_shader_data(vertex_path, &vs)) 
    {
        DM_LOG_ERROR("Could not load vertex shader");
        return false;
    }
    if(!dm_dx12_load_shader_data(pixel_path,  &ps)) 
    {
        DM_LOG_ERROR("Could not load pixel shader");
        return false;
    }
    
    pso_desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vs);
    pso_desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(vs);

    pso_desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(ps);
    pso_desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(ps);

    // === input assembler ===
    D3D12_INPUT_ELEMENT_DESC input_element_descs[DM_RENDER_MAX_INPUT_ELEMENTS] = { 0 };

    for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
    {
        input_element_descs[i].SemanticName      = desc.input_assembler.input_elements[i].name; 
        input_element_descs[i].InputSlot         = desc.input_assembler.input_elements[i].slot;
        input_element_descs[i].AlignedByteOffset = i==0 ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;

        switch(desc.input_assembler.input_elements[i].format)
        {
            case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
            input_element_descs[i].Format = DXGI_FORMAT_R32G32_FLOAT;
            break;

            default:
            DM_LOG_ERROR("Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
            case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
            input_element_descs[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            break;

            case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
            input_element_descs[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            break;
        }

        switch(desc.input_assembler.input_elements[i].class)
        {
            default:
            DM_LOG_ERROR("Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
            case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
            input_element_descs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            break;

            case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
            input_element_descs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            break;
        }
    }

    pso_desc.InputLayout.pInputElementDescs = input_element_descs;
    pso_desc.InputLayout.NumElements        = desc.input_assembler.input_element_count;

    switch(desc.input_assembler.topology)
    {
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

        default:
        DM_LOG_ERROR("Unknow primitive topology. Assuming D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST");
        break;
    }

    // === viewport and scissor ===
    switch(desc.viewport.type)
    {
        default:
        DM_LOG_ERROR("Unknown viewport type. Assumin DM_VIEWPORT_TYPE_DEFAULT");
        case DM_VIEWPORT_TYPE_DEFAULT:
        pipeline.viewport.Width    = (float)renderer->width;
        pipeline.viewport.Height   = (float)renderer->height;
        pipeline.viewport.MaxDepth = 1.f;
        break;

        case DM_VIEWPORT_TYPE_CUSTOM:
        DM_LOG_FATAL("Custom viewport for dx12 pipeline not supported yet");
        return false;
    }

    switch(desc.scissor.type)
    {
        default:
        DM_LOG_ERROR("Unknown scissor type. Assuming DM_SCISSOR_TYPE_DEFAULT");
        case DM_SCISSOR_TYPE_DEFAULT:
        pipeline.scissor.right  = (float)renderer->width;
        pipeline.scissor.bottom = (float)renderer->height;
        break;

        case DM_SCISSOR_TYPE_CUSTOM:
        DM_LOG_FATAL("Custom scissor for dx12 not supported yet");
        return false;
    }

    
    pipeline.root_signature = temp;
    temp = NULL;


    // === pipeline state ===
    pso_desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count           = 1;
    pso_desc.SampleMask                 = 0xffffffff;
    pso_desc.NumRenderTargets           = 1;
    pso_desc.BlendState.RenderTarget[0] = blend_desc;
    pso_desc.pRootSignature             = pipeline.root_signature;

    hr = ID3D12Device5_CreateGraphicsPipelineState(dx12_renderer->device, &pso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        // must release root signature since count won't increment
        ID3D12RootSignature_Release(pipeline.root_signature);

        DM_LOG_FATAL("ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(dx12_renderer);
        return false;
    }
    pipeline.state = temp;
    temp = NULL;
    
    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);
    ID3D10Blob_Release(root_blob);

    // 
    dm_memcpy(dx12_renderer->rast_pipelines + dx12_renderer->rast_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = dx12_renderer->rast_pipe_count++;

    return true;
}

/************
 * RESOURCES
**************/
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_committed_resource(D3D12_HEAP_PROPERTIES properties, D3D12_RESOURCE_DESC desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, ID3D12Device5* device)
{
    void* temp = NULL;
    HRESULT hr = ID3D12Device5_CreateCommittedResource(device, &properties, flags, &desc, state, 0, &IID_ID3D12Resource, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
        return false;
    }

    *resource = temp;
    temp = NULL;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_copy_memory(ID3D12Resource* resource, void* data, size_t size)
{
    void* temp = NULL;

    D3D12_RANGE range = { 0 };
    range.End = size-1;
    
    ID3D12Resource_Map(resource, 0, &range, &temp);
    if(!temp)
    {
        DM_LOG_FATAL("ID3D12Resource_Map failed");
        return false;
    }

    dm_memcpy(temp, data, size);
    ID3D12Resource_Unmap(resource, 0, &range);
    
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_buffer(const size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, dm_dx12_renderer* dx12_renderer)
{
    D3D12_RESOURCE_DESC desc = { 0 };
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width            = size;
    desc.Height           = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, dx12_renderer->device)) return false;  

    return true;
}

bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_vertex_buffer buffer = { 0 };

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[dx12_renderer->current_frame];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, host_buffer, dx12_renderer)) return false;
        buffer.host_buffers[i] = dx12_renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, device_buffer, dx12_renderer)) return false;
        buffer.device_buffers[i] = dx12_renderer->resource_count++;

        buffer.view[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.view[i].SizeInBytes   = desc.size;
        buffer.view[i].StrideInBytes = desc.stride;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    } 

    //
    dm_memcpy(dx12_renderer->vertex_buffers + dx12_renderer->vb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->vb_count++;

    return true;
}

bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_format = DXGI_FORMAT_R16_UINT;
        break;

        default:
        DM_LOG_ERROR("Unknown index type. Assuming uint32");
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_format = DXGI_FORMAT_R32_UINT;
        break;
    }

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[dx12_renderer->current_frame];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, host_buffer, dx12_renderer)) return false;
        buffer.host_buffers[i] = dx12_renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, device_buffer, dx12_renderer)) return false;
        buffer.device_buffers[i] = dx12_renderer->resource_count++;

        buffer.view[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.view[i].SizeInBytes    = desc.size;
        buffer.view[i].Format         = buffer.index_format;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);
    }

    //
    {
        dm_memcpy(dx12_renderer->index_buffers + dx12_renderer->ib_count, &buffer, sizeof(buffer));
        handle->index = dx12_renderer->ib_count++;
    }

    return true;
}

bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_constant_buffer buffer = { 0 };
    const size_t aligned_size = (desc.size + 255) & ~255;
    const size_t big_size = 1024 * 64;
        
    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(big_size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, device_buffer, dx12_renderer)) return false;
        buffer.device_buffers[i] = dx12_renderer->resource_count++;
        buffer.size     = aligned_size; 
        buffer.big_size = big_size;

        D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = { 0 };
        view_desc.SizeInBytes    = aligned_size;
        view_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[buffer.device_buffers[i]]);

        ID3D12Device5_CreateConstantBufferView(dx12_renderer->device, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        buffer.handle[i] = dx12_renderer->resource_heap[i].cpu_handle.current;

        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;

        if(!desc.data) continue;

        hr = ID3D12Resource_Map(dx12_renderer->resources[buffer.device_buffers[i]], 0,NULL, &buffer.mapped_addresses[i]);
        if(!dm_platform_win32_decode_hresult(hr) || !buffer.mapped_addresses[i])
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            return false;
        }

        dm_memcpy(buffer.mapped_addresses[i], desc.data, desc.size);
    }

    //
    dm_memcpy(dx12_renderer->constant_buffers + dx12_renderer->cb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->cb_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_texture(const size_t width, const size_t height, const dm_texture_format format, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, ID3D12Device5* device)
{
    D3D12_RESOURCE_DESC desc = { 0 };
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width            = width;
    desc.Height           = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags            = D3D12_RESOURCE_FLAG_NONE;
    switch(format)
    {
        case DM_TEXTURE_FORMAT_FLOAT_3:
        desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

        default:
        DM_LOG_FATAL("Unknown or unsupported texture format");
        return false;
    }

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, device)) return false;  

    return true;
}

bool dm_renderer_backend_create_texture(dm_texture_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_texture texture = { 0 };

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    const size_t size = desc.width * desc.height * desc.n_channels;

    DXGI_FORMAT format;
    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        format = DXGI_FORMAT_R8G8B8A8_UINT;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_3:
        format = DXGI_FORMAT_R32G32B32_FLOAT;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

        default:
        DM_LOG_FATAL("Unknown or unsupported texture format");
        return false;
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host texture is actually a buffer
        ID3D12Resource** host_resource = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, host_resource, dx12_renderer)) return false;
        texture.host_textures[i] = dx12_renderer->resource_count++;

        if(!desc.data)
        {
            DM_LOG_FATAL("Need data to create a texture");
            return false;
        }

        if(!dm_dx12_copy_memory(*host_resource, desc.data, size)) return false;

        // device texture
        ID3D12Resource** device_resource = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_texture(desc.width, desc.height, desc.format, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, device_resource, dx12_renderer->device)) return false;
        texture.device_textures[i] = dx12_renderer->resource_count++;

        D3D12_BOX texture_as_box = { 0 };
        texture_as_box.right  = desc.width;
        texture_as_box.bottom = desc.height;
        texture_as_box.back   = 1;

        D3D12_TEXTURE_COPY_LOCATION src = { 0 };
        src.pResource                          = *host_resource;
        src.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Width    = desc.width;
        src.PlacedFootprint.Footprint.Height   = desc.height;
        src.PlacedFootprint.Footprint.Depth    = 1;
        src.PlacedFootprint.Footprint.RowPitch = desc.width * desc.n_channels;
        src.PlacedFootprint.Footprint.Format   = format;

        D3D12_TEXTURE_COPY_LOCATION dest = { 0 };
        dest.pResource = *device_resource;
        dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        ID3D12GraphicsCommandList7_CopyTextureRegion(command_list, &dest, 0,0,0, &src, &texture_as_box);

        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = *device_resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

        // view
        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                  = format;
        view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipLevels     = 1;

        ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, *device_resource, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        texture.handle[i] = dx12_renderer->resource_heap[i].cpu_handle.current;

        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
    }

    //
    dm_memcpy(dx12_renderer->textures + dx12_renderer->texture_count, &texture, sizeof(texture));
    handle->index = dx12_renderer->texture_count++;

    return true;
}

/******************
 * RENDER COMMANDS
* ******************/
bool dm_render_command_backend_bind_raster_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_raster_pipeline pipeline = dx12_renderer->rast_pipelines[handle.index];

    // reset viewport and scissors
    {
        pipeline.viewport.Height = renderer->height;
        pipeline.viewport.Width  = renderer->width;

        pipeline.scissor.right  = renderer->width;
        pipeline.scissor.bottom = renderer->height;
    }

    ID3D12GraphicsCommandList7_SetGraphicsRootSignature(command_list, pipeline.root_signature);
    ID3D12GraphicsCommandList7_SetPipelineState(command_list, pipeline.state);
    ID3D12GraphicsCommandList7_RSSetViewports(command_list, 1, &pipeline.viewport);
    ID3D12GraphicsCommandList7_RSSetScissorRects(command_list, 1, &pipeline.scissor);
    ID3D12GraphicsCommandList7_IASetPrimitiveTopology(command_list, pipeline.topology);
    
    for(uint8_t i=0; i<pipeline.root_param_count; i++)
    {
        const D3D12_GPU_DESCRIPTOR_HANDLE table_handle = pipeline.root_param_handle[i][current_frame];
        ID3D12GraphicsCommandList7_SetGraphicsRootDescriptorTable(command_list, i, table_handle);
    }

    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_vertex_buffer vb = dx12_renderer->vertex_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetVertexBuffers(command_list, 0, 1, &vb.view[current_frame]);

    return true;
}

bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    dm_dx12_vertex_buffer buffer = dx12_renderer->vertex_buffers[handle.index];

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    ID3D12Resource* host_buffer   = dx12_renderer->resources[buffer.host_buffers[current_frame]];
    ID3D12Resource* device_buffer = dx12_renderer->resources[buffer.device_buffers[current_frame]];

    if(!dm_dx12_copy_memory(host_buffer, data, size)) return false;

    D3D12_RESOURCE_BARRIER barriers[2] = { 0 };

    barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.pResource   = device_buffer;

    barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[1].Transition.pResource   = device_buffer;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[0]);
    ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, device_buffer, 0, host_buffer, 0, size);
    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[1]);

    return true;
}

bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    if(!dx12_renderer->constant_buffers[handle.index].mapped_addresses[current_frame])
    {
        DM_LOG_FATAL("Constant buffer has an invalid address");
        return false;
    }

    dm_memcpy(dx12_renderer->constant_buffers[handle.index].mapped_addresses[current_frame], data, size);

    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_index_buffer ib = dx12_renderer->index_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetIndexBuffer(command_list, &ib.view[current_frame]);

    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle buffer, uint8_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const dm_dx12_constant_buffer constant_buffer = dx12_renderer->constant_buffers[buffer.index];

    const uint8_t current_frame = dx12_renderer->current_frame;

    dm_dx12_descriptor_heap* binding_heap = &dx12_renderer->binding_heap[current_frame];

    ID3D12Device5_CopyDescriptorsSimple(dx12_renderer->device, 1, binding_heap->cpu_handle.current, constant_buffer.handle[current_frame], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    binding_heap->cpu_handle.current.ptr += binding_heap->size; 

    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle texture, uint8_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const dm_dx12_texture t = dx12_renderer->textures[texture.index];

    const uint8_t current_frame = dx12_renderer->current_frame;

    dm_dx12_descriptor_heap* binding_heap = &dx12_renderer->binding_heap[current_frame];

    ID3D12Device5_CopyDescriptorsSimple(dx12_renderer->device, 1, binding_heap->cpu_handle.current, t.handle[current_frame], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    binding_heap->cpu_handle.current.ptr += binding_heap->size;

    return true;
}

bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_DrawInstanced(command_list, vertex_count, instance_count, vertex_offset, instance_offset);
        
    return true;
}

bool dm_render_command_backend_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[dx12_renderer->current_frame];

    ID3D12GraphicsCommandList7_DrawIndexedInstanced(command_list, index_count, instance_count, index_offset, vertex_offset, instance_offset);

    return true;
}

/********
 * DEBUG
**********/

#ifdef DM_DEBUG
void dm_dx12_get_debug_message(dm_dx12_renderer* dx12_renderer)
{
    const uint32_t num_messages = IDXGIInfoQueue_GetNumStoredMessages(dx12_renderer->info_queue, DXGI_DEBUG_ALL);
    for(uint32_t i=0; i<num_messages; i++)
    {
        size_t len = 0;
        IDXGIInfoQueue_GetMessage(dx12_renderer->info_queue, DXGI_DEBUG_ALL, i, NULL, &len);
        DXGI_INFO_QUEUE_MESSAGE* buffer = dm_alloc(len);
        IDXGIInfoQueue_GetMessage(dx12_renderer->info_queue, DXGI_DEBUG_ALL, i, buffer, &len);

        DM_LOG_ERROR("%s\n", buffer->pDescription);
        dm_free((void**)&buffer);
    }
}
#endif

#endif
