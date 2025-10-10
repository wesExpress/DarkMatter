#include "dm.h"

#ifdef DM_DIRECTX12
#include <windows.h>

#define COBJMACROS
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#undef COBJMACROS

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
    size_t                size, count;
    
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

    D3D_PRIMITIVE_TOPOLOGY topology;
    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor;
} dm_dx12_raster_pipeline;

typedef struct dm_dx12_compute_pipeline_t
{
    ID3D12PipelineState* state;
} dm_dx12_compute_pipeline;

typedef uint16_t dm_dx12_resource_index;

typedef struct dm_dx12_resource_t
{
    dm_dx12_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_resource_index device[DM_MAX_FRAMES_IN_FLIGHT];

    uint32_t descriptor_index[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_resource;

typedef struct dm_dx12_vertex_buffer_t
{
    dm_dx12_resource resource;

    D3D12_VERTEX_BUFFER_VIEW views[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    dm_dx12_resource resource;

    DXGI_FORMAT             index_format;
    D3D12_INDEX_BUFFER_VIEW views[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    dm_dx12_resource resource;

    size_t                      size, big_size;
    void*                       mapped_addresses[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_constant_buffer;

typedef struct dm_dx12_texture_t
{
    dm_dx12_resource resource;

    DXGI_FORMAT format;
    uint8_t     bytes_per_channel, n_channels;
} dm_dx12_texture;

typedef struct dm_dx12_sampler_t
{
    uint32_t descriptor_index[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_sampler;

typedef struct dm_dx12_storage_buffer_t
{
    dm_dx12_resource resource;

    size_t                      size;
} dm_dx12_storage_buffer;

#ifdef DM_HARDWARE_RAYTRACING
typedef struct dm_dx12_sbt_t
{
    dm_dx12_resource_index index[DM_MAX_FRAMES_IN_FLIGHT];
    size_t  stride, size;
    size_t  offset[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_sbt;

typedef struct dm_dx12_raytracing_pipeline_t
{
    ID3D12StateObject*   pso;
    dm_dx12_sbt          raygen_sbt, miss_sbt, hitgroup_sbt;
} dm_dx12_raytracing_pipeline;

typedef struct dm_dx12_as_t
{
    dm_dx12_resource_index result;
    dm_dx12_resource_index scratch;
    size_t  scratch_size;
} dm_dx12_as;

typedef struct dm_dx12_blas_t
{
    dm_dx12_as as[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_blas;

typedef struct dm_dx12_tlas_t
{
    dm_dx12_as as[DM_MAX_FRAMES_IN_FLIGHT];

    dm_resource_handle instance_buffer;
} dm_dx12_tlas;

#define DM_DX12_MAX_RESOURCES ((2 + 3 + (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS + DM_MAX_TEXTURES + DM_MAX_BLAS + DM_MAX_TLAS) * 2) * DM_MAX_FRAMES_IN_FLIGHT)
#else
#define DM_DX12_MAX_RESOURCES ((2 + 3 + (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS + DM_MAX_TEXTURES) * 2) * DM_MAX_FRAMES_IN_FLIGHT)
#endif // DM_HARDWARE_RAYTRACING   

struct dm_renderer_t
{
    ID3D12Device5* device;
    IDXGISwapChain4* swap_chain;

    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* command_list[DM_MAX_FRAMES_IN_FLIGHT];

    ID3D12CommandQueue*         compute_command_queue;
    ID3D12CommandAllocator*     compute_command_allocator[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* compute_command_list[DM_MAX_FRAMES_IN_FLIGHT];

    dm_dx12_fence fences[DM_MAX_FRAMES_IN_FLIGHT];
    HANDLE        fence_event;

    dm_dx12_fence compute_fences[DM_MAX_FRAMES_IN_FLIGHT];
    HANDLE        compute_fence_event;

    uint32_t                render_targets[DM_MAX_FRAMES_IN_FLIGHT], depth_stencil_targets[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap rtv_heap, depth_stencil_heap;
    dm_dx12_descriptor_heap resource_heap[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap sampler_heap[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12RootSignature*    bindless_root_signature;
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

    dm_dx12_raster_pipeline rast_pipelines[DM_MAX_RASTER_PIPES];
    uint32_t                rast_pipe_count;

    dm_dx12_compute_pipeline compute_pipelines[DM_MAX_COMPUTE_PIPES];
    uint32_t                 comp_pipe_count;

    dm_dx12_vertex_buffer vertex_buffers[DM_MAX_VBS];
    uint32_t              vb_count;

    dm_dx12_index_buffer  index_buffers[DM_MAX_IBS];
    uint32_t              ib_count;

    dm_dx12_constant_buffer constant_buffers[DM_MAX_CBS];
    uint32_t                cb_count;

    dm_dx12_texture textures[DM_MAX_TEXTURES];
    uint32_t        texture_count;

    dm_dx12_storage_buffer storage_buffers[DM_MAX_SBS];
    uint32_t               sb_count;

    dm_dx12_sampler samplers[DM_MAX_SAMPLERS];
    uint32_t sampler_count;

#ifdef DM_HARDWARE_RAYTRACING
    dm_dx12_raytracing_pipeline rt_pipelines[DM_MAX_RT_PIPES];
    uint32_t                    rt_pipe_count;

    dm_dx12_tlas tlas[DM_MAX_TLAS];
    uint32_t     tlas_count;

    dm_dx12_blas blas[DM_MAX_BLAS];
    uint32_t     blas_count;
#endif

    ID3D12Resource* resources[DM_DX12_MAX_RESOURCES];
    uint32_t        resource_count;

    D3D12_VIEWPORT viewports[DM_MAX_VIEWPORTS];
    uint16_t viewport_count;

    D3D12_RECT scissors[DM_MAX_SCISSORS];
    uint16_t scissor_count;

    dm_pipeline_type active_pipeline_type;
    uint8_t current_frame;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
    IDXGIDebug1* dxgi_debug;
    IDXGIInfoQueue* info_queue;
#endif
};

bool dm_win32_decode_hresult(HRESULT hr);

#ifdef DM_DEBUG
void dm_dx12_get_debug_message(dm_renderer* renderer)
{
    const uint32_t num_messages = IDXGIInfoQueue_GetNumStoredMessages(renderer->info_queue, DXGI_DEBUG_ALL);
    for(uint32_t i=0; i<num_messages; i++)
    {
        size_t len = 0;
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, NULL, &len);
        DXGI_INFO_QUEUE_MESSAGE* buffer = dm_alloc(len);
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, buffer, &len);

        dm_log(DM_LOG_ERROR, "%s\n", buffer->pDescription);
        dm_free((void**)&buffer);
    }
}
#else
void dm_dx12_get_debug_message(dm_renderer* renderer) {}
#endif

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_wait_for_previous_frame(bool advance, dm_renderer* renderer)
{
    HRESULT hr;

    dm_dx12_fence* fence = &renderer->fences[renderer->current_frame];

    hr = ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value); 
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    if(advance) renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);

    fence = &renderer->fences[renderer->current_frame];

    if(ID3D12Fence_GetCompletedValue(fence->fence) < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, renderer->fence_event);
        if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

        WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
    }

    fence->value++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_descriptor_heap(uint32_t descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, dm_dx12_descriptor_heap* heap, dm_renderer* renderer)
{
    HRESULT hr;
    void* temp = NULL;

    D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
    desc.Type            = type;
    desc.NumDescriptors  = descriptor_count;
    desc.Flags          |= flags;

    hr = ID3D12Device5_CreateDescriptorHeap(renderer->device, &desc, &IID_ID3D12DescriptorHeap, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateDescriptorHeap failed"); return false; }
    heap->heap = temp;
    temp = NULL;

    heap->size = ID3D12Device5_GetDescriptorHandleIncrementSize(renderer->device, type);

    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap->heap, &heap->cpu_handle.begin);
    heap->cpu_handle.current = heap->cpu_handle.begin;

    if(flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap->heap, &heap->gpu_handle.begin);
        heap->gpu_handle.current = heap->gpu_handle.begin;
    }

    return true;
}

bool dm_renderer_init(dm_context* context)
{
    dm_renderer renderer = { 0 };

#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing DirectX12 backend...");
#endif
     HRESULT hr;

    void* temp = NULL;

    // create device
#ifdef DM_DEBUG
    hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12GetDebugInterface failed"); return false; }
    renderer.debug = temp;
    temp = NULL;

    ID3D12Debug_EnableDebugLayer(renderer.debug);

    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "DXGIGetDebugInterface failed"); return false; }
    renderer.dxgi_debug = temp;
    temp = NULL;

    hr = IDXGIDebug1_QueryInterface(renderer.dxgi_debug, &IID_IDXGIInfoQueue, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGIDebug1_QueryInterface failed"); return false; }
    renderer.info_queue = temp;
    temp = NULL;
#endif

    hr = CreateDXGIFactory1(&IID_IDXGIFactory4, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "CreateDXGIFactory1 failed"); return false; }
    renderer.factory = temp;
    temp = NULL;

    int adapter_index = 0;
    bool found = false;

    DXGI_ADAPTER_DESC1 adapter_desc = { 0 };
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_1;

    while(IDXGIFactory4_EnumAdapters1(renderer.factory, adapter_index, &renderer.adapter) != DXGI_ERROR_NOT_FOUND)
    {
        IDXGIAdapter1_GetDesc1(renderer.adapter, &adapter_desc);

        if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter_index++;
            continue;
        }

        hr = D3D12CreateDevice((IUnknown*)renderer.adapter, feature_level, &IID_ID3D12Device, NULL);
        if(SUCCEEDED(hr))
        {
            found = true;
            break;
        }

        adapter_index++;
    }

    if(!found) { dm_log(DM_LOG_FATAL, "No suitable DirectX12 adapter found"); return false; }

    hr = D3D12CreateDevice((IUnknown*)renderer.adapter, feature_level, &IID_ID3D12Device5, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12CreateDevice failed"); return false; }

    dm_log(DM_LOG_INFO, "Device: %ls", adapter_desc.Description);

    renderer.device = temp;
    temp = NULL;

    // command queue
    D3D12_COMMAND_QUEUE_DESC desc = { 0 };
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = ID3D12Device5_CreateCommandQueue(renderer.device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer.command_queue = temp;
    temp = NULL;

    hr = ID3D12Device5_CreateCommandQueue(renderer.device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer.compute_command_queue = temp;
    temp = NULL;

    // swap chain
    DXGI_SAMPLE_DESC sample_desc = { 0 };
    sample_desc.Count = 1;

    DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
    swap_desc.BufferCount  = DM_MAX_FRAMES_IN_FLIGHT;
    swap_desc.Width        = dm_get_window_width(context);
    swap_desc.Height       = dm_get_window_height(context);
    swap_desc.BufferUsage  = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.SampleDesc   = sample_desc;
    swap_desc.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;        

    HWND hwnd = GetActiveWindow();

    hr = IDXGIFactory4_CreateSwapChainForHwnd(renderer.factory, (IUnknown*)renderer.command_queue, hwnd, &swap_desc, NULL,NULL, (IDXGISwapChain1**)&renderer.swap_chain);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGIFactory4_CreateSwapChainForHwnd failed"); return false; }

    IDXGIFactory4_MakeWindowAssociation(renderer.factory, hwnd, DXGI_MWA_NO_ALT_ENTER); 

    renderer.current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer.swap_chain);

    // command allocators and lists
    const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer.device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer.command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer.device, 0, type, renderer.command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer.command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer.command_list[i]);
    }

    // open first for initial recording
    ID3D12GraphicsCommandList7_Reset(renderer.command_list[0], renderer.command_allocator[0], NULL);

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer.device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer.compute_command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer.device, 0, type, renderer.compute_command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer.compute_command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer.compute_command_list[i]);
    }

    // rtv heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer.rtv_heap, &renderer)) return false;

    // depth stencil heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer.depth_stencil_heap, &renderer)) return false;

    // resource heap(s)
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer.resource_heap[i], &renderer)) return false;
        if(!dm_dx12_create_descriptor_heap(DM_MAX_SAMPLERS, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer.sampler_heap[i], &renderer)) return false;
    }

    // bindless root signature 
    D3D12_ROOT_PARAMETER1 root_params[1] = { 0 };
    root_params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].Constants.ShaderRegister = 0;
    root_params[0].Constants.RegisterSpace  = 0;
    root_params[0].Constants.Num32BitValues = 10;
    root_params[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = { 0 };
    root_sig_desc.Version                = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_sig_desc.Desc_1_1.Flags         = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
    root_sig_desc.Desc_1_1.NumParameters = 1;
    root_sig_desc.Desc_1_1.pParameters   = root_params;

    ID3DBlob* blob = NULL;
    hr = D3D12SerializeVersionedRootSignature(&root_sig_desc, &blob, NULL);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "D3D12SerializeVersionedRootSignature failed"); return false; }

    hr = ID3D12Device_CreateRootSignature(renderer.device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateRootSignature failed"); return false; }
    renderer.bindless_root_signature = temp;

    temp = NULL;
    ID3D10Blob_Release(blob);

    // render targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = IDXGISwapChain4_GetBuffer(renderer.swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer.resources[renderer.resource_count] = temp;
        temp = NULL;
        ID3D12Resource* rt = renderer.resources[renderer.resource_count];
        renderer.render_targets[i] = renderer.resource_count++;

        D3D12_RENDER_TARGET_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer.rtv_heap.cpu_handle.current;

        ID3D12Device5_CreateRenderTargetView(renderer.device, rt, &view_desc, *handle);

        handle->ptr += renderer.rtv_heap.size;
    }

    // depth stencil targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = dm_get_window_width(context);
        resource_desc.Height           = dm_get_window_height(context);
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        void* temp = NULL;
        hr = ID3D12Device5_CreateCommittedResource(renderer.device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed"); dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed"); return false; }
        renderer.resources[renderer.resource_count] = temp;
        renderer.depth_stencil_targets[i] = renderer.resource_count++;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer.depth_stencil_heap.cpu_handle.current;

        ID3D12Device5_CreateDepthStencilView(renderer.device, renderer.resources[renderer.depth_stencil_targets[i]], &view_desc, *handle);

        handle->ptr += renderer.depth_stencil_heap.size;
    }

    // fence stuff
    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer.device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer.fences[i].fence = temp;
        temp = NULL;
    }

    renderer.fences[0].value = 1;

    renderer.fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer.fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer.device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer.compute_fences[i].fence = temp;
        temp = NULL;
    }

    renderer.compute_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer.compute_fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }

    //
    context->renderer = dm_alloc(sizeof(dm_renderer));
    dm_memcpy(context->renderer, &renderer, sizeof(renderer));

    return true;
}

