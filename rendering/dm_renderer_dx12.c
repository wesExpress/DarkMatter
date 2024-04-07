#include "dm.h"

#ifdef DM_DIRECTX12

#include "platform/dm_platform_win32.h"

#define COBJMACROS
#include <unknwn.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#ifdef DM_DEBUG
#include <dxgidebug.h>
#endif

#include <assert.h>

#define DM_DX12_NUM_FRAMES 3

typedef struct dm_dx12_renderpass_t
{
    D3D12_VIEWPORT viewport;
    D3D12_RECT     rect;
} dm_dx12_renderpass;

typedef struct dm_dx12_pipeline_t
{
    ID3D12RootSignature*     root_signature;
    ID3D12PipelineState*     state;
    
    D3D12_PRIMITIVE_TOPOLOGY topology;
    
    uint32_t cb_count;
    
    uint32_t cbv_heap_offset, tex_offset;
} dm_dx12_pipeline;

typedef struct dm_dx12_vertex_buffer_t
{
    ID3D12Resource*          buffer[DM_DX12_NUM_FRAMES];
    D3D12_VERTEX_BUFFER_VIEW view[DM_DX12_NUM_FRAMES];
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    ID3D12Resource*         buffer;
    D3D12_INDEX_BUFFER_VIEW view;
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    ID3D12Resource* buffer[DM_DX12_NUM_FRAMES];
    void*           mapped_address[DM_DX12_NUM_FRAMES];
} dm_dx12_constant_buffer;

#ifdef DM_RAYTRACING
typedef struct dm_dx12_blas_t
{
    ID3D12Resource* scratch_buffer;
    ID3D12Resource* result_buffer;
} dm_dx12_blas;

typedef struct dm_dx12_tlas_t
{
    uint32_t instance_count;
    size_t   update_scratch_size;
    
    D3D12_RAYTRACING_INSTANCE_DESC* instance_data[DM_DX12_NUM_FRAMES];
    ID3D12Resource* instance_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* scratch_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* result_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* update_scratch_buffer[DM_DX12_NUM_FRAMES];
} dm_dx12_tlas;

#define DM_DX12_MAX_BLAS 10
typedef struct dm_dx12_acceleration_structure_t
{
    dm_dx12_tlas tlas;
    
    dm_dx12_blas blas[DM_DX12_MAX_BLAS];
    uint32_t     blas_count;
} dm_dx12_acceleration_structure;

#define DM_DX12_RAYTRACE_NUM_SHADER_IDS 3

typedef struct dm_dx12_raytracing_pipeline_t
{
    ID3D12RootSignature* root_signature;
    
    ID3D12StateObject*   state_object;
    ID3D12Resource*      shader_ids;
    
    ID3D12Resource* output_image;
    
    dm_raytracing_pipeline_resource resources[4];
    uint32_t                        resource_count;
    
    uint32_t         cb_count;
    
    uint32_t output_width, output_height;
} dm_dx12_raytracing_pipeline;

#define DM_DX12_MAX_ACCELERATION_STRUCTURES 10
#define DM_DX12_MAX_RAYTRACING_PIPELINES    10
#endif

// attempt at descriptor heap sizing / offsetting
#define DM_DX12_DESCRIPTOR_HEAP_MAX_CBV 10
#define DM_DX12_DESCRIPTOR_HEAP_MAX_UAV 20
#define DM_DX12_DESCRIPTOR_HEAP_MAX_SRV 10

#define DM_DX12_DESCRIPTOR_HEAP_CBV_INDEX 0
#define DM_DX12_DESCRIPTOR_HEAP_TEX_INDEX 1

#define DM_DX12_MAX_PASSES   10
#define DM_DX12_MAX_PIPES    10
#define DM_DX12_MAX_BUFFERS 100
typedef struct dm_dx12_renderer_t
{
    ID3D12Device5*        device;
    IDXGISwapChain3*      swap_chain;
    
    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_DX12_NUM_FRAMES];
    ID3D12GraphicsCommandList4* command_list[DM_DX12_NUM_FRAMES];
    
    ID3D12DescriptorHeap* rtv_descriptor_heap;
    ID3D12DescriptorHeap* depth_stencil_descriptor_heap;
    ID3D12DescriptorHeap* resource_descriptor_heap[DM_DX12_NUM_FRAMES];
    
    ID3D12Resource* render_target[DM_DX12_NUM_FRAMES];
    ID3D12Resource* depth_stencil_buffer;
    
    ID3D12Fence* fence;
    HANDLE       fence_event;
    uint64_t     fence_value;
    uint64_t     frame_fence_values[DM_DX12_NUM_FRAMES];
    
    uint32_t current_frame_index;
    uint32_t pass_count, pipe_count, vb_count, ib_count, cb_count;
    
    dm_dx12_renderpass      passes[DM_DX12_MAX_PASSES];
    dm_dx12_pipeline        pipes[DM_DX12_MAX_PIPES];
    dm_dx12_vertex_buffer   vertex_buffers[DM_DX12_MAX_BUFFERS];
    dm_dx12_index_buffer    index_buffers[DM_DX12_MAX_BUFFERS];
    dm_dx12_constant_buffer constant_buffers[DM_DX12_MAX_BUFFERS];
    
    // misc
    uint32_t handle_increment_size_cbv_srv_uav, handle_increment_size_rtv;
    
#ifdef DM_RAYTRACING
    uint32_t as_count, raytracing_pipes_count;
    dm_dx12_acceleration_structure accel_structs[DM_DX12_MAX_ACCELERATION_STRUCTURES];
    dm_dx12_raytracing_pipeline    raytracing_pipelines[DM_DX12_MAX_RAYTRACING_PIPELINES];
#endif
    
#ifdef DM_DEBUG
    ID3D12Debug* debug;
#endif
} dm_dx12_renderer;

static const D3D12_HEAP_PROPERTIES DM_DX12_UPLOAD_HEAP = { .Type=D3D12_HEAP_TYPE_UPLOAD };
static const D3D12_HEAP_PROPERTIES DM_DX12_DEFAULT_HEAP = { .Type=D3D12_HEAP_TYPE_DEFAULT };
static const D3D12_RESOURCE_DESC DM_DX12_BASIC_BUFFER_DESC = {
    .Dimension=D3D12_RESOURCE_DIMENSION_BUFFER,
    .Width=0,
    .Height=1,
    .DepthOrArraySize=1,
    .MipLevels=1,
    .SampleDesc={.Count=1},
    .Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR
};

#define DM_DX12_GET_RENDERER dm_dx12_renderer* dx12_renderer = renderer->internal_renderer

void dm_dx12_renderer_destroy_pipe(dm_dx12_pipeline* pipe);
void dm_dx12_renderer_destroy_vertex_buffer(dm_dx12_vertex_buffer* buffer);
void dm_dx12_renderer_destroy_index_buffer(dm_dx12_index_buffer* buffer);
void dm_dx12_renderer_destroy_constant_buffer(dm_dx12_constant_buffer* buffer);

#ifdef DM_RAYTRACING
void dm_dx12_renderer_destroy_acceleration_structure(dm_dx12_acceleration_structure* as);
void dm_dx12_renderer_destroy_raytracing_pipeline(dm_dx12_raytracing_pipeline* pipe);
#endif

/*********************
 DX12 ENUM CONVERSIONS
***********************/
DM_INLINE
DXGI_FORMAT dm_vertex_t_to_dxgi_format(dm_vertex_attrib_desc desc)
{
	switch (desc.data_t)
	{
        case DM_VERTEX_DATA_T_BYTE:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R8_SINT;
            case 2:  return DXGI_FORMAT_R8G8_SINT;
            case 4:  return DXGI_FORMAT_R8G8B8A8_SINT;
            default: return DXGI_FORMAT_R8G8B8A8_SINT;
        }
        break;
        
        case DM_VERTEX_DATA_T_UBYTE:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R8_UINT;
            case 2:  return DXGI_FORMAT_R8G8_UINT;
            case 4:  return DXGI_FORMAT_R8G8B8A8_UINT;
            default: return DXGI_FORMAT_R8G8B8A8_UINT;
        }
        
        case DM_VERTEX_DATA_T_UBYTE_NORM:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R8_UNORM;
            case 2:  return DXGI_FORMAT_R8G8_UNORM;
            case 4:  return DXGI_FORMAT_R8G8B8A8_UNORM;
            default: return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        
        break;
        case DM_VERTEX_DATA_T_SHORT:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R16_SINT;
            case 2:  return DXGI_FORMAT_R16G16_SINT;
            case 4:  return DXGI_FORMAT_R16G16B16A16_SINT;
            default: return DXGI_FORMAT_R16G16B16A16_SINT;
        }
        break;
        
        case DM_VERTEX_DATA_T_USHORT:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R16_UINT;
            case 2:  return DXGI_FORMAT_R16G16_UINT;
            case 4:  return DXGI_FORMAT_R16G16B16A16_UINT;
            default: return DXGI_FORMAT_R16G16B16A16_UINT;
        }
        break;
        
        case DM_VERTEX_DATA_T_INT:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R32_SINT;
            case 2:  return DXGI_FORMAT_R32G32_SINT;
            case 3:  return DXGI_FORMAT_R32G32B32_SINT;
            case 4:  return DXGI_FORMAT_R32G32B32A32_SINT;
            default: return DXGI_FORMAT_R32G32B32A32_SINT;
        }
        break;
        
        case DM_VERTEX_DATA_T_UINT:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R32_UINT;
            case 2:  return DXGI_FORMAT_R32G32_UINT;
            case 3:  return DXGI_FORMAT_R32G32B32_UINT;
            case 4:  return DXGI_FORMAT_R32G32B32A32_UINT;
            default: return DXGI_FORMAT_R32G32B32A32_UINT;
        }
        break;
        
        case DM_VERTEX_DATA_T_DOUBLE:
        case DM_VERTEX_DATA_T_FLOAT:
        switch (desc.count)
        {
            case 1:  return DXGI_FORMAT_R32_FLOAT;
            case 2:  return DXGI_FORMAT_R32G32_FLOAT;
            case 3:  return DXGI_FORMAT_R32G32B32_FLOAT;
            case 4:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
            default: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        break;
        
        default:
        DM_LOG_FATAL("Unknown vertex format type!");
        return DXGI_FORMAT_UNKNOWN;
	}
}

