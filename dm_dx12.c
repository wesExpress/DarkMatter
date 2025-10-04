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

typedef struct dm_dx12_renderer_t
{
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

    dm_pipeline_type active_pipeline_type;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
    IDXGIDebug1* dxgi_debug;
    IDXGIInfoQueue* info_queue;
#endif
} dm_dx12_renderer;

bool dm_win32_decode_hresult(HRESULT hr);

// === commands ===
bool dm_render_command_begin_frame(void* backend) { return true; }
bool dm_render_command_end_frame(void* backend) { return true; }
bool dm_render_command_begin_update_backend(void* backend) { return true; }
bool dm_render_command_end_update_backend(void* backend) { return true; }
bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, void* backend) { return true; }
bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, void* backend) { return true; }
bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, void* backend) { return true; }
bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, void* backend) { return true; }
bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, void* backend) { return true; }
bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, void* backend) { return true; }
bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, void* backend) { return true; }
bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, void* backend) { return true; }

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
