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

typedef struct dm_dx12_vertex_buffer_t
{
    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    D3D12_VERTEX_BUFFER_VIEW views[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    DXGI_FORMAT             index_format;

    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    D3D12_INDEX_BUFFER_VIEW views[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    size_t                      size, big_size;
    void*                       mapped_addresses[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_constant_buffer;

typedef struct dm_dx12_texture_t
{
    uint8_t host_textures[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_textures[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    DXGI_FORMAT format;
    uint8_t     bytes_per_channel, n_channels;
} dm_dx12_texture;

typedef struct dm_dx12_storage_buffer_t
{
    uint8_t host_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint8_t device_buffers[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    size_t                      size;
} dm_dx12_storage_buffer;

typedef struct dm_dx12_sbt_t
{
    uint8_t index[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    size_t  stride, size;
} dm_dx12_sbt;

typedef struct dm_dx12_raytracing_pipeline_t
{
    ID3D12StateObject*   pso;
    dm_dx12_sbt          raygen_sbt, miss_sbt, hitgroup_sbt;
    uint32_t             max_instance_count, hit_group_count;
} dm_dx12_raytracing_pipeline;

typedef struct dm_dx12_as_t
{
    uint8_t result;
    uint8_t scratch;
    size_t  scratch_size;
} dm_dx12_as;

typedef struct dm_dx12_blas_t
{
    dm_dx12_as as[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_blas;

typedef struct dm_dx12_tlas_t
{
    dm_dx12_as as[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    dm_resource_handle instance_buffer;
    //D3D12_RAYTRACING_INSTANCE_DESC* instance_data[DM_DX12_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_tlas;

#define DM_DX12_MAX_RAST_PIPES 10
#define DM_DX12_MAX_RT_PIPES   10
#define DM_DX12_MAX_COMP_PIPES 10

#define DM_DX12_MAX_VBS        100
#define DM_DX12_MAX_IBS        100
#define DM_DX12_MAX_CBS        100
#define DM_DX12_MAX_SBS        100
#define DM_DX12_MAX_AS         10 
#define DM_DX12_MAX_TEXTURES   100

#define DM_DX12_MAX_CBV 1000
#define DM_DX12_MAX_UBV 1000
#define DM_DX12_MAX_SRV 1000
#define DM_DX12_MAX_RESOURCES (DM_DX12_MAX_CBV + DM_DX12_MAX_UBV + DM_DX12_MAX_SRV)

typedef struct dm_dx12_renderer_t
{
    ID3D12Device5*   device;
    IDXGISwapChain4* swap_chain;

    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* command_list[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    ID3D12CommandQueue*         compute_command_queue;
    ID3D12CommandAllocator*     compute_command_allocator[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* compute_command_list[DM_DX12_MAX_FRAMES_IN_FLIGHT];

    dm_dx12_fence fences[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    HANDLE        fence_event;

    dm_dx12_fence compute_fences[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    HANDLE        compute_fence_event;

    uint32_t                render_targets[DM_DX12_MAX_FRAMES_IN_FLIGHT], depth_stencil_targets[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap rtv_heap, depth_stencil_heap;
    dm_dx12_descriptor_heap resource_heap[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap sampler_heap[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    ID3D12RootSignature*    bindless_root_signature;
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

    dm_dx12_raster_pipeline rast_pipelines[DM_DX12_MAX_RAST_PIPES];
    uint32_t                rast_pipe_count;

    dm_dx12_raytracing_pipeline rt_pipelines[DM_DX12_MAX_FRAMES_IN_FLIGHT];
    uint32_t                    rt_pipe_count;
    
    dm_dx12_compute_pipeline compute_pipelines[DM_DX12_MAX_COMP_PIPES];
    uint32_t                 comp_pipe_count;

    dm_dx12_vertex_buffer vertex_buffers[DM_DX12_MAX_VBS];
    uint32_t              vb_count;

    dm_dx12_index_buffer  index_buffers[DM_DX12_MAX_IBS];
    uint32_t              ib_count;

    dm_dx12_constant_buffer constant_buffers[DM_DX12_MAX_CBS];
    uint32_t                cb_count;

    dm_dx12_texture textures[DM_DX12_MAX_TEXTURES];
    uint32_t        texture_count;

    dm_dx12_storage_buffer storage_buffers[DM_DX12_MAX_SBS];
    uint32_t               sb_count;

    dm_dx12_tlas tlas[DM_DX12_MAX_AS];
    uint32_t     tlas_count;

    dm_dx12_blas blas[DM_DX12_MAX_AS];
    uint32_t     blas_count;

    ID3D12Resource* resources[DM_DX12_MAX_RESOURCES];
    uint32_t        resource_count;

    dm_pipeline_type active_pipeline_type;

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
bool dm_dx12_wait_for_previous_frame(bool advance, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;

    dm_dx12_fence* fence = &dx12_renderer->fences[dx12_renderer->current_frame];

    hr = ID3D12CommandQueue_Signal(dx12_renderer->command_queue, fence->fence, fence->value); 
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandQueue_Signal failed");
        return false;
    }

    if(advance) dx12_renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(dx12_renderer->swap_chain);

    fence = &dx12_renderer->fences[dx12_renderer->current_frame];

    if(ID3D12Fence_GetCompletedValue(fence->fence) < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, dx12_renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Fence_SetEventOnCompletion failed");
            return false;
        }

        WaitForSingleObjectEx(dx12_renderer->fence_event, INFINITE, FALSE);
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
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_1;

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

        hr = ID3D12Device5_CreateCommandQueue(dx12_renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandQueue failed");
            return false;
        }
        dx12_renderer->compute_command_queue = temp;
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

        // render
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

            ID3D12GraphicsCommandList7_Close(dx12_renderer->command_list[i]);
        }

        // open first for initial recording
        ID3D12GraphicsCommandList7_Reset(dx12_renderer->command_list[0], dx12_renderer->command_allocator[0], NULL);

        // compute
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            hr = ID3D12Device5_CreateCommandAllocator(dx12_renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
                return false;
            }
            dx12_renderer->compute_command_allocator[i] = temp;
            temp = NULL;

            hr = ID3D12Device5_CreateCommandList(dx12_renderer->device, 0, type, dx12_renderer->compute_command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandList failed");
                return false;
            }
            dx12_renderer->compute_command_list[i] = temp;
            temp = NULL;

            ID3D12GraphicsCommandList7_Close(dx12_renderer->compute_command_list[i]);
        }
    }

    // rtv heap
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &dx12_renderer->rtv_heap, dx12_renderer)) return false;
    }

    // depth stencil heap
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &dx12_renderer->depth_stencil_heap, dx12_renderer)) return false;
    }

    // resource heap(s)
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &dx12_renderer->resource_heap[i], dx12_renderer)) return false;
            if(!dm_dx12_create_descriptor_heap(100, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &dx12_renderer->sampler_heap[i], dx12_renderer)) return false;
        }

        // bindless heap
        D3D12_ROOT_PARAMETER1 root_params[1] = { 0 };
        root_params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        root_params[0].Constants.ShaderRegister = 0;
        root_params[0].Constants.RegisterSpace  = 0;
        root_params[0].Constants.Num32BitValues = DM_MAX_ROOT_CONSTANTS;
        root_params[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_STATIC_SAMPLER_DESC sampler = { 0 };
        sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MaxLOD           = D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = { 0 };
        root_sig_desc.Version                = D3D_ROOT_SIGNATURE_VERSION_1_1;
        root_sig_desc.Desc_1_1.Flags         = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
        root_sig_desc.Desc_1_1.NumParameters = 1;
        root_sig_desc.Desc_1_1.pParameters   = root_params;
        root_sig_desc.Desc_1_1.NumStaticSamplers = 1;
        root_sig_desc.Desc_1_1.pStaticSamplers = &sampler;

        ID3DBlob* blob = NULL;
        void* temp = NULL;
        hr = D3D12SerializeVersionedRootSignature(&root_sig_desc, &blob, NULL);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("D3D12SerializeVersionedRootSignature failed");
            return false;
        }

        hr = ID3D12Device_CreateRootSignature(dx12_renderer->device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device_CreateRootSignature failed");
            return false;
        }
        dx12_renderer->bindless_root_signature = temp;

        temp = NULL;
        ID3D10Blob_Release(blob);
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
            dx12_renderer->resources[dx12_renderer->resource_count] = temp;
            temp = NULL;
            ID3D12Resource* rt = dx12_renderer->resources[dx12_renderer->resource_count];
            dx12_renderer->render_targets[i] = dx12_renderer->resource_count++;

            D3D12_RENDER_TARGET_VIEW_DESC view_desc = { 0 };
            view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
            view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            D3D12_CPU_DESCRIPTOR_HANDLE* handle = &dx12_renderer->rtv_heap.cpu_handle.current;

            ID3D12Device5_CreateRenderTargetView(dx12_renderer->device, rt, &view_desc, *handle);

            handle->ptr += dx12_renderer->rtv_heap.size;
        }
    }

    // depth stencil targets
    {
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
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
            resource_desc.Width            = context->renderer.width;
            resource_desc.Height           = context->renderer.height;
            resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            resource_desc.SampleDesc.Count = 1;
            resource_desc.DepthOrArraySize = 1;
            resource_desc.MipLevels        = 1;

            void* temp = NULL;
            hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device_CreateCommittedResource failed");
                DM_LOG_ERROR("Creating depth stencil buffer failed");
                return false;
            }
            dx12_renderer->resources[dx12_renderer->resource_count] = temp;
            dx12_renderer->depth_stencil_targets[i] = dx12_renderer->resource_count++;

            D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
            view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
            view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            view_desc.Flags         = D3D12_DSV_FLAG_NONE;

            D3D12_CPU_DESCRIPTOR_HANDLE* handle = &dx12_renderer->depth_stencil_heap.cpu_handle.current;

            ID3D12Device5_CreateDepthStencilView(dx12_renderer->device, dx12_renderer->resources[dx12_renderer->depth_stencil_targets[i]], &view_desc, *handle);

            handle->ptr += dx12_renderer->depth_stencil_heap.size;
        }
    }

    // fence stuff
    {
        // render
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

        dx12_renderer->fences[0].value = 1;

        dx12_renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!dx12_renderer->fence_event)
        {
            DM_LOG_FATAL("CreateEvent failed");
            return false;
        }

        // compute
        for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
        {
            D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
            hr = ID3D12Device5_CreateFence(dx12_renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
            if(!dm_platform_win32_decode_hresult(hr) || !temp)
            {
                DM_LOG_FATAL("ID3D12Device5_CreateFence failed");
                return false;
            }
            dx12_renderer->compute_fences[i].fence = temp;
            temp = NULL;
        }

        dx12_renderer->compute_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!dx12_renderer->compute_fence_event)
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

    ID3D12CommandList*  command_lists[] = { (ID3D12CommandList*)dx12_renderer->command_list[0] };

    ID3D12GraphicsCommandList7_Close(dx12_renderer->command_list[0]);
    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    hr =ID3D12CommandQueue_Signal(dx12_renderer->command_queue, dx12_renderer->fences[0].fence, dx12_renderer->fences[0].value);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandQueue_Signal failed");
        return false;
    }

    hr = ID3D12Fence_SetEventOnCompletion(dx12_renderer->fences[0].fence, dx12_renderer->fences[0].value, dx12_renderer->fence_event);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Fence_SetEventOnCompletion failed");
        return false;
    }

    WaitForSingleObjectEx(dx12_renderer->fence_event, INFINITE, FALSE);

    dx12_renderer->fences[0].value++;

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
        if(!dm_dx12_wait_for_previous_frame(false, dx12_renderer))
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
        ID3D12PipelineState_Release(dx12_renderer->rast_pipelines[i].state);
    }

    for(uint32_t i=0; i<dx12_renderer->rt_pipe_count; i++)
    {
        ID3D12PipelineState_Release(dx12_renderer->rt_pipelines[i].pso);
    }

    for(uint32_t i=0; i<dx12_renderer->comp_pipe_count; i++)
    {
        ID3D12PipelineState_Release(dx12_renderer->compute_pipelines[i].state);
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12GraphicsCommandList7_Release(dx12_renderer->compute_command_list[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->compute_command_allocator[i]);
        ID3D12Fence_Release(dx12_renderer->compute_fences[i].fence);

        ID3D12GraphicsCommandList7_Release(dx12_renderer->command_list[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocator[i]);
        ID3D12Fence_Release(dx12_renderer->fences[i].fence);

        ID3D12DescriptorHeap_Release(dx12_renderer->resource_heap[i].heap);
        ID3D12DescriptorHeap_Release(dx12_renderer->sampler_heap[i].heap);
    }

    CloseHandle(dx12_renderer->compute_fence_event);
    CloseHandle(dx12_renderer->fence_event);

    ID3D12RootSignature_Release(dx12_renderer->bindless_root_signature);
    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_heap.heap);
    ID3D12DescriptorHeap_Release(dx12_renderer->depth_stencil_heap.heap);
    IDXGISwapChain4_Release(dx12_renderer->swap_chain);
    ID3D12CommandQueue_Release(dx12_renderer->compute_command_queue);
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

    ID3D12DescriptorHeap* heaps[] = { dx12_renderer->resource_heap[current_frame].heap, dx12_renderer->sampler_heap[current_frame].heap };
    ID3D12GraphicsCommandList7_SetDescriptorHeaps(command_list, _countof(heaps), heaps);

    dx12_renderer->active_pipeline_type = DM_PIPELINE_TYPE_UNKNOWN;

    ID3D12GraphicsCommandList7_SetGraphicsRootSignature(command_list, dx12_renderer->bindless_root_signature);
    ID3D12GraphicsCommandList7_SetComputeRootSignature(command_list, dx12_renderer->bindless_root_signature);

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12CommandQueue*         command_queue = dx12_renderer->command_queue;
    ID3D12CommandAllocator*     command_allocator = dx12_renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    hr = ID3D12GraphicsCommandList7_Close(command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Close failed");
        return false;
    }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);


    UINT present_flag = 0;
    present_flag = DXGI_PRESENT_ALLOW_TEARING;

    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, context->renderer.vsync, present_flag);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }

    if(!dm_dx12_wait_for_previous_frame(true, dx12_renderer))
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
        dm_dx12_fence* fence = &dx12_renderer->fences[i];
        size_t fence_value = fence->value++;
        ID3D12CommandQueue_Signal(dx12_renderer->command_queue, fence->fence, fence->value);
        if(ID3D12Fence_GetCompletedValue(fence->fence) < fence_value)
        {
            ID3D12Fence_SetEventOnCompletion(fence->fence, fence_value, dx12_renderer->fence_event);
            WaitForSingleObjectEx(dx12_renderer->fence_event, INFINITE, FALSE);
        }
    }

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(dx12_renderer->resources[dx12_renderer->render_targets[i]]);
        ID3D12Resource_Release(dx12_renderer->resources[dx12_renderer->depth_stencil_targets[i]]);
    }

    hr = IDXGISwapChain4_ResizeBuffers(dx12_renderer->swap_chain, DM_DX12_MAX_FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_ResizeBuffers failed");
        return false;
    }

    void* temp = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle = dx12_renderer->rtv_heap.cpu_handle.begin;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_target_handle  = dx12_renderer->depth_stencil_heap.cpu_handle.begin;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* render_target = dx12_renderer->resources[dx12_renderer->render_targets[i]];

        hr = IDXGISwapChain4_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("IDXGISwapChain4_GetBuffer failed");
            return false;
        }
        dx12_renderer->resources[dx12_renderer->render_targets[i]] = temp;
        temp = NULL;

        D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        ID3D12Device_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->resources[dx12_renderer->render_targets[i]], &desc, render_target_handle);
        render_target_handle.ptr += dx12_renderer->rtv_heap.size;

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

        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12Device_CreateCommittedResource failed");
            DM_LOG_ERROR("Creating depth stencil buffer failed");
            return false;
        }
        dx12_renderer->resources[dx12_renderer->depth_stencil_targets[i]] = temp;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        ID3D12Device5_CreateDepthStencilView(dx12_renderer->device, dx12_renderer->resources[dx12_renderer->depth_stencil_targets[i]], &view_desc, depth_target_handle);
        depth_target_handle.ptr += dx12_renderer->depth_stencil_heap.size;
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

bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    void* temp = NULL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

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
                DM_LOG_ERROR("Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
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
                DM_LOG_ERROR("Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
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
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
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
    
    // === pipeline state ===
    pso_desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count           = 1;
    pso_desc.SampleMask                 = 0xffffffff;
    pso_desc.NumRenderTargets           = 1;
    pso_desc.BlendState.RenderTarget[0] = blend_desc;
    pso_desc.DepthStencilState          = depth_desc;
    pso_desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
    pso_desc.pRootSignature             = dx12_renderer->bindless_root_signature;

    hr = ID3D12Device5_CreateGraphicsPipelineState(dx12_renderer->device, &pso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(dx12_renderer);
        return false;
    }
    pipeline.state = temp;
    temp = NULL;
    
    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);

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
bool dm_dx12_create_buffer(const size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Resource** resource, dm_dx12_renderer* dx12_renderer)
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

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, dx12_renderer->device)) return false;  

    return true;
}

bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_vertex_buffer buffer = { 0 };

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d", "vertex_host_buffer", dx12_renderer->vb_count);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.host_buffers[i] = dx12_renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d", "vertex_device_buffer", dx12_renderer->vb_count);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.device_buffers[i] = dx12_renderer->resource_count++;

        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].StrideInBytes  = desc.stride;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.NumElements         = desc.size / desc.stride;
        view_desc.Buffer.StructureByteStride = desc.stride;
        
        ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, *device_buffer, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;

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

    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    //
    dm_memcpy(dx12_renderer->vertex_buffers + dx12_renderer->vb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->vb_count++;

    return true;
}

bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
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

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d", "index_host_buffer", dx12_renderer->ib_count);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.host_buffers[i] = dx12_renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d", "index_device_buffer", dx12_renderer->ib_count);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.device_buffers[i] = dx12_renderer->resource_count++;
        
        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].Format         = buffer.index_format;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.NumElements         = desc.size / desc.element_size;
        switch(desc.index_type)
        {
            case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
            view_desc.Buffer.StructureByteStride = sizeof(uint16_t);
            break;

            case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
            view_desc.Buffer.StructureByteStride = sizeof(uint32_t);
            break;

            default:
            DM_LOG_FATAL("Should NOT be here");
            return false;
        }
        
        ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, *device_buffer, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;

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

    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    //
    dm_memcpy(dx12_renderer->index_buffers + dx12_renderer->ib_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->ib_count++;

    return true;
}

bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_constant_buffer buffer = { 0 };
    const size_t aligned_size = (desc.size + 255) & ~255;
    const size_t big_size = 1024 * 64;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(big_size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, device_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d", "constant_buffer", dx12_renderer->cb_count);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.device_buffers[i] = dx12_renderer->resource_count++;
        buffer.size     = aligned_size; 
        buffer.big_size = big_size;

        D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = { 0 };
        view_desc.SizeInBytes    = aligned_size;
        view_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[buffer.device_buffers[i]]);

        ID3D12Device5_CreateConstantBufferView(dx12_renderer->device, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);

        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;

        hr = ID3D12Resource_Map(dx12_renderer->resources[buffer.device_buffers[i]], 0,NULL, &buffer.mapped_addresses[i]);
        if(!dm_platform_win32_decode_hresult(hr) || !buffer.mapped_addresses[i])
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            return false;
        }

        if(!desc.data) continue;

        dm_memcpy(buffer.mapped_addresses[i], desc.data, desc.size);
    }

    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    //
    dm_memcpy(dx12_renderer->constant_buffers + dx12_renderer->cb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->cb_count++;

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

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_texture(dm_texture_desc desc, D3D12_RESOURCE_FLAGS flags, dm_dx12_texture* texture, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    DXGI_FORMAT format;
    uint8_t     bytes_per_channel;
    size_t size = desc.width * desc.height * desc.n_channels;

    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        format            = DXGI_FORMAT_R8G8B8A8_UINT;
        bytes_per_channel = 1;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        format            = DXGI_FORMAT_R8G8B8A8_UNORM;
        bytes_per_channel = 1;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_3:
        format            = DXGI_FORMAT_R32G32B32_FLOAT;
        bytes_per_channel = 4;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        format            = DXGI_FORMAT_R32G32B32A32_FLOAT;
        bytes_per_channel = 4;
        break;

        default:
        DM_LOG_FATAL("Unknown or unsupported texture format");
        return false;
    }

    texture->format            = format;
    texture->bytes_per_channel = bytes_per_channel;
    texture->n_channels        = desc.n_channels;

    size *= bytes_per_channel;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host texture is actually a buffer
        ID3D12Resource** host_resource = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_resource, dx12_renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "texture_host", dx12_renderer->texture_count, i);
        ID3D12Resource_SetName(*host_resource, name);