DM_INLINE
D3D12_INPUT_CLASSIFICATION dm_vertex_class_to_dx12_class(dm_vertex_attrib_class dm_class)
{
	switch (dm_class)
	{
        case DM_VERTEX_ATTRIB_CLASS_VERTEX:   return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        
        default:
		DM_LOG_FATAL("Unknown vertex attribute input class!");
		return DM_VERTEX_ATTRIB_CLASS_UNKNOWN;
	}
}

DM_INLINE
D3D12_CULL_MODE dm_cull_to_dx12_cull(dm_cull_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_CULL_FRONT_BACK:
        case DM_CULL_FRONT:      return D3D12_CULL_MODE_FRONT;
        case DM_CULL_BACK:       return D3D12_CULL_MODE_BACK;
        case DM_CULL_NONE:       return D3D12_CULL_MODE_NONE;
        
        default:
		DM_LOG_FATAL("Unknown cull mode!");
		return D3D12_CULL_MODE_NONE;
	}
}

DM_INLINE
D3D12_PRIMITIVE_TOPOLOGY_TYPE dm_toplogy_to_dx12_topology(dm_primitive_topology dm_top)
{
	switch (dm_top)
	{
        case DM_TOPOLOGY_POINT_LIST:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case DM_TOPOLOGY_LINE_LIST:      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case DM_TOPOLOGY_TRIANGLE_LIST:  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        
        default:
		DM_LOG_FATAL("Unknown primitive topology!");
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}

DM_INLINE
D3D12_PRIMITIVE_TOPOLOGY dm_toplogy_to_dx11_topology(dm_primitive_topology dm_top)
{
	switch (dm_top)
	{
        case DM_TOPOLOGY_POINT_LIST:     return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case DM_TOPOLOGY_LINE_LIST:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case DM_TOPOLOGY_LINE_STRIP:     return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case DM_TOPOLOGY_TRIANGLE_LIST:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case DM_TOPOLOGY_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        
        default:
		DM_LOG_FATAL("Unknown primitive topology!");
		return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

DM_INLINE
D3D12_COMPARISON_FUNC dm_comp_to_directx_comp(dm_comparison dm_comp)
{
	switch (dm_comp)
	{
        case DM_COMPARISON_ALWAYS:   return D3D12_COMPARISON_FUNC_ALWAYS;
        case DM_COMPARISON_NEVER:    return D3D12_COMPARISON_FUNC_NEVER;
        case DM_COMPARISON_EQUAL:    return D3D12_COMPARISON_FUNC_EQUAL;
        case DM_COMPARISON_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case DM_COMPARISON_LESS:     return D3D12_COMPARISON_FUNC_LESS;
        case DM_COMPARISON_LEQUAL:   return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case DM_COMPARISON_GREATER:  return D3D12_COMPARISON_FUNC_GREATER;
        case DM_COMPARISON_GEQUAL:   return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        
        default:
		DM_LOG_FATAL("Unknown comparison function!");
		return D3D12_COMPARISON_FUNC_ALWAYS + 1;
	}
}

DM_INLINE
D3D12_FILTER dm_image_filter_to_dx12_filter(dm_filter filter)
{
	switch (filter)
	{
        case DM_FILTER_NEAREST:
        case DM_FILTER_LINEAR: return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        
        default:
		DM_LOG_FATAL("Unknown filter function!");
		return D3D12_FILTER_MAXIMUM_ANISOTROPIC + 1;
	}
}

DM_INLINE
D3D12_TEXTURE_ADDRESS_MODE dm_texture_mode_to_dx12_mode(dm_texture_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_TEXTURE_MODE_WRAP:          return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case DM_TEXTURE_MODE_BORDER:        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case DM_TEXTURE_MODE_EDGE:          return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case DM_TEXTURE_MODE_MIRROR_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case DM_TEXTURE_MODE_MIRROR_EDGE:   return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        
        default:
		DM_LOG_FATAL("Unknown texture mode!");
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE + 1;
	}
}

DM_INLINE
D3D12_BLEND dm_blend_func_to_dx12_func(dm_blend_func func)
{
    switch(func)
    {
        case DM_BLEND_FUNC_ZERO:                return D3D12_BLEND_ZERO;
        case DM_BLEND_FUNC_ONE:                 return D3D12_BLEND_ONE;
        case DM_BLEND_FUNC_SRC_COLOR:           return D3D12_BLEND_SRC_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
        case DM_BLEND_FUNC_DST_COLOR:           return D3D12_BLEND_DEST_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
        case DM_BLEND_FUNC_SRC_ALPHA:           return D3D12_BLEND_SRC_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
        case DM_BLEND_FUNC_DST_ALPHA:           return D3D12_BLEND_DEST_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
        
        case DM_BLEND_FUNC_CONST_COLOR: 
        case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR:
        case DM_BLEND_FUNC_CONST_ALPHA:
        case DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA: 
        default:
        DM_LOG_ERROR("Unsupported blend for DirectX, returning D3D12_BLEND_SRC_COLOR");
        return D3D12_BLEND_SRC_COLOR;
    }
}

DM_INLINE
D3D12_BLEND_OP dm_blend_eq_to_dx12_op(dm_blend_equation eq)
{
    switch(eq)
    {
        case DM_BLEND_EQUATION_ADD:              return D3D12_BLEND_OP_ADD;
        case DM_BLEND_EQUATION_SUBTRACT:         return D3D12_BLEND_OP_SUBTRACT;
        case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
        case DM_BLEND_EQUATION_MIN:              return D3D12_BLEND_OP_MIN;
        case DM_BLEND_EQUATION_MAX:              return D3D12_BLEND_OP_MAX;
        
        default:
        DM_LOG_ERROR("Unknown blend equation, returning DM_BLEND_OP_ADD");
        return D3D12_BLEND_OP_ADD;
    }
}

// helpers
void dm_dx12_flush(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr = ID3D12CommandQueue_Signal(dx12_renderer->command_queue, dx12_renderer->fence, dx12_renderer->fence_value);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_ERROR("ID3D12CommandQueue_Signal failed");
        return;
    }
    
    uint64_t completed = ID3D12Fence_GetCompletedValue(dx12_renderer->fence);
    
    if(completed < dx12_renderer->fence_value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(dx12_renderer->fence, dx12_renderer->fence_value, dx12_renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_ERROR("ID3D12Fence_SetEventOnCompletion failed");
            return;
        }
        WaitForSingleObject(dx12_renderer->fence_event, INFINITE);
    }
    
    dx12_renderer->fence_value++;
}

/*******
Backend
*********/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing DirectX12 backend..");
    
    HRESULT hr;
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_dx12_renderer));
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    
    IDXGIFactory2* factory;
    IDXGIAdapter1* adapter;
    
#ifdef DM_DEBUG
    hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, &IID_IDXGIFactory2, &factory);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        hr = CreateDXGIFactory2(0, &IID_IDXGIFactory2, &factory);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("CreateDXGIFactory2 failed");
            return false;
        }
    }
#else
    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory2, &factory);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("CreateDXGIFactory2 failed");
        return false;
    }
#endif
    
    // debug layer
#ifdef DM_DEBUG
    {
        hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &dx12_renderer->debug);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("D3D12GetDebugInterface failed");
            return false;
        }
        
        ID3D12Debug_EnableDebugLayer(dx12_renderer->debug);
    }
#endif
    
    // device
    {
        size_t max_dedicated_video_memory = 0;
        for(uint32_t i=0; DXGI_ERROR_NOT_FOUND != IDXGIFactory1_EnumAdapters1(factory, i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 dxgi_adapter_desc1 = { 0 };
            
            IDXGIAdapter1_GetDesc1(adapter, &dxgi_adapter_desc1);
            
            if(dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            
            hr = D3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device5, &dx12_renderer->device);
            bool valid_device = dm_platform_win32_decode_hresult(hr);
            valid_device = valid_device & (dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory);
            if(!valid_device) continue;
            
            max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
            break;
        }
        
        if(!adapter)
        {
            DM_LOG_FATAL("No hardware adapter found. System does not support DX12");
            IDXGIAdapter1_Release(adapter);
            IDXGIFactory2_Release(factory);
            return false;
        }
        
#ifdef DM_RAYTRACING
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = { 0 };
            
            hr = ID3D12Device5_CheckFeatureSupport(dx12_renderer->device, D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options));
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("ID3D12Device5_CheckFeatureSupport failed");
                IDXGIAdapter1_Release(adapter);
                IDXGIFactory2_Release(factory);
                return false;
            }
            
            if(options.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
            {
                DM_LOG_FATAL("Raytracing was requested at compile time, but hardware does not support it");
                IDXGIAdapter1_Release(adapter);
                IDXGIFactory2_Release(factory);
                return false;
            }
        }
