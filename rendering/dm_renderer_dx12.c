#include "dm.h"

#ifdef DM_DIRECTX12

#include "platform/dm_platform_win32.h"

#define COBJMACROS
#include <unknwn.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#define NUM_FRAMES 3

typedef enum dm_dx12_buffer_type
{
    DM_DX12_BUFFER_TYPE_VERTEX,
    DM_DX12_BUFFER_TYPE_INDEX,
    DM_DX12_BUFFER_TYPE_UNKNOWN,
} dm_dx12_buffer_type;

typedef struct dm_dx12_buffer_t
{
    ID3D12Resource* buffer;
    union
    {
        D3D12_VERTEX_BUFFER_VIEW vb_view;
        D3D12_INDEX_BUFFER_VIEW  ib_view;
    };
    dm_dx12_buffer_type type;
} dm_dx12_buffer;

typedef struct dm_dx12_pipeline_t
{
    ID3D12PipelineState*     pipeline_state;
    D3D12_PRIMITIVE_TOPOLOGY topology;
} dm_dx12_pipeline;

typedef struct dm_dx12_renderpass_t
{
    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor;
} dm_dx12_renderpass;

#define DM_DX12_MAX_RESOURCES 20
typedef struct dm_dx12_renderer_t
{
    HWND      hwnd;
    HINSTANCE h_instance;
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;
    
    ID3D12Device2*             device;
    ID3D12CommandQueue*        command_queue;
    ID3D12CommandAllocator*    command_allocators[NUM_FRAMES];
    IDXGISwapChain*            swap_chain;
    ID3D12Resource*            back_buffers[NUM_FRAMES];
    ID3D12DescriptorHeap*      rtv_descriptor_heap;
    ID3D12GraphicsCommandList* command_list;
    ID3D12RootSignature*       root_signature;
    ID3D12Fence* fence;
    
    ID3D12Resource* depth_buffer;
    
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor;
    
    UINT rtv_descriptor_size;
    UINT current_backbuffer_index;
    uint64_t fence_value;
    uint64_t frame_fence_values[NUM_FRAMES];
    
    HANDLE fence_event;
    
    uint32_t buffer_count, pipe_count;
    dm_dx12_buffer   buffers[DM_DX12_MAX_RESOURCES];
    dm_dx12_pipeline pipelines[DM_DX12_MAX_RESOURCES];
    
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

void dm_dx12_destroy_buffer(dm_render_handle handle, dm_dx12_renderer* dx12_renderer);
void dm_dx12_destroy_pipeline(dm_render_handle handle, dm_dx12_renderer* dx12_renderer);

/*********************
 DX11 ENUM CONVERSIONS
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

/******
DEVICE
********/
bool dm_dx12_init_device(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    // adapter
    UINT create_factory_flags = 0;
#ifdef DM_DEBUG
    create_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    
    hr = CreateDXGIFactory2(create_factory_flags, &IID_IDXGIFactory4, &dx12_renderer->factory);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("CreateDXGIFactory2 failed");
        return false;
    }
    
    size_t max_dedicated_video_memory = 0;
    for(uint32_t i=0; DXGI_ERROR_NOT_FOUND != IDXGIFactory1_EnumAdapters1(dx12_renderer->factory, i, &dx12_renderer->adapter); i++)
    {
        DXGI_ADAPTER_DESC1 dxgi_adapter_desc1 = { 0 };
        
        IDXGIAdapter1_GetDesc1(dx12_renderer->adapter, &dxgi_adapter_desc1);
        
        if(dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        
        hr = D3D12CreateDevice((IUnknown*)dx12_renderer->adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, &dx12_renderer->device);
        bool valid_device = dm_platform_win32_decode_hresult(hr);
        valid_device = valid_device & (dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory);
        if(!valid_device) continue;
        
        max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
        break;
    }
    
    if(!dx12_renderer->adapter)
    {
        DM_LOG_FATAL("No hardware adapter found. System does not support DX12");
        return false;
    }
    
    // debug layer
#ifdef DM_DEBUG
    ID3D12InfoQueue* info_queue;
    hr = dx12_renderer->device->lpVtbl->QueryInterface(dx12_renderer->device, &IID_ID3D12InfoQueue, &info_queue);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IUnknown_QueryInterface failed");
        return false;
    }
    
    ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_ERROR, true);
    //ID3D12InfoQueue_SetBreakOnSeverity(info_queue, D3D12_MESSAGE_SEVERITY_WARNING, true);
    
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
    
    ID3D12InfoQueue_Release(info_queue);
