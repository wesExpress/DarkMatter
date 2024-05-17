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

//#define DM_DX12_GPU_BASED_VALIDATION
#define DM_DX12_DRED_VALIDATION

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

typedef enum dm_dx12_resource_type_t
{
    DM_DX12_RESOURCE_TYPE_VERTEX_BUFFER,
    DM_DX12_RESOURCE_TYPE_INDEX_BUFFER,
    DM_DX12_RESOURCE_TYPE_CONSTANT_BUFFER,
    DM_DX12_RESOURCE_TYPE_TEXTURE,
} dm_dx12_resource_type;

typedef struct dm_dx12_resource_t
{
    dm_dx12_resource_type type;
    size_t heap_offset;
    
    union
    {
        ID3D12Resource* buffer[DM_DX12_NUM_FRAMES];
        ID3D12Resource* textures[DM_DX12_NUM_FRAMES]; 
    };
    
    void* mapped_address;
} dm_dx12_resource;

typedef struct dm_dx12_vertex_buffer_t
{
    ID3D12Resource* buffer[DM_DX12_NUM_FRAMES];
    void*           mapped_addresses[DM_DX12_NUM_FRAMES];
    size_t          stride, count;
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    ID3D12Resource* buffer;
    size_t          count;
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    ID3D12Resource* buffer[DM_DX12_NUM_FRAMES];
    void*           mapped_address[DM_DX12_NUM_FRAMES];
    size_t          size;
} dm_dx12_constant_buffer;

typedef struct dm_dx12_structured_buffer_t
{
    ID3D12Resource* buffer[DM_DX12_NUM_FRAMES];
    size_t          stride, count;
} dm_dx12_structured_buffer;

typedef struct dm_dx12_texture_t
{
    ID3D12Resource* texture[DM_DX12_NUM_FRAMES];
} dm_dx12_texture;

#ifdef DM_RAYTRACING
typedef struct dm_dx12_blas_t
{
    ID3D12Resource* scratch_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* result_buffer[DM_DX12_NUM_FRAMES];
} dm_dx12_blas;

typedef struct dm_dx12_tlas_t
{
    uint32_t instance_count, max_instance_count;
    size_t   update_scratch_size;
    
    D3D12_RAYTRACING_INSTANCE_DESC* instance_data[DM_DX12_NUM_FRAMES];
    
    ID3D12Resource* instance_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* scratch_buffer[DM_DX12_NUM_FRAMES];
    ID3D12Resource* result_buffer[DM_DX12_NUM_FRAMES];
} dm_dx12_tlas;

#define DM_DX12_MAX_BLAS 10
typedef struct dm_dx12_acceleration_structure_t
{
    dm_dx12_tlas tlas;
    
    dm_dx12_blas blas[DM_DX12_MAX_BLAS];
    uint32_t     blas_count;
} dm_dx12_acceleration_structure;

typedef struct dm_dx12_rt_shader_binding_table_t
{
    ID3D12Resource* table[DM_DX12_NUM_FRAMES];
    uint8_t*        mapped_address[DM_DX12_NUM_FRAMES];
    
    ID3D12RootSignature* global_root_signature;
    ID3D12RootSignature* raygen_root_signature;
    ID3D12RootSignature* miss_root_signatures[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS];
    ID3D12RootSignature* hit_root_signatures[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS];
    
    uint32_t record_count, miss_count, hit_group_count, max_instance_count, instance_count;
    size_t   record_size;
    
    size_t global_heap_offset;
} dm_dx12_rt_shader_binding_table;

typedef struct dm_dx12_raytracing_pipeline_t
{
    ID3D12StateObject*   state_object;
    
    dm_dx12_rt_shader_binding_table sbt;
    
    uint32_t descriptor_heap_offset;
} dm_dx12_raytracing_pipeline;

#define DM_DX12_MAX_ACCELERATION_STRUCTURES 10
#define DM_DX12_MAX_RAYTRACING_PIPELINES    10
#endif

// attempt at descriptor heap sizing / offsetting
#define DM_DX12_DESCRIPTOR_HEAP_MAX     1000
#define DM_DX12_DESCRIPTOR_HEAP_MAX_SRV 2 * DM_DX12_DESCRIPTOR_HEAP_MAX
#define DM_DX12_DESCRIPTOR_HEAP_MAX_CBV DM_DX12_DESCRIPTOR_HEAP_MAX
#define DM_DX12_DESCRIPTOR_HEAP_MAX_UAV DM_DX12_DESCRIPTOR_HEAP_MAX

#define DM_DX12_MAX_RESOURCE 1000
#define DM_DX12_MAX_PASSES    10
#define DM_DX12_MAX_PIPES     10
#define DM_DX12_MAX_BUFFERS   3 * DM_DX12_MAX_RESOURCE
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
    
    uint32_t current_frame_index, descriptor_heap_offset;
    uint32_t pass_count, pipe_count, vb_count, ib_count, cb_count, sb_count, texture_count, resource_count;
    
    dm_dx12_renderpass        passes[DM_DX12_MAX_PASSES];
    dm_dx12_pipeline          pipes[DM_DX12_MAX_PIPES];
    dm_dx12_vertex_buffer     vertex_buffers[DM_DX12_MAX_BUFFERS];
    dm_dx12_index_buffer      index_buffers[DM_DX12_MAX_BUFFERS];
    dm_dx12_constant_buffer   constant_buffers[DM_DX12_MAX_BUFFERS];
    dm_dx12_structured_buffer structured_buffers[DM_DX12_MAX_RESOURCE];
    dm_dx12_texture           textures[DM_DX12_MAX_RESOURCE];
    
    dm_dx12_resource resources[DM_DX12_MAX_RESOURCE];
    
#ifdef DM_RAYTRACING
    uint32_t as_count, rt_pipe_count;
    
    dm_dx12_acceleration_structure accel_structs[DM_DX12_MAX_ACCELERATION_STRUCTURES];
    dm_dx12_raytracing_pipeline    raytracing_pipelines[DM_DX12_MAX_RAYTRACING_PIPELINES];
#endif
    
    // misc
    uint32_t handle_increment_size_cbv_srv_uav, handle_increment_size_rtv;
    
#ifdef DM_DEBUG
    ID3D12Debug* debug;
#ifdef DM_DX12_DRED_VALIDATION
    ID3D12DeviceRemovedExtendedDataSettings* dred_settings;
#endif // dred
#endif // debug
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
void dm_dx12_renderer_destroy_structured_buffer(dm_dx12_structured_buffer* buffer);
void dm_dx12_renderer_destroy_texture(dm_dx12_texture* texture);

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
    HRESULT hr;
    
    hr = ID3D12CommandQueue_Signal(dx12_renderer->command_queue, dx12_renderer->fence, dx12_renderer->fence_value);
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
            DM_LOG_FATAL("Debug interface initialization failed");
            return false;
        }
        
        ID3D12Debug_EnableDebugLayer(dx12_renderer->debug);
        
#ifdef DM_DX12_GPU_BASED_VALIDATION
        ID3D12Debug1* debug_controller;
        
        hr = ID3D12Debug_QueryInterface(dx12_renderer->debug, &IID_ID3D12Debug1, &debug_controller);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Debug_QueryInterface failed");
            DM_LOG_FATAL("GPU Validation layer initialization failed");
            return false;
        }
        
        ID3D12Debug1_SetEnableGPUBasedValidation(debug_controller, true);
#endif
        
#ifdef DM_DX12_DRED_VALIDATION
        hr = D3D12GetDebugInterface(&IID_ID3D12DeviceRemovedExtendedDataSettings, &dx12_renderer->dred_settings);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("D3D12GetDebugInterface");
            DM_LOG_FATAL("DRED Settings initialization failed");
            return false;
        }
        
        ID3D12DeviceRemovedExtendedDataSettings_SetAutoBreadcrumbsEnablement(dx12_renderer->dred_settings, D3D12_DRED_ENABLEMENT_FORCED_ON);
        ID3D12DeviceRemovedExtendedDataSettings_SetPageFaultEnablement(dx12_renderer->dred_settings, D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
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
        hr = ID3D12Device5_QueryInterface(dx12_renderer->device, &IID_ID3D12InfoQueue, &info_queue);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("IUnknown_QueryInterface failed");
            IDXGIFactory2_Release(factory);
            IDXGIAdapter1_Release(adapter);
            return false;
        }
        
        ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