#endif
        
        dx12_renderer->handle_increment_size_rtv = ID3D12Device5_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        dx12_renderer->handle_increment_size_cbv_srv_uav = ID3D12Device5_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    
    // more debug
#ifdef DM_DEBUG
    {
        ID3D12InfoQueue* info_queue;
        hr = dx12_renderer->device->lpVtbl->QueryInterface(dx12_renderer->device, &IID_ID3D12InfoQueue, &info_queue);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("IUnknown_QueryInterface failed");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
        
        ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_ERROR, true);
        
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
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
        
        ID3D12InfoQueue_Release(info_queue);
    }
#endif
    
    // command queue
    {
        D3D12_COMMAND_QUEUE_DESC command_queue_desc = { 0 };
        command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        
        hr = ID3D12Device5_CreateCommandQueue(dx12_renderer->device, &command_queue_desc, &IID_ID3D12CommandQueue, &dx12_renderer->command_queue);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandQueue failed");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
    }
    
    // fence
    {
        hr = ID3D12Device5_CreateFence(dx12_renderer->device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &dx12_renderer->fence);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateFence failed");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
        
        dx12_renderer->frame_fence_values[0]++;
        
        dx12_renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!dx12_renderer->fence_event)
        {
            DM_LOG_FATAL("CreateEvent failed");
            hr = HRESULT_FROM_WIN32(GetLastError());
            dm_platform_win32_decode_hresult(hr);
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            
            return false;
        }
    }
    
    // swapchain
    {
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { 0 };
        swap_chain_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferCount      = DM_DX12_NUM_FRAMES;
        swap_chain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        
        dm_internal_w32_data* win32_data = context->platform_data.internal_data;
        IDXGISwapChain1* swap_chain;
        
        IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)dx12_renderer->command_queue, win32_data->hwnd, &swap_chain_desc, NULL, NULL, &swap_chain);
        IDXGISwapChain1_QueryInterface(swap_chain, &IID_IDXGISwapChain3, &dx12_renderer->swap_chain);
        
        // cleanup
        IDXGISwapChain1_Release(swap_chain);
    }
    
    // rtv descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };
        heap_desc.NumDescriptors = DM_DX12_NUM_FRAMES;
        heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        
        hr = ID3D12Device5_CreateDescriptorHeap(dx12_renderer->device, &heap_desc, &IID_ID3D12DescriptorHeap, &dx12_renderer->rtv_descriptor_heap);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateDescriptorHeap failed");
            DM_LOG_FATAL("Could not create DirectX12 render target descriptor heap");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
    }
    
    // depth stencil descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };
        heap_desc.NumDescriptors = 1;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        hr = ID3D12Device5_CreateDescriptorHeap(dx12_renderer->device, &heap_desc, &IID_ID3D12DescriptorHeap, &dx12_renderer->depth_stencil_descriptor_heap);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateDescriptorHeap failed");
            DM_LOG_FATAL("Could not create DirectX12 depth stencil descriptor heap");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
    }
    
    // resource descriptor heap
    // big block of: MAX_CBVs + MAX_TEXs + MAX_SRV -> per frame!
    {
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };;
            heap_desc.NumDescriptors = DM_DX12_DESCRIPTOR_HEAP_MAX_CBV + DM_DX12_DESCRIPTOR_HEAP_MAX_UAV + DM_DX12_DESCRIPTOR_HEAP_MAX_SRV;
            heap_desc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            
            hr = ID3D12Device5_CreateDescriptorHeap(dx12_renderer->device, &heap_desc, &IID_ID3D12DescriptorHeap, &dx12_renderer->resource_descriptor_heap[i]);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("ID3D12Device5_CreateDescriptorHeap failed");
                DM_LOG_FATAL("Could not create DirectX12 resource descriptor heap for frame: %u", i);
                IDXGIFactory2_Release(factory);
                IDXGIAdapter1_Release(adapter);
                return false;
            }
        }
    }
    
    // render target
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
        
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            hr = IDXGISwapChain1_GetBuffer(dx12_renderer->swap_chain, (UINT)i, &IID_ID3D12Resource, &dx12_renderer->render_target[i]);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("IDXGISwapChain1_GetBuffer failed");
                IDXGIFactory2_Release(factory);
                IDXGIAdapter1_Release(adapter);
                return false;
            }
            
            ID3D12Device5_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->render_target[i], NULL, rtv_descriptor_handle);
            rtv_descriptor_handle.ptr += dx12_renderer->handle_increment_size_rtv;
        }
    }
    
    // depth stenicl buffer
    {
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;
        
        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->depth_stencil_descriptor_heap, &depth_stencil_handle);
        
        D3D12_RESOURCE_DESC depth_desc = { 0 };
        depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depth_desc.Format    = DXGI_FORMAT_D32_FLOAT;
        depth_desc.Width     = context->renderer.width;
        depth_desc.Height    = context->renderer.height;
        depth_desc.Flags     = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depth_desc.DepthOrArraySize = 1;
        depth_desc.SampleDesc.Count = 1;
        depth_desc.MipLevels = 1;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &depth_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &dx12_renderer->depth_stencil_buffer);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
            DM_LOG_FATAL("Could not create DirectX12 depth-stencil buffer");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
        
        // view
        D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = { 0 };
        depth_stencil_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_desc.Flags         = D3D12_DSV_FLAG_NONE;
        
        ID3D12Device5_CreateDepthStencilView(dx12_renderer->device, dx12_renderer->depth_stencil_buffer, &depth_stencil_desc, depth_stencil_handle);
    }
    
    // command allocator
    {
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            hr = ID3D12Device5_CreateCommandAllocator(dx12_renderer->device, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &dx12_renderer->command_allocator[i]);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
                IDXGIFactory2_Release(factory);
                IDXGIAdapter1_Release(adapter);
                return false;
            }
        }
    }
    
    // command list
    {
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            hr = ID3D12Device5_CreateCommandList1(dx12_renderer->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, &IID_ID3D12GraphicsCommandList, &dx12_renderer->command_list[i]);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("ID3D12Device5_CreateCommandList1 failed");
                IDXGIFactory2_Release(factory);
                IDXGIAdapter1_Release(adapter);
                return false;
            }
        }
    }
    
    IDXGIFactory2_Release(factory);
    IDXGIAdapter1_Release(adapter);
    
    //
    dm_dx12_flush(dx12_renderer);
    dx12_renderer->current_frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    
    dm_dx12_flush(dx12_renderer);
    dx12_renderer->current_frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    // resource destruction
    for(uint32_t i=0; i<dx12_renderer->pipe_count; i++)
    {
        dm_dx12_renderer_destroy_pipe(&dx12_renderer->pipes[i]);
    }
    
    for(uint32_t i=0; i<dx12_renderer->vb_count; i++)
    {
        dm_dx12_renderer_destroy_vertex_buffer(&dx12_renderer->vertex_buffers[i]);
    }
    
    for(uint32_t i=0; i<dx12_renderer->ib_count; i++)
    {
        dm_dx12_renderer_destroy_index_buffer(&dx12_renderer->index_buffers[i]);
    }
    
    
    for(uint32_t i=0; i<dx12_renderer->cb_count; i++)
    {
        dm_dx12_renderer_destroy_constant_buffer(&dx12_renderer->constant_buffers[i]);
    }
    
#ifdef DM_RAYTRACING
    for(uint32_t i=0; i<dx12_renderer->as_count; i++)
    {
        dm_dx12_renderer_destroy_acceleration_structure(&dx12_renderer->accel_structs[i]);
    }
    
    for(uint32_t i=0; i<dx12_renderer->raytracing_pipes_count; i++)
    {
        dm_dx12_renderer_destroy_raytracing_pipeline(&dx12_renderer->raytracing_pipelines[i]);
    }
#endif
    
    // renderer destruction
    ID3D12Resource_Release(dx12_renderer->depth_stencil_buffer);
    
    IDXGISwapChain3_Release(dx12_renderer->swap_chain);
    
    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_descriptor_heap);
    ID3D12DescriptorHeap_Release(dx12_renderer->depth_stencil_descriptor_heap);
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12DescriptorHeap_Release(dx12_renderer->resource_descriptor_heap[i]);
        ID3D12GraphicsCommandList4_Release(dx12_renderer->command_list[i]);
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocator[i]);
        ID3D12Resource_Release(dx12_renderer->render_target[i]);
    }
    
    ID3D12Fence_Release(dx12_renderer->fence);
    CloseHandle(dx12_renderer->fence_event);
    
    ID3D12CommandQueue_Release(dx12_renderer->command_queue);
    