#endif
    
    return true;
}

/**********
SWAP CHAIN
************/
bool dm_dx12_init_swap_chain(const uint32_t width, const uint32_t height, dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
    swap_desc.Width            = width;
    swap_desc.Height           = height;
    swap_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM,
    swap_desc.SampleDesc.Count = 1;
    swap_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount      = NUM_FRAMES;
    swap_desc.Scaling          = DXGI_SCALING_NONE;
    swap_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode        = DXGI_ALPHA_MODE_UNSPECIFIED;
    
    hr = IDXGIFactory4_CreateSwapChainForHwnd(dx12_renderer->factory, (IUnknown*)dx12_renderer->command_queue, dx12_renderer->hwnd, &swap_desc, NULL, NULL, (IDXGISwapChain1**)&dx12_renderer->swap_chain);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGIFactory4_CreateSwapChainForHwnd failed");
        return false;
    }
    
    hr = IDXGIFactory4_MakeWindowAssociation(dx12_renderer->factory, dx12_renderer->hwnd, DXGI_MWA_NO_ALT_ENTER);
    if(dm_platform_win32_decode_hresult(hr)) return true;
    
    DM_LOG_FATAL("IDXGIFactory4_MakeWindowAssociation failed");
    return false;
}

/*************
COMMAND QUEUE
***************/
bool dm_dx12_init_command_queue(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    D3D12_COMMAND_QUEUE_DESC desc = { 0 };
    desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    hr = ID3D12Device_CreateCommandQueue(dx12_renderer->device, &desc, &IID_ID3D12CommandQueue, &dx12_renderer->command_queue);
    if(dm_platform_win32_decode_hresult(hr)) return true;
    
    DM_LOG_FATAL("ID3D12Device_CreateCommandQueue failed");
    return false;
}

/***************
DESCRIPTOR HEAP
*****************/
bool dm_dx12_init_descriptor_heap(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = { 0 };
    heap_desc.NumDescriptors = NUM_FRAMES;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    
    hr = ID3D12Device_CreateDescriptorHeap(dx12_renderer->device, &heap_desc, &IID_ID3D12DescriptorHeap, &dx12_renderer->rtv_descriptor_heap);
    
    if(dm_platform_win32_decode_hresult(hr)) return true;
    
    DM_LOG_FATAL("ID3D12Device_CreateDescriptorHeap failed");
    return false;
}

D3D12_CPU_DESCRIPTOR_HANDLE dm_dx12_offset_descriptor_handle(const uint32_t index, dm_dx12_renderer* dx12_renderer)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
    const UINT size = ID3D12Device_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtv_descriptor_handle.ptr += index * size;
    
    return rtv_descriptor_handle;
}

/*************
RENDER TARGET
***************/
bool dm_dx12_init_render_targets(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
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
    
    return true;
}

/******************
COMMAND ALLOCATORS
********************/
bool dm_dx12_init_command_allocators(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        hr = ID3D12Device_CreateCommandAllocator(dx12_renderer->device, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &dx12_renderer->command_allocators[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Device_CreateCommandAllocator failed");
            return false;
        }
    }
    
    return true;
}