#ifndef DM_DX12_DRED_VALIDATION
        ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        
        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_MESSAGE_ID       deny_ids[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
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
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };
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
#ifdef DM_RAYTRACING
        depth_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
#endif
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
    
    for(uint8_t i=0; i<dx12_renderer->sb_count; i++)
    {
        dm_dx12_renderer_destroy_structured_buffer(&dx12_renderer->structured_buffers[i]);
    }
    
    for(uint32_t i=0; i<dx12_renderer->texture_count; i++)
    {
        dm_dx12_renderer_destroy_texture(&dx12_renderer->textures[i]);
    }
    
#ifdef DM_RAYTRACING
    for(uint32_t i=0; i<dx12_renderer->as_count; i++)
    {
        dm_dx12_renderer_destroy_acceleration_structure(&dx12_renderer->accel_structs[i]);
    }
    
    for(uint32_t i=0; i<dx12_renderer->rt_pipe_count; i++)
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
    
#ifdef DM_DX12_DRED_VALIDATION
    ID3D12DeviceRemovedExtendedDataSettings_Release(dx12_renderer->dred_settings);
#endif
    
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
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = dx12_renderer->render_target[current_frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(command_list, 1, &barrier);
    
    // render target stuff
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
        rtv_descriptor_handle.ptr += current_frame_index * dx12_renderer->handle_increment_size_rtv;
        
        D3D12_CPU_DESCRIPTOR_HANDLE depth_descriptor_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->depth_stencil_descriptor_heap, &depth_descriptor_handle);
        
        static const FLOAT clear_color[] = { 1,0,1,1 };
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

#ifdef DM_DEBUG
#ifdef DM_DX12_DRED_VALIDATION
void dm_dx12_track_down_device_removal(dm_dx12_renderer* dx12_renderer);
#endif
#endif

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = dx12_renderer->render_target[current_frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
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
        
#ifdef DM_DEBUG
#ifdef DM_DX12_DRED_VALIDATION
        dm_dx12_track_down_device_removal(dx12_renderer);
#endif
#endif
        
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
#ifdef DM_RAYTRACING
        depth_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
#endif
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
    
    internal_buffer.stride = desc.stride;
    internal_buffer.count  = desc.count;
    
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
        
        ID3D12Resource_Map(internal_buffer.buffer[i], 0, NULL, &internal_buffer.mapped_addresses[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            ID3D12Resource_Release(internal_buffer.buffer[i]);
            return false;
        }
        
        if(!desc.data) continue;
        
        dm_memcpy(internal_buffer.mapped_addresses[i], desc.data, desc.size);
        
        dm_dx12_flush(dx12_renderer);
    }
    
    //
    dm_memcpy(dx12_renderer->vertex_buffers + dx12_renderer->vb_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->vb_count++);
    
    return true;
}

bool dm_renderer_backend_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_index_buffer internal_buffer = { 0 };
    internal_buffer.count = desc.count;
    
    D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
    buffer_desc.Width = desc.size;
    
    hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_buffer.buffer);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
        DM_LOG_FATAL("Creating DirectX12 index buffer failed");
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
    
    //
    dm_memcpy(dx12_renderer->index_buffers + dx12_renderer->ib_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->ib_count++);
    
    return true;
}

void dm_dx12_renderer_destroy_vertex_buffer(dm_dx12_vertex_buffer* buffer)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Unmap(buffer->buffer[i], 0, NULL);
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
    const size_t size = (255 + data_size) & ~255;
    
    dm_dx12_constant_buffer internal_buffer = { 0 };
    internal_buffer.size = size;
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        D3D12_RESOURCE_DESC desc = DM_DX12_BASIC_BUFFER_DESC;
        desc.Width = size;
        
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
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->cb_count++);
    
    return true;
}

void dm_dx12_renderer_destroy_constant_buffer(dm_dx12_constant_buffer* buffer)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(buffer->buffer[i]);
    }
}

bool dm_renderer_backend_create_structured_buffer(const dm_structured_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_structured_buffer internal_buffer = { 0 };
    internal_buffer.stride = desc.stride;
    internal_buffer.count  = desc.count;
    
    D3D12_RESOURCE_DESC resource_desc = DM_DX12_BASIC_BUFFER_DESC;
    resource_desc.Width = desc.size;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    for(uint8_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, &internal_buffer.buffer[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            return false;
        }
    }
    
    //
    dm_memcpy(dx12_renderer->structured_buffers + dx12_renderer->sb_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->sb_count++);
    
    return true;
}

void dm_dx12_renderer_destroy_structured_buffer(dm_dx12_structured_buffer* buffer)
{
    for(uint8_t i=0; i<DM_DX12_NUM_FRAMES; i++)
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
    pipe_desc.RasterizerState.FillMode = desc.flags & DM_PIPELINE_FLAG_WIREFRAME ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    pipe_desc.RasterizerState.CullMode = dm_cull_to_dx12_cull(desc.cull_mode);
    
    // misc
    internal_pipe.topology = dm_toplogy_to_dx11_topology(desc.primitive_topology);
    pipe_desc.PrimitiveTopologyType = dm_toplogy_to_dx12_topology(desc.primitive_topology);
    
    pipe_desc.NumRenderTargets = 1;
    pipe_desc.RTVFormats[0]    = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipe_desc.SampleDesc.Count = 1;
    pipe_desc.SampleMask       = 0xFFFFFFFF;
    
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
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->pipe_count++);
    
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

DM_INLINE
DXGI_FORMAT dm_texture_data_type_to_dxgi_format(dm_texture_desc desc)
{
    switch(desc.data_count)
    {
        case 1:
        switch(desc.data_type)
        {
            case DM_TEXTURE_DATA_TYPE_INT_8:   return DXGI_FORMAT_R8_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_8:  return DXGI_FORMAT_R8_UINT;
            case DM_TEXTURE_DATA_TYPE_UNORM_8: return DXGI_FORMAT_R8_UNORM;
            
            case DM_TEXTURE_DATA_TYPE_INT_16:   return DXGI_FORMAT_R16_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_16:  return DXGI_FORMAT_R16_UINT;
            case DM_TEXTURE_DATA_TYPE_UNORM_16: return DXGI_FORMAT_R16_UNORM;
            case DM_TEXTURE_DATA_TYPE_FLOAT_16: return DXGI_FORMAT_R16_FLOAT;
            
            case DM_TEXTURE_DATA_TYPE_INT_32:   return DXGI_FORMAT_R32_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_32:  return DXGI_FORMAT_R32_UINT;
            case DM_TEXTURE_DATA_TYPE_FLOAT_32: return DXGI_FORMAT_R32_FLOAT;
        }
        break;
        
        case 2:
        switch(desc.data_type)
        {
            case DM_TEXTURE_DATA_TYPE_INT_8:    return DXGI_FORMAT_R8G8_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_8:   return DXGI_FORMAT_R8G8_UINT;
            case DM_TEXTURE_DATA_TYPE_UNORM_8:  return DXGI_FORMAT_R8G8_UNORM;
            
            case DM_TEXTURE_DATA_TYPE_FLOAT_16: return DXGI_FORMAT_R16G16_FLOAT;
            case DM_TEXTURE_DATA_TYPE_INT_16:   return DXGI_FORMAT_R16G16_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_16:  return DXGI_FORMAT_R16G16_UINT;
            
            case DM_TEXTURE_DATA_TYPE_FLOAT_32: return DXGI_FORMAT_R32G32_FLOAT;
            case DM_TEXTURE_DATA_TYPE_INT_32:   return DXGI_FORMAT_R32G32_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_32:  return DXGI_FORMAT_R32G32_UINT;
        }
        break;
        
        case 3:
        switch(desc.data_type)
        {
            case DM_TEXTURE_DATA_TYPE_FLOAT_32: return DXGI_FORMAT_R32G32B32_FLOAT;
            case DM_TEXTURE_DATA_TYPE_INT_32:   return DXGI_FORMAT_R32G32B32_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_32:  return DXGI_FORMAT_R32G32B32_UINT;
        }
        break;
        
        case 4:
        switch(desc.data_type)
        {
            case DM_TEXTURE_DATA_TYPE_INT_8:    return DXGI_FORMAT_R8G8B8A8_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_8:   return DXGI_FORMAT_R8G8B8A8_UINT;
            case DM_TEXTURE_DATA_TYPE_UNORM_8:  return DXGI_FORMAT_R8G8B8A8_UNORM;
            
            case DM_TEXTURE_DATA_TYPE_FLOAT_16: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DM_TEXTURE_DATA_TYPE_INT_16:   return DXGI_FORMAT_R16G16B16A16_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_16:  return DXGI_FORMAT_R16G16B16A16_UINT;
            
            case DM_TEXTURE_DATA_TYPE_FLOAT_32: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case DM_TEXTURE_DATA_TYPE_INT_32:   return DXGI_FORMAT_R32G32B32A32_SINT;
            case DM_TEXTURE_DATA_TYPE_UINT_32:  return DXGI_FORMAT_R32G32B32A32_UINT;
        }
        break;
    }
    
    DM_LOG_FATAL("Unknown or invalid texture data type");
    return DXGI_FORMAT_UNKNOWN;
}