#ifdef DM_DEBUG
    IDXGIDebug1* dxgi_debug = NULL;
    
    HRESULT hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &dxgi_debug);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_ERROR("DXGIGetDebugInterface1 failed");
    }
    else
    {
        IDXGIDebug1_ReportLiveObjects(dxgi_debug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        IDXGIDebug1_Release(dxgi_debug);
    }
    
    ID3D12Debug_Release(dx12_renderer->debug);
#endif
    
    ID3D12Device5_Release(dx12_renderer->device);
    
    dm_free(&context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    hr = ID3D12CommandAllocator_Reset(command_allocator);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandAllocator_Reset failed");
        return false;
    }
    
    hr = ID3D12GraphicsCommandList4_Reset(command_list, command_allocator, NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList4_Reset failed");
        return false;
    }
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = dx12_renderer->render_target[current_frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(command_list, 1, &barrier);
    
    // render target stuff
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
        rtv_descriptor_handle.ptr += current_frame_index * dx12_renderer->handle_increment_size_rtv;
        
        D3D12_CPU_DESCRIPTOR_HANDLE depth_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->depth_stencil_descriptor_heap, &depth_descriptor_handle);
        
        
        static const FLOAT clear_color[] = { 0,0,0,1 };
        ID3D12GraphicsCommandList4_ClearRenderTargetView(command_list, rtv_descriptor_handle, clear_color, 0, NULL);
        ID3D12GraphicsCommandList4_ClearDepthStencilView(command_list, depth_descriptor_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0,0,NULL);
        ID3D12GraphicsCommandList4_OMSetRenderTargets(command_list, 1, &rtv_descriptor_handle, FALSE, &depth_descriptor_handle);
    }
    
    // viewport and scissor
    {
        D3D12_VIEWPORT viewport = { 0 };
        viewport.Width = (float)renderer->width;
        viewport.Height = (float)renderer->height;
        viewport.MaxDepth = 1.f;
        
        D3D12_RECT scissor = { 0 };
        scissor.right = renderer->width;
        scissor.bottom = renderer->height;
        
        ID3D12GraphicsCommandList4_RSSetViewports(command_list, 1, &viewport);
        ID3D12GraphicsCommandList4_RSSetScissorRects(command_list, 1, &scissor);
    }
    
    // descriptor heap
    {
        ID3D12GraphicsCommandList4_SetDescriptorHeaps(command_list, 1, &dx12_renderer->resource_descriptor_heap[current_frame_index]);
    }
    
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = dx12_renderer->render_target[current_frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(command_list, 1, &barrier);
    
    hr = ID3D12GraphicsCommandList4_Close(command_list);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList4_Close failed");
        return false;
    }
    
    ID3D12GraphicsCommandList4* command_lists[] = {
        command_list
    };
    
    ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, DM_ARRAY_LEN(command_lists), (ID3D12CommandList**)command_lists);
    
    uint32_t v = context->renderer.vsync ? 1 : 0;
    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, v, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }
    
    dm_dx12_flush(dx12_renderer);
    dx12_renderer->frame_fence_values[current_frame_index] = dx12_renderer->fence_value;
    
    dx12_renderer->current_frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_flush(dx12_renderer);
    dx12_renderer->current_frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    ID3D12Resource_Release(dx12_renderer->depth_stencil_buffer);
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(dx12_renderer->render_target[i]);
        
        dx12_renderer->frame_fence_values[i] = dx12_renderer->frame_fence_values[dx12_renderer->current_frame_index];
    }
    
    hr = IDXGISwapChain3_ResizeBuffers(dx12_renderer->swap_chain, 0,renderer->width,renderer->height, DXGI_FORMAT_UNKNOWN, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain3_ResizeBuffers failed");
        return false;
    }
    
    dx12_renderer->current_frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    // resize render target
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
        
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            hr = IDXGISwapChain3_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &dx12_renderer->render_target[i]);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("IDXGISwapChain3_GetBuffer failed");
                return false;
            }
            
            ID3D12Device5_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->render_target[i], NULL, rtv_descriptor_handle);
            rtv_descriptor_handle.ptr += dx12_renderer->handle_increment_size_rtv;
        }
    }
    
    // depth buffer
    {
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;
        
        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->depth_stencil_descriptor_heap, &depth_stencil_handle);
        
        D3D12_RESOURCE_DESC depth_desc = { 0 };
        depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depth_desc.Format    = DXGI_FORMAT_D32_FLOAT;
        depth_desc.Width     = renderer->width;
        depth_desc.Height    = renderer->height;
        depth_desc.Flags     = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depth_desc.DepthOrArraySize = 1;
        depth_desc.SampleDesc.Count = 1;
        depth_desc.MipLevels = 1;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &depth_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &dx12_renderer->depth_stencil_buffer);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommandAllocator failed");
            DM_LOG_FATAL("Could not resize DirectX12 depth-stencil buffer");
            return false;
        }
        
        // view
        D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = { 0 };
        depth_stencil_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_desc.Flags         = D3D12_DSV_FLAG_NONE;
        
        ID3D12Device5_CreateDepthStencilView(dx12_renderer->device, dx12_renderer->depth_stencil_buffer, &depth_stencil_desc, depth_stencil_handle);
    }
    
    return true;
}

/*****************
Resource Creation
*******************/
bool dm_renderer_backend_create_vertex_buffer(const dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_vertex_buffer internal_buffer = { 0 };
    
    D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
    buffer_desc.Width = desc.size;
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_buffer.buffer[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Creating DirectX12 buffer failed");
            return false;
        }
        
        if(desc.data)
        {
            void* ptr = NULL;
            
            ID3D12Resource_Map(internal_buffer.buffer[i], 0, NULL, &ptr);
            if(!dm_platform_win32_decode_hresult(hr))
            {
                DM_LOG_FATAL("ID3D12Resource_Map failed");
                ID3D12Resource_Release(internal_buffer.buffer[i]);
                return false;
            }
            
            dm_memcpy(ptr, desc.data, desc.size);
            ID3D12Resource_Unmap(internal_buffer.buffer[i], 0, NULL);
        }
        
        if(desc.data) dm_dx12_flush(dx12_renderer);
        
        internal_buffer.view[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(internal_buffer.buffer[i]);
        internal_buffer.view[i].StrideInBytes  = desc.stride;
        internal_buffer.view[i].SizeInBytes    = desc.size;
    }
    
    //
    dm_memcpy(dx12_renderer->vertex_buffers + dx12_renderer->vb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = dx12_renderer->vb_count++;
    
    return true;
}

bool dm_renderer_backend_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_index_buffer internal_buffer = { 0 };
    
    D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
    buffer_desc.Width = desc.size;
    
    hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_buffer.buffer);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
        DM_LOG_FATAL("Creating DirectX12 buffer failed");
        return false;
    }
    
    assert(desc.data);
    {
        void* ptr = NULL;
        
        ID3D12Resource_Map(internal_buffer.buffer, 0, NULL, &ptr);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            ID3D12Resource_Release(internal_buffer.buffer);
            return false;
        }
        
        dm_memcpy(ptr, desc.data, desc.size);
        ID3D12Resource_Unmap(internal_buffer.buffer, 0, NULL);
    }
    
    dm_dx12_flush(dx12_renderer);
    
    internal_buffer.view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(internal_buffer.buffer);
    internal_buffer.view.SizeInBytes    = desc.size;
    internal_buffer.view.Format = desc.data_type==DM_INDEX_BUFFER_DATA_UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    
    //
    dm_memcpy(dx12_renderer->index_buffers + dx12_renderer->ib_count, &internal_buffer, sizeof(internal_buffer));
    *handle = dx12_renderer->ib_count++;
    
    return true;
}

void dm_dx12_renderer_destroy_vertex_buffer(dm_dx12_vertex_buffer* buffer)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(buffer->buffer[i]);
    }
}

void dm_dx12_renderer_destroy_index_buffer(dm_dx12_index_buffer* buffer)
{
    ID3D12Resource_Release(buffer->buffer);
}

bool dm_renderer_backend_create_constant_buffer(const void* data, size_t data_size, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    
    dm_dx12_constant_buffer internal_buffer = { 0 };
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        D3D12_RESOURCE_DESC desc = DM_DX12_BASIC_BUFFER_DESC;
        desc.Width = (255 + data_size) & ~255;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_buffer.buffer[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            return false;
        }
        
        hr = ID3D12Resource_Map(internal_buffer.buffer[i], 0, NULL, &internal_buffer.mapped_address[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            return false;
        }
        
        if(!data) continue;
        
        dm_memcpy(internal_buffer.mapped_address[i], data, data_size);
    }
    
    //
    dm_memcpy(dx12_renderer->constant_buffers + dx12_renderer->cb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = dx12_renderer->cb_count++;
    
    return true;
}

void dm_dx12_renderer_destroy_constant_buffer(dm_dx12_constant_buffer* buffer)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(buffer->buffer[i]);
    }
}

bool dm_dx12_create_input_element(dm_vertex_attrib_desc attrib_desc, D3D12_INPUT_ELEMENT_DESC* element_desc)
{
    DXGI_FORMAT format = dm_vertex_t_to_dxgi_format(attrib_desc);
    if (format == DXGI_FORMAT_UNKNOWN) return false;
    D3D12_INPUT_CLASSIFICATION input_class = dm_vertex_class_to_dx12_class(attrib_desc.attrib_class);
    if (input_class == DM_VERTEX_ATTRIB_CLASS_UNKNOWN) return false;
    
    element_desc->SemanticName = attrib_desc.name;
    element_desc->SemanticIndex = attrib_desc.index;
    element_desc->Format = format;
    element_desc->AlignedByteOffset = attrib_desc.offset;
    element_desc->InputSlotClass = input_class;
    if (input_class == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA)
    {
        element_desc->InstanceDataStepRate = 1;
        element_desc->InputSlot = 1;
    }
    
    return true;
}