/*****
FENCE
*******/
bool dm_dx12_init_fence(dm_dx12_renderer* dx12_renderer)
{
    HRESULT hr;
    
    hr = ID3D12Device_CreateFence(dx12_renderer->device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &dx12_renderer->fence);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device_CreateFence failed");
        return false;
    }
    dx12_renderer->frame_fence_values[0]++;
    
    dx12_renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!dx12_renderer->fence_event)
    {
        DM_LOG_FATAL("CreateEvent failed");
        hr = HRESULT_FROM_WIN32(GetLastError());
        dm_platform_win32_decode_hresult(hr);
        return false;
    }
    
    return true;
}

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
    
    if(!dm_dx12_init_device(dx12_renderer))             return false;
    if(!dm_dx12_init_command_queue(dx12_renderer))      return false;
    if(!dm_dx12_init_swap_chain(context->platform_data.window_data.width, context->platform_data.window_data.height, dx12_renderer))         return false;
    if(!dm_dx12_init_descriptor_heap(dx12_renderer))    return false;
    if(!dm_dx12_init_render_targets(dx12_renderer))     return false;
    if(!dm_dx12_init_command_allocators(dx12_renderer)) return false;
    if(!dm_dx12_init_fence(dx12_renderer))              return false;
    
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
    
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_desc = { 0 };
    root_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    root_desc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ID3DBlob* serialized_desc = NULL;
    hr = D3D12SerializeVersionedRootSignature(&root_desc, &serialized_desc, NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("D3D12SerializeVersionedRootSignature failed");
        ID3D10Blob_Release(serialized_desc);
        return false;
    }
    
    hr = ID3D12Device2_CreateRootSignature(dx12_renderer->device, 0, ID3D10Blob_GetBufferPointer(serialized_desc), ID3D10Blob_GetBufferSize(serialized_desc), &IID_ID3D12RootSignature, &dx12_renderer->root_signature);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device2_CreateRootSignature failed");
        ID3D10Blob_Release(serialized_desc);
        return false;
    }
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    ID3D10Blob_Release(serialized_desc);
    serialized_desc = NULL;
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    // resource destruction
    for(uint32_t i=0; i<dx12_renderer->buffer_count; i++)
    {
        dm_dx12_destroy_buffer(i, dx12_renderer);
    }
    
    for(uint32_t i=0; i<dx12_renderer->pipe_count; i++)
    {
        dm_dx12_destroy_pipeline(i, dx12_renderer);
    }
    
    // renderer destruction
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        ID3D12CommandAllocator_Release(dx12_renderer->command_allocators[i]);
        ID3D12Resource_Release(dx12_renderer->back_buffers[i]);
        
        dx12_renderer->command_allocators[i] = NULL;
        dx12_renderer->back_buffers[i] = NULL;
    }
    
    ID3D12RootSignature_Release(dx12_renderer->root_signature);
    dx12_renderer->root_signature = NULL;
    
    ID3D12GraphicsCommandList_Release(dx12_renderer->command_list);
    dx12_renderer->command_list = NULL;
    
    ID3D12CommandQueue_Release(dx12_renderer->command_queue);
    dx12_renderer->command_queue = NULL;
    
    ID3D12DescriptorHeap_Release(dx12_renderer->rtv_descriptor_heap);
    dx12_renderer->rtv_descriptor_heap = NULL;
    
    IDXGISwapChain4_Release(dx12_renderer->swap_chain);
    dx12_renderer->swap_chain = NULL;
    
    ID3D12Fence_Release(dx12_renderer->fence);
    CloseHandle(dx12_renderer->fence_event);
    dx12_renderer->fence = NULL;
    
    IDXGIFactory4_Release(dx12_renderer->factory);
    IDXGIAdapter1_Release(dx12_renderer->adapter);
    dx12_renderer->factory = NULL;
    dx12_renderer->adapter = NULL;
    
#ifdef DM_DEBUG
    HRESULT hr;
    ID3D12DebugDevice* debug_device;
    
    hr = dx12_renderer->device->lpVtbl->QueryInterface(dx12_renderer->device, &IID_ID3D12DebugDevice, &debug_device);
    ID3D12Debug_Release(dx12_renderer->debugger);
    ID3D12DebugDevice_ReportLiveDeviceObjects(debug_device, D3D12_RLDO_DETAIL);
    ID3D12DebugDevice_Release(debug_device);
    dx12_renderer->debugger = NULL;
    debug_device = NULL;