bool dm_renderer_finish_init(dm_renderer* renderer)
{
    HRESULT hr;

    ID3D12CommandQueue* command_queue = renderer->command_queue;

    ID3D12CommandList*  command_lists[] = { (ID3D12CommandList*)renderer->command_list[0] };

    ID3D12GraphicsCommandList7_Close(renderer->command_list[0]);
    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    hr =ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[0].fence, renderer->fences[0].value);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    hr = ID3D12Fence_SetEventOnCompletion(renderer->fences[0].fence, renderer->fences[0].value, renderer->fence_event);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

    WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);

    renderer->fences[0].value++;
    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->current_frame = i;
        ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[i].fence, renderer->fences[i].value);
        if(!dm_dx12_wait_for_previous_frame(false, renderer)) { dm_log(DM_LOG_ERROR, "Waiting for previous frame failed"); continue; }
    }

    for(uint32_t i=0; i<renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_MAX_FRAMES_IN_FLIGHT; j++)
        {
            ID3D12Resource_Unmap(renderer->resources[renderer->constant_buffers[i].resource.device[j]], 0,0);
            renderer->constant_buffers[i].mapped_addresses[j] = NULL;
        }
    }

    for(uint32_t i=0; i<renderer->resource_count; i++)
    {
        ID3D12Resource_Release(renderer->resources[i]);
    }

    for(uint32_t i=0; i<renderer->rast_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rast_pipelines[i].state);
    }