#endif
        texture->host_textures[i] = dx12_renderer->resource_count++;

        if(desc.data)
        {
            if(!dm_dx12_copy_memory(*host_resource, desc.data, size)) return false;
        }

        // device texture
        ID3D12Resource** device_resource = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_texture_resource(desc.width, desc.height, format, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, flags, device_resource, dx12_renderer->device)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "texture_device", dx12_renderer->texture_count, i);
        ID3D12Resource_SetName(*device_resource, name);
#endif
        texture->device_textures[i] = dx12_renderer->resource_count++;

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
    }

    dm_memcpy(dx12_renderer->textures + dx12_renderer->texture_count, texture, sizeof(dm_dx12_texture));

    return true;
}

bool dm_renderer_backend_create_storage_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_texture texture = { 0 };
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    if(!dm_dx12_create_texture(desc, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &texture, dx12_renderer)) return false;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* device_resource = dx12_renderer->resources[texture.device_textures[i]];

        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = device_resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

        D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = texture.format;
        view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        
        ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, device_resource, NULL, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);

        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;
    }

    handle->index = dx12_renderer->texture_count++;
    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    return true;
}

bool dm_renderer_backend_create_sampled_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_texture texture = { 0 };
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    if(!dm_dx12_create_texture(desc, D3D12_RESOURCE_FLAG_NONE, &texture, dx12_renderer)) return false;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* device_resource = dx12_renderer->resources[texture.device_textures[i]];

        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = device_resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                  = texture.format;
        view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipLevels     = 1;

        ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, device_resource, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);

        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;
    }

    handle->index = dx12_renderer->texture_count++;
    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    return true;
}

bool dm_renderer_backend_create_sampler(dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    D3D12_SAMPLER_DESC sampler = { 0 };
    sampler.Filter         = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MaxLOD         = D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Device5_CreateSampler(dx12_renderer->device, &sampler, dx12_renderer->sampler_heap[i].cpu_handle.current);

        dx12_renderer->sampler_heap[i].cpu_handle.current.ptr += dx12_renderer->sampler_heap[i].size;
        dx12_renderer->sampler_heap[i].count++;
    }

    handle->descriptor_index = dx12_renderer->sampler_heap[0].count - 1;

    return true;
}

bool dm_renderer_backend_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_storage_buffer buffer = { 0 };

    D3D12_RESOURCE_FLAGS device_flags = D3D12_RESOURCE_FLAG_NONE;
    if(desc.write) device_flags       = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_STATES device_states = D3D12_RESOURCE_STATE_COMMON;
    if(desc.write) device_states        = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    buffer.size = desc.size;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[0];

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d", "storage_buffer_host", dx12_renderer->sb_count);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.host_buffers[i] = dx12_renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, device_flags, device_buffer, dx12_renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d", "storage_buffer_device", dx12_renderer->sb_count);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.device_buffers[i] = dx12_renderer->resource_count++;

        if(desc.data)
        {
            if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;
        }

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = device_states;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

        // view
        if(desc.write)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = { 0 };
            view_desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
            view_desc.Buffer.NumElements         = desc.size / desc.stride;
            view_desc.Buffer.StructureByteStride = desc.stride;

            ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, *device_buffer, NULL, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current); 
        }
        else
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
            view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
            view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
            view_desc.Buffer.NumElements         = desc.size / desc.stride;
            view_desc.Buffer.StructureByteStride = desc.stride;
            view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, *device_buffer, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        }
        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;
    }

    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    //
    dm_memcpy(dx12_renderer->storage_buffers + dx12_renderer->sb_count, &buffer, sizeof(buffer));
    handle->index = dx12_renderer->sb_count++;

    return true;
}

/*************
 * RAYTRACING
 **************/
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_add_sbt_record(size_t size, uint8_t index, wchar_t* name, ID3D12StateObjectProperties* props, dm_dx12_sbt* sbt, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;

    ID3D12Resource** resource = &dx12_renderer->resources[dx12_renderer->resource_count];
    if(!dm_dx12_create_buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, resource, dx12_renderer)) return false;
    sbt->index[index] = dx12_renderer->resource_count++;

    // sbts must be in NON_PIXEL_SHADER_RESOURCE state
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.pResource   = *resource;

    ID3D12GraphicsCommandList7_ResourceBarrier(dx12_renderer->command_list[0], 1, &barrier);

    void* id = ID3D12StateObjectProperties_GetShaderIdentifier(props, name);
    if(!id)
    {
        DM_LOG_FATAL("ID3D12StateObjectProperties_GetShaderIdentifier failed");
        return false;
    }
    
    void* temp = NULL;
    hr = ID3D12Resource_Map(*resource, 0,0, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Resource_Map failed");
        return false;
    }

    dm_memcpy(temp, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    ID3D12Resource_Unmap(*resource, 0,0);
    temp = NULL;
    
    return true;
}

bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    void* temp = NULL;

    dm_dx12_raytracing_pipeline pipeline = { 0 };

    // === shaders ===
    D3D12_DXIL_LIBRARY_DESC library_desc = { 0 };
    D3D12_HIT_GROUP_DESC    hit_group_desc[DM_RT_PIPE_MAX_HIT_GROUPS] = { 0 };
    LPCWSTR l_exports[DM_RT_PIPE_MAX_HIT_GROUPS] = { 0 };

    ID3DBlob* shader = NULL;

    const char* path = desc.shader_path;

    if(!dm_dx12_load_shader_data(path, &shader))
    {
        DM_LOG_ERROR("Could not load hit group shader: %s", shader);
        return false;
    }

    library_desc.DXILLibrary.pShaderBytecode = ID3D10Blob_GetBufferPointer(shader);
    library_desc.DXILLibrary.BytecodeLength  = ID3D10Blob_GetBufferSize(shader);

    wchar_t hit_group_name[DM_RT_PIPE_MAX_HIT_GROUPS][512];
    wchar_t closest_hit[DM_RT_PIPE_MAX_HIT_GROUPS][512];
    wchar_t any_hit[DM_RT_PIPE_MAX_HIT_GROUPS][512];
    wchar_t intersection[DM_RT_PIPE_MAX_HIT_GROUPS][512];

    for(uint8_t i=0; i<desc.hit_group_count; i++)
    {
        switch(desc.hit_groups[i].type)
        {
            case DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES:
            hit_group_desc[i].Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            break;

            default:
            DM_LOG_FATAL("Unsupported hit group type");
            return false;
        }

        swprintf(hit_group_name[i], 512, L"%hs", desc.hit_groups[i].name);
        hit_group_desc[i].HitGroupExport = hit_group_name[i];

        if(desc.hit_groups[i].flags & DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST) 
        {
            swprintf(closest_hit[i], 512, L"%hs", desc.hit_groups[i].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST]);
            hit_group_desc[i].ClosestHitShaderImport = closest_hit[i];
        }
        if(desc.hit_groups[i].flags & DM_RT_PIPE_HIT_GROUP_FLAG_ANY)
        {
            swprintf(any_hit[i], 512, L"%hs", desc.hit_groups[i].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_ANY]);
            hit_group_desc[i].AnyHitShaderImport = any_hit[i];
        }
        if(desc.hit_groups[i].flags & DM_RT_PIPE_HIT_GROUP_FLAG_INTERSECTION)
        {
            swprintf(intersection[i], 512, L"%hs", desc.hit_groups[i].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_INTERSECTION]);
            hit_group_desc[i].IntersectionShaderImport = intersection[i];
        }
    }

    D3D12_RAYTRACING_SHADER_CONFIG shader_config = { 0 };
    shader_config.MaxAttributeSizeInBytes = sizeof(float) * 2;
    shader_config.MaxPayloadSizeInBytes   = desc.payload_size;

    D3D12_GLOBAL_ROOT_SIGNATURE global_sig = { 0 };
    global_sig.pGlobalRootSignature = dx12_renderer->bindless_root_signature;

    D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = { 0 };
    if(desc.max_depth > 31)
    {
        DM_LOG_WARN("Max depth for raytracing cannot exceed 31. Capping at this value");
        desc.max_depth = 31;
    }

    pipeline_config.MaxTraceRecursionDepth = desc.max_depth;
    
    D3D12_STATE_SUBOBJECT dxil_subobject = { 0 };
    dxil_subobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    dxil_subobject.pDesc = &library_desc;

    D3D12_STATE_SUBOBJECT hit_group_subobject = { 0 };
    hit_group_subobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hit_group_subobject.pDesc = &hit_group_desc[0];

    D3D12_STATE_SUBOBJECT shader_config_subobject = { 0 };
    shader_config_subobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shader_config_subobject.pDesc = &shader_config;

    D3D12_STATE_SUBOBJECT global_sig_subobject = { 0 };
    global_sig_subobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    global_sig_subobject.pDesc = &global_sig;

    D3D12_STATE_SUBOBJECT pipeline_config_subobject = { 0 };
    pipeline_config_subobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipeline_config_subobject.pDesc = &pipeline_config;

    D3D12_STATE_SUBOBJECT subobjects[] = { dxil_subobject, hit_group_subobject, shader_config_subobject, global_sig_subobject, pipeline_config_subobject };

    D3D12_STATE_OBJECT_DESC object_desc = { 0 };
    object_desc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    object_desc.NumSubobjects = _countof(subobjects);
    object_desc.pSubobjects   = subobjects;

    hr = ID3D12Device5_CreateStateObject(dx12_renderer->device, &object_desc, &IID_ID3D12StateObject, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreateStateObject failed");
        return false;
    }
    pipeline.pso = temp;
    temp = NULL;

    // shader binding tables
    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        const size_t ray_gen_size = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        const size_t miss_size    = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        size_t hit_size           = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * desc.hit_group_count;
        hit_size                  = DM_ALIGN_BYTES(hit_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        
        ID3D12StateObjectProperties* props = { 0 };
        hr = ID3D12PipelineState_QueryInterface(pipeline.pso, &IID_ID3D12StateObjectProperties, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            DM_LOG_FATAL("ID3D12PipelineState_QueryInterface failed");
            return false;
        }
        props = temp;
        temp = NULL;

        wchar_t raygen_name[512];
        wchar_t miss_name[512];

        swprintf(raygen_name, 512, L"%hs", desc.raygen);
        swprintf(miss_name,   512, L"%hs", desc.miss);

        if(!dm_dx12_add_sbt_record(ray_gen_size, i, raygen_name, props, &pipeline.raygen_sbt, dx12_renderer)) return false;
        if(!dm_dx12_add_sbt_record(miss_size, i, miss_name, props, &pipeline.miss_sbt, dx12_renderer)) return false;
        if(!dm_dx12_add_sbt_record(hit_size, i, hit_group_name[0], props, &pipeline.hitgroup_sbt, dx12_renderer)) return false;

        pipeline.raygen_sbt.stride   = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        pipeline.raygen_sbt.size     = ray_gen_size;

        pipeline.miss_sbt.stride     = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT; 
        pipeline.miss_sbt.size       = miss_size;

        pipeline.hitgroup_sbt.stride = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        pipeline.hitgroup_sbt.size   = hit_size;

        ID3D12StateObjectProperties_Release(props);
    }

    pipeline.hit_group_count    = desc.hit_group_count;
    pipeline.max_instance_count = desc.max_instance_count;

    // 
    dm_memcpy(dx12_renderer->rt_pipelines + dx12_renderer->rt_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = dx12_renderer->rt_pipe_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_acceleration_structure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* inputs, uint8_t index, dm_dx12_as* as, dm_dx12_renderer* dx12_renderer)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = { 0 };
    ID3D12Device5_GetRaytracingAccelerationStructurePrebuildInfo(dx12_renderer->device, inputs, &prebuild_info);

    as->scratch_size = prebuild_info.ScratchDataSizeInBytes;

    const D3D12_RESOURCE_FLAGS resource_flag   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    const D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    ID3D12Resource** scratch_buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
    if(!dm_dx12_create_buffer(as->scratch_size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, resource_flag, scratch_buffer, dx12_renderer)) return false;
    as->scratch = dx12_renderer->resource_count++;

    ID3D12Resource** buffer = &dx12_renderer->resources[dx12_renderer->resource_count];
    if(!dm_dx12_create_buffer(prebuild_info.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, resource_state, resource_flag, buffer, dx12_renderer)) return false;
    as->result = dx12_renderer->resource_count++;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = { 0 };
    build_desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[as->scratch]);
    build_desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[as->result]);
    build_desc.Inputs                           = *inputs;

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = *buffer;

    ID3D12GraphicsCommandList7_BuildRaytracingAccelerationStructure(dx12_renderer->command_list[0], &build_desc, 0, NULL);
    ID3D12GraphicsCommandList7_ResourceBarrier(dx12_renderer->command_list[0], 1, &barrier);

    return true;
}

