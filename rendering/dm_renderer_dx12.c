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
} dm_dx12_raster_pipeline;

typedef struct dm_dx12_buffer_indices_t
{
    uint8_t buffer[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t upload[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_buffer_indices;

typedef struct dm_dx12_vertex_buffer_t
{
    D3D12_VERTEX_BUFFER_VIEW view[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_buffer_indices   indices;
} dm_dx12_vertex_buffer;

#define DM_DX12_MAX_RAST_PIPES 10
#define DM_DX12_MAX_VBS        100
#define DM_DX12_MAX_IBS        100
#define DM_DX12_MAX_TEXTURES   100
#define DM_DX12_MAX_RESOURCES  ((DM_DX12_MAX_VBS + DM_DX12_MAX_IBS + DM_DX12_MAX_TEXTURES) * 2 * DM_DX12_MAX_FRAMES_IN_FLIGHT)
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
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

    dm_dx12_raster_pipeline rast_pipelines[DM_DX12_MAX_RAST_PIPES];
    uint32_t                rast_pipe_count;

    dm_dx12_vertex_buffer vertex_buffers[DM_DX12_MAX_VBS];
    uint32_t              vb_count;

    ID3D12Resource* resources[DM_DX12_MAX_RESOURCES];
    uint32_t        resource_count;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
#endif

    uint8_t current_frame;
} dm_dx12_renderer;

#define DM_DX12_GET_RENDERER dm_dx12_renderer* dx12_renderer = renderer->internal_renderer

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
        D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = DM_DX12_MAX_FRAMES_IN_FLIGHT;

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

    for(uint32_t i=0; i<dx12_renderer->resource_count; i++)
    {
        ID3D12Resource_Release(dx12_renderer->resources[i]);
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(dx12_renderer->render_targets[i]);
        ID3D12GraphicsCommandList_Release(dx12_renderer->command_list[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocator[i]);
        ID3D12Fence_Release(dx12_renderer->fences[i].fence);
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
    const uint8_t frame_index = dx12_renderer->current_frame;

    ID3D12CommandAllocator*     command_allocator = dx12_renderer->command_allocator[frame_index];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[frame_index];

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
    barrier.Transition.pResource   = dx12_renderer->render_targets[frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx12_renderer->rtv_heap.cpu_handle.begin;
    rtv_handle.ptr += frame_index * dx12_renderer->rtv_heap.size;

    float clear_color[] = { 0.2f,0.5f,0.7f,1.f };

    ID3D12GraphicsCommandList7_OMSetRenderTargets(command_list, 1, &rtv_handle, FALSE, NULL);
    ID3D12GraphicsCommandList7_ClearRenderTargetView(command_list, rtv_handle, clear_color, 0, NULL);

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

    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, 0,0);
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
        return false;
    }

    return true;
}

bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // === shaders ===
    {
        uint32_t flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        ID3DBlob* vs = NULL;
        ID3DBlob* ps = NULL;

        const char* vertex_path = desc.rasterizer.vertex_shader_desc.path;
        const char* pixel_path = desc.rasterizer.pixel_shader_desc.path;

        if(!dm_dx12_load_shader_data(vertex_path, &vs)) return false;
        if(!dm_dx12_load_shader_data(pixel_path,  &ps)) return false;
        
        pso_desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vs);
        pso_desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(vs);

        pso_desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(ps);
        pso_desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(ps);

        ID3D10Blob_Release(vs);
        ID3D10Blob_Release(ps);
    }

    // === input assembler ===
    {
        D3D12_INPUT_ELEMENT_DESC input_element_descs[DM_RENDER_MAX_INPUT_ELEMENTS];
        for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
        {
            D3D12_INPUT_ELEMENT_DESC* input = &input_element_descs[i];
            dm_input_element_desc element   = desc.input_assembler.input_elements[i];

            input->SemanticName      = element.name; 
            input->InputSlot         = element.slot;
            //input->AlignedByteOffset = element.offset;
            input->AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

            switch(element.format)
            {
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
                input->Format = DXGI_FORMAT_R32G32_FLOAT;
                break;

                case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                input->Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;

                default:
                DM_LOG_ERROR("Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
                input->Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;
            }

            switch(element.class)
            {
                case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
                input->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                break;

                case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
                input->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                break;

                default:
                DM_LOG_ERROR("Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
                input->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX;
                break;
            }
        }

        pso_desc.InputLayout.pInputElementDescs = input_element_descs;
        pso_desc.InputLayout.NumElements        = desc.input_assembler.input_element_count;
    }

    // 
    {
        dm_memcpy(dx12_renderer->rast_pipelines + dx12_renderer->rast_pipe_count, &pipeline, sizeof(pipeline));
        handle->index = dx12_renderer->rast_pipe_count++;
    }

    return true;
}

/************
 * RESOURCES
**************/
#ifndef DM_DEBUG
DM_INLINE
#endif
ID3D12Resource* dm_dx12_create_committed_resource(D3D12_HEAP_PROPERTIES properties, D3D12_RESOURCE_DESC desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Device5* device)
{
    void* temp = NULL;
    HRESULT hr = ID3D12Device5_CreateCommittedResource(device, &properties, flags, &desc, state, 0, &IID_ID3D12Resource, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
    }

    return temp;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
ID3D12Resource* dm_dx12_create_upload_resource(D3D12_RESOURCE_DESC desc, ID3D12Device5* device)
{
    D3D12_HEAP_PROPERTIES properties = { 0 };
    properties.Type = D3D12_HEAP_TYPE_UPLOAD;

    return dm_dx12_create_committed_resource(properties, desc, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, device);
}

#ifndef DM_DEBUG
DM_INLINE
#endif
ID3D12Resource* dm_dx12_create_default_resource(D3D12_RESOURCE_DESC desc, ID3D12Device5* device)
{
    D3D12_HEAP_PROPERTIES properties = { 0 };
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    return dm_dx12_create_committed_resource(properties, desc, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, device);
}

bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_vertex_buffer buffer = { 0 };

    D3D12_RESOURCE_DESC rd = { 0 };
    rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    rd.Width            = desc.size;
    rd.Height           = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels        = 1;
    rd.Format           = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count = 1;
    rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags            = D3D12_RESOURCE_FLAG_NONE;

    void* temp = NULL;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[dx12_renderer->current_frame];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // default buffer
        dx12_renderer->resources[dx12_renderer->resource_count] = dm_dx12_create_default_resource(rd, dx12_renderer->device);
        if(!dx12_renderer->resources[dx12_renderer->resource_count]) return false;

        buffer.indices.buffer[i] = dx12_renderer->resource_count++;

        buffer.view[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[buffer.indices.buffer[i]]);
        buffer.view[i].SizeInBytes   = desc.size;
        buffer.view[i].StrideInBytes = desc.stride;

        // upload buffer
        dx12_renderer->resources[dx12_renderer->resource_count] = dm_dx12_create_upload_resource(rd, dx12_renderer->device);
        if(!dx12_renderer->resources[dx12_renderer->resource_count]) return false;

        buffer.indices.upload[i] = dx12_renderer->resource_count++;

        if(!desc.data) continue;

        D3D12_RANGE range = { 0 };
        range.End = desc.size-1;
        ID3D12Resource_Map(dx12_renderer->resources[buffer.indices.upload[i]], 0, &range, &temp);
        dm_memcpy(temp, desc.data, desc.size);
        ID3D12Resource_Unmap(dx12_renderer->resources[buffer.indices.upload[i]], 0, &range);

        ID3D12Resource* src_buffer = dx12_renderer->resources[buffer.indices.upload[i]];
        ID3D12Resource* dst_buffer = dx12_renderer->resources[buffer.indices.buffer[i]];

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, dst_buffer, 0, src_buffer, 0, desc.size);
    } 

    //
    dm_memcpy(dx12_renderer->vertex_buffers + dx12_renderer->vb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->vb_count++;

    return true;
}

/******************
 * RENDER COMMANDS
* ******************/
bool dm_render_command_backend_bind_raster_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    if(handle.type != DM_RENDER_RESOURCE_TYPE_RASTER_PIPELINE)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a raster pipeline");
        return false;
    }

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_raster_pipeline pipeline = dx12_renderer->rast_pipelines[handle.index];

    // TODO: needs to be configurable
    ID3D12GraphicsCommandList7_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    if(handle.type != DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to bind a resource that is not a vertex buffer");
        return false;
    }

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_vertex_buffer vb = dx12_renderer->vertex_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetVertexBuffers(command_list, 0, 1, &vb.view[current_frame]);

    return true;
}

bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_DrawInstanced(command_list, vertex_count, instance_count, vertex_offset, instance_offset);
        
    return true;
}

#endif
