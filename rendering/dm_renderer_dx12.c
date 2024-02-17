#include "dm.h"

#ifdef DM_DIRECTX12

#include "platform/dm_platform_win32.h"

#define COBJMACROS
#include <unknwn.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#define NUM_FRAMES 3

typedef struct dm_dx12_renderer_t
{
    HWND      hwnd;
    HINSTANCE h_instance;
    
    ID3D12Device2*             device;
    ID3D12CommandQueue*        command_queue;
    ID3D12CommandAllocator*    command_allocators[NUM_FRAMES];
    ID3D12GraphicsCommandList* command_list;
    IDXGISwapChain*            swap_chain;
    ID3D12Resource*            back_buffers[NUM_FRAMES];
    ID3D12DescriptorHeap*      rtv_descriptor_heap;
    
    ID3D12Fence* fence;
    
    UINT rtv_descriptor_size;
    UINT current_backbuffer_index;
    uint64_t fence_value;
    uint64_t frame_fence_values[NUM_FRAMES];
    
    HANDLE fence_event;
    
#ifdef DM_DEBUG
    ID3D12Debug* debugger;
#endif
} dm_dx12_renderer;

#define DM_DX12_GET_RENDERER dm_dx12_renderer* dx12_renderer = renderer->internal_renderer

static float r = 0.5f;
static float g = 0.5f;
static float b = 0.5f;

static int r_dir = -1;
static int g_dir = -1;
static int b_dir = 1;

/****
MISC
******/


/*****
FENCE
*******/
void dm_dx12_wait_for_frame(ID3D12CommandQueue* command_queue, ID3D12Fence* fence, uint64_t* fence_value, HANDLE fence_event)
{
    HRESULT hr = ID3D12CommandQueue_Signal(command_queue, fence, *fence_value);
    if(!dm_platform_win32_decode_hresult(hr)) 
    {
        DM_LOG_ERROR("ID3D12CommandQueue_Signal failed");
        return;
    }
    
    uint64_t completed = ID3D12Fence_GetCompletedValue(fence);
    
    if(completed < *fence_value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence, *fence_value, fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_ERROR("ID3D12Fence_SetEventOnCompletion failed");
            return;
        }
        WaitForSingleObject(fence_event, INFINITE);
    }
    
    (*fence_value)++;
}

/*******
BACKEND
*********/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing DirectX12 backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_dx12_renderer));
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    
    dm_internal_w32_data* w32_data = context->platform_data.internal_data;
    
    dx12_renderer->hwnd = w32_data->hwnd;
    dx12_renderer->h_instance = w32_data->h_instance;
    
    HRESULT hr;
    
#ifdef DM_DEBUG
    hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &dx12_renderer->debugger);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("D3D12GetDebugInterface failed");
        return false;
    }
    
    ID3D12Debug_EnableDebugLayer(dx12_renderer->debugger);
#endif
    
    // adapter
    IDXGIFactory4* dxgi_factory;
    UINT create_factory_flags = 0;
#ifdef DM_DEBUG
    create_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    
    hr = CreateDXGIFactory2(create_factory_flags, &IID_IDXGIFactory4, &dxgi_factory);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("CreateDXGIFactory2 failed");
        return false;
    }
    
    IDXGIAdapter1* dxgi_adapter1;
    
    size_t max_dedicated_video_memory = 0;
    for(uint32_t i=0; DXGI_ERROR_NOT_FOUND != IDXGIFactory1_EnumAdapters1(dxgi_factory, i, &dxgi_adapter1); i++)
    {
        DXGI_ADAPTER_DESC1 dxgi_adapter_desc1 = { 0 };
        
        IDXGIAdapter1_GetDesc1(dxgi_adapter1, &dxgi_adapter_desc1);
        
        if(dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        
        hr = D3D12CreateDevice((IUnknown*)dxgi_adapter1, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, &dx12_renderer->device);
        bool valid_device = dm_platform_win32_decode_hresult(hr);
        valid_device = valid_device & (dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory);
        if(!valid_device) continue;
        
        max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
        break;
    }
    
    if(!dxgi_adapter1)
    {
        DM_LOG_FATAL("No hardware adapter found. System does not support DX12");
        return false;
    }
    
    // debug layer
#ifdef DM_DEBUG
    ID3D12InfoQueue* info_queue;
    hr = dx12_renderer->device->lpVtbl->QueryInterface(dx12_renderer->device, &IID_ID3D12InfoQueue, &info_queue);
    //hr = IUnknown_QueryInterface((IUnknown*)dx12_renderer->device, &IID_ID3D12InfoQueue, &info_queue);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IUnknown_QueryInterface failed");
        return false;
    }
    
    ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_ERROR, true);
    ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_WARNING, true);
    
    D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
    D3D12_MESSAGE_ID       deny_ids[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
    };
    
    D3D12_INFO_QUEUE_FILTER filter = { 0 };
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    filter.DenyList.NumIDs        = _countof(deny_ids);
    filter.DenyList.pIDList       = deny_ids;
    
    hr = ID3D12InfoQueue_PushStorageFilter(info_queue, &filter);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12InfoQueue_PushStorageFilter failed");
        return false;
    }