// BOTTOM-LEVEL ACCELERATION STRUCTURE
bool dm_renderer_backend_create_blas(dm_blas_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_blas blas = { 0 };

    const D3D12_RESOURCE_FLAGS resource_flag   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    const D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = { 0 };

        switch(desc.geometry_type)
        {
            case DM_BLAS_GEOMETRY_TYPE_TRIANGLES:
            {
                dm_dx12_vertex_buffer vb = dx12_renderer->vertex_buffers[desc.vertex_buffer.index];
                dm_dx12_index_buffer  ib = dx12_renderer->index_buffers[desc.index_buffer.index];

                geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

                geometry_desc.Triangles.VertexCount                = desc.vertex_count;
                geometry_desc.Triangles.VertexBuffer.StartAddress  = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[vb.device_buffers[i]]);
                geometry_desc.Triangles.VertexBuffer.StrideInBytes = desc.vertex_stride;
                switch(desc.vertex_type)
                {
                    case DM_BLAS_VERTEX_TYPE_FLOAT_4:
                    case DM_BLAS_VERTEX_TYPE_FLOAT_3:
                    geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                    break;

                    default:
                    DM_LOG_FATAL("Unknown vertex type");
                    return false;
                }

                geometry_desc.Triangles.IndexCount  = desc.index_count;
                geometry_desc.Triangles.IndexBuffer = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[ib.device_buffers[i]]);
                switch(desc.index_type)
                {
                    case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
                    geometry_desc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
                    break;

                    case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
                    geometry_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
                    break;

                    default:
                    DM_LOG_FATAL("Unsupported index type");
                    return false;
                }
            } break;

            default:
            DM_LOG_FATAL("Unknown or unsupported geometry type");
            return false;
        }
        
        switch(desc.flags)
        {
            case DM_BLAS_GEOMETRY_FLAG_OPAQUE:
            geometry_desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            break;

            default:
            DM_LOG_FATAL("Unknown raytracing geometry flags");
            return false;
        }
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS build_inputs = { 0 };
        build_inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        build_inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        build_inputs.pGeometryDescs = &geometry_desc;
        build_inputs.NumDescs       = 1;
        build_inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;

        if(!dm_dx12_create_acceleration_structure(&build_inputs, i, &blas.as[i], dx12_renderer)) return false;
    }

    dm_memcpy(dx12_renderer->blas + dx12_renderer->blas_count, &blas, sizeof(blas));
    handle->index = dx12_renderer->blas_count++;

    return true;
}

bool dm_renderer_backend_create_tlas(dm_tlas_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_tlas tlas = { 0 };

    const D3D12_RESOURCE_FLAGS resource_flag   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    const D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // get the instance buffer
        dm_assert(desc.instance_buffer.type==DM_RESOURCE_TYPE_STORAGE_BUFFER);

        dm_dx12_storage_buffer instance_buffer = dx12_renderer->storage_buffers[desc.instance_buffer.index];
        ID3D12Resource* internal_buffer = dx12_renderer->resources[instance_buffer.device_buffers[i]];

        D3D12_RESOURCE_BARRIER pre_barrier = { 0 };
        pre_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        pre_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        pre_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        pre_barrier.Transition.pResource   = internal_buffer;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = { 0 };
        inputs.NumDescs      = desc.instance_count;
        inputs.InstanceDescs = ID3D12Resource_GetGPUVirtualAddress(internal_buffer);
        inputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;

        ID3D12GraphicsCommandList7_ResourceBarrier(dx12_renderer->command_list[0], 1, &pre_barrier);
        if(!dm_dx12_create_acceleration_structure(&inputs, i, &tlas.as[i], dx12_renderer)) return false;

        // srv
        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        view_desc.Format                                   = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.RaytracingAccelerationStructure.Location = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[tlas.as[i].result]);

        ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, NULL, &view_desc, dx12_renderer->resource_heap[i].cpu_handle.current);
        dx12_renderer->resource_heap[i].cpu_handle.current.ptr += dx12_renderer->resource_heap[i].size;
        dx12_renderer->resource_heap[i].count++;
    }

    handle->descriptor_index = dx12_renderer->resource_heap[0].count - 1;

    tlas.instance_buffer = desc.instance_buffer;

    //
    dm_memcpy(dx12_renderer->tlas + dx12_renderer->tlas_count, &tlas, sizeof(tlas));
    handle->index = dx12_renderer->tlas_count++;

    return true;
}

bool dm_renderer_backend_get_blas_gpu_address(dm_resource_handle handle, size_t* address, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    const dm_dx12_blas blas = dx12_renderer->blas[handle.index];
    const uint8_t current_frame = dx12_renderer->current_frame;

    *address = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[blas.as[current_frame].result]);

    return true;
}

/**********
 * COMPUTE
 ***********/