bool dm_renderer_backend_create_pipeline(const dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    assert(desc.vertex_shader_data && desc.pixel_shader_data);
    
    dm_dx12_pipeline internal_pipe = { 0 };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe_desc  = { 0 };
    D3D12_RENDER_TARGET_BLEND_DESC     blend_desc = { 0 };
    D3D12_DEPTH_STENCIL_DESC           depth_desc = { 0 };
    
    // blending
    if(desc.flags & DM_PIPELINE_FLAG_BLEND)
    {
        blend_desc.SrcBlend  = dm_blend_func_to_dx12_func(desc.blend_src_f);
        blend_desc.DestBlend = dm_blend_func_to_dx12_func(desc.blend_dest_f);
        blend_desc.BlendOp   = dm_blend_eq_to_dx12_op(desc.blend_eq);
        
        blend_desc.SrcBlendAlpha  = dm_blend_func_to_dx12_func(desc.blend_src_alpha_f);
        blend_desc.DestBlendAlpha = dm_blend_func_to_dx12_func(desc.blend_dest_alpha_f);
        blend_desc.BlendOpAlpha   = dm_blend_eq_to_dx12_op(desc.blend_alpha_eq);
        
        blend_desc.LogicOp               = D3D12_LOGIC_OP_NOOP;
        blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    
    // depth
    if(desc.flags & DM_PIPELINE_FLAG_DEPTH)
    {
        depth_desc.DepthEnable    = TRUE;
        depth_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depth_desc.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
        pipe_desc.DSVFormat       = DXGI_FORMAT_D32_FLOAT;
    }
    
    pipe_desc.BlendState.RenderTarget[0] = blend_desc;
    pipe_desc.DepthStencilState          = depth_desc;
    
    // rasterizer state
    {
        pipe_desc.RasterizerState.FillMode = desc.flags & DM_PIPELINE_FLAG_WIREFRAME ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
        pipe_desc.RasterizerState.CullMode = dm_cull_to_dx12_cull(desc.cull_mode);
    }
    
    // misc
    {
        internal_pipe.topology = dm_toplogy_to_dx11_topology(desc.primitive_topology);
        pipe_desc.PrimitiveTopologyType = dm_toplogy_to_dx12_topology(desc.primitive_topology);
        
        pipe_desc.NumRenderTargets = 1;
        pipe_desc.RTVFormats[0]    = DXGI_FORMAT_R8G8B8A8_UNORM;
        pipe_desc.SampleDesc.Count = 1;
        pipe_desc.SampleMask       = 0xFFFFFFFF;
    }
    
    switch(desc.winding_order)
    {
        case DM_WINDING_CLOCK:
        pipe_desc.RasterizerState.FrontCounterClockwise = false;
        break;
        
        case DM_WINDING_COUNTER_CLOCK:
        pipe_desc.RasterizerState.FrontCounterClockwise = true;
        break;
        
        default:
        DM_LOG_FATAL("Unknown winding order");
        return false;
    }
    
    // shaders
    pipe_desc.VS.pShaderBytecode = desc.vertex_shader_data;
    pipe_desc.VS.BytecodeLength  = desc.vertex_shader_size;
    
    pipe_desc.PS.pShaderBytecode = desc.pixel_shader_data;
    pipe_desc.PS.BytecodeLength  = desc.pixel_shader_size;
    
    D3D12_INPUT_ELEMENT_DESC* input_desc = dm_alloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * 4 * desc.attrib_count);
    uint32_t count = 0;
    
    // vertex attributes
    for(uint32_t i=0; i<desc.attrib_count; i++)
    {
        dm_vertex_attrib_desc attrib_desc = desc.attribs[i];
        
        if ((attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            for (uint32_t j = 0; j < attrib_desc.count; j++)
            {
                dm_vertex_attrib_desc sub_desc = attrib_desc;
                if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) sub_desc.data_t = DM_VERTEX_DATA_T_INT;
                else sub_desc.data_t = DM_VERTEX_DATA_T_FLOAT;
                
                sub_desc.offset = sub_desc.offset + sizeof(float) * j;
                
                D3D12_INPUT_ELEMENT_DESC element_desc = { 0 };
                if (!dm_dx12_create_input_element(sub_desc, &element_desc)) return false;
                element_desc.SemanticIndex = j;
                element_desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
                
                input_desc[count++] = element_desc;
            }
        }
        else
        {
            D3D12_INPUT_ELEMENT_DESC element_desc = { 0 };
            if (!dm_dx12_create_input_element(attrib_desc, &element_desc)) return false;
            
            // append the element_desc to the array
            input_desc[count++] = element_desc;
        }	
    }
    
    pipe_desc.InputLayout.pInputElementDescs = input_desc;
    pipe_desc.InputLayout.NumElements        = count;
    
    // root signature
    {
        D3D12_ROOT_PARAMETER params[5] = { 0 };
        uint32_t param_count = 0;
        
        if(desc.cb_count>0)
        {
            D3D12_ROOT_DESCRIPTOR cbv_descriptor = { 0 };
            
            D3D12_DESCRIPTOR_RANGE cbv_range[1] = { 0 };
            cbv_range[0].RangeType      = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            cbv_range[0].NumDescriptors = desc.cb_count;
            cbv_range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            
            D3D12_ROOT_DESCRIPTOR_TABLE cbv_table = { 0 };
            cbv_table.NumDescriptorRanges = DM_ARRAY_LEN(cbv_range);
            cbv_table.pDescriptorRanges   = cbv_range;
            
            params[param_count].ParameterType   = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[param_count].DescriptorTable = cbv_table;
            
            internal_pipe.cb_count = desc.cb_count;
            
            param_count++;
        }
        
        D3D12_ROOT_SIGNATURE_DESC root_desc = { 0 };
        root_desc.NumParameters = param_count;
        root_desc.pParameters   = params;
        root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ID3DBlob* blob = NULL;
        ID3DBlob* error_blob = NULL;
        hr = D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &error_blob);
        if (!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("D3D12SerializeRootSignature failed");
            DM_LOG_FATAL("%s", (char*)ID3D10Blob_GetBufferPointer(error_blob));
            return false;
        }
        
        hr = ID3D12Device5_CreateRootSignature(dx12_renderer->device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &internal_pipe.root_signature);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateRootSignature failed");
            DM_LOG_FATAL("Could not create root signature for raytracing pipeline");
            return false;
        }
        
        ID3D10Blob_Release(blob);
    }
    
    pipe_desc.pRootSignature = internal_pipe.root_signature;
    
    // state object
    hr = ID3D12Device5_CreateGraphicsPipelineState(dx12_renderer->device, &pipe_desc, &IID_ID3D12PipelineState, &internal_pipe.state);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateGraphicsPipelineState failed");
        return false;
    }
    
    // resource views
    // TODO: have no idea how to handle the handle pointer moving
    if(desc.cb_count>0)
    {
        for(uint32_t i=0; i<desc.cb_count; i++)
        {
            for(uint32_t j=0; j<DM_DX12_NUM_FRAMES; j++)
            {
                ID3D12Resource* buffer = dx12_renderer->constant_buffers[desc.cbs[i]].buffer[j];
                if(!buffer)
                {
                    DM_LOG_FATAL("DirectX12 pipeline contains invalid constant buffer reference");
                    return false;
                }
                
                D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = { 0 };
                view_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(buffer);
                view_desc.SizeInBytes    = (255 + desc.cb_sizes[i]) & ~255;
                
                D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
                ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->resource_descriptor_heap[j], &handle);
                
                ID3D12Device5_CreateConstantBufferView(dx12_renderer->device, &view_desc, handle);
            }
        }
    }
    
    //
    dm_memcpy(dx12_renderer->pipes + dx12_renderer->pipe_count, &internal_pipe, sizeof(internal_pipe));
    *handle = dx12_renderer->pipe_count++;
    
    dm_free(&input_desc);
    
    return true;
}

void dm_dx12_renderer_destroy_pipe(dm_dx12_pipeline* pipe)
{
    ID3D12RootSignature_Release(pipe->root_signature);
    ID3D12PipelineState_Release(pipe->state);
}

bool dm_renderer_backend_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

#ifdef DM_RAYTRACING
// acceleration structure
DM_INLINE
D3D12_RAYTRACING_GEOMETRY_TYPE dm_blas_geom_type_to_dx12_geom_type(dm_blas_geometry_type type)
{
    switch(type)
    {
        case DM_BLAS_GEOMETRY_TYPE_TRIANGLES: return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        
        default:
        DM_LOG_ERROR("Unknown raytracing geometry type! Returning triangles...");
        return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    }
}

DM_INLINE
D3D12_RAYTRACING_GEOMETRY_FLAGS dm_blas_geom_flag_to_dx12_geom_flag(dm_blas_geometry_flag flag)
{
    switch(flag)
    {
        case DM_BLAS_GEOMETRY_FLAG_OPAQUE: return D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        
        default:
        DM_LOG_ERROR("Unknown raytracing geometry flag! Returning opaque...");
        return D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    }
}