bool dm_renderer_backend_create_texture(dm_texture_desc texture_desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_texture internal_texture = { 0 };
    
    D3D12_RESOURCE_DESC desc = { 0 };
    desc.DepthOrArraySize = 1;
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Format           = dm_texture_data_type_to_dxgi_format(texture_desc);
    desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    desc.Width            = texture_desc.width;
    desc.Height           = texture_desc.height;
    desc.MipLevels        = 1;
    desc.SampleDesc.Count = 1;
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, &internal_texture.texture[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            return false;
        }
    }
    
    //
    dm_memcpy(dx12_renderer->textures + dx12_renderer->texture_count, &internal_texture, sizeof(internal_texture));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->texture_count++);
    
    return true;
}

bool dm_renderer_backend_resize_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_TEXTURE)
    {
        DM_LOG_FATAL("Trying to resize resource that is not a texture");
        return false;
    }
    
    dm_dx12_texture* internal_texture = &dx12_renderer->textures[handle];
    ID3D12DescriptorHeap* heap = NULL;
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        heap = dx12_renderer->resource_descriptor_heap[i];
        
        ID3D12Resource_Release(internal_texture->texture[i]);
        
        D3D12_RESOURCE_DESC image_desc = { 0 };
        image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        image_desc.Width            = width;
        image_desc.Height           = height;
        image_desc.DepthOrArraySize = 1;
        image_desc.MipLevels        = 1;
        image_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        image_desc.SampleDesc.Count = 1;
        image_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &image_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, &internal_texture->texture[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Could not resize DirectX12 texture");
            return false;
        }
    }
    
    return true;
}

void dm_dx12_renderer_destroy_texture(dm_dx12_texture* texture)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(texture->texture[i]);
    }
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

DM_INLINE 
bool dm_dx12_create_acceleration_structure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs, ID3D12Resource** scratch_buffer, ID3D12Resource** result_buffer, size_t* update_scratch_size, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = { 0 };
    ID3D12Device5_GetRaytracingAccelerationStructurePrebuildInfo(dx12_renderer->device, &inputs, &prebuild_info);
    
    if(update_scratch_size) *update_scratch_size = prebuild_info.UpdateScratchDataSizeInBytes;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    // scratch buffer
    D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
    buffer_desc.Width = prebuild_info.ScratchDataSizeInBytes;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_COMMON, NULL, &IID_ID3D12Resource, scratch_buffer);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
        DM_LOG_FATAL("Could not create scratch buffer");
        return false;
    }
    
    // result buffer
    buffer_desc       = DM_DX12_BASIC_BUFFER_DESC;
    buffer_desc.Width = prebuild_info.ResultDataMaxSizeInBytes;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, &IID_ID3D12Resource, result_buffer);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
        DM_LOG_FATAL("Could not create result buffer");
        return false;
    }
    
    // acceleration structure
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = { 0 };
    build_desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(*result_buffer);
    build_desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(*scratch_buffer);
    build_desc.Inputs                           = inputs;
    
    ID3D12CommandAllocator_Reset(command_allocator);
    ID3D12GraphicsCommandList4_Reset(command_list, command_allocator, NULL);
    ID3D12GraphicsCommandList4_BuildRaytracingAccelerationStructure(command_list, &build_desc, 0, NULL);
    ID3D12GraphicsCommandList4_Close(command_list);
    
    ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, 1, (ID3D12CommandList**)&command_list);
    dm_dx12_flush(dx12_renderer);
    
    ///
    return true;
}

DM_INLINE
bool dm_dx12_create_blas(dm_blas_desc blas_desc, dm_dx12_acceleration_structure* internal_as, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    ID3D12Resource* vertex_buffer = NULL;
    ID3D12Resource* index_buffer  = NULL;
    
    if(DM_RENDER_HANDLE_GET_TYPE(blas_desc.vertex_buffer)==DM_RENDER_RESOURCE_TYPE_INVALID)
    {
        DM_LOG_FATAL("Trying to create DirectX12 blas with invalid vertex buffer");
        return false;
    }
    
    const uint32_t blas_count = internal_as->blas_count;
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        // blas geometry filling in
        vertex_buffer = dx12_renderer->vertex_buffers[DM_RENDER_HANDLE_GET_INDEX(blas_desc.vertex_buffer)].buffer[i];
        index_buffer  = DM_RENDER_HANDLE_GET_TYPE(blas_desc.index_buffer)!=DM_RENDER_RESOURCE_TYPE_INVALID ? dx12_renderer->index_buffers[DM_RENDER_HANDLE_GET_INDEX(blas_desc.index_buffer)].buffer : NULL;
        
        D3D12_RAYTRACING_GEOMETRY_DESC geom_desc = { 0 };
        geom_desc.Type  = dm_blas_geom_type_to_dx12_geom_type(blas_desc.geom_type);
        geom_desc.Flags = dm_blas_geom_flag_to_dx12_geom_flag(blas_desc.geom_flag);
        
        switch(geom_desc.Type)
        {
            case D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES:
            {
                geom_desc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
                geom_desc.Triangles.VertexCount                = blas_desc.vertex_count;
                geom_desc.Triangles.VertexBuffer.StrideInBytes = blas_desc.vertex_size;
                geom_desc.Triangles.VertexBuffer.StartAddress  = ID3D12Resource_GetGPUVirtualAddress(vertex_buffer);
                
                if(index_buffer)
                {
                    switch(blas_desc.index_size)
                    {
                        case 2:
                        geom_desc.Triangles.IndexFormat= DXGI_FORMAT_R16_UINT;
                        break;
                        
                        case 4:
                        geom_desc.Triangles.IndexFormat= DXGI_FORMAT_R32_UINT;
                        break;
                        
                        default:
                        DM_LOG_FATAL("Unsupported index size for DirectX12 BLAS");
                        return false;
                    }
                    
                    geom_desc.Triangles.IndexCount  = blas_desc.index_count;
                    geom_desc.Triangles.IndexBuffer = ID3D12Resource_GetGPUVirtualAddress(index_buffer);
                }
            } break;
            
            default:
            DM_LOG_FATAL("Unsupported or unknown DirectX12 BLAS geometry type");
            return false;
        }
        
        // actual acceleration structure
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = { 0 };
        inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        inputs.NumDescs       = 1;
        inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.pGeometryDescs = &geom_desc;
        
        if(!dm_dx12_create_acceleration_structure(inputs, &internal_as->blas[blas_count].scratch_buffer[i], &internal_as->blas[blas_count].result_buffer[i], NULL, dx12_renderer)) return false;
    }
    
    internal_as->blas_count++;
    
    vertex_buffer = NULL;
    index_buffer  = NULL;
    
    return true;
}