bool dm_compute_backend_create_compute_pipeline(dm_compute_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    void* temp = NULL;
    dm_dx12_compute_pipeline pipeline = { 0 };

    D3D12_COMPUTE_PIPELINE_STATE_DESC cso_desc = { 0 };

    // === shader ===
    ID3DBlob* shader = NULL;
    
    const char* shader_path = desc.shader.path;

    if(!dm_dx12_load_shader_data(shader_path, &shader)) 
    {
        DM_LOG_ERROR("Could not load shader: %s", desc.shader.path);
        return false;
    }

    cso_desc.CS.pShaderBytecode = ID3D10Blob_GetBufferPointer(shader);
    cso_desc.CS.BytecodeLength  = ID3D10Blob_GetBufferSize(shader);

    // pipeline state
    cso_desc.pRootSignature = dx12_renderer->bindless_root_signature;

    hr = ID3D12Device5_CreateComputePipelineState(dx12_renderer->device, &cso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        DM_LOG_FATAL("ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(dx12_renderer);
        return false;
    }
    pipeline.state = temp;

    temp = NULL;

    //
    dm_memcpy(dx12_renderer->compute_pipelines + dx12_renderer->comp_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = dx12_renderer->comp_pipe_count++;

    ID3D10Blob_Release(shader);

    return true;
}

/******************
 * RENDER COMMANDS
* ******************/
bool dm_render_command_backend_begin_render_pass(float r, float g, float b, float a, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->resources[dx12_renderer->render_targets[current_frame]];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    
    // rtv heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx12_renderer->rtv_heap.cpu_handle.begin;
    rtv_handle.ptr += current_frame * dx12_renderer->rtv_heap.size;

    // depth stencil heap
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dx12_renderer->depth_stencil_heap.cpu_handle.begin;
    dsv_handle.ptr += current_frame * dx12_renderer->depth_stencil_heap.size;

    float clear_color[] = { r,g,b,a };

    ID3D12GraphicsCommandList7_OMSetRenderTargets(command_list, 1, &rtv_handle, FALSE, &dsv_handle);
    ID3D12GraphicsCommandList7_ClearRenderTargetView(command_list, rtv_handle, clear_color, 0, NULL);
    ID3D12GraphicsCommandList7_ClearDepthStencilView(command_list, dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f,0, 0, NULL);

    return true;
}

bool dm_render_command_backend_end_render_pass(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = dx12_renderer->resources[dx12_renderer->render_targets[current_frame]];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

    return true;
}

bool dm_render_command_backend_bind_raster_pipeline(dm_resource_handle handle, dm_renderer* renderer)
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

    ID3D12GraphicsCommandList7_SetPipelineState(command_list, pipeline.state);
    ID3D12GraphicsCommandList7_RSSetViewports(command_list, 1, &pipeline.viewport);
    ID3D12GraphicsCommandList7_RSSetScissorRects(command_list, 1, &pipeline.scissor);
    ID3D12GraphicsCommandList7_IASetPrimitiveTopology(command_list, pipeline.topology);

    dx12_renderer->active_pipeline_type = DM_PIPELINE_TYPE_RASTER;

    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_raytracing_pipeline pipeline = dx12_renderer->rt_pipelines[handle.index];

    dm_dx12_tlas as = dx12_renderer->tlas[0];

    D3D12_GPU_VIRTUAL_ADDRESS address = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[as.as[current_frame].result]);

    ID3D12GraphicsCommandList7_SetPipelineState1(command_list, pipeline.pso);

    dx12_renderer->active_pipeline_type = DM_PIPELINE_TYPE_RAYTRACING;

    return true;
}

bool dm_render_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    switch(dx12_renderer->active_pipeline_type)
    {
        case DM_PIPELINE_TYPE_RASTER:
        ID3D12GraphicsCommandList7_SetGraphicsRoot32BitConstants(command_list, slot, count, data, offset);
        break;

        //case DM_DX12_PIPELINE_TYPE_COMPUTE:
        case DM_PIPELINE_TYPE_RAYTRACING:
        ID3D12GraphicsCommandList7_SetComputeRoot32BitConstants(command_list, slot, count, data, offset);
        break;
            
        default:
        DM_LOG_FATAL("Unknown/unsupported pipeline type bound. Should NOT be here.");
        return false;
    }

    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_vertex_buffer vb = dx12_renderer->vertex_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetVertexBuffers(command_list, slot, 1, &vb.views[current_frame]);

    return true;
}

bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
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

bool dm_render_command_backend_update_index_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    dm_dx12_index_buffer buffer = dx12_renderer->index_buffers[handle.index];

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    ID3D12Resource* host_buffer   = dx12_renderer->resources[buffer.host_buffers[current_frame]];
    ID3D12Resource* device_buffer = dx12_renderer->resources[buffer.device_buffers[current_frame]];

    if(!dm_dx12_copy_memory(host_buffer, data, size)) return false;

    D3D12_RESOURCE_BARRIER barriers[2] = { 0 };

    barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.pResource   = device_buffer;

    barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.pResource   = device_buffer;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[0]);
    ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, device_buffer, 0, host_buffer, 0, size);
    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barriers[1]);

    return true;
}

bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
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

bool dm_render_command_backend_bind_index_buffer(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
    dm_dx12_index_buffer ib = dx12_renderer->index_buffers[handle.index];

    ID3D12GraphicsCommandList7_IASetIndexBuffer(command_list, &ib.views[current_frame]);

    return true;
}

bool dm_render_command_backend_update_storage_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_storage_buffer buffer = dx12_renderer->storage_buffers[handle.index];

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

bool dm_render_command_backend_resize_texture(uint32_t width, uint32_t height, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_texture* texture = &dx12_renderer->textures[handle.index];

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    size_t size = width * height * texture->bytes_per_channel * texture->n_channels;

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    for(uint8_t i=0; i<DM_DX12_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // first release existing resources
        ID3D12Resource_Release(dx12_renderer->resources[texture->host_textures[i]]);
        ID3D12Resource_Release(dx12_renderer->resources[texture->device_textures[i]]);

        // recreate resources
        ID3D12Resource** host_resource = &dx12_renderer->resources[texture->host_textures[i]];
        if(!dm_dx12_create_buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_resource, dx12_renderer)) return false;

        ID3D12Resource** device_resource = &dx12_renderer->resources[texture->device_textures[i]];
        if(!dm_dx12_create_texture_resource(width, height, texture->format, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, flags, device_resource, dx12_renderer->device)) return false;

        // view
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = *device_resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

        D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = texture->format;
        view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        
        D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = dx12_renderer->resource_heap[i].cpu_handle.begin;
        heap_handle.ptr += dx12_renderer->resource_heap[i].size * handle.descriptor_index;
        ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, *device_resource, NULL, &view_desc, heap_handle);
    }

    return true;
}


bool dm_render_command_backend_update_tlas(uint32_t instance_count, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_tlas tlas = dx12_renderer->tlas[handle.index];

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    dm_dx12_storage_buffer instance_buffer = dx12_renderer->storage_buffers[tlas.instance_buffer.index];

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = { 0 };
    build_desc.Inputs.Type                      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    build_desc.Inputs.Flags                     = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    build_desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[tlas.as[current_frame].result]);
    build_desc.SourceAccelerationStructureData  = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[tlas.as[current_frame].result]);
    build_desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[tlas.as[current_frame].scratch]);
    build_desc.Inputs.InstanceDescs             = ID3D12Resource_GetGPUVirtualAddress(dx12_renderer->resources[instance_buffer.device_buffers[current_frame]]);
    build_desc.Inputs.NumDescs                  = instance_count;
    build_desc.Inputs.DescsLayout               = D3D12_ELEMENTS_LAYOUT_ARRAY;

    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = dx12_renderer->resources[tlas.as[current_frame].result];

    ID3D12GraphicsCommandList7_BuildRaytracingAccelerationStructure(command_list, &build_desc, 0, NULL);
    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);

    return true;
}