bool dm_renderer_backend_create_acceleration_structure(dm_acceleration_structure_desc as_desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_acceleration_structure internal_as = { 0 };
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    
    // blas
    {
        ID3D12Resource* vertex_buffer = NULL;
        ID3D12Resource* index_buffer = NULL;
        
        for(uint32_t i=0; i<as_desc.blas_count; i++)
        {
            dm_blas_desc blas_desc = as_desc.blas_descs[i];
            assert(blas_desc.vertex_buffer!=DM_RENDER_HANDLE_INVALID);
            
            vertex_buffer = dx12_renderer->vertex_buffers[blas_desc.vertex_buffer].buffer[current_frame_index];
            index_buffer  = blas_desc.index_buffer!=DM_RENDER_HANDLE_INVALID ? dx12_renderer->index_buffers[blas_desc.index_buffer].buffer : NULL;
            
            D3D12_RAYTRACING_GEOMETRY_DESC geom_desc = { 0 };
            geom_desc.Type  = dm_blas_geom_type_to_dx12_geom_type(blas_desc.geom_type);
            geom_desc.Flags = dm_blas_geom_flag_to_dx12_geom_flag(blas_desc.geom_flag);
            
            switch(geom_desc.Type)
            {
                case D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES:
                {
                    geom_desc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
                    geom_desc.Triangles.VertexCount                = blas_desc.vertex_count;
                    geom_desc.Triangles.VertexBuffer.StartAddress  = ID3D12Resource_GetGPUVirtualAddress(vertex_buffer);
                    geom_desc.Triangles.VertexBuffer.StrideInBytes = blas_desc.vertex_size;
                    
                    if(index_buffer)
                    {
                        DXGI_FORMAT index_size;
                        switch(blas_desc.index_size)
                        {
                            case 2:
                            index_size = DXGI_FORMAT_R16_UINT;
                            break;
                            
                            case 4:
                            index_size = DXGI_FORMAT_R32_UINT;
                            break;
                            
                            default:
                            DM_LOG_FATAL("Invalid index size: %u", blas_desc.index_size * 4);
                            return false;
                        }
                        
                        geom_desc.Triangles.IndexFormat = index_size;
                        geom_desc.Triangles.IndexCount  = blas_desc.index_count;
                        geom_desc.Triangles.IndexBuffer = ID3D12Resource_GetGPUVirtualAddress(index_buffer);
                    }
                } break;
                
                default:
                DM_LOG_FATAL("DirectX12 Raytracing geometry type not supported");
                return false;
            }
            
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = { 0 };
            inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            inputs.NumDescs       = 1;
            inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
            inputs.pGeometryDescs = &geom_desc;
            
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = { 0 };
            ID3D12Device5_GetRaytracingAccelerationStructurePrebuildInfo(dx12_renderer->device, &inputs, &prebuild_info);
            
            // scratch buffer
            {
                D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
                buffer_desc.Width = prebuild_info.ScratchDataSizeInBytes;
                buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_COMMON, NULL, &IID_ID3D12Resource, &internal_as.blas[i].scratch_buffer);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_ERROR("Could not create scratch buffer for DirectX12 acceleration structure");
                    return false;
                }
            }
            
            // result buffer
            {
                D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
                buffer_desc.Width = prebuild_info.ResultDataMaxSizeInBytes;
                buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, &IID_ID3D12Resource, &internal_as.blas[i].result_buffer);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_ERROR("Could not create result buffer for DirectX12 acceleration structure");
                    return false;
                }
            }
            
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = { 0 };
            build_desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(internal_as.blas[i].result_buffer);
            build_desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(internal_as.blas[i].scratch_buffer);
            build_desc.Inputs                           = inputs;
            
            const uint32_t current_frame_index = dx12_renderer->current_frame_index;
            ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
            ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
            
            ID3D12CommandAllocator_Reset(command_allocator);
            ID3D12GraphicsCommandList4_Reset(command_list, command_allocator, NULL);
            ID3D12GraphicsCommandList4_BuildRaytracingAccelerationStructure(command_list, &build_desc, 0, NULL);
            ID3D12GraphicsCommandList4_Close(command_list);
            
            ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, 1, (ID3D12CommandList**)&command_list);
            
            dm_dx12_flush(dx12_renderer);
            
            internal_as.blas_count++;
        }
    }
    
    // tlas
    {
        // instances
        {
            internal_as.tlas.instance_count = as_desc.tlas_desc.instance_count;
            
            D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
            buffer_desc.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * internal_as.tlas.instance_count;
            
            for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
            {
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_as.tlas.instance_buffer[i]);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_FATAL("Could not create acceleration structure top-level instance buffer");
                    return false;
                }
                
                ID3D12Resource_Map(internal_as.tlas.instance_buffer[i], 0, NULL, (void**)&internal_as.tlas.instance_data[i]);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Resource_Map failed");
                    return false;
                }
                
                for(uint32_t j=0; j<internal_as.tlas.instance_count; j++)
                {
                    uint32_t blas_index         = as_desc.tlas_desc.mesh_instance[j];
                    ID3D12Resource* blas_buffer = internal_as.blas[blas_index].result_buffer;
                    
                    internal_as.tlas.instance_data[i][j].InstanceID   = j;
                    internal_as.tlas.instance_data[i][j].InstanceMask = 1;
                    internal_as.tlas.instance_data[i][j].AccelerationStructure = ID3D12Resource_GetGPUVirtualAddress(blas_buffer);
                    if(i==0) continue;
                    
                    internal_as.tlas.instance_data[i][j].InstanceContributionToHitGroupIndex = (j * 2) + 2;
                }
            }
        }
        
        // build
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs[DM_DX12_NUM_FRAMES] = { 0 };
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info[DM_DX12_NUM_FRAMES] = { 0 };
        
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            inputs[i].Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            inputs[i].Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
            inputs[i].NumDescs      = internal_as.tlas.instance_count;
            inputs[i].DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
            inputs[i].InstanceDescs = ID3D12Resource_GetGPUVirtualAddress(internal_as.tlas.instance_buffer[i]);
            
            ID3D12Device5_GetRaytracingAccelerationStructurePrebuildInfo(dx12_renderer->device, &inputs[i], &prebuild_info[i]);
            
            internal_as.tlas.update_scratch_size = prebuild_info[i].UpdateScratchDataSizeInBytes;
        }
        
        // scratch
        {
            for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
            {
                D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
                buffer_desc.Width = prebuild_info[i].ScratchDataSizeInBytes;
                buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_COMMON, NULL, &IID_ID3D12Resource, &internal_as.tlas.scratch_buffer[i]);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_FATAL("Could not create scratch buffer for TLAS");
                    return false;
                }
            }
        }
        
        // result
        {
            for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
            {
                D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
                buffer_desc.Width = prebuild_info[i].ResultDataMaxSizeInBytes;
                buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, &IID_ID3D12Resource, &internal_as.tlas.result_buffer[i]);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_FATAL("Could not create result buffer for TLAS");
                    return false;
                }
            }
        }
        
        const uint32_t current_frame_index = dx12_renderer->current_frame_index;
        ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
        ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
        
        for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = { 0 };
            build_desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(internal_as.tlas.result_buffer[i]);
            build_desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(internal_as.tlas.scratch_buffer[i]);
            build_desc.Inputs                           = inputs[i];
            
            
            ID3D12CommandAllocator_Reset(command_allocator);
            ID3D12GraphicsCommandList4_Reset(command_list, command_allocator, NULL);
            ID3D12GraphicsCommandList4_BuildRaytracingAccelerationStructure(command_list, &build_desc, 0, NULL);
            ID3D12GraphicsCommandList4_Close(command_list);
            
            ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, 1, (ID3D12CommandList**)&command_list);
            dm_dx12_flush(dx12_renderer);
        }
        
        // update scratch buffer
        {
            D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
            buffer_desc.Width = internal_as.tlas.update_scratch_size;
            buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            
            for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
            {
                hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_COMMON, NULL, &IID_ID3D12Resource, &internal_as.tlas.update_scratch_buffer[i]);
                if(!dm_platform_win32_decode_hresult(hr))
                {
                    DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                    DM_LOG_FATAL("Could not create update scratch buffer for TLAS");
                    return false;
                }
            }
        }
    }
    
    //
    dm_memcpy(dx12_renderer->accel_structs + dx12_renderer->as_count, &internal_as, sizeof(internal_as));
    *handle = dx12_renderer->as_count++;
    
    return true;
}

void dm_dx12_renderer_destroy_acceleration_structure(dm_dx12_acceleration_structure* as)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(as->tlas.scratch_buffer[i]);
        ID3D12Resource_Release(as->tlas.result_buffer[i]);
        ID3D12Resource_Release(as->tlas.update_scratch_buffer[i]);
        ID3D12Resource_Release(as->tlas.instance_buffer[i]);
    }
    
    
    for(uint32_t i=0; i<as->blas_count; i++)
    {
        ID3D12Resource_Release(as->blas[i].scratch_buffer);
        ID3D12Resource_Release(as->blas[i].result_buffer);
    }
}