#endif
    ID3D12Device_Release(dx12_renderer->device);
    dx12_renderer->device = NULL;
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    ID3D12CommandAllocator* command_allocator = dx12_renderer->command_allocators[dx12_renderer->current_backbuffer_index];
    ID3D12Resource* back_buffer = dx12_renderer->back_buffers[dx12_renderer->current_backbuffer_index];
    
    hr = ID3D12CommandAllocator_Reset(command_allocator);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12CommandAllocator_Reset failed");
        return false;
    }
    
    hr = ID3D12GraphicsCommandList_Reset(dx12_renderer->command_list, command_allocator, NULL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12GraphicsCommandList_Reset failed");
        return false;
    }
    
    D3D12_VIEWPORT viewport = { 0 };
    viewport.Width = (float)renderer->width;
    viewport.Height = (float)renderer->height;
    
    D3D12_RECT scissor = { 0 };
    scissor.right = renderer->width;
    scissor.bottom = renderer->height;
    
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(dx12_renderer->command_list, dx12_renderer->root_signature);
    ID3D12GraphicsCommandList_RSSetViewports(dx12_renderer->command_list, 1, &viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(dx12_renderer->command_list, 1, &scissor);
    
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = back_buffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ID3D12GraphicsCommandList_ResourceBarrier(dx12_renderer->command_list, 1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = dm_dx12_offset_descriptor_handle(dx12_renderer->current_backbuffer_index, dx12_renderer);
    
    r += 0.01f * r_dir;
    g += 0.02f * g_dir;
    b += 0.005f * b_dir;
    
    if(r < 0 || r > 1) r_dir *= -1;
    if(g < 0 || g > 1) g_dir *= -1;
    if(b < 0 || b > 1) b_dir *= -1;
    
    FLOAT clear_color[] = { r,g,b, 1.f };
    
    ID3D12GraphicsCommandList_OMSetRenderTargets(dx12_renderer->command_list, 1, &rtv_descriptor_handle, FALSE, NULL);
    ID3D12GraphicsCommandList_ClearRenderTargetView(dx12_renderer->command_list, rtv_descriptor_handle, clear_color, 0, NULL);
    
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx12_renderer* dx12_renderer = context->renderer.internal_renderer;
    HRESULT hr;
    
    const UINT current_frame = dx12_renderer->current_backbuffer_index;
    
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
    
    ID3D12GraphicsCommandList* command_lists[] = {
        dx12_renderer->command_list
    };
    
    ID3D12CommandQueue_ExecuteCommandLists(dx12_renderer->command_queue, _countof(command_lists), (ID3D12CommandList**)command_lists);
    
    hr = IDXGISwapChain4_Present(dx12_renderer->swap_chain, 1, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain4_Present failed");
        return false;
    }
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->frame_fence_values[dx12_renderer->current_backbuffer_index] = dx12_renderer->fence_value;
    
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(width <= 0)  width = 1;
    if(height <= 0) height = 1;
    
    dm_dx12_wait_for_frame(dx12_renderer->command_queue, dx12_renderer->fence, &dx12_renderer->fence_value, dx12_renderer->fence_event);
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        ID3D12Resource_Release(dx12_renderer->back_buffers[i]);
        
        dx12_renderer->frame_fence_values[i] = dx12_renderer->frame_fence_values[dx12_renderer->current_backbuffer_index];
    }
    
    DXGI_SWAP_CHAIN_DESC swap_chain_desc = { 0 };
    
    hr = IDXGISwapChain3_GetDesc(dx12_renderer->swap_chain, &swap_chain_desc);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain3_GetDesc failed");
        return false;
    }
    
    hr = IDXGISwapChain3_ResizeBuffers(dx12_renderer->swap_chain, NUM_FRAMES, width, height, swap_chain_desc.BufferDesc.Format, swap_chain_desc.Flags);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("IDXGISwapChain3_ResizeBuffers failed");
        return false;
    }
    
    dx12_renderer->current_backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex((IDXGISwapChain3*)dx12_renderer->swap_chain);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_handle = { 0 };
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(dx12_renderer->rtv_descriptor_heap, &rtv_descriptor_handle);
    const UINT size = ID3D12Device_GetDescriptorHandleIncrementSize(dx12_renderer->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    for(uint32_t i=0; i<NUM_FRAMES; i++)
    {
        hr = IDXGISwapChain3_GetBuffer(dx12_renderer->swap_chain, i, &IID_ID3D12Resource, &dx12_renderer->back_buffers[i]);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("IDXGISwapChain3_GetBuffer failed");
            return false;
        }
        
        ID3D12Device_CreateRenderTargetView(dx12_renderer->device, dx12_renderer->back_buffers[i], NULL, rtv_descriptor_handle);
        rtv_descriptor_handle.ptr += size;
    }
    
    return true;
}

void* dm_renderer_backend_get_internal_texture_ptr(dm_render_handle handle, dm_renderer* renderer)
{
    return NULL;
}

/*****************
RESOURCE CREATION
*******************/
void dm_dx12_destroy_buffer(dm_render_handle handle, dm_dx12_renderer* dx12_renderer)
{
    if(handle > dx12_renderer->buffer_count)
    {
        DM_LOG_ERROR("Trying to destroy invalid DirectX12 buffer");
        return;
    }
    
    dm_dx12_buffer* internal_buffer = &dx12_renderer->buffers[handle];
    
    ID3D12Resource_Release(internal_buffer->buffer);
}

bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_buffer internal_buffer = { 0 };
    
    D3D12_HEAP_PROPERTIES heap_properites = { 0 };
    heap_properites.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resource_desc = { 0 };
    resource_desc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Width               = desc.buffer_size;
    resource_desc.Height              = 1;
    resource_desc.DepthOrArraySize    = 1;
    resource_desc.MipLevels           = 1;
    resource_desc.SampleDesc.Count    = 1;
    resource_desc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    hr = ID3D12Device2_CreateCommittedResource(dx12_renderer->device, &heap_properites, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &internal_buffer.buffer);
    
    if(!internal_buffer.buffer)
    {
        DM_LOG_FATAL("Could not create DirectX12 buffer");
        return false;
    }
    
    if(data)
    {
        void* gpu_data = NULL;
        
        D3D12_RANGE read_range = { 0 };
        hr = ID3D12Resource_Map(internal_buffer.buffer, 0, &read_range, &gpu_data);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D12Resource_Map failed");
            DM_LOG_FATAL("Could not create D3D12 buffer");
            return false;
        }
        
        dm_memcpy(gpu_data, data, desc.buffer_size);
        
        ID3D12Resource_Unmap(internal_buffer.buffer, 0, NULL);
    }
    
    switch(desc.type)
    {
        case DM_BUFFER_TYPE_VERTEX:
        {
            internal_buffer.type = DM_DX12_BUFFER_TYPE_VERTEX;
            
            internal_buffer.vb_view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(internal_buffer.buffer);
            internal_buffer.vb_view.StrideInBytes  = desc.elem_size;
            internal_buffer.vb_view.SizeInBytes    = desc.buffer_size;
        } break;
        
        case DM_BUFFER_TYPE_INDEX:
        {
            internal_buffer.type = DM_DX12_BUFFER_TYPE_INDEX;
            
            internal_buffer.ib_view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(internal_buffer.buffer);
            //internal_buffer.ib_view.StrideInBytes  = desc.elem_size;
            internal_buffer.ib_view.SizeInBytes    = desc.buffer_size;
        } break;
        
        default:
        DM_LOG_FATAL("Unknown DM buffer type: %u", desc.type);
        return false;
    }
    
    dm_memcpy(dx12_renderer->buffers + dx12_renderer->buffer_count, &internal_buffer, sizeof(internal_buffer));
    dx12_renderer->buffer_count++;
    
    return true;
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    return false;
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