bool dm_render_command_backend_dispatch_rays(uint16_t x, uint16_t y, dm_resource_handle pipeline, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    dm_dx12_raytracing_pipeline rt_pipeline = dx12_renderer->rt_pipelines[pipeline.index];

    ID3D12Resource* raygen_sbt = dx12_renderer->resources[rt_pipeline.raygen_sbt.index[current_frame]];
    ID3D12Resource* miss_sbt   = dx12_renderer->resources[rt_pipeline.miss_sbt.index[current_frame]];
    ID3D12Resource* hit_sbt    = dx12_renderer->resources[rt_pipeline.hitgroup_sbt.index[current_frame]];

    D3D12_DISPATCH_RAYS_DESC dispatch_desc = { 0 };
    dispatch_desc.Width  = x;
    dispatch_desc.Height = y;
    dispatch_desc.Depth  = 1;

    dispatch_desc.RayGenerationShaderRecord.StartAddress = ID3D12Resource_GetGPUVirtualAddress(raygen_sbt); 
    dispatch_desc.RayGenerationShaderRecord.SizeInBytes  = rt_pipeline.raygen_sbt.size;

    dispatch_desc.MissShaderTable.StartAddress  = ID3D12Resource_GetGPUVirtualAddress(miss_sbt);
    dispatch_desc.MissShaderTable.SizeInBytes   = rt_pipeline.miss_sbt.size;
    dispatch_desc.MissShaderTable.StrideInBytes = rt_pipeline.miss_sbt.stride;

    dispatch_desc.HitGroupTable.StartAddress  = ID3D12Resource_GetGPUVirtualAddress(hit_sbt);
    dispatch_desc.HitGroupTable.SizeInBytes   = rt_pipeline.hitgroup_sbt.size;
    dispatch_desc.HitGroupTable.StrideInBytes = rt_pipeline.hitgroup_sbt.stride;

    ID3D12GraphicsCommandList7_DispatchRays(command_list, &dispatch_desc);
    
    return true;
}

bool dm_render_command_backend_copy_image_to_screen(dm_resource_handle image, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    dm_dx12_texture texture = dx12_renderer->textures[image.index];

    const uint8_t current_frame = dx12_renderer->current_frame;
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];

    ID3D12Resource* image_resource = dx12_renderer->resources[texture.device_textures[current_frame]];
    ID3D12Resource* render_target  = dx12_renderer->resources[dx12_renderer->render_targets[current_frame]];

    D3D12_RESOURCE_BARRIER before_barriers[2] = { 0 };

    before_barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    before_barriers[0].Transition.pResource   = image_resource;
    before_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    before_barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    before_barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    before_barriers[1].Transition.pResource   = render_target;
    before_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    before_barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_BARRIER after_barriers[2] = { 0 };

    after_barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    after_barriers[0].Transition.pResource   = render_target;
    after_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    after_barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    after_barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    after_barriers[1].Transition.pResource   = image_resource;
    after_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    after_barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 2, before_barriers);
    ID3D12GraphicsCommandList7_CopyResource(command_list, render_target, image_resource); 
    ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 2, after_barriers);

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

/*******************
 * COMPUTE COMMANDS
 ********************/
bool dm_compute_command_backend_begin_recording(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12CommandAllocator*     command_allocator = dx12_renderer->compute_command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->compute_command_list[current_frame];

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

    ID3D12DescriptorHeap* heaps[] = { dx12_renderer->resource_heap->heap };
    ID3D12GraphicsCommandList7_SetDescriptorHeaps(command_list, _countof(heaps), heaps);

    return true;
}

bool dm_compute_command_backend_end_recording(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    const uint8_t current_frame = dx12_renderer->current_frame;

    ID3D12CommandQueue*         command_queue = dx12_renderer->compute_command_queue;
    ID3D12CommandAllocator*     command_allocator = dx12_renderer->compute_command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->compute_command_list[current_frame];

    hr = ID3D12GraphicsCommandList7_Close(command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList7_Close failed");
        return false;
    }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    dm_dx12_fence* fence = &dx12_renderer->fences[current_frame];

    hr = ID3D12CommandQueue_Signal(command_queue, fence->fence, fence->value);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandQueue_Signal failed");
        return false;
    }

    const uint64_t v = ID3D12Fence_GetCompletedValue(fence->fence);
    if(v < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, dx12_renderer->compute_fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Fence_SetEventOnCompletion failed");
            return false;
        }

        WaitForSingleObject(dx12_renderer->compute_fence_event, INFINITE);
    }

    fence->value++;

    return true;
}

void dm_compute_command_backend_bind_compute_pipeline(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    dm_dx12_compute_pipeline pipeline = dx12_renderer->compute_pipelines[handle.index];
    const uint8_t current_frame = dx12_renderer->current_frame;
#if DM_DX12_COMPUTE_COMMAND_LIST 
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->compute_command_list[current_frame];
#else
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
#endif

    ID3D12GraphicsCommandList7_SetPipelineState(command_list, pipeline.state);

    dx12_renderer->active_pipeline_type = DM_PIPELINE_TYPE_COMPUTE;
}

void dm_compute_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;
#if DM_DX12_COMPUTE_COMMAND_LIST 
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->compute_command_list[current_frame];
#else
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
#endif

    ID3D12GraphicsCommandList7_SetComputeRoot32BitConstants(command_list, slot,count,data,offset);
}

void dm_compute_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;

    const uint8_t current_frame = dx12_renderer->current_frame;

    if(!dx12_renderer->constant_buffers[handle.index].mapped_addresses[current_frame])
    {
        DM_LOG_FATAL("Constant buffer has an invalid address");
        return;
    }

    dm_memcpy(dx12_renderer->constant_buffers[handle.index].mapped_addresses[current_frame], data, size);
}

void dm_compute_command_backend_dispatch(const uint16_t x, const uint16_t y, const uint16_t z, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;

    const uint8_t current_frame = dx12_renderer->current_frame;
#if DM_DX12_COMPUTE_COMMAND_LIST 
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->compute_command_list[current_frame];
#else
    ID3D12GraphicsCommandList7* command_list = dx12_renderer->command_list[current_frame];
#endif

    ID3D12GraphicsCommandList7_Dispatch(command_list, x,y,z);
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