// raytracing pipeline
bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_raytracing_pipeline internal_pipe = { 0 };
    
    // root signature
    {
        D3D12_ROOT_PARAMETER params[4] = { 0 }; // output, textures, buffers, constant buffers
        
        D3D12_DESCRIPTOR_RANGE ranges[4] = { 0 };
        
        for(uint32_t i=0; i<desc.resource_count; i++)
        {
            dm_raytracing_pipeline_resource resource = desc.resources[i];
            
            switch(resource.type)
            {
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_OUTPUT_TEXTURE:
                ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
                
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_BUFFER:
                ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
                
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_CONSTANT_BUFFER:
                ranges[i].RangeType    = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                internal_pipe.cb_count = resource.count;
                break;
                
                default:
                DM_LOG_FATAL("Unknown or unsupported raytracing pipeline resource type for DirectX12");
                return false;
            }
            
            ranges[i].NumDescriptors                    = resource.count;
            ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            
            params[i].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[i].DescriptorTable.NumDescriptorRanges = 1;
            params[i].DescriptorTable.pDescriptorRanges   = &ranges[i];
        }
        
        // create the root signature
        D3D12_ROOT_SIGNATURE_DESC root_desc = { 0 };
        root_desc.NumParameters = desc.resource_count;
        root_desc.pParameters   = params;
        
        ID3DBlob* blob = NULL;
        ID3DBlob* error_blob = NULL;
        hr = D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &error_blob);
        if (!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("D3D12SerializeRootSignature failed");
            if(error_blob)
            {
                DM_LOG_FATAL("%s", (char*)ID3D10Blob_GetBufferPointer(error_blob));
            }
            return false;
        }
        
        hr = ID3D12Device5_CreateRootSignature(dx12_renderer->device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &internal_pipe.root_signature);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateRootSignature failed");
            DM_LOG_FATAL("Could not create root signature for raytracing pipeline");
            return false;
        }
        
        dm_memcpy(internal_pipe.resources, desc.resources, sizeof(desc.resources));
        internal_pipe.resource_count = desc.resource_count;
        
        ID3D10Blob_Release(blob);
    }
    
    // PSO
    {
        D3D12_DXIL_LIBRARY_DESC lib = { 0 };
        lib.DXILLibrary.pShaderBytecode = desc.shader_data;
        lib.DXILLibrary.BytecodeLength  = desc.shader_data_size;
        
        D3D12_HIT_GROUP_DESC hit_group = { 0 };
        
        hit_group.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        wchar_t ws[2][100];
        
        {
            swprintf(ws[0], 100, L"%hs", desc.hit_groups[0].name);
            hit_group.HitGroupExport = ws[0];
        }
        
        {
            swprintf(ws[1], 100, L"%hs", desc.hit_groups[0].closest_hit);
            hit_group.ClosestHitShaderImport = ws[1];
        }
        
        D3D12_RAYTRACING_SHADER_CONFIG shader_config = { 0 };
        shader_config.MaxPayloadSizeInBytes   = desc.payload_size;
        shader_config.MaxAttributeSizeInBytes = sizeof(float) * 2;
        
        D3D12_GLOBAL_ROOT_SIGNATURE global_sig = { internal_pipe.root_signature };
        
        D3D12_RAYTRACING_PIPELINE_CONFIG pipe_config = { 0 };
        pipe_config.MaxTraceRecursionDepth = desc.max_recursion;
        
        D3D12_STATE_SUBOBJECT subobjects[10];
        
        uint32_t sub_obj_index = 0;
        {
            subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            subobjects[sub_obj_index++].pDesc = &lib;
            
            subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            subobjects[sub_obj_index++].pDesc = &hit_group;
            
            subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            subobjects[sub_obj_index++].pDesc = &shader_config;
            
            subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;;
            subobjects[sub_obj_index++].pDesc = &global_sig;
            
            subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            subobjects[sub_obj_index++].pDesc = &pipe_config;
        }
        
        D3D12_STATE_OBJECT_DESC obj_desc = { 0 };
        obj_desc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        obj_desc.NumSubobjects = sub_obj_index;
        obj_desc.pSubobjects   = subobjects;
        
        hr = ID3D12Device5_CreateStateObject(dx12_renderer->device, &obj_desc, &IID_ID3D12StateObject, &internal_pipe.state_object);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateStateObject failed");
            DM_LOG_FATAL("Could not create raytracing pipeline state object");
            return false;
        }
    }
    
    // shader ids
    {
        D3D12_RESOURCE_DESC id_desc = DM_DX12_BASIC_BUFFER_DESC;
        id_desc.Width = DM_DX12_RAYTRACE_NUM_SHADER_IDS * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &id_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_pipe.shader_ids);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Could not create raytracing pipeline shader ids");
            return false;
        }
        
        ID3D12StateObjectProperties* props;
        ID3D12StateObject_QueryInterface(internal_pipe.state_object, &IID_ID3D12StateObjectProperties, &props);
        
        void* data = NULL;
        
        hr = ID3D12Resource_Map(internal_pipe.shader_ids, 0, NULL, &data);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            return false;
        }
        
        wchar_t ws[3][100];
        void* id   = NULL;
        {
            swprintf(ws[0], 100, L"%hs", desc.raygen);
            id = ID3D12StateObjectProperties_GetShaderIdentifier(props, ws[0]);
            dm_memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            data = (char*)data + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        }
        
        {
            swprintf(ws[1], 100, L"%hs", desc.hit_groups[0].miss);
            id = ID3D12StateObjectProperties_GetShaderIdentifier(props, ws[1]);
            dm_memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            data = (char*)data + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        }
        
        {
            swprintf(ws[2], 100, L"%hs", desc.hit_groups[0].name);
            id = ID3D12StateObjectProperties_GetShaderIdentifier(props, ws[2]);
            dm_memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            data = (char*)data + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        }
        
        data = NULL;
        
        ID3D12Resource_Unmap(internal_pipe.shader_ids, 0, NULL);
        
        ID3D12StateObjectProperties_Release(props);
    }
    
    // resource creation and view creation
    {
        for(uint32_t i=0; i<desc.resource_count; i++)
        {
            dm_raytracing_pipeline_resource resource = desc.resources[i];
            
            switch(resource.type)
            {
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_OUTPUT_TEXTURE:
                {
                    D3D12_RESOURCE_DESC image_desc = { 0 };
                    image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                    image_desc.Width            = renderer->width;
                    image_desc.Height           = renderer->height;
                    image_desc.DepthOrArraySize = 1;
                    image_desc.MipLevels        = 1;
                    image_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
                    image_desc.SampleDesc.Count = 1;
                    image_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                    
                    hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &image_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, &internal_pipe.output_image);
                    if(!dm_platform_win32_decode_hresult(hr))
                    {
                        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
                        DM_LOG_FATAL("Could not create raytracing pipeline output image");
                        return false;
                    }
                } break;
                
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_BUFFER:
                {
                    for(uint32_t j=0; j<resource.count; j++)
                    {
                        // acceleration structures
                        if(resource.is_as[j]) continue;
                        
                        // buffers
                        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
                        view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        view_desc.Format = DXGI_FORMAT_UNKNOWN;
                        view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                        view_desc.Buffer.StructureByteStride = resource.strides[j];
                        view_desc.Buffer.NumElements         = resource.counts[j];
                        
                        for(uint32_t k=0; k<DM_DX12_NUM_FRAMES; k++)
                        {
                            ID3D12DescriptorHeap* heap = dx12_renderer->resource_descriptor_heap[k];
                            
                        }
                    }
                } break;
                
                case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_CONSTANT_BUFFER:
                {
                    dm_dx12_constant_buffer* cb = NULL;
                    
                    for(uint32_t i=0; i<resource.count; i++)
                    {
                        cb = &dx12_renderer->constant_buffers[resource.handles[i]];
                        
                        const size_t buffer_size = (255 + resource.sizes[i]) & ~255;
                        
                        // for each frame
                        for(uint32_t j=0; j<DM_DX12_NUM_FRAMES; j++)
                        {
                            if(!cb->buffer[j])
                            {
                                DM_LOG_FATAL("DirectX12 raytracing pipeline has invalid constant buffer reference");
                                return false;
                            }
                            
                            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = { 0 };
                            cbv_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(cb->buffer[j]);
                            cbv_desc.SizeInBytes    = buffer_size;
                            
                            D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
                            ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->resource_descriptor_heap[j], &handle);
                            handle.ptr += dx12_renderer->handle_increment_size_cbv_srv_uav + i * dx12_renderer->handle_increment_size_cbv_srv_uav;
                            
                            ID3D12Device5_CreateConstantBufferView(dx12_renderer->device, &cbv_desc, handle);
                        }
                    }
                } break;
            }
        }
    }
    
    //
    dm_memcpy(dx12_renderer->raytracing_pipelines + dx12_renderer->raytracing_pipes_count, &internal_pipe, sizeof(internal_pipe));
    *handle = dx12_renderer->raytracing_pipes_count++;
    
    return true;
}

void dm_dx12_renderer_destroy_raytracing_pipeline(dm_dx12_raytracing_pipeline* pipe)
{
    ID3D12Resource_Release(pipe->output_image);
    ID3D12Resource_Release(pipe->shader_ids);
    ID3D12StateObject_Release(pipe->state_object);
    ID3D12RootSignature_Release(pipe->root_signature);
}
#endif

/********
COMMANDS
**********/
bool dm_render_command_backend_begin_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_end_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    
    if(handle > dx12_renderer->pipe_count || dx12_renderer->pipe_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 pipeline");
        return false;
    }
    
    dm_dx12_pipeline* internal_pipe = &dx12_renderer->pipes[handle];
    
    const uint32_t current_frame = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame];
    
    // root sig, state, topology
    {
        ID3D12GraphicsCommandList4_SetGraphicsRootSignature(command_list, internal_pipe->root_signature);
        ID3D12GraphicsCommandList4_SetPipelineState(command_list, internal_pipe->state);
        ID3D12GraphicsCommandList4_IASetPrimitiveTopology(command_list, internal_pipe->topology);
    }
    
    if(internal_pipe->cb_count==0) return true;
    
    // constant buffers
#if 1
    {
        ID3D12DescriptorHeap* descriptor_heap = dx12_renderer->resource_descriptor_heap[current_frame];
        
        D3D12_GPU_DESCRIPTOR_HANDLE descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(descriptor_heap, &descriptor_handle);
        
        // number is the ROOT paramter index, 
        ID3D12GraphicsCommandList4_SetGraphicsRootDescriptorTable(command_list, 0, descriptor_handle);
    }