bool dm_dx12_create_tlas(dm_acceleration_structure_desc as_desc, dm_dx12_acceleration_structure* internal_as, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    if(!as_desc.tlas_desc.instance_transforms)
    {
        DM_LOG_FATAL("Need to have non-NULL transforms to initialize acceleration structure");
        return false;
    }
    if(!as_desc.tlas_desc.instance_masks)
    {
        DM_LOG_FATAL("Need to have non-NULL instance masks to initialize acceleration structure");
        return false;
    }
    
    internal_as->tlas.max_instance_count = as_desc.tlas_desc.max_instance_count;
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocator[current_frame_index];
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        // instance buffer
        D3D12_RESOURCE_DESC buffer_desc = DM_DX12_BASIC_BUFFER_DESC;
        buffer_desc.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * as_desc.tlas_desc.max_instance_count;
        
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_as->tlas.instance_buffer[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Could not create acceleration structure top-level instance buffer");
            return false;
        }
        
        ID3D12Resource_Map(internal_as->tlas.instance_buffer[i], 0, NULL, (void**)&internal_as->tlas.instance_data[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            return false;
        }
        
        uint32_t i_offset = 0;
        for(uint32_t j=0; j<as_desc.tlas_desc.instance_count; j++)
        {
            uint32_t blas_index         = as_desc.tlas_desc.instance_meshes[j];
            ID3D12Resource* blas_buffer = internal_as->blas[blas_index].result_buffer[i];
            
            internal_as->tlas.instance_data[i][j].InstanceID   = j;
            internal_as->tlas.instance_data[i][j].InstanceMask = as_desc.tlas_desc.instance_masks[j];
            
            dm_memcpy(internal_as->tlas.instance_data[i][j].Transform, as_desc.tlas_desc.instance_transforms[j], sizeof(float) * 4 * 3);
            
            internal_as->tlas.instance_data[i][j].AccelerationStructure = ID3D12Resource_GetGPUVirtualAddress(blas_buffer);
            internal_as->tlas.instance_data[i][j].InstanceContributionToHitGroupIndex = i_offset;
            
            i_offset += 1;
        }
        
        // actual acceleration structure
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = { 0 };
        inputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        inputs.NumDescs      = internal_as->tlas.max_instance_count;
        inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.InstanceDescs = ID3D12Resource_GetGPUVirtualAddress(internal_as->tlas.instance_buffer[i]);
        
        if(!dm_dx12_create_acceleration_structure(inputs, &internal_as->tlas.scratch_buffer[i], &internal_as->tlas.result_buffer[i], &internal_as->tlas.update_scratch_size, dx12_renderer)) return false;
    }
    
    return true;
}

bool dm_renderer_backend_create_acceleration_structure(dm_acceleration_structure_desc as_desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_acceleration_structure internal_as = { 0 };
    
    for(uint32_t i=0; i<as_desc.blas_count; i++)
    {
        if(!dm_dx12_create_blas(as_desc.blas_descs[i], &internal_as, dx12_renderer)) return false;
    }
    
    if(!dm_dx12_create_tlas(as_desc, &internal_as, dx12_renderer)) return false;
    
    //
    dm_memcpy(dx12_renderer->accel_structs + dx12_renderer->as_count, &internal_as, sizeof(internal_as));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->as_count++);
    
    return true;
}

void dm_dx12_renderer_destroy_acceleration_structure(dm_dx12_acceleration_structure* as)
{
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(as->tlas.scratch_buffer[i]);
        ID3D12Resource_Release(as->tlas.result_buffer[i]);
        
        ID3D12Resource_Unmap(as->tlas.instance_buffer[i], 0, NULL);
        as->tlas.instance_data[i] = NULL;
        ID3D12Resource_Release(as->tlas.instance_buffer[i]);
        
        for(uint32_t j=0; j<as->blas_count; j++)
        {
            ID3D12Resource_Release(as->blas[j].scratch_buffer[i]);
            ID3D12Resource_Release(as->blas[j].result_buffer[i]);
        }
    }
}

// raytracing pipeline
DM_INLINE
void dm_dx12_rt_sbt_add_to_range(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t shader_register, D3D12_DESCRIPTOR_RANGE* ranges, uint32_t* range_index, uint32_t* range_count)
{
    if(*range_index == -1)
    {
        *range_index = (*range_count)++;
        ranges[*range_index].RangeType                         = type;
        ranges[*range_index].BaseShaderRegister                = shader_register;
        ranges[*range_index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }
    
    ranges[*range_index].NumDescriptors++;
}

DM_INLINE
void dm_dx12_rt_sbt_add_uav_to_range(uint32_t shader_register, D3D12_DESCRIPTOR_RANGE* ranges, uint32_t* range_index, uint32_t* range_count)
{
    dm_dx12_rt_sbt_add_to_range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shader_register, ranges, range_index, range_count);
}

DM_INLINE
void dm_dx12_rt_sbt_add_srv_to_range(uint32_t shader_register, D3D12_DESCRIPTOR_RANGE* ranges, uint32_t* range_index, uint32_t* range_count)
{
    dm_dx12_rt_sbt_add_to_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, ranges, range_index, range_count);
}

DM_INLINE
void dm_dx12_rt_sbt_add_cbv_to_range(uint32_t shader_register, D3D12_DESCRIPTOR_RANGE* ranges, uint32_t* range_index, uint32_t* range_count)
{
    dm_dx12_rt_sbt_add_to_range(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, shader_register, ranges, range_index, range_count);
}

DM_INLINE
bool dm_dx12_raytracing_pipeline_sbt_make_root_signature(uint32_t shader_stage, dm_rt_pipeline_shader_params params, ID3D12RootSignature** root_sig, ID3D12Device5* device)
{
    HRESULT hr;
    
    D3D12_ROOT_SIGNATURE_DESC sig_desc       = { 0 };
    D3D12_DESCRIPTOR_RANGE    ranges[3]      = { 0 }; // uav, srv, cbv supported
    D3D12_ROOT_PARAMETER      root_parameter = { 0 };
    
    int uav_index = -1;
    int srv_index = -1;
    int cbv_index = -1;
    
    uint32_t range_count  = 0;
    
    dm_render_resource_type type;
    uint16_t                index;
    uint8_t                 r;
    
    for(uint32_t i=0; i<params.count; i++)
    {
        type  = DM_RENDER_HANDLE_GET_TYPE(params.handles[i]);
        index = DM_RENDER_HANDLE_GET_INDEX(params.handles[i]);
        
        r = params.registers[i];
        
        switch(type)
        {
            case DM_RENDER_RESOURCE_TYPE_TEXTURE:
            case DM_RENDER_RESOURCE_TYPE_STRUCTURED_BUFFER:
            dm_dx12_rt_sbt_add_uav_to_range(r, ranges, &uav_index, &range_count);
            break;
            
            case DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER:
            case DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER:
            case DM_RENDER_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE:
            dm_dx12_rt_sbt_add_srv_to_range(r, ranges, &srv_index, &range_count);
            break;
            
            case DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER:
            dm_dx12_rt_sbt_add_cbv_to_range(r, ranges, &cbv_index, &range_count);
            break;
            
            default:
            DM_LOG_FATAL("Unknown shader parameter for DirectX12 shader binding table");
            return false;
        }
    }
    
    if(range_count>0)
    {
        root_parameter.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameter.DescriptorTable.NumDescriptorRanges = range_count;
        root_parameter.DescriptorTable.pDescriptorRanges   = ranges;
        
        sig_desc.NumParameters = 1;
        sig_desc.pParameters   = &root_parameter;
        
        if(shader_stage!=0)
        {
            sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        }
    }
    
    ID3DBlob* blob;
    ID3DBlob* error_blob;
    
    hr = D3D12SerializeRootSignature(&sig_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &error_blob);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("D3D12SerializeRootSignature failed");
        if(error_blob)
        {
            DM_LOG_FATAL("%s", (char*)ID3D10Blob_GetBufferPointer(error_blob));
        }
        return false;
    }
    
    hr = ID3D12Device5_CreateRootSignature(device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, root_sig);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_CreateRootSignature failed");
        return false;
    }
    
    ID3D10Blob_Release(blob);
    
    return true;
}