#endif
    
    // command queue
    D3D12_COMMAND_QUEUE_DESC desc = { 0 };
    desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    hr = ID3D12Device_CreateCommandQueue(dx12_renderer->device, &desc, &IID_ID3D12CommandQueue, &dx12_renderer->command_queue);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device_CreateCommandQueue failed");
        return false;
    }
    
    // swapchain
    DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
    swap_desc.Width            = context->platform_data.window_data.width;
    swap_desc.Height           = context->platform_data.window_data.height;
    swap_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM,
    swap_desc.SampleDesc.Count = 1;
    swap_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount      = NUM_FRAMES;
    swap_desc.Scaling          = DXGI_SCALING_NONE;
    swap_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode        = DXGI_ALPHA_MODE_UNSPECIFIED;
    
    hr = IDXGIFactory4_CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)dx12_renderer->command_queue, dx12_renderer->hwnd, &swap_desc, NULL, NULL, (IDXGISwapChain1**)&dx12_renderer->swap_chain);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGIFactory4_CreateSwapChainForHwnd failed");
        return false;
    }
    
    hr = IDXGIFactory4_MakeWindowAssociation(dxgi_factory, dx12_renderer->hwnd, DXGI_MWA_NO_ALT_ENTER);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGIFactory4_MakeWindowAssociation failed");
        return false;
    }
    
    // descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };
    heap_desc.NumDescriptors = NUM_FRAMES;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    
    hr = ID3D12Device_CreateDescriptorHeap(dx12_renderer->device, &heap_desc, &IID_ID3D12DescriptorHeap, &dx12_renderer->rtv_descriptor_heap);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device_CreateDescriptorHeap failed");
        return false;
    }
    
    // render targets
    UINT rtv_descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
    
    for(size_t i=0; i<NUM_FRAMES; i++)
    {
        hr = IDXGISwapChain1_GetBuffer(dx12_renderer->swap_chain, (UINT)i, &IID_ID3D12Resource, &dx12_renderer->back_buffers[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("IDXGISwapChain1_GetBuffer failed");
            return false;
        }
        
        ID3D12Device_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->back_buffers[i], NULL, rtv_descriptor_handle);
        rtv_descriptor_handle.ptr += rtv_descriptor_size;
    }
    
    // command allocators
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        hr = ID3D12Device_CreateCommandAllocator(dx12_renderer->device, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &dx12_renderer->command_allocators[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device_CreateCommandAllocator failed");
            return false;
        }
    }
    
    // command list
    hr = ID3D12Device_CreateCommandList(dx12_renderer->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, dx12_renderer->command_allocators[0], NULL, &IID_ID3D12CommandList, &dx12_renderer->command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device_CreateCommandList failed");
        return false;
    }
    
    hr = ID3D12GraphicsCommandList_Close(dx12_renderer->command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList_Close failed");
        return false;
    }
    
    // fence
    hr = ID3D12Device_CreateFence(dx12_renderer->device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &dx12_renderer->fence);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device_CreateFence failed");
        return false;
    }
    dx12_renderer->fence_value++;
    
    dx12_renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!dx12_renderer->fence_event)
    {
        DM_LOG_FATAL("CreateEvent failed");
        hr = HRESULT_FROM_WIN32(GetLastError());
        dm_platform_win32_decode_hresult(hr);
        return false;
    }
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    // cleanup
    IDXGIFactory4_Release(dxgi_factory);
    IDXGIAdapter1_Release(dxgi_adapter1);