#ifdef DM_HARDWARE_RAYTRACING
    for(uint32_t i=0; i<renderer->rt_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rt_pipelines[i].pso);
    }
#endif

    for(uint32_t i=0; i<renderer->comp_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->compute_pipelines[i].state);
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12GraphicsCommandList7_Release(renderer->compute_command_list[i]);
        ID3D12CommandAllocator_Release(renderer->compute_command_allocator[i]);
        ID3D12Fence_Release(renderer->compute_fences[i].fence);

        ID3D12GraphicsCommandList7_Release(renderer->command_list[i]);
        ID3D12CommandAllocator_Release(renderer->command_allocator[i]);
        ID3D12Fence_Release(renderer->fences[i].fence);

        ID3D12DescriptorHeap_Release(renderer->resource_heap[i].heap);
        ID3D12DescriptorHeap_Release(renderer->sampler_heap[i].heap);
    }

    CloseHandle(renderer->compute_fence_event);
    CloseHandle(renderer->fence_event);

    ID3D12RootSignature_Release(renderer->bindless_root_signature);
    ID3D12DescriptorHeap_Release(renderer->rtv_heap.heap);
    ID3D12DescriptorHeap_Release(renderer->depth_stencil_heap.heap);
    IDXGISwapChain4_Release(renderer->swap_chain);
    ID3D12CommandQueue_Release(renderer->compute_command_queue);
    ID3D12CommandQueue_Release(renderer->command_queue);
    ID3D12Device5_Release(renderer->device);