#endif
    
    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    
    if(handle > dx12_renderer->vb_count || dx12_renderer->vb_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 vertex buffer");
        return false;
    }
    
    dm_dx12_vertex_buffer* internal_buffer = &dx12_renderer->vertex_buffers[handle];
    
    const uint32_t current_frame = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame];
    
    ID3D12GraphicsCommandList4_IASetVertexBuffers(command_list, slot,1, &internal_buffer->view[current_frame]);
    
    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    
    if(handle > dx12_renderer->ib_count || dx12_renderer->ib_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 index buffer");
        return false;
    }
    
    dm_dx12_index_buffer* internal_buffer = &dx12_renderer->index_buffers[handle];
    
    const uint32_t current_frame = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame];
    
    ID3D12GraphicsCommandList4_IASetIndexBuffer(command_list, &internal_buffer->view);
    
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->cb_count || dx12_renderer->cb_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 constant buffer");
        return false;
    }
    
    dm_dx12_constant_buffer* internal_buffer = &dx12_renderer->constant_buffers[handle];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame_index];
    
    ID3D12GraphicsCommandList4_SetGraphicsRootConstantBufferView(command_list, slot, ID3D12Resource_GetGPUVirtualAddress(internal_buffer->buffer[current_frame_index]));
    
    return true;
}

bool dm_render_command_backend_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->vb_count || dx12_renderer->vb_count==0)
    {
        DM_LOG_FATAL("Trying to update invalid DirectX12 vertex buffer");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    dm_dx12_vertex_buffer* internal_buffer = &dx12_renderer->vertex_buffers[handle];
    
    void* pData = NULL;
    
    hr = ID3D12Resource_Map(internal_buffer->buffer[current_frame_index], 0, NULL, &pData);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Resource_Map failed");
        DM_LOG_FATAL("Could not update DirectX12 vertex buffer");
        return false;
    }
    
    dm_memcpy(pData, data, data_size);
    
    ID3D12Resource_Unmap(internal_buffer->buffer[current_frame_index], 0, NULL);
    
    pData = NULL;
    
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->cb_count || dx12_renderer->cb_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 constant buffer");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    
    dm_dx12_constant_buffer* internal_buffer = &dx12_renderer->constant_buffers[handle];
    
    void* dest = NULL;
    dest = internal_buffer->mapped_address[current_frame_index];
    
    if(!dest)
    {
        DM_LOG_FATAL("DirecX12 constant buffer has invalid mapped address");
        return false;
    }
    
    dm_memcpy(dest, data, data_size);
    
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_instanced(uint32_t index_count, uint32_t vertex_count, uint32_t inst_count, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    
    const uint32_t current_frame = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame];
    
    ID3D12GraphicsCommandList4_DrawIndexedInstanced(command_list, index_count, inst_count, index_offset, vertex_offset, inst_offset);
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
}

#ifdef DM_RAYTRACING
bool dm_render_command_backend_bind_acceleration_structure(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->as_count || dx12_renderer->as_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 acceleration structure");
        return false;
    }
    
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[handle];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    ID3D12Resource* result_buffer = internal_as->tlas.result_buffer[current_frame_index];
    ID3D12GraphicsCommandList4_SetComputeRootShaderResourceView(command_list, slot, ID3D12Resource_GetGPUVirtualAddress(result_buffer));
    
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_instance(dm_render_handle handle, uint32_t instance_id, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->as_count || dx12_renderer->as_count==0)
    {
        DM_LOG_FATAL("Trying to update invalid DirectX12 acceleration structure");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[handle];
    if(instance_id > internal_as->tlas.instance_count)
    {
        DM_LOG_FATAL("Trying to update invalid instance in DirectX12 acceleration structure");
        return false;
    }
    
    dm_memcpy(internal_as->tlas.instance_data[current_frame_index][instance_id].Transform, data, sizeof(float) * 4 * 3);
    
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_tlas(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->as_count || dx12_renderer->as_count==0)
    {
        DM_LOG_FATAL("Trying to update invalid DirectX12 acceleration structure");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[handle];
    
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = { 0 };
        desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(internal_as->tlas.result_buffer[current_frame_index]);
        desc.Inputs.Type                      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        desc.Inputs.Flags                     = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        desc.Inputs.NumDescs                  = internal_as->tlas.instance_count;
        desc.Inputs.DescsLayout               = D3D12_ELEMENTS_LAYOUT_ARRAY;
        desc.Inputs.InstanceDescs             = ID3D12Resource_GetGPUVirtualAddress(internal_as->tlas.instance_buffer[current_frame_index]);
        desc.SourceAccelerationStructureData  = ID3D12Resource_GetGPUVirtualAddress(internal_as->tlas.result_buffer[current_frame_index]);
        desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(internal_as->tlas.update_scratch_buffer[current_frame_index]);
        
        
        ID3D12GraphicsCommandList4_BuildRaytracingAccelerationStructure(command_list, &desc, 0, NULL);
    }
    
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = internal_as->tlas.result_buffer[current_frame_index];
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->raytracing_pipes_count || dx12_renderer->raytracing_pipes_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 raytracing pipeline");
        return false;
    }
    
    dm_dx12_raytracing_pipeline* internal_pipe = &dx12_renderer->raytracing_pipelines[handle];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list    = dx12_renderer->command_list[current_frame_index];
    ID3D12DescriptorHeap*       descriptor_heap = dx12_renderer->resource_descriptor_heap[current_frame_index];
    
    // resize output and depth buffer
    if(internal_pipe->output_width!=renderer->width || internal_pipe->output_height!=renderer->height)
    {
        ID3D12Resource_Release(internal_pipe->output_image);
        
        D3D12_RESOURCE_DESC image_desc = { 0 };
        image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        image_desc.Width            = renderer->width;
        image_desc.Height           = renderer->height;
        image_desc.DepthOrArraySize = 1;
        image_desc.MipLevels        = 1;
        image_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        image_desc.SampleDesc.Count = 1;
        image_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &image_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, &internal_pipe->output_image);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Could not create raytracing pipeline output image");
            return false;
        }
        
        internal_pipe->output_width  = renderer->width;
        internal_pipe->output_height = renderer->height;
        
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        
        // output is first, so we don't need to offset
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(descriptor_heap, &descriptor_handle);
        
        ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, internal_pipe->output_image, NULL, &desc, descriptor_handle);
    }
    
    // state, root sig
    {
        ID3D12GraphicsCommandList4_SetPipelineState1(command_list, internal_pipe->state_object);
        ID3D12GraphicsCommandList4_SetComputeRootSignature(command_list, internal_pipe->root_signature);
    }
    
    // resource binding
    D3D12_GPU_DESCRIPTOR_HANDLE descriptor_handle = { 0 };
    ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(descriptor_heap, &descriptor_handle);
    
    for(uint32_t i=0; i<internal_pipe->resource_count; i++)
    {
        dm_raytracing_pipeline_resource resource = internal_pipe->resources[i];
        
        switch(resource.type)
        {
            case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_OUTPUT_TEXTURE:
            ID3D12GraphicsCommandList4_SetComputeRootDescriptorTable(command_list, i, descriptor_handle);
            break;
            
            case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_BUFFER:
            {
            } break;
            
            case DM_RAYTRACING_PIPELINE_RESOURCE_TYPE_CONSTANT_BUFFER:
            {
                descriptor_handle.ptr += dx12_renderer->handle_increment_size_cbv_srv_uav;
                ID3D12GraphicsCommandList4_SetComputeRootDescriptorTable(command_list, i, descriptor_handle);
            } break;
        }
    }
    
    return true;
}

bool dm_render_command_backend_dispatch_rays(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->raytracing_pipes_count || dx12_renderer->raytracing_pipes_count==0)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 raytracing pipeline");
        return false;
    }
    
    dm_dx12_raytracing_pipeline* internal_pipe = &dx12_renderer->raytracing_pipelines[handle];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    D3D12_RESOURCE_DESC render_target_desc = { 0 };
    ID3D12Resource_GetDesc(internal_pipe->output_image, &render_target_desc);
    
    
    D3D12_DISPATCH_RAYS_DESC desc = { 0 };
    desc.RayGenerationShaderRecord.StartAddress = ID3D12Resource_GetGPUVirtualAddress(internal_pipe->shader_ids);
    desc.RayGenerationShaderRecord.SizeInBytes  = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    
    desc.MissShaderTable.StartAddress = ID3D12Resource_GetGPUVirtualAddress(internal_pipe->shader_ids) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    desc.MissShaderTable.SizeInBytes  = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    
    desc.HitGroupTable.StartAddress = ID3D12Resource_GetGPUVirtualAddress(internal_pipe->shader_ids) + 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    desc.HitGroupTable.SizeInBytes  = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    
    desc.Width  = render_target_desc.Width;
    desc.Height = render_target_desc.Height;
    desc.Depth  = 1;
    
    ID3D12GraphicsCommandList4_DispatchRays(command_list, &desc);
    
    ID3D12Resource* back_buffer   = dx12_renderer->render_target[dx12_renderer->current_frame_index];
    ID3D12Resource* render_target = internal_pipe->output_image;
    
    // barriers
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = render_target;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = back_buffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    ID3D12GraphicsCommandList4_CopyResource(command_list, back_buffer, render_target);
    
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = back_buffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = render_target;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    return true;
}
#endif // dm_raytracing

#endif