DM_INLINE
bool dm_dx12_raytracing_pipeline_sbt_add_entry(const char* name, uint8_t** data, const size_t offset, ID3D12StateObjectProperties* props)
{
    wchar_t ws[100];
    void*   id = NULL;
    
    swprintf(ws, 100, L"%hs", name);
    id = ID3D12StateObjectProperties_GetShaderIdentifier(props, ws);
    if(!id)
    {
        DM_LOG_FATAL("Unknown shader identifier for DirectX12 shader binding table: %s", name);
        return false;
    }
    
    dm_memcpy(*data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *data += offset;
    
    id = NULL;
    
    return true;
}

DM_INLINE
bool dm_dx12_rt_sbt_add_resource_view(dm_render_handle handle, bool is_global, size_t param_offset, dm_dx12_rt_shader_binding_table* sbt, dm_dx12_renderer* dx12_renderer)
{
    dm_render_resource_type type  = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t                index = DM_RENDER_HANDLE_GET_INDEX(handle);
    
    ID3D12DescriptorHeap* heap              = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle  = { 0 };
    
    for(uint8_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        heap = dx12_renderer->resource_descriptor_heap[i];
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap, &heap_handle);
        heap_handle.ptr += dx12_renderer->descriptor_heap_offset;
        
        switch(type)
        {
            case DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER:
            {
                dm_dx12_vertex_buffer buffer = dx12_renderer->vertex_buffers[index];
                
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
                
                view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
                view_desc.Buffer.NumElements         = buffer.count;
                view_desc.Buffer.StructureByteStride = buffer.stride;
                view_desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
                
                ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, buffer.buffer[i], &view_desc, heap_handle);
            } break;
            
            case DM_RENDER_RESOURCE_TYPE_STRUCTURED_BUFFER:
            {
                dm_dx12_structured_buffer buffer = dx12_renderer->structured_buffers[index];
                
                D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = { 0 };
                view_desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
                view_desc.Buffer.NumElements         = buffer.count;
                view_desc.Buffer.StructureByteStride = buffer.stride;
                view_desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
                
                ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, buffer.buffer[i], NULL, &view_desc, heap_handle);
            } break;
            
            case DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER:
            {
                dm_dx12_index_buffer buffer = dx12_renderer->index_buffers[index];
                
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
                view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
                view_desc.Buffer.NumElements         = buffer.count;
                view_desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
                view_desc.Buffer.StructureByteStride = sizeof(uint32_t);
                
                ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, buffer.buffer, &view_desc, heap_handle);
            } break;
            
            case DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER:
            {
                dm_dx12_constant_buffer buffer = dx12_renderer->constant_buffers[index];
                
                D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = { 0 };
                view_desc.SizeInBytes    = buffer.size;
                view_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(buffer.buffer[i]);
                
                ID3D12Device5_CreateConstantBufferView(dx12_renderer->device, &view_desc, heap_handle);
            } break;
            
            case DM_RENDER_RESOURCE_TYPE_TEXTURE:
            {
                dm_dx12_texture texture = dx12_renderer->textures[index];
                
                D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = { 0 };
                view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                
                ID3D12Device5_CreateUnorderedAccessView(dx12_renderer->device, texture.texture[i], NULL, &view_desc, heap_handle);
            } break;
            
            case DM_RENDER_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE:
            {
                dm_dx12_acceleration_structure as = dx12_renderer->accel_structs[index];
                
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
                view_desc.Format                  = DXGI_FORMAT_UNKNOWN;
                view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.ViewDimension           = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                view_desc.RaytracingAccelerationStructure.Location = ID3D12Resource_GetGPUVirtualAddress(as.tlas.result_buffer[i]);
                
                ID3D12Device5_CreateShaderResourceView(dx12_renderer->device, NULL, &view_desc, heap_handle);
            } break;
            
            default:
            DM_LOG_FATAL("Invalid resource");
            return false;
        }
        
        // global params are not in the shader binding table
        if(is_global) continue;
        
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap, &gpu_handle);
        gpu_handle.ptr += dx12_renderer->descriptor_heap_offset;
        
        *(uint64_t*)(sbt->mapped_address[i] + param_offset) = gpu_handle.ptr;
    }
    
    dx12_renderer->descriptor_heap_offset += dx12_renderer->handle_increment_size_cbv_srv_uav;
    
    return true;
}

bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_raytracing_pipeline internal_pipe = { 0 };
    
    assert(desc.hit_group_count<=DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS);
    
    internal_pipe.sbt.hit_group_count = desc.hit_group_count;
    internal_pipe.sbt.miss_count      = desc.miss_count;
    
    // global root signature
    if(!dm_dx12_raytracing_pipeline_sbt_make_root_signature(0, desc.global_params, &internal_pipe.sbt.global_root_signature, dx12_renderer->device)) return false;
    
    // raygen root signature
    if(!dm_dx12_raytracing_pipeline_sbt_make_root_signature(1, desc.raygen_params, &internal_pipe.sbt.raygen_root_signature, dx12_renderer->device)) return false;
    
    // miss root signatures
    for(uint32_t i=0; i<desc.miss_count; i++)
    {
        if(!dm_dx12_raytracing_pipeline_sbt_make_root_signature(2, desc.miss_params[i], &internal_pipe.sbt.miss_root_signatures[i], dx12_renderer->device)) return false;
    }
    
    // hit groups
    D3D12_HIT_GROUP_DESC hit_groups[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS] = { 0 };
    wchar_t exports[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS][DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_UNKNOWN][100];
    
    for(uint32_t i=0; i<desc.hit_group_count; i++)
    {
        uint32_t export_index = 0;
        
        swprintf(exports[i][export_index], 100, L"%hs", desc.hit_groups[i].name);
        hit_groups[i].HitGroupExport = exports[i][export_index];
        export_index++;
        
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_ANY_HIT)
        {
            swprintf(exports[i][export_index], 100, L"%hs", desc.hit_groups[i].any_hit);
            hit_groups[i].AnyHitShaderImport = exports[i][export_index];
            export_index++;
        }
        
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_CLOSEST_HIT)
        {
            swprintf(exports[i][export_index], 100, L"%hs", desc.hit_groups[i].closest_hit);
            hit_groups[i].ClosestHitShaderImport = exports[i][export_index];
            export_index++;
        }
        
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_INTERSECTION)
        {
            swprintf(exports[i][export_index], 100, L"%hs", desc.hit_groups[i].intersection);
            hit_groups[i].IntersectionShaderImport = exports[i][export_index];
            export_index++;
        }
        
        switch(desc.hit_groups[i].geom_type)
        {
            case DM_RAYTRACING_PIPELINE_HIT_GROUP_GEOMETRY_TYPE_TRIANGLES:
            hit_groups[i].Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            break;
            
            case DM_RAYTRACING_PIPELINE_HIT_GROUP_GEOMETRY_TYPE_PROCEDURAL:
            hit_groups[i].Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            break;
            
            default:
            DM_LOG_FATAL("Unknown hit group geometry type");
            return false;
        }
        
        // local root signature
        if(!dm_dx12_raytracing_pipeline_sbt_make_root_signature(3, desc.hit_groups[i].params, &internal_pipe.sbt.hit_root_signatures[i], dx12_renderer->device)) return false;
    }
    
    // PSO
    // have: 
    //    -4 objects from: library, shader config, pipeline config, global sig
    //    -local signature for raygen
    //    -root association for raygen
    //    -association for all shaders
    //    -MAX_HIT_GROUPS of hit groups
    //    -MAX_HIT_GROUPS of local signatures for hit groups
    //    -MAX_HIT_GROUPS of root associations for hit groups
    D3D12_STATE_SUBOBJECT subobjects[4 + 1 + 1 + 1 + 3 * DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS];
    
    wchar_t ws_raygen[512] = { 0 };
    wchar_t ws_hit[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS][512] = { 0 };
    wchar_t ws_exports[1 + 2 * DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS * 3][512] = { 0 };
    
    LPCWSTR l_raygen  = 0;
    LPCWSTR l_hits[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS] = { 0 };
    LPCWSTR l_exports[1 + 2 * DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS] = { 0 };
    uint32_t sub_obj_index = 0;
    
    // library
    D3D12_DXIL_LIBRARY_DESC lib = { 0 };
    lib.DXILLibrary.pShaderBytecode = desc.shader_data;
    lib.DXILLibrary.BytecodeLength  = desc.shader_data_size;
    
    subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[sub_obj_index++].pDesc = &lib;
    
    // raygen root sig
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION raygen_assoc = { 0 };
    if(desc.raygen_params.count>0)
    {
        subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        subobjects[sub_obj_index++].pDesc = &internal_pipe.sbt.raygen_root_signature;
        
        // raygen assoc
        raygen_assoc.NumExports = 1;
        swprintf(ws_raygen, 100, L"%hs", desc.raygen);
        l_raygen = ws_raygen;
        raygen_assoc.pExports = &l_raygen;
        raygen_assoc.pSubobjectToAssociate = &subobjects[sub_obj_index-1];
        
        subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        subobjects[sub_obj_index++].pDesc = &raygen_assoc;
    }
    
    // hit groups
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hit_assoc[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS] = { 0 };
    
    for(uint32_t i=0; i<desc.hit_group_count; i++)
    {
        subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        subobjects[sub_obj_index++].pDesc = &hit_groups[i];
        
        if(desc.hit_groups[i].params.count==0) continue;
        
        subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        subobjects[sub_obj_index++].pDesc = &internal_pipe.sbt.hit_root_signatures[i];
        
        hit_assoc[i].NumExports = 1;
        swprintf(ws_hit[i], 100, L"%s", exports[i][0]);
        l_hits[i] = ws_hit[i];
        hit_assoc[i].pExports = &l_hits[i];
        hit_assoc[i].pSubobjectToAssociate = &subobjects[sub_obj_index-1];
        
        subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        subobjects[sub_obj_index++].pDesc = &hit_assoc[i];
    }
    
    // shader config
    D3D12_RAYTRACING_SHADER_CONFIG shader_config = { 0 };
    shader_config.MaxPayloadSizeInBytes   = desc.payload_size;
    shader_config.MaxAttributeSizeInBytes = sizeof(float) * 2;
    
    D3D12_RAYTRACING_PIPELINE_CONFIG pipe_config = { 0 };
    pipe_config.MaxTraceRecursionDepth = desc.max_recursion;
    
    subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    subobjects[sub_obj_index++].pDesc = &shader_config;
    
    // config assoc
    uint32_t export_index = 0;
    
    swprintf(ws_exports[export_index++], 100, L"%hs", desc.raygen);
    
    for(uint32_t i=0; i<desc.miss_count; i++)
    {
        swprintf(ws_exports[export_index++], 100, L"%hs", desc.miss[i]);
    }
    
    for(uint32_t i=0; i<desc.hit_group_count; i++)
    {
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_ANY_HIT)
        {
            swprintf(ws_exports[export_index++], 100, L"%hs", desc.hit_groups[i].any_hit);
        }
        
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_CLOSEST_HIT)
        {
            swprintf(ws_exports[export_index++], 100, L"%hs", desc.hit_groups[i].closest_hit);
        }
        
        if(desc.hit_groups[i].flags & DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_INTERSECTION)
        {
            swprintf(ws_exports[export_index++], 100, L"%hs", desc.hit_groups[i].intersection);
        }
    }
    
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION export_assoc = { 0 };
    export_assoc.NumExports            = export_index;
    for(uint32_t i=0; i<export_index; i++)
    {
        l_exports[i] = ws_exports[i];
    }
    export_assoc.pExports              = &(l_exports[0]);
    export_assoc.pSubobjectToAssociate = &subobjects[sub_obj_index-1];
    
    subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    subobjects[sub_obj_index++].pDesc = &export_assoc;
    
    // global root sig
    D3D12_GLOBAL_ROOT_SIGNATURE global_sig = { 0 };
    global_sig.pGlobalRootSignature        = internal_pipe.sbt.global_root_signature;
    
    subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    subobjects[sub_obj_index++].pDesc = &global_sig;
    
    // pipeline config
    subobjects[sub_obj_index].Type    = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    subobjects[sub_obj_index++].pDesc = &pipe_config;
    
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
    
    // shader binding table
    internal_pipe.sbt.max_instance_count = desc.max_instance_count;
    internal_pipe.sbt.instance_count     = desc.instance_count;
    internal_pipe.sbt.miss_count         = desc.miss_count;
    internal_pipe.sbt.hit_group_count    = desc.hit_group_count;
    
    size_t record_size                   = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    
    uint32_t max_param_count = 0;
    
    max_param_count = desc.global_params.count > max_param_count ? desc.global_params.count : max_param_count;
    max_param_count = desc.raygen_params.count > max_param_count ? desc.raygen_params.count : max_param_count;
    
    for(uint32_t i=0; i<desc.miss_count; i++)
    {
        max_param_count = desc.miss_params[i].count > max_param_count ? desc.miss_params[i].count : max_param_count;
    }
    
    for(uint32_t i=0; i<desc.hit_group_count; i++)
    {
        if(desc.hit_groups[i].params.count > max_param_count) max_param_count = desc.hit_groups[i].params.count;
    }
    
    // max number of params per record
    internal_pipe.sbt.record_count = max_param_count;
    
    // get size in bytes of parameters and align
    record_size += sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * internal_pipe.sbt.record_count;
    
    static const align = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    size_t remainder = record_size % align;
    internal_pipe.sbt.record_size = remainder ? record_size + (align - remainder) : record_size;
    
    // 1 raygen, (miss count) misses, (hit group count * max instances) hit groups
    const uint32_t shader_table_count = 1 + desc.miss_count + desc.hit_group_count * desc.max_instance_count;
    
    D3D12_RESOURCE_DESC sbt_desc = DM_DX12_BASIC_BUFFER_DESC;
    sbt_desc.Width = shader_table_count * internal_pipe.sbt.record_size;
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        hr = ID3D12Device5_CreateCommittedResource(dx12_renderer->device, &DM_DX12_UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &sbt_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_pipe.sbt.table[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device5_CreateCommittedResource failed");
            DM_LOG_FATAL("Could not create DirectX12 raytracing pipeline shader ids");
            return false;
        }
        
        hr = ID3D12Resource_Map(internal_pipe.sbt.table[i], 0, NULL, &internal_pipe.sbt.mapped_address[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            DM_LOG_FATAL("Could not map onto DirectX12 shader binding table");
            return false;
        }
        
        ID3D12StateObjectProperties* props;
        ID3D12StateObject_QueryInterface(internal_pipe.state_object, &IID_ID3D12StateObjectProperties, &props);
        
        uint8_t* data = internal_pipe.sbt.mapped_address[i];
#ifdef DM_DEBUG
        uint8_t* ref  = NULL;
        
        ref = data;
#endif
        
        ID3D12DescriptorHeap* heap              = dx12_renderer->resource_descriptor_heap[i];
        D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = { 0 };
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap, &heap_handle);
        heap_handle.ptr += dx12_renderer->descriptor_heap_offset;
        
        
        // raygen shader
        if(!dm_dx12_raytracing_pipeline_sbt_add_entry(desc.raygen, &data, internal_pipe.sbt.record_size, props)) return false;
        
        // miss shaders
        for(uint32_t i=0; i<desc.miss_count; i++)
        {
            if(!dm_dx12_raytracing_pipeline_sbt_add_entry(desc.miss[i], &data, internal_pipe.sbt.record_size, props)) return false;
        }
        
        // hit groups
        for(uint32_t j=0; j<desc.max_instance_count; j++)
        {
            for(uint32_t k=0; k<desc.hit_group_count; k++)
            {
                if(!dm_dx12_raytracing_pipeline_sbt_add_entry(desc.hit_groups[k].name, &data, internal_pipe.sbt.record_size, props)) return false;
            }
        }
        
        // sanity checks the above algorithms worked
#ifdef DM_DEBUG
        assert(*data % internal_pipe.sbt.record_size == 0);
        assert(data - ref == sbt_desc.Width);
        
        ref  = NULL;
#endif
        data = NULL;
        
        ID3D12StateObjectProperties_Release(props);
    }
    
    // add in resources to sbt
    {
        // global
        internal_pipe.sbt.global_heap_offset = dx12_renderer->descriptor_heap_offset;
        
        size_t sbt_offset = 0;
        for(uint16_t i=0; i<desc.global_params.count; i++)
        {
            if(!dm_dx12_rt_sbt_add_resource_view(desc.global_params.handles[i], true, sbt_offset, &internal_pipe.sbt, dx12_renderer)) return false;
        }
        
        // raygen
        sbt_offset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        for(uint16_t i=0; i<desc.raygen_params.count; i++)
        {
            if(!dm_dx12_rt_sbt_add_resource_view(desc.raygen_params.handles[i], false, sbt_offset, &internal_pipe.sbt, dx12_renderer)) return false;
        }
        
        // misses
        for(uint8_t j=0; j<desc.miss_count; j++)
        {
            sbt_offset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + internal_pipe.sbt.record_size * (1 + j);
            for(uint16_t i=0; i<desc.miss_params[j].count; i++)
            {
                if(!dm_dx12_rt_sbt_add_resource_view(desc.miss_params[j].handles[i], false, sbt_offset, &internal_pipe.sbt, dx12_renderer)) return false;
            }
        }
        
        // hit groups
        uint16_t shader_count = 1 + desc.miss_count;
        for(uint32_t i=0; i<desc.instance_count; i++)
        {
            dm_render_handle handle;
            for(uint8_t j=0; j<desc.hit_group_count; j++)
            {
                sbt_offset = internal_pipe.sbt.record_size * shader_count + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                
                const uint8_t param_count = desc.hit_groups[j].params.count;
                for(uint8_t k=0; k<param_count; k++)
                {
                    handle = desc.hit_groups[j].params.handles[i * param_count + k];
                    
                    if(!dm_dx12_rt_sbt_add_resource_view(handle, false, sbt_offset, &internal_pipe.sbt, dx12_renderer)) return false;
                    sbt_offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
                }
                
                shader_count++;
            }
        }
    }
    
    //
    dm_memcpy(dx12_renderer->raytracing_pipelines + dx12_renderer->rt_pipe_count, &internal_pipe, sizeof(internal_pipe));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx12_renderer->rt_pipe_count++);
    
    return true;
}