#ifdef DM_DEBUG
    IDXGIDebug1* dbg = NULL;
    void* temp = NULL;
    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_ERROR, "ID3D12Debug_QueryInterface failed"); ID3D12Debug_Release(renderer->debug); }
    else
    {
        dbg = temp;
        temp = NULL;
        ID3D12Debug_Release(renderer->debug);
        IDXGIDebug1_ReportLiveObjects(dbg, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        IDXGIDebug1_Release(dbg);
    }
#endif // DM_DEBUG
    
    dm_free((void**)&context->renderer);
}

bool dm_renderer_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_dx12_fence* fence = &renderer->fences[i];
        size_t fence_value = fence->value++;
        ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value);
        if(ID3D12Fence_GetCompletedValue(fence->fence) < fence_value)
        {
            ID3D12Fence_SetEventOnCompletion(fence->fence, fence_value, renderer->fence_event);
            WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
        }
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(renderer->resources[renderer->render_targets[i]]);
        ID3D12Resource_Release(renderer->resources[renderer->depth_stencil_targets[i]]);
    }

    hr = IDXGISwapChain4_ResizeBuffers(renderer->swap_chain, DM_MAX_FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_ResizeBuffers failed"); return false; }

    void* temp = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle = renderer->rtv_heap.cpu_handle.begin;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_target_handle  = renderer->depth_stencil_heap.cpu_handle.begin;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* render_target = renderer->resources[renderer->render_targets[i]];

        hr = IDXGISwapChain4_GetBuffer(renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer->resources[renderer->render_targets[i]] = temp;
        temp = NULL;

        D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        ID3D12Device_CreateRenderTargetView(renderer->device, renderer->resources[renderer->render_targets[i]], &desc, render_target_handle);
        render_target_handle.ptr += renderer->rtv_heap.size;

        // depth stencil
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        hr = ID3D12Device5_CreateCommittedResource(renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_win32_decode_hresult(hr) || !temp)
        {
            dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed");
            dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed");
            return false;
        }
        renderer->resources[renderer->depth_stencil_targets[i]] = temp;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        ID3D12Device5_CreateDepthStencilView(renderer->device, renderer->resources[renderer->depth_stencil_targets[i]], &view_desc, depth_target_handle);
        depth_target_handle.ptr += renderer->depth_stencil_heap.size;
    }

    renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);
    return true;
}

uint32_t dm_get_resource_index(dm_resource_handle handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    const uint8_t current_frame = renderer->current_frame;

    switch(handle.type)
    {
        case DM_RESOURCE_TYPE_VERTEX_BUFFER:   return renderer->vertex_buffers[handle.index].resource.descriptor_index[current_frame];
        case DM_RESOURCE_TYPE_INDEX_BUFFER:    return renderer->index_buffers[handle.index].resource.descriptor_index[current_frame];
        case DM_RESOURCE_TYPE_CONSTANT_BUFFER: return renderer->constant_buffers[handle.index].resource.descriptor_index[current_frame];
        case DM_RESOURCE_TYPE_STORAGE_BUFFER:  return renderer->storage_buffers[handle.index].resource.descriptor_index[current_frame];
        case DM_RESOURCE_TYPE_TEXTURE:         return renderer->textures[handle.index].resource.descriptor_index[current_frame];
        case DM_RESOURCE_TYPE_SAMPLER:         return renderer->samplers[handle.index].descriptor_index[current_frame];
        
        default: return 0;
    }
}

// === resources ===
bool dm_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_context* context)
{
    return true;
}

void dm_create_viewport(dm_viewport viewport, dm_viewport_index* index, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    D3D12_VIEWPORT v = {
        .TopLeftX=viewport.left,
        .TopLeftY=viewport.top,
        .Width=viewport.right,
        .Height=viewport.bottom,
        .MaxDepth=1.f,.MinDepth=0.f
    };

    renderer->viewports[renderer->viewport_count] = v;
    *index = renderer->viewport_count++;
}