#ifdef DM_DEBUG
    ID3D12InfoQueue_Release(info_queue);
#endif
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocators[i]);
        ID3D12Resource_Release(dx12_renderer->back_buffers[i]);
    }
    ID3D12GraphicsCommandList_Release(dx12_renderer->command_list);
    ID3D12CommandQueue_Release(dx12_renderer->command_queue);
    
    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_descriptor_heap);
    
    IDXGISwapChain4_Release(dx12_renderer->swap_chain);
    
    ID3D12Fence_Release(dx12_renderer->fence);
    
#ifdef DM_DEBUG
    HRESULT hr;
    ID3D12DebugDevice* debug_device;
    
    hr = dx12_renderer->device->lpVtbl->QueryInterface(dx12_renderer->device, &IID_ID3D12DebugDevice, &debug_device);
    ID3D12DebugDevice_ReportLiveDeviceObjects(debug_device, D3D12_RLDO_DETAIL);
    ID3D12DebugDevice_Release(debug_device);
    ID3D12Debug_Release(dx12_renderer->debugger);
#endif
    ID3D12Device_Release(dx12_renderer->device);
    
    dm_free(context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocators[dx12_renderer->current_backbuffer_index];
    ID3D12Resource* back_buffer = dx12_renderer->back_buffers[dx12_renderer->current_backbuffer_index];
    
    ID3D12CommandAllocator_Reset(command_allocator);
    ID3D12GraphicsCommandList_Reset(dx12_renderer->command_list, command_allocator, NULL);
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = back_buffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(dx12_renderer->command_list, 1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
    const UINT size = ID3D12Device_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtv_descriptor_handle.ptr += dx12_renderer->current_backbuffer_index * size;
    
    r += 0.01f * r_dir;
    g += 0.02f * g_dir;
    b += 0.005f * b_dir;
    
    if(r < 0 || r > 1) r_dir *= -1;
    if(g < 0 || g > 1) g_dir *= -1;
    if(b < 0 || b > 1) b_dir *= -1;
    
    FLOAT clear_color[] = { r,g,b, 1.f };
    
    ID3D12GraphicsCommandList_ClearRenderTargetView(dx12_renderer->command_list, rtv_descriptor_handle, clear_color, 0, NULL);
    
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;
    
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocators[dx12_renderer->current_backbuffer_index];
    ID3D12Resource* back_buffer = dx12_renderer->back_buffers[dx12_renderer->current_backbuffer_index];
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = back_buffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(dx12_renderer->command_list, 1, &barrier);
    
    hr = ID3D12GraphicsCommandList_Close(dx12_renderer->command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandList_Close failed");
        return false;
    }
    
    ID3D12GraphicsCommandList* const command_lists[] = {
        dx12_renderer->command_list
    };
    
    ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, _countof(command_lists), (ID3D12CommandList**)command_lists);
    
    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, 1, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }
    
    //dm_dx12_fence_signal(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->frame_fence_values[dx12_renderer->current_backbuffer_index]);
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    return true;
}

void dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
}

void* dm_renderer_backend_get_internal_texture_ptr(dm_render_handle handle, dm_renderer* renderer)
{
    return NULL;
}

/*****************
RESOURCE CREATION
*******************/
bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_dynamic_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

/***************
RENDER COMMANDS
*****************/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_begin_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_end_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
}

/****************
COMPUTE COMMANDS
******************/
bool dm_compute_backend_create_shader(dm_compute_shader_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_create_buffer(size_t data_size, size_t elem_size, dm_compute_buffer_type type, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_create_uniform(size_t data_size, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_bind_buffer(dm_compute_handle handle, uint32_t offset, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_update_buffer(dm_compute_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return false;
}

void* dm_compute_backend_command_get_buffer_data(dm_compute_handle handle, dm_renderer* renderer)
{
    return NULL;
}

bool dm_compute_backend_command_bind_shader(dm_compute_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_dispatch(uint32_t threads_per_group_x, uint32_t threads_per_group_y, uint32_t threads_per_group_z, uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z, dm_renderer* renderer)
{
    return false;
}

#endif