void dm_dx12_renderer_destroy_raytracing_pipeline(dm_dx12_raytracing_pipeline* pipe)
{
    ID3D12RootSignature_Release(pipe->sbt.global_root_signature);
    ID3D12RootSignature_Release(pipe->sbt.raygen_root_signature);
    
    for(uint32_t i=0; i<pipe->sbt.miss_count; i++)
    {
        ID3D12RootSignature_Release(pipe->sbt.miss_root_signatures[i]);
    }
    
    for(uint32_t i=0; i<pipe->sbt.hit_group_count; i++)
    {
        ID3D12RootSignature_Release(pipe->sbt.hit_root_signatures[i]);
    }
    
    for(uint32_t i=0; i<DM_DX12_NUM_FRAMES; i++)
    {
        ID3D12Resource_Unmap(pipe->sbt.table[i], 0, NULL);
        ID3D12Resource_Release(pipe->sbt.table[i]);
        
        pipe->sbt.mapped_address[i] = NULL;
    }
    
    ID3D12StateObject_Release(pipe->state_object);
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
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_PIPELINE)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a pipeline");
        return false;
    }
    
    dm_dx12_pipeline* internal_pipe = &dx12_renderer->pipes[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
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
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a constant buffer");
        return false;
    }
    
    dm_dx12_constant_buffer* internal_buffer = &dx12_renderer->constant_buffers[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame_index];
    
    ID3D12GraphicsCommandList4_SetGraphicsRootConstantBufferView(command_list, slot, ID3D12Resource_GetGPUVirtualAddress(internal_buffer->buffer[current_frame_index]));
    
    return true;
}

bool dm_render_command_backend_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to update a resource that is not a vertex buffer");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    
    dm_dx12_vertex_buffer* internal_buffer = &dx12_renderer->vertex_buffers[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    if(!internal_buffer->mapped_addresses[current_frame_index])
    {
        DM_LOG_FATAL("DirectX12 vertex buffer has invalid mapped address");
        return false;
    }
    
    dm_memcpy(internal_buffer->mapped_addresses[current_frame_index], data, data_size);
    
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_clear_texture(dm_render_handle handle, dm_renderer* renderer)
{
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER)
    {
        DM_LOG_FATAL("Trying to update resource that is not a constant buffer");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    
    dm_dx12_constant_buffer* internal_buffer = &dx12_renderer->constant_buffers[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
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
    DM_LOG_WARN("Not supported");
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_LOG_WARN("Not supported");
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
    DM_LOG_WARN("Not supported");
    return true;
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    DM_LOG_WARN("Not supported");
}

#ifdef DM_RAYTRACING
bool dm_render_command_backend_update_acceleration_structure_instance(dm_render_handle handle, uint32_t instance_id, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE)
    {
        DM_LOG_FATAL("Trying to update resource that is not an acceleration structure");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    dm_memcpy(internal_as->tlas.instance_data[current_frame_index][instance_id].Transform, data, sizeof(float) * 4 * 3);
    
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_instance_range(dm_render_handle handle, uint32_t instance_start, uint32_t instance_end, void* data, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE)
    {
        DM_LOG_FATAL("Trying to update resource that is not an acceleration structure");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    uint32_t instance = instance_start;
    const uint32_t instance_count = instance_end - instance_start;
    
    dm_mat4* transforms = data;
    
    for(uint32_t i=0; i<instance_count; i++)
    {
        dm_memcpy(internal_as->tlas.instance_data[current_frame_index][instance].Transform, transforms + instance, sizeof(float) * 4 * 3);
        instance++;
    }
    
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_tlas(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE)
    {
        DM_LOG_FATAL("Trying to update resource that is not an acceleration structure");
        return false;
    }
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list  = dx12_renderer->command_list[current_frame_index];
    
    dm_dx12_acceleration_structure* internal_as = &dx12_renderer->accel_structs[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    ID3D12Resource* instance_buffer = internal_as->tlas.instance_buffer[current_frame_index];
    ID3D12Resource* result_buffer   = internal_as->tlas.result_buffer[current_frame_index];
    ID3D12Resource* scratch_buffer  = internal_as->tlas.scratch_buffer[current_frame_index];
    
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = { 0 };
    desc.DestAccelerationStructureData    = ID3D12Resource_GetGPUVirtualAddress(result_buffer);
    desc.Inputs.Type                      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags                     = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs                  = internal_as->tlas.max_instance_count;
    desc.Inputs.DescsLayout               = D3D12_ELEMENTS_LAYOUT_ARRAY;
    desc.Inputs.InstanceDescs             = ID3D12Resource_GetGPUVirtualAddress(instance_buffer);
    desc.SourceAccelerationStructureData  = ID3D12Resource_GetGPUVirtualAddress(result_buffer);
    desc.ScratchAccelerationStructureData = ID3D12Resource_GetGPUVirtualAddress(scratch_buffer);
    
    // uav barrier
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = result_buffer;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    ID3D12GraphicsCommandList4_BuildRaytracingAccelerationStructure(command_list, &desc, 0, NULL);
    
    // uav barrier
    {
        D3D12_RESOURCE_BARRIER barrier = { 0 };
        barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = result_buffer;
        
        ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 1, &barrier);
    }
    
    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_RT_PIPELINE)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a raytracing pipeline");
        return false;
    }
    
    dm_dx12_raytracing_pipeline* internal_pipe = &dx12_renderer->raytracing_pipelines[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    const uint32_t current_frame_index = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list    = dx12_renderer->command_list[current_frame_index];
    ID3D12DescriptorHeap*       descriptor_heap = dx12_renderer->resource_descriptor_heap[current_frame_index];
    
    // state
    ID3D12GraphicsCommandList4_SetPipelineState1(command_list, internal_pipe->state_object);
    
    // global root signature
    ID3D12GraphicsCommandList4_SetComputeRootSignature(command_list, internal_pipe->sbt.global_root_signature);
    
    // global descriptor table
    D3D12_GPU_DESCRIPTOR_HANDLE heap_handle = { 0 };
    ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(dx12_renderer->resource_descriptor_heap[current_frame_index], &heap_handle);
    heap_handle.ptr += internal_pipe->sbt.global_heap_offset;
    
    ID3D12GraphicsCommandList4_SetComputeRootDescriptorTable(command_list, 0, heap_handle);
    
    return true;
}

bool dm_render_command_backend_dispatch_rays(uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_RT_PIPELINE)
    {
        DM_LOG_FATAL("Trying to dispatch rays with resource that is not a raytracing pipeline");
        return false;
    }
    
    dm_dx12_raytracing_pipeline* internal_pipe = &dx12_renderer->raytracing_pipelines[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    const uint32_t current_frame_index       = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame_index];
    
    const size_t shader_size        = internal_pipe->sbt.record_size;
    const size_t miss_count         = internal_pipe->sbt.miss_count;
    const size_t hit_group_count    = internal_pipe->sbt.hit_group_count;
    const size_t max_instance_count = internal_pipe->sbt.max_instance_count;
    const size_t instance_count     = internal_pipe->sbt.instance_count;
    
    const D3D12_GPU_VIRTUAL_ADDRESS start_address = ID3D12Resource_GetGPUVirtualAddress(internal_pipe->sbt.table[current_frame_index]);
    
    D3D12_DISPATCH_RAYS_DESC desc = { 0 };
    desc.Width  = width;
    desc.Height = height;
    desc.Depth  = 1;
    
    // raygen
    desc.RayGenerationShaderRecord.StartAddress = start_address;
    desc.RayGenerationShaderRecord.SizeInBytes  = shader_size;
    
    // miss groups
    if(miss_count)
    {
        desc.MissShaderTable.StartAddress  = start_address + shader_size;
        desc.MissShaderTable.SizeInBytes   = shader_size * miss_count;
        desc.MissShaderTable.StrideInBytes = shader_size;
    }
    
    // hit groups
    if(hit_group_count)
    {
        desc.HitGroupTable.StartAddress  = start_address + shader_size + miss_count * shader_size;
        desc.HitGroupTable.SizeInBytes   = shader_size * hit_group_count * instance_count;
        desc.HitGroupTable.StrideInBytes = shader_size;
    }
    
    // ray dispatch
    ID3D12GraphicsCommandList4_DispatchRays(command_list, &desc);
    
    return true;
}

bool dm_render_command_backend_copy_texture_to_screen(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(DM_RENDER_HANDLE_GET_TYPE(handle)!=DM_RENDER_RESOURCE_TYPE_TEXTURE)
    {
        DM_LOG_FATAL("Trying to copy resource that is not a texture");
        return false;
    }
    
    dm_dx12_texture* internal_texture = &dx12_renderer->textures[DM_RENDER_HANDLE_GET_INDEX(handle)];
    
    const uint32_t current_frame_index       = dx12_renderer->current_frame_index;
    ID3D12GraphicsCommandList4* command_list = dx12_renderer->command_list[current_frame_index];
    
    ID3D12Resource* screen  = dx12_renderer->render_target[current_frame_index];
    ID3D12Resource* texture = internal_texture->texture[current_frame_index];
    
    D3D12_RESOURCE_BARRIER prior_barriers[2] = { 0 };
    D3D12_RESOURCE_BARRIER post_barriers[2] = { 0 };
    
    // texture from unordered access to copy source
    prior_barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    prior_barriers[0].Transition.pResource   = texture;
    prior_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    prior_barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    
    // screen from render target to copy destination
    prior_barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    prior_barriers[1].Transition.pResource   = screen;
    prior_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    prior_barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    
    ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 2, prior_barriers);
    
    // copy
    ID3D12GraphicsCommandList4_CopyResource(command_list, screen, texture);
    
    // screen from copy destination to render target
    post_barriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    post_barriers[0].Transition.pResource   = screen;
    post_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    post_barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    // texture from copy source to unordered access
    post_barriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    post_barriers[1].Transition.pResource   = texture;
    post_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    post_barriers[1].Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    
    ID3D12GraphicsCommandList4_ResourceBarrier(command_list, 2, post_barriers);
    
    return true;
}