void dm_create_scissor(dm_scissor scissor, dm_scissor_index* index, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    D3D12_RECT rect = {
        .top=scissor.top,
        .bottom=scissor.bottom,
        .left=scissor.left,
        .right=scissor.right
    };

    renderer->scissors[renderer->scissor_count] = rect;
    *index = renderer->scissor_count++;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_load_shader_data(const char* path, ID3D10Blob** blob)
{
    HRESULT hr;
    wchar_t ws[512];
    swprintf(ws, 512, L"%hs", path);
    hr = D3DReadFileToBlob(ws, blob);
    if(!dm_win32_decode_hresult(hr))
    {
        dm_log(DM_LOG_FATAL, "D3DReadFileToBlob failed");
        dm_log(DM_LOG_ERROR, "Could not load shader: %s", path);
        return false;
    }

    return true;
}

bool dm_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    void* temp = NULL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // === rasterizer ===
    switch(desc.rasterizer.cull_mode)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown cull mode. Assuming D3D12_CULL_MODE_BACK");
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
        dm_log(DM_LOG_ERROR, "Unknown polygon fill mode. Assuming D3D12_FILL_MODE_SOLID");
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
        dm_log(DM_LOG_ERROR, "Unknown front face. Assuming counter clockwise");
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = TRUE;
        break;
    }

    // TODO: needs to be configurable
    // === blending ===
    D3D12_RENDER_TARGET_BLEND_DESC blend_desc = { 0 };
    
    blend_desc.BlendEnable           = TRUE;
    blend_desc.BlendOp               = D3D12_BLEND_OP_ADD;
    blend_desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha         = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.LogicOpEnable         = FALSE;
    blend_desc.LogicOp               = D3D12_LOGIC_OP_NOOP;

    // TODO: needs to be configurable
    // === depth/stencil ===
    D3D12_DEPTH_STENCIL_DESC depth_desc = { 0 };

    depth_desc.DepthEnable      = desc.depth_stencil.depth ? TRUE : FALSE;
    depth_desc.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
    depth_desc.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_desc.StencilEnable    = FALSE;
    depth_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_desc.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;

    // === shaders ===
    ID3DBlob* vs = NULL;
    ID3DBlob* ps = NULL;
    
    const char* vertex_path = desc.rasterizer.vertex_shader_desc.path;
    const char* pixel_path  = desc.rasterizer.pixel_shader_desc.path;

    if(!dm_dx12_load_shader_data(vertex_path, &vs)) { dm_log(DM_LOG_ERROR, "Could not load vertex shader"); return false; }
    if(!dm_dx12_load_shader_data(pixel_path,  &ps)) { dm_log(DM_LOG_ERROR, "Could not load pixel shader"); return false; }
    
    pso_desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vs);
    pso_desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(vs);

    pso_desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(ps);
    pso_desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(ps);

    // === input assembler ===
    D3D12_INPUT_ELEMENT_DESC input_element_descs[DM_RENDER_MAX_INPUT_ELEMENTS] = { 0 };
    uint8_t element_index = 0;

    for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
    {
        uint8_t matrix_count = desc.input_assembler.input_elements[i].format == DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4 ? 4 : 1;

        for(uint8_t j=0; j<matrix_count; j++)
        {
            input_element_descs[element_index].SemanticName      = desc.input_assembler.input_elements[i].name; 
            input_element_descs[element_index].SemanticIndex     = j;
            input_element_descs[element_index].InputSlot         = desc.input_assembler.input_elements[i].slot;
            input_element_descs[element_index].AlignedByteOffset = i==0 ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;

            switch(desc.input_assembler.input_elements[i].format)
            {
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32_FLOAT;
                break;

                default:
                dm_log(DM_LOG_ERROR, "Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;

                case DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4:
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            }

            switch(desc.input_assembler.input_elements[i].class)
            {
                default:
                dm_log(DM_LOG_ERROR, "Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
                case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
                input_element_descs[element_index].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                break;

                case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
                input_element_descs[element_index].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                input_element_descs[element_index].InstanceDataStepRate = 1;
                input_element_descs[element_index].InputSlot            = 1;
                break;
            }

            element_index++;
        }
    }

    pso_desc.InputLayout.pInputElementDescs = input_element_descs;
    pso_desc.InputLayout.NumElements        = element_index;

    switch(desc.input_assembler.topology)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknow primitive topology. Assuming D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST");
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    }

    // === pipeline state ===
    pso_desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count           = 1;
    pso_desc.SampleMask                 = 0xffffffff;
    pso_desc.NumRenderTargets           = 1;
    pso_desc.BlendState.RenderTarget[0] = blend_desc;
    pso_desc.DepthStencilState          = depth_desc;
    pso_desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
    pso_desc.pRootSignature             = renderer->bindless_root_signature;

    hr = ID3D12Device5_CreateGraphicsPipelineState(renderer->device, &pso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp)
    {
        dm_log(DM_LOG_FATAL, "ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(renderer);
        return false;
    }
    pipeline.state = temp;
    temp = NULL;
    
    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);

    // 
    renderer->rast_pipelines[renderer->rast_pipe_count] = pipeline;
    handle->index = renderer->rast_pipe_count++;
    handle->type = DM_PIPELINE_TYPE_RASTER;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_committed_resource(D3D12_HEAP_PROPERTIES properties, D3D12_RESOURCE_DESC desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, ID3D12Device5* device)
{
    void* temp = NULL;
    HRESULT hr = ID3D12Device5_CreateCommittedResource(device, &properties, flags, &desc, state, 0, &IID_ID3D12Resource, &temp);
    if(!dm_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommittedResource failed"); return false; }

    *resource = temp;
    temp = NULL;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_copy_memory(ID3D12Resource* resource, const void* data, size_t size)
{
    void* temp = NULL;

    D3D12_RANGE range = { 0 };
    range.End = size-1;
    
    ID3D12Resource_Map(resource, 0, &range, &temp);
    if(!temp) { dm_log(DM_LOG_FATAL, "ID3D12Resource_Map failed"); return false; }

    dm_memcpy(temp, data, size);
    ID3D12Resource_Unmap(resource, 0, &range);
    
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_buffer(const size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Resource** resource, dm_renderer* renderer)
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
    desc.Flags            = flags;

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, renderer->device)) return false;  

    return true;
}
bool dm_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    dm_dx12_vertex_buffer buffer = { 0 };

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "vertex_host_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "vertex_device_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;

        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].StrideInBytes  = desc.stride;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.NumElements         = desc.size / desc.stride;
        view_desc.Buffer.StructureByteStride = desc.stride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    } 

    //
    renderer->vertex_buffers[renderer->vb_count] = buffer;
    handle->index = renderer->vb_count++;
    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    return true;
}

bool dm_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    dm_dx12_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_format = DXGI_FORMAT_R16_UINT;
        break;

        default:
        dm_log(DM_LOG_ERROR, "Unknown index type. Assuming uint32");
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_format = DXGI_FORMAT_R32_UINT;
        break;
    }

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "index_host_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "index_device_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;
        
        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].Format         = buffer.index_format;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        switch(desc.index_type)
        {
            case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
            view_desc.Buffer.StructureByteStride = sizeof(uint16_t);
            break;

            case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
            view_desc.Buffer.StructureByteStride = sizeof(uint32_t);
            break;

            default:
            dm_log(DM_LOG_FATAL, "Should NOT be here");
            return false;
        }
        view_desc.Buffer.NumElements         = desc.size / view_desc.Buffer.StructureByteStride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    }

    //
    renderer->index_buffers[renderer->ib_count] = buffer;
    handle->index = renderer->ib_count++;
    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    return true;
}