bool dm_renderer_backend_create_pipeline(dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attribs, uint32_t attrib_count, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    dm_dx12_pipeline internal_pipe = { 0 };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = { 0 };
    desc.pRootSignature = dx12_renderer->root_signature;
    
    D3D12_RENDER_TARGET_BLEND_DESC blend_desc = { 0 };
    //blend_desc.BlendEnable = pipe_desc.blend ? true : false;
    
    blend_desc.SrcBlend    = dm_blend_func_to_dx12_func(pipe_desc.blend_src_f);
    blend_desc.DestBlend   = dm_blend_func_to_dx12_func(pipe_desc.blend_dest_f);
    blend_desc.BlendOp     = dm_blend_eq_to_dx12_op(pipe_desc.blend_eq);
    
    blend_desc.SrcBlendAlpha  = dm_blend_func_to_dx12_func(pipe_desc.blend_src_alpha_f);
    blend_desc.DestBlendAlpha = dm_blend_func_to_dx12_func(pipe_desc.blend_dest_alpha_f);
    blend_desc.BlendOpAlpha   = dm_blend_func_to_dx12_func(pipe_desc.blend_alpha_eq);
    
    blend_desc.LogicOp = D3D12_LOGIC_OP_NOOP;
    blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    desc.BlendState.RenderTarget[0] = blend_desc;
    
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = dm_cull_to_dx12_cull(pipe_desc.cull_mode);
    
    //desc.DepthStencilState.DepthEnable   = pipe_desc.depth ? true : false;
    //desc.DepthStencilState.StencilEnable = pipe_desc.stencil ? true : false;
    
    internal_pipe.topology = dm_toplogy_to_dx11_topology(pipe_desc.primitive_topology);
    
    desc.PrimitiveTopologyType = dm_toplogy_to_dx12_topology(pipe_desc.primitive_topology);
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    
    desc.SampleMask = 0xFFFFFFFF;
    
    switch (pipe_desc.winding_order)
    {
        case DM_WINDING_CLOCK:
        {
            desc.RasterizerState.FrontCounterClockwise = false;
        } break;
        case DM_WINDING_COUNTER_CLOCK:
        {
            desc.RasterizerState.FrontCounterClockwise = true;
        } break;
        default:
        DM_LOG_FATAL("Unknown winding order!");
        return false;
    }
    
    // shaders
    // vertex shader
    wchar_t ws[100];
    swprintf(ws, 100, L"%hs", pipe_desc.vertex_shader);
    
    ID3DBlob* blob = NULL;
    hr = D3DReadFileToBlob(ws, &blob); 
    if(!dm_platform_win32_decode_hresult(hr) || !blob) 
    { 
        DM_LOG_FATAL("D3DReadFileToBlob failed! (vertex shader)"); 
        return false; 
    }
    
    desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(blob);
    desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(blob);
    
    // vertex attribs
    D3D12_INPUT_ELEMENT_DESC* input_desc = dm_alloc(attrib_count * 4 * sizeof(D3D12_INPUT_ELEMENT_DESC));
    uint32_t count = 0;
    
    for(uint32_t i=0; i<attrib_count; i++)
    {
        dm_vertex_attrib_desc attrib_desc = attribs[i];
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
    
    desc.InputLayout.pInputElementDescs = input_desc;
    desc.InputLayout.NumElements        = count;
    
    // pixel shader
    swprintf(ws, 100, L"%hs", pipe_desc.pixel_shader);
    
    hr = D3DReadFileToBlob(ws, &blob); 
    if(!dm_platform_win32_decode_hresult(hr) || !blob) 
    { 
        DM_LOG_FATAL("D3DReadFileToBlob failed! (pixel shader)"); 
        return false; 
    }
    
    desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(blob);
    desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(blob);
    
    // finally create
    hr = ID3D12Device2_CreateGraphicsPipelineState(dx12_renderer->device, &desc, &IID_ID3D12PipelineState, &internal_pipe.pipeline_state);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D12Device2_CreateGraphicsPipelineState failed");
        return false;
    }
    
    // add to array
    dm_memcpy(dx12_renderer->pipelines + dx12_renderer->pipe_count, &internal_pipe, sizeof(internal_pipe));
    dx12_renderer->pipe_count++;
    
    // cleanup
    dm_free(&input_desc);
    ID3D10Blob_Release(blob);
    blob = NULL;
    
    return true;
}

void dm_dx12_destroy_pipeline(dm_render_handle handle, dm_dx12_renderer* dx12_renderer)
{
    if(handle > dx12_renderer->pipe_count)
    {
        DM_LOG_ERROR("Trying to destroy invalid DirectX12 pipeline");
        return;
    }
    
    dm_dx12_pipeline* internal_pipe = &dx12_renderer->pipelines[handle];
    
    
    ID3D12PipelineState_Release(internal_pipe->pipeline_state);
    internal_pipe->pipeline_state = NULL;
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
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->pipe_count)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 pipeline");
        return false;
    }
    
    dm_dx12_pipeline* internal_pipe = &dx12_renderer->pipelines[handle];
    
    ID3D12GraphicsCommandList_SetPipelineState(dx12_renderer->command_list, internal_pipe->pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(dx12_renderer->command_list, internal_pipe->topology);
    
    return true;
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
    DM_DX12_GET_RENDERER;
    HRESULT hr;
    
    if(handle > dx12_renderer->buffer_count)
    {
        DM_LOG_FATAL("Trying to bind invalid DirectX12 buffer");
        return false;
    }
    
    dm_dx12_buffer* internal_buffer = &dx12_renderer->buffers[handle];
    
    switch(internal_buffer->type)
    {
        case DM_DX12_BUFFER_TYPE_VERTEX:
        ID3D12GraphicsCommandList_IASetVertexBuffers(dx12_renderer->command_list, 0,1, &internal_buffer->vb_view);
        break;
        
        default:
        DM_LOG_FATAL("Unknown buffer type");
        return false;
    }
    
    return true;
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
    DM_DX12_GET_RENDERER;
    
    ID3D12GraphicsCommandList_DrawInstanced(dx12_renderer->command_list, count, 1, start, 0);
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