#endif // dm_raytracing

#ifdef DM_DEBUG
#ifdef DM_DX12_DRED_VALIDATION
DM_INLINE
void dm_dx12_decode_breadcrumb_op(D3D12_AUTO_BREADCRUMB_OP op, uint8_t flag)
{
    char buffer[512];
    
    switch(op)
    {
        case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
        sprintf(buffer, "set marker");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
        sprintf(buffer, "begin event");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
        sprintf(buffer, "end event");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
        sprintf(buffer, "draw instanced");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
        sprintf(buffer, "draw indexed instanced");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
        sprintf(buffer, "execute indirect");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
        sprintf(buffer, "dispatch");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
        sprintf(buffer, "copy buffer region");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
        sprintf(buffer, "copy texture region");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
        sprintf(buffer, "copy resource");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
        sprintf(buffer, "copy tiles");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
        sprintf(buffer, "resolve subresource");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
        sprintf(buffer, "clear render target view");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
        sprintf(buffer, "clear unordered access view");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
        sprintf(buffer, "clear depth stencil view");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
        sprintf(buffer, "resource barrier");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
        sprintf(buffer, "execute bundle");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
        sprintf(buffer, "present");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
        sprintf(buffer, "resolve query data");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
        sprintf(buffer, "begin submission");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
        sprintf(buffer, "end submission");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
        sprintf(buffer, "decode frame");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
        sprintf(buffer, "process frames");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
        sprintf(buffer, "atomic copy buffer uint");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
        sprintf(buffer, "atomic copy buffer uint64");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
        sprintf(buffer, "resolve subresource region");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
        sprintf(buffer, "wrtie buffer immediate");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
        sprintf(buffer, "decode frame 1");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
        sprintf(buffer, "set projected resource session");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
        sprintf(buffer, "decode frame 2");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
        sprintf(buffer, "process frames 1");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
        sprintf(buffer, "build raytracing acceleration structure");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
        sprintf(buffer, "emit raytracing acceleration structure post build info");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
        sprintf(buffer, "copy raytracing acceleration structure");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
        sprintf(buffer, "dispatch rays");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
        sprintf(buffer, "initialize meta command");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
        sprintf(buffer, "execute meta command");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
        sprintf(buffer, "estimate motion");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
        sprintf(buffer, "resolve motion vector heap");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
        sprintf(buffer, "set pipeline state1");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
        sprintf(buffer, "initialize extension command");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
        sprintf(buffer, "execute extension command");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
        sprintf(buffer, "dispatch mesh");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME:
        sprintf(buffer, "encode frame");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA:
        sprintf(buffer, "resolve encoder output meta data");
        break;
        
#if 0
        case D3D12_AUTO_BREADCRUMB_OP_BARRIER:
        sprintf(buffer, "barrier");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_BEGIN_COMMAND_LIST:
        sprintf(buffer, "begin command list");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHGRAPH:
        sprintf(buffer, "dispatch graph");
        break;
        
        case D3D12_AUTO_BREADCRUMB_OP_SETPROGRAM:
        sprintf(buffer, "set program");
        break;
#endif
    }
    
    switch(flag)
    {
        case 0:
        DM_LOG_INFO("Completed: %s", buffer);
        break;
        
        case 1:
        DM_LOG_FATAL("Failure at: %s", buffer);
        break;
        
        case 2:
        DM_LOG_WARN("Not begun: %s", buffer);
        break;
    }
}

void dm_dx12_track_down_device_removal(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    hr = ID3D12Device5_GetDeviceRemovedReason(dx12_renderer->device);
    if(hr==S_OK) return;
    
    DM_LOG_ERROR("DirectX12 device removed");
    
    ID3D12DeviceRemovedExtendedData* dred;
    hr = ID3D12Device5_QueryInterface(dx12_renderer->device, &IID_ID3D12DeviceRemovedExtendedData, &dred);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device5_QueryInterface failed");
        DM_LOG_FATAL("Could not obtain DirectX12 DRED data");
        return;
    }
    
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs_output = { 0 };
    D3D12_DRED_PAGE_FAULT_OUTPUT       page_fault_output = { 0 };
    
    hr = ID3D12DeviceRemovedExtendedData_GetAutoBreadcrumbsOutput(dred, &breadcrumbs_output);
    hr = ID3D12DeviceRemovedExtendedData_GetPageFaultAllocationOutput(dred, &page_fault_output);
    
    D3D12_AUTO_BREADCRUMB_NODE head = *breadcrumbs_output.pHeadAutoBreadcrumbNode;
    
    while(head.BreadcrumbCount == *head.pLastBreadcrumbValue)
    {
        head = *head.pNext;
    }
    
    DM_LOG_DEBUG("Tracking down command failure...");
    for(uint32_t i=0; i<head.BreadcrumbCount; i++)
    {
        uint8_t flag = 0;
        if(i == *head.pLastBreadcrumbValue) flag = 1;
        if(i >  *head.pLastBreadcrumbValue)  flag = 2;
        
        dm_dx12_decode_breadcrumb_op(head.pCommandHistory[i], flag);
    }
    
    ID3D12DeviceRemovedExtendedData_Release(dred);
}
#endif
#endif

#endif