bool dm_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    dm_dx12_constant_buffer buffer = { 0 };
    const size_t aligned_size = (desc.size + 255) & ~255;
    const size_t big_size = 1024 * 64;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(big_size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d", "constant_buffer", renderer->cb_count);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;
        buffer.size     = aligned_size; 
        buffer.big_size = big_size;

        D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = { 0 };
        view_desc.SizeInBytes    = aligned_size;
        view_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(renderer->resources[buffer.resource.device[i]]);

        ID3D12Device5_CreateConstantBufferView(renderer->device, &view_desc, renderer->resource_heap[i].cpu_handle.current);

        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        hr = ID3D12Resource_Map(renderer->resources[buffer.resource.device[i]], 0,NULL, &buffer.mapped_addresses[i]);
        if(!dm_win32_decode_hresult(hr) || !buffer.mapped_addresses[i]) { dm_log(DM_LOG_FATAL, "ID3D12Resource_Map failed"); return false; }

        if(!desc.data) continue;

        dm_memcpy(buffer.mapped_addresses[i], desc.data, desc.size);
    }

    //
    dm_memcpy(renderer->constant_buffers + renderer->cb_count, &buffer, sizeof(buffer));
    handle->index = renderer->cb_count++;
    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    return true;
}

bool dm_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    dm_dx12_storage_buffer buffer = { 0 };

    buffer.size = desc.size;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d %d", "storage_buffer_host", renderer->sb_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d %d", "storage_buffer_device", renderer->sb_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;

        if(desc.data)
        {
            if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;
        }

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        // view
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_view_desc = { 0 };
        uav_view_desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        uav_view_desc.Buffer.NumElements         = desc.size / desc.stride;
        uav_view_desc.Buffer.StructureByteStride = desc.stride;

        ID3D12Device5_CreateUnorderedAccessView(renderer->device, *device_buffer, NULL, &uav_view_desc, renderer->resource_heap[i].cpu_handle.current); 

        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_view_desc = { 0 };
        srv_view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        srv_view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        srv_view_desc.Buffer.NumElements         = desc.size / desc.stride;
        srv_view_desc.Buffer.StructureByteStride = desc.stride;
        srv_view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &srv_view_desc, renderer->resource_heap[i].cpu_handle.current);

        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    }

    //
    dm_memcpy(renderer->storage_buffers + renderer->sb_count, &buffer, sizeof(buffer));
    handle->index = renderer->sb_count++;
    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_texture_resource(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Resource** resource, ID3D12Device5* device)
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
    desc.Flags            = flags;
    desc.Format           = format;

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, device)) return false;  

    return true;
}

bool dm_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    dm_dx12_texture texture = { 0 };

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    DXGI_FORMAT format;
    uint8_t     bytes_per_channel;
    size_t size = desc.width * desc.height * desc.n_channels;

    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        format            = DXGI_FORMAT_R8G8B8A8_UINT;
        bytes_per_channel = sizeof(char);
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        format            = DXGI_FORMAT_R8G8B8A8_UNORM;
        bytes_per_channel = sizeof(char);
        break;

        case DM_TEXTURE_FORMAT_FLOAT_3:
        format            = DXGI_FORMAT_R32G32B32_FLOAT;
        bytes_per_channel = sizeof(float);
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        format            = DXGI_FORMAT_R32G32B32A32_FLOAT;
        bytes_per_channel = sizeof(float);
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported texture format");
        return false;
    }

    texture.format            = format;
    texture.bytes_per_channel = bytes_per_channel;
    texture.n_channels        = desc.n_channels;

    size *= bytes_per_channel;

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host texture is actually a buffer
        ID3D12Resource** host_resource = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_resource, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "texture_host", renderer->texture_count, i);
        ID3D12Resource_SetName(*host_resource, name);
#endif
        texture.resource.host[i] = renderer->resource_count++;

        if(desc.data)
        {
            if(!dm_dx12_copy_memory(*host_resource, desc.data, size)) return false;
        }

        // device texture
        ID3D12Resource** device_resource = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_texture_resource(desc.width, desc.height, format, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, flags, device_resource, renderer->device)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "texture_device", renderer->texture_count, i);
        ID3D12Resource_SetName(*device_resource, name);
#endif
        texture.resource.device[i] = renderer->resource_count++;

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
        src.PlacedFootprint.Footprint.RowPitch = desc.width * desc.n_channels * bytes_per_channel;
        src.PlacedFootprint.Footprint.Format   = format;

        D3D12_TEXTURE_COPY_LOCATION dest = { 0 };
        dest.pResource = *device_resource;
        dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        ID3D12GraphicsCommandList7_CopyTextureRegion(command_list, &dest, 0,0,0, &src, &texture_as_box);

        // UAV
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_view_desc = { 0 };
        uav_view_desc.Format        = texture.format;
        uav_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        
        texture.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateUnorderedAccessView(renderer->device, *device_resource, NULL, &uav_view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_view_desc = { 0 };
        srv_view_desc.Format                  = texture.format;
        srv_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_view_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_view_desc.Texture2D.MipLevels     = 1;

        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_resource, &srv_view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;
    }

    //
    dm_memcpy(renderer->textures + renderer->texture_count, &texture, sizeof(texture));
    handle->index = renderer->texture_count++;
    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
D3D12_TEXTURE_ADDRESS_MODE dm_dx12_convert_address_mode(dm_sampler_address_mode mode)
{
    switch(mode)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown/unsupported address mode. Assuming D3D12_TEXTURE_ADDRESS_MODE_BORDER");
        case DM_SAMPLER_ADDRESS_MODE_BORDER:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        
        case DM_SAMPLER_ADDRESS_MODE_WRAP:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    }
}

bool dm_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    HRESULT hr;

    D3D12_SAMPLER_DESC sampler = { 0 };

    sampler.Filter         = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU       = dm_dx12_convert_address_mode(desc.address_u);
    sampler.AddressV       = dm_dx12_convert_address_mode(desc.address_v);
    sampler.AddressW       = dm_dx12_convert_address_mode(desc.address_w);
    sampler.MaxLOD         = D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Device5_CreateSampler(renderer->device, &sampler, renderer->sampler_heap[i].cpu_handle.current);

        
        renderer->samplers[renderer->sampler_count].descriptor_index[i] = renderer->sampler_heap[i].count;
        renderer->sampler_heap[i].cpu_handle.current.ptr += renderer->sampler_heap[i].size;
        renderer->sampler_heap[i].count++;
    }

    handle->index = renderer->sampler_count++;
    handle->type = DM_RESOURCE_TYPE_SAMPLER;

    return true;
}

// === commands ===
bool dm_render_command_begin_frame_backend(dm_renderer* renderer) 
{
    HRESULT hr;

    const uint8_t current_frame = renderer->current_frame;

    ID3D12CommandAllocator*     command_allocator = renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    hr = ID3D12CommandAllocator_Reset(command_allocator);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandAllocator_Reset failed"); return false; }

    hr = ID3D12GraphicsCommandList7_Reset(command_list, command_allocator, NULL);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12GraphicsCommandList7_Reset failed"); return false; }

    ID3D12DescriptorHeap* heaps[] = { renderer->resource_heap[current_frame].heap, renderer->sampler_heap[current_frame].heap };
    ID3D12GraphicsCommandList7_SetDescriptorHeaps(command_list, DM_COUNTOF(heaps), heaps);

    ID3D12GraphicsCommandList7_SetGraphicsRootSignature(command_list, renderer->bindless_root_signature);
    ID3D12GraphicsCommandList7_SetComputeRootSignature(command_list, renderer->bindless_root_signature);

    renderer->active_pipeline_type = DM_PIPELINE_TYPE_INVALID;

    return true; 
}

bool dm_render_command_end_frame_backend(bool vsync, dm_renderer* renderer) 
{ 
    HRESULT hr;

    const uint8_t current_frame = renderer->current_frame;

    ID3D12CommandQueue*         command_queue = renderer->command_queue;
    ID3D12CommandAllocator*     command_allocator = renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    hr = ID3D12GraphicsCommandList7_Close(command_list);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12GraphicsCommandList7_Close failed"); return false; }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    UINT present_flag = 0;
    present_flag = DXGI_PRESENT_ALLOW_TEARING;

    hr = IDXGISwapChain4_Present(renderer->swap_chain, vsync, present_flag);
    if(!dm_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_Present failed"); return false; }

    if(!dm_dx12_wait_for_previous_frame(true, renderer)) { dm_log(DM_LOG_FATAL, "Waiting for previous frame failed"); return false; }

    return true; 
}

bool dm_render_command_begin_update_backend(dm_renderer* renderer) { return true; }
bool dm_render_command_end_update_backend(dm_renderer* renderer) { return true; }

void dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = renderer->resources[renderer->render_targets[current_frame]];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    
    // rtv heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = renderer->rtv_heap.cpu_handle.begin;
    rtv_handle.ptr += current_frame * renderer->rtv_heap.size;

    // depth stencil heap
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = renderer->depth_stencil_heap.cpu_handle.begin;
    dsv_handle.ptr += current_frame * renderer->depth_stencil_heap.size;

    float clear_color[] = { r,g,b,a };

    ID3D12GraphicsCommandList7_OMSetRenderTargets(command_list, 1, &rtv_handle, FALSE, &dsv_handle);
    ID3D12GraphicsCommandList7_ClearRenderTargetView(command_list, rtv_handle, clear_color, 0, NULL);
    ID3D12GraphicsCommandList7_ClearDepthStencilView(command_list, dsv_handle, D3D12_CLEAR_FLAG_DEPTH, depth,0, 0, NULL);
}

void dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, dm_renderer* renderer) 
{
    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = renderer->resources[renderer->render_targets[current_frame]];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
}

void dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];
    dm_dx12_raster_pipeline pipeline = renderer->rast_pipelines[handle.index];

    ID3D12GraphicsCommandList7_SetPipelineState(command_list, pipeline.state);
    ID3D12GraphicsCommandList7_IASetPrimitiveTopology(command_list, pipeline.topology);

    renderer->active_pipeline_type = DM_PIPELINE_TYPE_RASTER;
}

void dm_render_command_set_viewport_backend(dm_viewport_index index, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_RSSetViewports(command_list, 1, &renderer->viewports[index]);
}

void dm_render_command_set_scissor_backend(dm_scissor_index index, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_RSSetScissorRects(command_list, 1, &renderer->scissors[index]);
}

bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, dm_renderer* renderer) 
{
    const uint8_t current_frame = renderer->current_frame;

    uint32_t indices[10] = { 0 };

    for(uint16_t i=0; i<count ;i++)
    {
        switch(handles[i].type)
        {
            case DM_RESOURCE_TYPE_VERTEX_BUFFER:
            indices[i] = renderer->vertex_buffers[handles[i].index].resource.descriptor_index[current_frame];
            break;

            case DM_RESOURCE_TYPE_INDEX_BUFFER:
            indices[i] = renderer->index_buffers[handles[i].index].resource.descriptor_index[current_frame];
            break;

            case DM_RESOURCE_TYPE_CONSTANT_BUFFER:
            indices[i] = renderer->constant_buffers[handles[i].index].resource.descriptor_index[current_frame];
            break;

            case DM_RESOURCE_TYPE_STORAGE_BUFFER:
            indices[i] = renderer->storage_buffers[handles[i].index].resource.descriptor_index[current_frame];
            break;

            case DM_RESOURCE_TYPE_TEXTURE:
            indices[i] = renderer->textures[handles[i].index].resource.descriptor_index[current_frame];
            break;

            case DM_RESOURCE_TYPE_SAMPLER:
            indices[i] = renderer->samplers[handles[i].index].descriptor_index[current_frame];
            break;

            default:
            dm_log(DM_LOG_FATAL, "Unknown/unsupported resource type. Should NOT be here...");
            return false;
        }
    }

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    switch(renderer->active_pipeline_type)
    {
        case DM_PIPELINE_TYPE_RASTER:
        ID3D12GraphicsCommandList7_SetGraphicsRoot32BitConstants(command_list, 0, count, indices, 0);
        break;

#if 0
        case DM_PIPELINE_TYPE_COMPUTE:
        //case DM_PIPELINE_TYPE_RAYTRACING:
        ID3D12GraphicsCommandList7_SetComputeRoot32BitConstants(command_list, 0, count, indices, 0);
        break;
#endif
            
        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported pipeline type bound. Should NOT be here.");
        return false;
    }

    return true; 
}

void dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];
    dm_dx12_vertex_buffer vb = renderer->vertex_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetVertexBuffers(command_list, slot, 1, &vb.views[current_frame]);
}

void dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, dm_renderer* renderer) 
{
    const uint8_t current_frame = renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];
    dm_dx12_index_buffer ib = renderer->index_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetIndexBuffer(command_list, &ib.views[current_frame]);
}

#ifndef DM_DEBUG
DM_INLINE
#endif
void dm_dx12_update_buffer(ID3D12Resource* source, ID3D12Resource* dest, size_t size, D3D12_RESOURCE_STATES state, ID3D12GraphicsCommandList7* command_list)
{
    D3D12_RESOURCE_BARRIER barriers[2] = { 0 };

    barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.StateBefore = state;
    barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.pResource   = dest;

    barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter  = state;
    barriers[1].Transition.pResource   = dest;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[0]);
    ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, dest, 0, source, 0, size);
    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[1]);
}

bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer) 
{
    dm_dx12_vertex_buffer buffer = renderer->vertex_buffers[handle.index];

    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12Resource* host_buffer   = renderer->resources[buffer.resource.host[current_frame]];
    ID3D12Resource* device_buffer = renderer->resources[buffer.resource.device[current_frame]];

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    if(!dm_dx12_copy_memory(host_buffer, data, size)) return false;
    dm_dx12_update_buffer(host_buffer, device_buffer, size, state, command_list);

    return true; 
}

bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer) 
{ 
    dm_dx12_index_buffer buffer = renderer->index_buffers[handle.index];

    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12Resource* host_buffer   = renderer->resources[buffer.resource.host[current_frame]];
    ID3D12Resource* device_buffer = renderer->resources[buffer.resource.device[current_frame]];

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if(!dm_dx12_copy_memory(host_buffer, data, size)) return false;
    dm_dx12_update_buffer(host_buffer, device_buffer, size, state, command_list);

    return true; 
}

bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer) 
{
    dm_dx12_constant_buffer* buffer = &renderer->constant_buffers[handle.index];

    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];
    dm_memcpy(buffer->mapped_addresses[current_frame], data, size);

    return true;
}

bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer) 
{ 
    dm_dx12_storage_buffer buffer = renderer->storage_buffers[handle.index];

    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12Resource* host_buffer   = renderer->resources[buffer.resource.host[current_frame]];
    ID3D12Resource* device_buffer = renderer->resources[buffer.resource.device[current_frame]];

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

    if(!dm_dx12_copy_memory(host_buffer, data, size)) return false;
    dm_dx12_update_buffer(host_buffer, device_buffer, size, state, command_list);

    return true; 
}

bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_renderer* renderer) { return true; }

void dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_renderer* renderer) 
{
    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_DrawInstanced(command_list, vertex_count, instance_count, vertex_offset, instance_offset);
}

void dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_renderer* renderer) 
{ 
    const uint8_t current_frame = renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    ID3D12GraphicsCommandList7_DrawIndexedInstanced(command_list, index_count, instance_count, index_offset, vertex_offset, instance_offset);
}

// === misc ===
bool dm_win32_decode_hresult(HRESULT hr)
{
    if (hr == S_OK) return true;
    
    dm_log(DM_LOG_FATAL, "Hresult failed with:");
    
    switch (hr)
    {
        case E_ABORT: 
        dm_log(DM_LOG_ERROR, "Operation aborted");
        break;
        
        case E_ACCESSDENIED:
        dm_log(DM_LOG_ERROR, "General access denied error");
        break;
        
        case E_FAIL:
        dm_log(DM_LOG_ERROR, "Unspecified failure");
        break;
        
        case E_HANDLE:
        dm_log(DM_LOG_ERROR, "Handle that is not valid");
        break;
        
        case E_INVALIDARG:
        dm_log(DM_LOG_ERROR, "One or more arguments are not valid");
        break;
        
        case E_NOINTERFACE:
        dm_log(DM_LOG_ERROR, "No such interface supported");
        break;
        
        case E_NOTIMPL:
        dm_log(DM_LOG_ERROR, "Not implemented");
        break;
        
        case E_OUTOFMEMORY:
        dm_log(DM_LOG_ERROR, "Failed to allocate necessary memory");
        break;
        
        case E_POINTER:
        dm_log(DM_LOG_ERROR, "Pointer that is not valid");
        break;
        
        case E_UNEXPECTED:
        dm_log(DM_LOG_ERROR, "Unexpected failure");
        break;
        
        case 0x80070002:
        dm_log(DM_LOG_ERROR, "File not found");
        break;
        
        case 0x887A0005:
        dm_log(DM_LOG_ERROR, "DXGI Error: Device removed: Device hung");
        break;
        
        case 0x887A0006:
        dm_log(DM_LOG_ERROR, "GPU will not respond to more commands due to invalid command");
        break;

        case 0x8876086c:
        dm_log(DM_LOG_ERROR, "Invalid call");
        break;
        
        default:
        dm_log(DM_LOG_ERROR, "Unknown error: %u", hr);
        break;
    }
    
    return false;
}
#endif // DM_DIRECTX12
