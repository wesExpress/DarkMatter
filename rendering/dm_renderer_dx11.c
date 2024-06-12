#include "dm.h"

#ifdef DM_DIRECTX11

#include "platform/dm_platform_win32.h"

#define COBJMACROS
#include <d3d11.h>
#include <dxgi.h>

typedef struct dm_dx11_vertex_buffer_t
{
    ID3D11Buffer* buffer;

    size_t stride, count;
} dm_dx11_vertex_buffer;

typedef struct dm_dx11_index_buffer_t
{
    ID3D11Buffer* buffer;
    DXGI_FORMAT   format;
} dm_dx11_index_buffer;

typedef struct dm_dx11_constant_buffer_t
{
    ID3D11Buffer*            buffer;
    D3D11_MAPPED_SUBRESOURCE mapped_resource;

    dm_constant_buffer_stage stage;
} dm_dx11_constant_buffer;

typedef struct dm_dx11_structured_buffer_t
{
    ID3D11Buffer* buffer;
    size_t stride, count;
} dm_dx11_structured_buffer;

typedef struct dm_dx11_texture_t
{
	ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* view;
    DXGI_FORMAT format; 
    
    uint32_t width, height;
} dm_dx11_texture;

typedef enum dm_dx11_pipeline_flag_t
{
    DM_DX11_PIPELINE_FLAG_NONE      = 0,
    DM_DX11_PIPELINE_FLAG_WIREFRAME = 1 << 0,
    DM_DX11_PIPELINE_FLAG_BLEND     = 1 << 1,
    DM_DX11_PIPELINE_FLAG_DEPTH     = 1 << 2,
    DM_DX11_PIPELINE_FLAG_STENCIL   = 1 << 3,
    DM_DX11_PIPELINE_FLAG_UNKNOWN   = 1 << 4
} dm_dx11_pipeline_flag;

typedef struct dm_dx11_pipeline_t
{
	ID3D11DepthStencilState* depth_stencil_state;
    ID3D11RasterizerState*   rasterizer_state;
    ID3D11RasterizerState*   wireframe_state;
    ID3D11BlendState*        blend_state;
    ID3D11SamplerState*      sample_state;

    D3D11_PRIMITIVE_TOPOLOGY default_topology;

    ID3D11VertexShader* vertex_shader;
    ID3D11PixelShader*  pixel_shader;
    ID3D11InputLayout*  input_layout;

    dm_dx11_pipeline_flag flags;
} dm_dx11_pipeline;

// directx renderer
typedef enum dm_dx11_resource_t
{
    DM_DX11_RESOURCE_BUFFER,
    DM_DX11_RESOURCE_SHADER,
    DM_DX11_RESOURCE_TEXTURE,
    DM_DX11_RESOURCE_FRAMEBUFFER,
    DM_DX11_RESOURCE_PIPELINE,
    DM_DX11_RESOURCE_UNKNOWN
} dm_dx11_resource;

#define DM_DX11_MAX_BUFFERS  1000
#define DM_DX11_MAX_TEXTURES 1000
#define DM_DX11_MAX_PIPES    10
typedef struct dm_dx11_renderer_t
{
    ID3D11Device*           device;
    ID3D11DeviceContext*    context;
    IDXGISwapChain*         swap_chain;
    ID3D11RenderTargetView* render_view;
    ID3D11Texture2D*        render_back_buffer;
    ID3D11DepthStencilView* depth_stencil_view;
    ID3D11Texture2D*        depth_stencil_back_buffer;
    
    HWND hwnd;
	HINSTANCE h_instance;
    
    uint16_t vb_count, ib_count, cb_count, sb_count, texture_count;
    uint16_t pipe_count, compute_pipe_count;

    dm_dx11_vertex_buffer     vertex_buffers[DM_DX11_MAX_BUFFERS];
    dm_dx11_index_buffer      index_buffers[DM_DX11_MAX_BUFFERS];
    dm_dx11_structured_buffer structured_buffers[DM_DX11_MAX_BUFFERS];
    dm_dx11_constant_buffer   constant_buffers[DM_DX11_MAX_BUFFERS];

    dm_dx11_texture textures[DM_DX11_MAX_TEXTURES];

    dm_dx11_pipeline pipelines[DM_DX11_MAX_PIPES];

#ifdef DM_DEBUG
    ID3D11Debug* debugger;
#endif
} dm_dx11_renderer;

#define DM_DX11_GET_RENDERER dm_dx11_renderer* dx11_renderer = renderer->internal_renderer

void dm_dx11_destroy_vertex_buffer(dm_dx11_vertex_buffer* buffer);
void dm_dx11_destroy_index_buffer(dm_dx11_index_buffer* buffer);
void dm_dx11_destroy_structured_buffer(dm_dx11_structured_buffer* buffer);
void dm_dx11_destroy_constant_buffer(dm_dx11_constant_buffer* buffer, ID3D11DeviceContext* context);
void dm_dx11_destroy_texture(dm_dx11_texture* texture, ID3D11DeviceContext* context);
void dm_dx11_destroy_pipeline(dm_dx11_pipeline* pipeline);

 /*********************
 DX11 ENUM CONVERSIONS
***********************/
DM_INLINE
D3D11_USAGE dm_buffer_usage_to_dx11(dm_buffer_usage usage)
{
	switch (usage)
	{
        case DM_BUFFER_USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
        case DM_BUFFER_USAGE_STATIC:  return D3D11_USAGE_IMMUTABLE;
        case DM_BUFFER_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
        
        default:
		DM_LOG_FATAL("Unknown buffer usage!");
		return D3D11_USAGE_STAGING+1;
	}
}

DM_INLINE
D3D11_BIND_FLAG dm_buffer_type_to_dx11(dm_buffer_type type)
{
	switch (type)
	{
        case DM_BUFFER_TYPE_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
        case DM_BUFFER_TYPE_INDEX:  return D3D11_BIND_INDEX_BUFFER;
        
        default:
		DM_LOG_FATAL("Unknown buffer type!");
		return D3D11_BIND_VIDEO_ENCODER+1;
	}
}

DM_INLINE
D3D11_CPU_ACCESS_FLAG dm_buffer_cpu_access_to_dx11(dm_buffer_cpu_access access)
{
	switch (access)
	{
        case DM_BUFFER_CPU_READ:  return D3D11_CPU_ACCESS_READ;
        case DM_BUFFER_CPU_WRITE: return D3D11_CPU_ACCESS_WRITE;
        
        default:
		DM_LOG_FATAL("Unknown cpu access!");
		return D3D11_CPU_ACCESS_READ+1;
	}
}

DM_INLINE
DXGI_FORMAT dm_vertex_t_to_dx11_format(dm_vertex_attrib_desc desc)
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
D3D11_INPUT_CLASSIFICATION dm_vertex_class_to_dx11_class(dm_vertex_attrib_class dm_class)
{
	switch (dm_class)
	{
        case DM_VERTEX_ATTRIB_CLASS_VERTEX:   return D3D11_INPUT_PER_VERTEX_DATA;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE: return D3D11_INPUT_PER_INSTANCE_DATA;
        
        default:
		DM_LOG_FATAL("Unknown vertex attribute input class!");
        return D3D11_INPUT_PER_VERTEX_DATA;
	}
}

DM_INLINE
D3D11_CULL_MODE dm_cull_to_dx11_cull(dm_cull_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_CULL_FRONT_BACK:
        case DM_CULL_FRONT:      return D3D11_CULL_FRONT;
        case DM_CULL_BACK:       return D3D11_CULL_BACK;
        case DM_CULL_NONE:       return D3D11_CULL_NONE;
        
        default:
		DM_LOG_FATAL("Unknown cull mode!");
		return D3D11_CULL_NONE;
	}
}

DM_INLINE
D3D11_PRIMITIVE_TOPOLOGY dm_toplogy_to_dx11_topology(dm_primitive_topology dm_top)
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
D3D11_COMPARISON_FUNC dm_comp_to_directx_comp(dm_comparison dm_comp)
{
	switch (dm_comp)
	{
        case DM_COMPARISON_ALWAYS:   return D3D11_COMPARISON_ALWAYS;
        case DM_COMPARISON_NEVER:    return D3D11_COMPARISON_NEVER;
        case DM_COMPARISON_EQUAL:    return D3D11_COMPARISON_EQUAL;
        case DM_COMPARISON_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
        case DM_COMPARISON_LESS:     return D3D11_COMPARISON_LESS;
        case DM_COMPARISON_LEQUAL:   return D3D11_COMPARISON_LESS_EQUAL;
        case DM_COMPARISON_GREATER:  return D3D11_COMPARISON_GREATER;
        case DM_COMPARISON_GEQUAL:   return D3D11_COMPARISON_GREATER_EQUAL;
        
        default:
		DM_LOG_FATAL("Unknown comparison function!");
		return D3D11_COMPARISON_ALWAYS + 1;
	}
}

DM_INLINE
D3D11_FILTER dm_image_filter_to_dx11_filter(dm_filter filter)
{
	switch (filter)
	{
        case DM_FILTER_NEAREST:
        case DM_FILTER_LINEAR: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        
        default:
		DM_LOG_FATAL("Unknown filter function!");
		return D3D11_FILTER_MAXIMUM_ANISOTROPIC + 1;
	}
}

DM_INLINE
D3D11_TEXTURE_ADDRESS_MODE dm_texture_mode_to_dx11_mode(dm_texture_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_TEXTURE_MODE_WRAP:          return D3D11_TEXTURE_ADDRESS_WRAP;
        case DM_TEXTURE_MODE_BORDER:        return D3D11_TEXTURE_ADDRESS_BORDER;
        case DM_TEXTURE_MODE_EDGE:          return D3D11_TEXTURE_ADDRESS_CLAMP;
        case DM_TEXTURE_MODE_MIRROR_REPEAT: return D3D11_TEXTURE_ADDRESS_MIRROR;
        case DM_TEXTURE_MODE_MIRROR_EDGE:   return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        
        default:
		DM_LOG_FATAL("Unknown texture mode!");
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE + 1;
	}
}

DM_INLINE
D3D11_BLEND dm_blend_func_to_dx11_func(dm_blend_func func)
{
    switch(func)
    {
        case DM_BLEND_FUNC_ZERO:                return D3D11_BLEND_ZERO;
        case DM_BLEND_FUNC_ONE:                 return D3D11_BLEND_ONE;
        case DM_BLEND_FUNC_SRC_COLOR:           return D3D11_BLEND_SRC_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
        case DM_BLEND_FUNC_DST_COLOR:           return D3D11_BLEND_DEST_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
        case DM_BLEND_FUNC_SRC_ALPHA:           return D3D11_BLEND_SRC_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
        case DM_BLEND_FUNC_DST_ALPHA:           return D3D11_BLEND_DEST_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
        
        case DM_BLEND_FUNC_CONST_COLOR: 
        case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR:
        case DM_BLEND_FUNC_CONST_ALPHA:
        case DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA: 
        default:
        DM_LOG_ERROR("Unsupported blend for DirectX, returning D3D11_BLEND_SRC_COLOR");
        return D3D11_BLEND_SRC_COLOR;
    }
}

DM_INLINE
D3D11_BLEND_OP dm_blend_eq_to_dx11_op(dm_blend_equation eq)
{
    switch(eq)
    {
        case DM_BLEND_EQUATION_ADD:              return D3D11_BLEND_OP_ADD;
        case DM_BLEND_EQUATION_SUBTRACT:         return D3D11_BLEND_OP_SUBTRACT;
        case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return D3D11_BLEND_OP_REV_SUBTRACT;
        case DM_BLEND_EQUATION_MIN:              return D3D11_BLEND_OP_MIN;
        case DM_BLEND_EQUATION_MAX:              return D3D11_BLEND_OP_MAX;
        
        default:
        DM_LOG_ERROR("Unknown blend equation, returning DM_BLEND_OP_ADD");
        return D3D11_BLEND_OP_ADD;
    }
}   

/***********************
 BACKEND IMPLEMENTATION
*************************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Directx11 Backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_dx11_renderer));
    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;
    
    dm_internal_w32_data* w32_data = context->platform_data.internal_data;
    
    dx11_renderer->hwnd = w32_data->hwnd;
    dx11_renderer->h_instance = w32_data->h_instance;

    HRESULT hr;

    // device
    {
         UINT flags = 0;
#if DM_DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL feature_level;


        // create the device and immediate context
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION,
                               &dx11_renderer->device, &feature_level, &dx11_renderer->context);
        if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->device || !dx11_renderer->context) 
        {
            DM_LOG_FATAL("D3D11CreateDevice failed"); 
            return false; 
        }

        if(feature_level != D3D_FEATURE_LEVEL_11_0) 
        { 
            DM_LOG_FATAL("Direct3D Feature Level 11 unsupported"); 
            return false; 
        }

        UINT msaa_quality;
        hr = ID3D11Device_CheckMultisampleQualityLevels(dx11_renderer->device, DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_quality);
        if(!dm_platform_win32_decode_hresult(hr)) 
        { 
            DM_LOG_FATAL("D3D11Device_CheckMultisampleQualityLevels failed!"); 
            return false; 
        }

        // if in debug, create the debugger to query live objects
#if DM_DEBUG
        void* tmp = NULL;
        hr = ID3D11Device_QueryInterface(dx11_renderer->device, &IID_ID3D11Debug, &tmp);
        if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
        { 
            DM_LOG_FATAL("D3D11Device_QueryInterface failed!"); 
            return false; 
        }
        dx11_renderer->debugger = tmp;
#endif
    }

    // swapchain
    {
        IDXGISwapChain* swap_chain = NULL;
    
        // make sure the device has been created and then grab it
        RECT client_rect;
        GetClientRect(dx11_renderer->hwnd, &client_rect);

        struct DXGI_SWAP_CHAIN_DESC desc = { 0 };
        desc.BufferDesc.Width                   = client_rect.right;
        desc.BufferDesc.Height                  = client_rect.bottom;
        desc.BufferDesc.RefreshRate.Numerator   = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.SampleDesc.Count                   = 1;
        desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount                        = 1;
        desc.OutputWindow                       = dx11_renderer->hwnd;
        desc.Windowed                           = TRUE;
        desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
        //desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        // obtain factory
        IDXGIDevice* dxgi_device = NULL;
        IDXGIAdapter* dxgi_adapter = NULL;
        IDXGIFactory* dxgi_factory = NULL;
        
        {
            void* tmp = NULL;
            hr = IDXGIAdapter_QueryInterface(dx11_renderer->device, &IID_IDXGIDevice, &tmp);
            if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
            { 
                DM_LOG_FATAL("D3D11Device_QueryInterface failed"); 
                return false; 
            }

            dxgi_device = tmp;
        }

        {
            void* tmp = NULL;
            hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter, &tmp);
            if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
            { 
                DM_LOG_FATAL("IDXGIDevice_GetParent failed"); 
                return false; 
            }
            
            dxgi_adapter = tmp;
        }

        {
            void* tmp = NULL;
            hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory, &tmp);
            if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
            { 
                DM_LOG_FATAL("IDXGIAdapter_GetParent failed"); 
                return false; 
            }

            dxgi_factory = tmp;
        }

        // create the swap chain
        {
            hr = IDXGIFactory_CreateSwapChain(dxgi_factory, (IUnknown*)dx11_renderer->device, &desc, &dx11_renderer->swap_chain);
            if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->swap_chain) 
            { 
                DM_LOG_FATAL("IDXGIFactory_CreateSwapChain failed"); 
                return false; 
            }
        }

        // release pack animal directx objects
        IDXGIDevice_Release(dxgi_device);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIFactory_Release(dxgi_factory);
    }

    // render target
    {
        void* tmp = NULL;
        hr = IDXGISwapChain_GetBuffer(dx11_renderer->swap_chain, 0, &IID_ID3D11Texture2D, &tmp);
        if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
        { 
            DM_LOG_FATAL("IDXGISwapChain_GetBuffer failed"); 
            return false; 
        }
        dx11_renderer->render_back_buffer = tmp;

        ID3D11Device_CreateRenderTargetView(dx11_renderer->device, (ID3D11Resource*)dx11_renderer->render_back_buffer, NULL, &dx11_renderer->render_view);
    }

    // depth stencil buffer
    {
        RECT client_rect;
        GetClientRect(dx11_renderer->hwnd, &client_rect);

        D3D11_TEXTURE2D_DESC desc = { 0 };
        desc.Width            = client_rect.right;
        desc.Height           = client_rect.bottom;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

        hr = ID3D11Device_CreateTexture2D(dx11_renderer->device, &desc, NULL, &dx11_renderer->depth_stencil_back_buffer);
        if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->depth_stencil_back_buffer) 
        { 
            DM_LOG_FATAL("ID3D11Device_CreateTexture2D failed"); 
            return false; 
        }

        hr = ID3D11Device_CreateDepthStencilView(dx11_renderer->device, (ID3D11Resource*)dx11_renderer->depth_stencil_back_buffer, 0, &dx11_renderer->depth_stencil_view);
        if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->depth_stencil_view) 
        { 
            DM_LOG_FATAL("ID3D11Device_CreateDepthStencilView failed"); 
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
    HRESULT hr;

    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;

    // resources
    for(uint16_t i=0; i<dx11_renderer->vb_count; i++)
    {
        dm_dx11_destroy_vertex_buffer(&dx11_renderer->vertex_buffers[i]);
    }

    for(uint16_t i=0; i<dx11_renderer->ib_count; i++)
    {
        dm_dx11_destroy_index_buffer(&dx11_renderer->index_buffers[i]);
    }
    
    for(uint16_t i=0; i<dx11_renderer->cb_count; i++)
    {
        dm_dx11_destroy_constant_buffer(&dx11_renderer->constant_buffers[i], dx11_renderer->context);
    }

    for(uint16_t i=0; i<dx11_renderer->sb_count; i++)
    {
        dm_dx11_destroy_structured_buffer(&dx11_renderer->structured_buffers[i]);
    }

    for(uint16_t i=0; i<dx11_renderer->texture_count; i++)
    {
        dm_dx11_destroy_texture(&dx11_renderer->textures[i], dx11_renderer->context);
    }

    for(uint8_t i=0; i<dx11_renderer->pipe_count; i++)
    {
        dm_dx11_destroy_pipeline(&dx11_renderer->pipelines[i]);
    }

    // backend
    IDXGISwapChain_Release(dx11_renderer->swap_chain);
    ID3D11Resource_Release(dx11_renderer->render_back_buffer);
    ID3D11RenderTargetView_Release(dx11_renderer->render_view);
    ID3D11Resource_Release(dx11_renderer->depth_stencil_back_buffer);
    ID3D11DepthStencilView_Release(dx11_renderer->depth_stencil_view);

    ID3D11DeviceContext_Release(dx11_renderer->context);

#if DM_DEBUG
    hr = ID3D11Debug_ReportLiveDeviceObjects(dx11_renderer->debugger, D3D11_RLDO_DETAIL);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_ERROR("ID3D11Debug_ReportLiveDeviceObjects failed");
    }
    ID3D11Debug_Release(dx11_renderer->debugger);
#endif
    
    ID3D11Device_Release(dx11_renderer->device);

    dm_free(&context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    ID3D11DeviceContext_OMSetRenderTargets(dx11_renderer->context, 1u, &dx11_renderer->render_view, dx11_renderer->depth_stencil_view);
    
    float color[] = { 1,0,1,1 };
    ID3D11DeviceContext_ClearRenderTargetView(dx11_renderer->context, dx11_renderer->render_view, color);
    ID3D11DeviceContext_ClearDepthStencilView(dx11_renderer->context, dx11_renderer->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    D3D11_VIEWPORT new_viewport = { 0 };
    new_viewport.Width = (FLOAT)renderer->width;
    new_viewport.Height = (FLOAT)renderer->height;
    new_viewport.MaxDepth = 1.0f;
    
    ID3D11DeviceContext_RSSetViewports(dx11_renderer->context, 1, &new_viewport);

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;
    HRESULT hr;

    uint32_t v = context->renderer.vsync ? 1 : 0;
    hr = IDXGISwapChain_Present(dx11_renderer->swap_chain, v, 0);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        if(hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            DM_LOG_FATAL("DirectX Device removed");
        }
        else
        {
            DM_LOG_FATAL("Something bad happened when presenting buffers");
        }

        return false;
    }

    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    ID3D11DeviceContext_OMSetRenderTargets(dx11_renderer->context, 0,0,0);
    
    ID3D11RenderTargetView_Release(dx11_renderer->render_view);
    ID3D11Resource_Release(dx11_renderer->render_back_buffer);
    ID3D11DepthStencilView_Release(dx11_renderer->depth_stencil_view);
    ID3D11Resource_Release(dx11_renderer->depth_stencil_back_buffer);
    
    hr = IDXGISwapChain_ResizeBuffers(dx11_renderer->swap_chain, 0,width,height,DXGI_FORMAT_UNKNOWN,0);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("IDXGISwapChain_ResizeBuffers failed");
        return false; 
    }
    
    void* tmp = NULL;
    hr = IDXGISwapChain_GetBuffer(dx11_renderer->swap_chain, 0, &IID_ID3D11Texture2D, &tmp);
    if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
    { 
        DM_LOG_FATAL("IDXGISwapChain_GetBuffer failed"); 
        return false; 
    }
    dx11_renderer->render_back_buffer = tmp;

    hr = ID3D11Device_CreateRenderTargetView(dx11_renderer->device, (ID3D11Resource*)dx11_renderer->render_back_buffer, NULL, &dx11_renderer->render_view);
    if(!dm_platform_win32_decode_hresult(hr) || !tmp) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateRenderTargetView failed"); 
        return false; 
    }

    RECT client_rect;
    GetClientRect(dx11_renderer->hwnd, &client_rect);

    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width            = client_rect.right;
    desc.Height           = client_rect.bottom;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    hr = ID3D11Device_CreateTexture2D(dx11_renderer->device, &desc, NULL, &dx11_renderer->depth_stencil_back_buffer);
    if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->depth_stencil_back_buffer) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateTexture2D failed"); 
        return false; 
    }

    hr = ID3D11Device_CreateDepthStencilView(dx11_renderer->device, (ID3D11Resource*)dx11_renderer->depth_stencil_back_buffer, 0, &dx11_renderer->depth_stencil_view);
    if(!dm_platform_win32_decode_hresult(hr) || !dx11_renderer->depth_stencil_view) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateDepthStencilView failed"); 
        return false; 
    }

    //    
    ID3D11DeviceContext_OMSetRenderTargets(dx11_renderer->context, 1u, &dx11_renderer->render_view, NULL);

    return true;
}

/*****************
RESOURCE CREATION
*******************/
bool dm_renderer_backend_create_vertex_buffer(const dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
	HRESULT hr;
    
	dm_dx11_vertex_buffer internal_buffer = {0};
    
	D3D11_BUFFER_DESC buffer_desc = { 0 };
	buffer_desc.Usage = desc.dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.ByteWidth = desc.size;
	buffer_desc.StructureByteStride = desc.stride;
	if(desc.dynamic) buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    D3D11_SUBRESOURCE_DATA sd = { 0 };
    sd.pSysMem = desc.data ? desc.data : NULL;
        
    hr = ID3D11Device_CreateBuffer(dx11_renderer->device, &buffer_desc, &sd, &internal_buffer.buffer);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateBuffer failed"); 
        return false; 
    }
    
    internal_buffer.stride = desc.stride;
    
    //
    dm_memcpy(dx11_renderer->vertex_buffers + dx11_renderer->vb_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx11_renderer->vb_count++);

    return true;
}

void dm_dx11_destroy_vertex_buffer(dm_dx11_vertex_buffer* buffer)
{
    ID3D11Buffer_Release(buffer->buffer);
}

bool dm_renderer_backend_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
	HRESULT hr;
    
	dm_dx11_index_buffer internal_buffer = {0};
    
	D3D11_BUFFER_DESC buffer_desc = { 0 };
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffer_desc.ByteWidth = desc.size;
	buffer_desc.StructureByteStride = desc.size / desc.count;
    
    switch(buffer_desc.StructureByteStride)
    {
        case 2: 
        internal_buffer.format = DXGI_FORMAT_R16_UINT;
        break;

        case 4:
        internal_buffer.format = DXGI_FORMAT_R32_UINT;
        break;

        default:
        DM_LOG_FATAL("Unsupported index size for DX11 index buffer");
        return false;
    }

    D3D11_SUBRESOURCE_DATA sd = { 0 };
    sd.pSysMem = desc.data ? desc.data : NULL;
        
    hr = ID3D11Device_CreateBuffer(dx11_renderer->device, &buffer_desc, &sd, &internal_buffer.buffer);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateBuffer failed"); 
        return false; 
    }
    
    //
    dm_memcpy(dx11_renderer->index_buffers + dx11_renderer->ib_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx11_renderer->ib_count++);

    return true;
}

void dm_dx11_destroy_index_buffer(dm_dx11_index_buffer* buffer)
{
    ID3D11Device_Release(buffer->buffer);
}

bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    if(!desc.data)
    {
        DM_LOG_FATAL("Must have data to initialize constant buffer");
        return false;
    }
    
    dm_dx11_constant_buffer internal_buffer = { 0 };

    D3D11_BUFFER_DESC buffer_desc = { 0 };
    buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.ByteWidth           = ((desc.data_size + 15) / 16) * 16;
    buffer_desc.StructureByteStride = sizeof(float);
    
    hr = ID3D11Device_CreateBuffer(dx11_renderer->device, &buffer_desc, 0, &internal_buffer.buffer);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateBuffer failed!");
        return false; 
    }

    ZeroMemory(&internal_buffer.mapped_resource, sizeof(internal_buffer.mapped_resource));
    hr = ID3D11DeviceContext_Map(dx11_renderer->context, (ID3D11Resource*)internal_buffer.buffer, 0, D3D11_MAP_WRITE, 0, &internal_buffer.mapped_resource);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11DeviceContext_Map failed");
        return false;
    }
    
    dm_memcpy(internal_buffer.mapped_resource.pData, desc.data, desc.data_size);

    //
    dm_memcpy(dx11_renderer->constant_buffers + dx11_renderer->cb_count, &internal_buffer, sizeof(internal_buffer));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx11_renderer->cb_count++);
    
    return true;
}

void dm_dx11_destroy_constant_buffer(dm_dx11_constant_buffer* buffer, ID3D11DeviceContext* context)
{
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)buffer->buffer, 0);
    buffer->mapped_resource.pData = NULL;

    ID3D11Buffer_Release(buffer->buffer);
}

bool dm_renderer_backend_create_structured_buffer(const dm_structured_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_dx11_destroy_structured_buffer(dm_dx11_structured_buffer* buffer)
{
    ID3D11Buffer_Release(buffer->buffer);
}

DM_INLINE
bool dm_dx11_create_input_element(dm_vertex_attrib_desc attrib_desc, D3D11_INPUT_ELEMENT_DESC* element_desc)
{
    DXGI_FORMAT format = dm_vertex_t_to_dx11_format(attrib_desc);
    if (format == DXGI_FORMAT_UNKNOWN) return false;
    D3D11_INPUT_CLASSIFICATION input_class = dm_vertex_class_to_dx11_class(attrib_desc.attrib_class);
    if (input_class == DM_VERTEX_ATTRIB_CLASS_UNKNOWN) return false;
    
    element_desc->SemanticName = attrib_desc.name;
    element_desc->SemanticIndex = attrib_desc.index;
    element_desc->Format = format;
    element_desc->AlignedByteOffset = attrib_desc.offset;
    element_desc->InputSlotClass = input_class;
    if (input_class == D3D11_INPUT_PER_INSTANCE_DATA)
    {
        element_desc->InstanceDataStepRate = 1;
        element_desc->InputSlot = 1;
    }
    
    return true;
}

bool dm_renderer_backend_create_pipeline(const dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_dx11_pipeline internal_pipe = { 0 };

    // blending
    if(desc.flags & DM_PIPELINE_FLAG_BLEND)
    {
        internal_pipe.flags |= DM_DX11_PIPELINE_FLAG_BLEND;

        D3D11_BLEND_DESC blend_desc = { 0 };
        blend_desc.RenderTarget[0].BlendEnable = true;

        blend_desc.RenderTarget[0].SrcBlend    = dm_blend_func_to_dx11_func(desc.blend_desc.src_func);
        blend_desc.RenderTarget[0].DestBlend   = dm_blend_func_to_dx11_func(desc.blend_desc.dest_func);
        blend_desc.RenderTarget[0].BlendOp     = dm_blend_eq_to_dx11_op(desc.blend_desc.eq);

        blend_desc.RenderTarget[0].SrcBlendAlpha  = dm_blend_func_to_dx11_func(desc.blend_alpha_desc.src_func);
        blend_desc.RenderTarget[0].DestBlendAlpha = dm_blend_func_to_dx11_func(desc.blend_alpha_desc.dest_func);
        blend_desc.RenderTarget[0].BlendOpAlpha   = dm_blend_eq_to_dx11_op(desc.blend_alpha_desc.eq);

        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = ID3D11Device_CreateBlendState(dx11_renderer->device, &blend_desc, &internal_pipe.blend_state);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D11Device_CreateBlendState failed");
            return false;
        }
    }

    // depth stencil testing
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = { 0 };

    if(desc.flags & DM_PIPELINE_FLAG_DEPTH)
    {
        internal_pipe.flags |= DM_DX11_PIPELINE_FLAG_DEPTH;

        depth_stencil_desc.DepthEnable = true;
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    }

    if(desc.flags & DM_PIPELINE_FLAG_STENCIL)
    {
        internal_pipe.flags |= DM_DX11_PIPELINE_FLAG_STENCIL;

        depth_stencil_desc.StencilEnable = true;
    }

    if(desc.flags & DM_PIPELINE_FLAG_DEPTH || desc.flags & DM_PIPELINE_FLAG_STENCIL)
    {
        hr = ID3D11Device_CreateDepthStencilState(dx11_renderer->device, &depth_stencil_desc, &internal_pipe.depth_stencil_state);
        if(!dm_platform_win32_decode_hresult(hr))
        {
            DM_LOG_FATAL("ID3D11Device_CreateDepthStencilState failed");
            return false;
        }
    }

    // rasterizer
    D3D11_RASTERIZER_DESC rast_desc      = { 0 };
    D3D11_RASTERIZER_DESC wireframe_desc = { 0 };

    rast_desc.FillMode      = D3D11_FILL_SOLID;
    wireframe_desc.FillMode = D3D11_FILL_WIREFRAME;

    rast_desc.CullMode      = dm_cull_to_dx11_cull(desc.raster_desc.cull);
    wireframe_desc.CullMode = dm_cull_to_dx11_cull(desc.raster_desc.cull);

    rast_desc.DepthClipEnable      = true;
    wireframe_desc.DepthClipEnable = true;

    if(desc.flags & DM_PIPELINE_FLAG_SCISSOR)
    {
        rast_desc.ScissorEnable      = true;
        wireframe_desc.ScissorEnable = true;
    }

    if(desc.flags & DM_PIPELINE_FLAG_WIREFRAME) internal_pipe.flags |= DM_DX11_PIPELINE_FLAG_WIREFRAME;

    switch(desc.raster_desc.winding)
    {
        case DM_WINDING_CLOCK:
        rast_desc.FrontCounterClockwise      = false;
        wireframe_desc.FrontCounterClockwise = false;
        break;

        case DM_WINDING_COUNTER_CLOCK:
        rast_desc.FrontCounterClockwise      = true;
        wireframe_desc.FrontCounterClockwise = true;
        break;

        default:
        DM_LOG_FATAL("Unknown winding order");
        return false;
    }

    hr = ID3D11Device_CreateRasterizerState(dx11_renderer->device, &rast_desc, &internal_pipe.rasterizer_state);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11Device_CreateRasterizerState failed");
        return false;
    }

    hr = ID3D11Device_CreateRasterizerState(dx11_renderer->device, &wireframe_desc, &internal_pipe.wireframe_state);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11Device_CreateRasterizerState failed (wireframe)");
        return false;
    }

    // topology
    internal_pipe.default_topology = dm_toplogy_to_dx11_topology(desc.raster_desc.topology);
    if(internal_pipe.default_topology==D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) return false;

    // sampler
    D3D11_SAMPLER_DESC sampler_desc = { 0 };
    sampler_desc.Filter         = dm_image_filter_to_dx11_filter(desc.sampler_desc.filter);
    sampler_desc.AddressU       = dm_texture_mode_to_dx11_mode(desc.sampler_desc.u);
    sampler_desc.AddressV       = dm_texture_mode_to_dx11_mode(desc.sampler_desc.v);
    sampler_desc.AddressW       = dm_texture_mode_to_dx11_mode(desc.sampler_desc.w);
    sampler_desc.ComparisonFunc = dm_comp_to_directx_comp(desc.sampler_desc.comparison_function);
    sampler_desc.MaxAnisotropy  = 1;
    if(sampler_desc.Filter!=D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT) sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = ID3D11Device_CreateSamplerState(dx11_renderer->device, &sampler_desc, &internal_pipe.sample_state);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11Device_CreateSamplerState failed");
        return false;
    }

    // shader
    // input layout
    D3D11_INPUT_ELEMENT_DESC* input_desc = dm_alloc(desc.vertex_attrib_count * 4 * sizeof(D3D11_INPUT_ELEMENT_DESC));
    uint32_t count = 0;
    
    for(uint32_t i=0; i<desc.vertex_attrib_count; i++)
    {
        dm_vertex_attrib_desc attrib_desc = desc.vertex_attribs[i];
        if ((attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            for (uint32_t j = 0; j < attrib_desc.count; j++)
            {
                dm_vertex_attrib_desc sub_desc = attrib_desc;
                if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) sub_desc.data_t = DM_VERTEX_DATA_T_INT;
                else sub_desc.data_t = DM_VERTEX_DATA_T_FLOAT;
                
                sub_desc.offset = sub_desc.offset + sizeof(float) * j;
                
                D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
                if (!dm_dx11_create_input_element(sub_desc, &element_desc)) return false;
                element_desc.SemanticIndex = j;
                element_desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                
                input_desc[count++] = element_desc;
            }
        }
        else
        {
            D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
            if (!dm_dx11_create_input_element(attrib_desc, &element_desc)) return false;
            
            // append the element_desc to the array
            input_desc[count++] = element_desc;
        }	
    }
    
    hr = ID3D11Device_CreateInputLayout(dx11_renderer->device, input_desc, (UINT)count, desc.shaders[DM_SHADER_TYPE_VERTEX].shader_data, desc.shaders[DM_SHADER_TYPE_VERTEX].shader_size, &internal_pipe.input_layout);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateInputLayout failed!"); 
        return false; 
    }

    dm_free((void**)&input_desc);

    hr = ID3D11Device_CreateVertexShader(dx11_renderer->device, desc.shaders[DM_SHADER_TYPE_VERTEX].shader_data, desc.shaders[DM_SHADER_TYPE_VERTEX].shader_size, NULL, &internal_pipe.vertex_shader);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11Device_CreateVertexShader failed");
        return false;
    }

    hr = ID3D11Device_CreatePixelShader(dx11_renderer->device, desc.shaders[DM_SHADER_TYPE_PIXEL].shader_data, desc.shaders[DM_SHADER_TYPE_PIXEL].shader_size, NULL, &internal_pipe.pixel_shader);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11Device_CreatePixelShader failed");
        return false;
    }

    //
    dm_memcpy(dx11_renderer->pipelines + dx11_renderer->pipe_count, &internal_pipe, sizeof(internal_pipe));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx11_renderer->pipe_count++);

    return true;
}

void dm_dx11_destroy_pipeline(dm_dx11_pipeline* pipeline)
{
    ID3D11VertexShader_Release(pipeline->vertex_shader);
    ID3D11PixelShader_Release(pipeline->pixel_shader);
    ID3D11InputLayout_Release(pipeline->input_layout);

    ID3D11RasterizerState_Release(pipeline->rasterizer_state);
    ID3D11RasterizerState_Release(pipeline->wireframe_state);

    ID3D11SamplerState_Release(pipeline->sample_state);

    if(pipeline->flags&DM_DX11_PIPELINE_FLAG_DEPTH | pipeline->flags&DM_DX11_PIPELINE_FLAG_STENCIL) ID3D11DepthStencilState_Release(pipeline->depth_stencil_state);

    if(pipeline->flags&DM_DX11_PIPELINE_FLAG_BLEND)ID3D11BlendState_Release(pipeline->blend_state);
}

DM_INLINE
DXGI_FORMAT dm_texture_data_to_dxgi_format(dm_texture_data_type type, dm_texture_data_bytes byte_size, dm_texture_data_count count)
{
    switch(type)
    {
        case DM_TEXTURE_DATA_TYPE_INT:
        switch(byte_size)
        {
            case DM_TEXTURE_DATA_BYTES_8:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R8_SINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R8G8_SINT;
                default:
                break;
            } 
            break;

            case DM_TEXTURE_DATA_BYTES_16:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R16_SINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R16G16_SINT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R16G16B16A16_SINT;
                default: break;
            }
            break;

            case DM_TEXTURE_DATA_BYTES_32:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R32_SINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R32G32_SINT;
                case DM_TEXTURE_DATA_COUNT_3: return DXGI_FORMAT_R32G32B32_SINT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R32G32B32A32_SINT;
                default: break;
            }

            default: break;
        }
        break;

        case DM_TEXTURE_DATA_TYPE_UINT:
        switch(byte_size)
        {
            case DM_TEXTURE_DATA_BYTES_8:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R8_UINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R8G8_UINT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R8G8B8A8_UINT;
                default: break;
            }
            break;
            
            case DM_TEXTURE_DATA_BYTES_16:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R16_UINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R16G16_UINT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R16G16B16A16_UINT;
                default: break;
            }
            break;

            case DM_TEXTURE_DATA_BYTES_32:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R32_UINT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R32G32_UINT;
                case DM_TEXTURE_DATA_COUNT_3: return DXGI_FORMAT_R32G32B32_UINT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R32G32B32A32_UINT;
                default: break;
            }

            default: break;
        }
        break;

        case DM_TEXTURE_DATA_TYPE_UNORM:
        switch(byte_size)
        {
            case DM_TEXTURE_DATA_BYTES_8:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R8_UNORM;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R8G8_UNORM;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R8G8B8A8_UNORM;
                default: break;
            }
            break;

            case DM_TEXTURE_DATA_BYTES_16:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R16_UNORM;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R16G16_UNORM;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R16G16B16A16_UNORM;
                default: break;
            } break;

            default: break;
        }
        break;

        case DM_TEXTURE_DATA_TYPE_FLOAT:
        switch(byte_size)
        {
            case DM_TEXTURE_DATA_BYTES_16:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R16_FLOAT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R16G16_FLOAT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                default: break;
            }
            break;

            case DM_TEXTURE_DATA_BYTES_32:
            switch(count)
            {
                case DM_TEXTURE_DATA_COUNT_1: return DXGI_FORMAT_R32_FLOAT;
                case DM_TEXTURE_DATA_COUNT_2: return DXGI_FORMAT_R32G32_FLOAT;
                case DM_TEXTURE_DATA_COUNT_3: return DXGI_FORMAT_R32G32B32_FLOAT;
                case DM_TEXTURE_DATA_COUNT_4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                default: break;
            }

            default: break;
        }
        break;

        default: break;
    }

    DM_LOG_ERROR("Unsupported combination of texture data type (%u), size (%u), count (%u) for DirectX11", type, byte_size, count);
    return DXGI_FORMAT_UNKNOWN;
}

bool dm_renderer_backend_create_texture(dm_texture_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    HRESULT hr;
    
	dm_dx11_texture internal_texture = {0};
    internal_texture.width  = desc.width;
    internal_texture.height = desc.height;
    
    // texture
	D3D11_TEXTURE2D_DESC tex_desc = { 0 };
	tex_desc.Width            = desc.width;
	tex_desc.Height           = desc.height;
	tex_desc.ArraySize        = 1;
	tex_desc.SampleDesc.Count = 1;
    tex_desc.MipLevels        = 1;
	tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.Usage            = D3D11_USAGE_DEFAULT;
	tex_desc.Format           = dm_texture_data_to_dxgi_format(desc.data_type, desc.data_byte_size, desc.data_count);
	tex_desc.MiscFlags        = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    if(tex_desc.Format==DXGI_FORMAT_UNKNOWN) return false;
    
    D3D11_SUBRESOURCE_DATA init_data = { desc.data, sizeof(uint32_t) * desc.width, 0 };
    
	hr = ID3D11Device_CreateTexture2D(dx11_renderer->device, &tex_desc, &init_data, &internal_texture.texture);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateTexture2D failed"); 
        return false; 
    }
    
    // shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
	view_desc.Format = tex_desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//view_desc.Texture2D.MipLevels = -1;
    view_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    
	hr = ID3D11Device_CreateShaderResourceView(dx11_renderer->device, (ID3D11Resource*)internal_texture.texture, &view_desc, &internal_texture.view);
    if(!dm_platform_win32_decode_hresult(hr)) 
    { 
        DM_LOG_FATAL("ID3D11Device_CreateShaderResourceView failed!");
        return false; 
    }
    
	ID3D11DeviceContext_GenerateMips(dx11_renderer->context, internal_texture.view);
    
    //    
    dm_memcpy(dx11_renderer->textures + dx11_renderer->texture_count, &internal_texture, sizeof(dm_dx11_texture));
    DM_RENDER_HANDLE_SET_INDEX(*handle, dx11_renderer->texture_count++);

    return true;
}

void dm_dx11_destroy_texture(dm_dx11_texture* texture, ID3D11DeviceContext* context)
{
    ID3D11Texture2D_Release(texture->texture);
}

bool dm_renderer_backend_resize_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    const dm_render_resource_type resource_type = DM_RENDER_HANDLE_GET_TYPE(handle);
    const uint16_t resource_index = DM_RENDER_HANDLE_GET_INDEX(handle);

    dm_dx11_texture* internal_texture = &dx11_renderer->textures[handle];

    ID3D11Texture2D_Release(internal_texture->texture);

    return true;
}

//compute
bool dm_compute_backend_create_pipeline(dm_compute_pipeline_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_compute_backend_create_structured_buffer(dm_structured_buffer_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_compute_backend_create_texture(dm_texture_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    return true;
}

/***************
RENDER COMMANDS
*****************/
bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_PIPELINE)
    {
        DM_LOG_FATAL("Resource is not a pipeline");
        return false;
    }

    dm_dx11_pipeline* internal_pipe = &dx11_renderer->pipelines[index];

    if(internal_pipe->flags & DM_DX11_PIPELINE_FLAG_WIREFRAME) ID3D11DeviceContext_RSSetState(dx11_renderer->context, internal_pipe->wireframe_state);
    else                                                       ID3D11DeviceContext_RSSetState(dx11_renderer->context, internal_pipe->rasterizer_state);

    ID3D11DeviceContext_IASetPrimitiveTopology(dx11_renderer->context, internal_pipe->default_topology);

    ID3D11DeviceContext_PSSetSamplers(dx11_renderer->context, 0, 1, &internal_pipe->sample_state);

    if(internal_pipe->flags & DM_DX11_PIPELINE_FLAG_BLEND) ID3D11DeviceContext_OMSetBlendState(dx11_renderer->context, internal_pipe->blend_state, 0, 0xffffffff);
    if(internal_pipe->flags & DM_DX11_PIPELINE_FLAG_DEPTH | internal_pipe->flags & DM_DX11_PIPELINE_FLAG_STENCIL) ID3D11DeviceContext_OMSetDepthStencilState(dx11_renderer->context, internal_pipe->depth_stencil_state, 1);

    ID3D11DeviceContext_VSSetShader(dx11_renderer->context, internal_pipe->vertex_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(dx11_renderer->context, internal_pipe->pixel_shader, NULL, 0);
    ID3D11DeviceContext_IASetInputLayout(dx11_renderer->context, internal_pipe->input_layout);

    return true;
}

bool dm_render_command_backend_pipeline_set_primitive_topology(dm_render_handle handle, dm_primitive_topology topology, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    D3D11_PRIMITIVE_TOPOLOGY new_topology = dm_toplogy_to_dx11_topology(topology);
    if(new_topology==D3D11_PRIMITIVE_UNDEFINED) return false;

    ID3D11DeviceContext_IASetPrimitiveTopology(dx11_renderer->context, new_topology);

    return true;
}

bool dm_render_command_backend_pipeline_toggle_wireframe(dm_render_handle handle, bool wireframe, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_PIPELINE)
    {
        DM_LOG_FATAL("Resource is not a pipeline");
        return false;
    }

    dm_dx11_pipeline* internal_pipe = &dx11_renderer->pipelines[index];

    if(wireframe) ID3D11DeviceContext_RSSetState(dx11_renderer->context, internal_pipe->rasterizer_state);
    else          ID3D11DeviceContext_RSSetState(dx11_renderer->context, internal_pipe->wireframe_state);

    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to bind a resource that is not a vertex buffer");
        return false;
    }

    dm_dx11_vertex_buffer* internal_buffer = &dx11_renderer->vertex_buffers[index];

    UINT offset = 0;
    uint32_t stride = (uint32_t)internal_buffer->stride;

    ID3D11DeviceContext_IASetVertexBuffers(dx11_renderer->context, slot, 1, &internal_buffer->buffer, &stride, &offset);

    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to bind a resource that is not an index buffer");
        return false;
    }

    dm_dx11_index_buffer* internal_buffer = &dx11_renderer->index_buffers[index];

    ID3D11DeviceContext_IASetIndexBuffer(dx11_renderer->context, internal_buffer->buffer, internal_buffer->format, slot);

    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a constant buffer");
        return false;
    }

    dm_dx11_constant_buffer* internal_buffer = &dx11_renderer->constant_buffers[index];

    if(internal_buffer->stage & DM_CONSTANT_BUFFER_STAGE_VERTEX) ID3D11DeviceContext_VSSetConstantBuffers(dx11_renderer->context, slot, 1, &internal_buffer->buffer);
    if(internal_buffer->stage & DM_CONSTANT_BUFFER_STAGE_PIXEL)  ID3D11DeviceContext_PSSetConstantBuffers(dx11_renderer->context, slot, 1, &internal_buffer->buffer);

    return true;
}

bool dm_render_command_backend_bind_structured_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_TEXTURE)
    {
        DM_LOG_FATAL("Trying to bind resource that is not a texture");
        return false;
    }

    dm_dx11_texture* internal_texture = &dx11_renderer->textures[index];

    ID3D11DeviceContext_PSSetShaderResources(dx11_renderer->context, slot, 1, &internal_texture->view);

    return true;
}

bool dm_dx11_update_buffer(ID3D11Buffer* buffer, const void* data, const size_t data_size, const size_t offset, ID3D11DeviceContext* context)
{
    HRESULT hr;

    D3D11_MAPPED_SUBRESOURCE msr = { 0 };

    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11DeviceContext_Map failed");
        return false;
    }

    dm_memcpy(msr.pData + offset, data, data_size);

    ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)buffer, 0);

    msr.pData = NULL;

    return true;
}

bool dm_render_command_backend_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to update resource that is not a vertex buffer");
        return false;
    }

    dm_dx11_vertex_buffer* internal_buffer = &dx11_renderer->vertex_buffers[index];

    if(!dm_dx11_update_buffer(internal_buffer->buffer, data, data_size, offset, dx11_renderer->context)) return false;

    return true;
}


bool dm_render_command_backend_update_index_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER)
    {
        DM_LOG_FATAL("Trying to update resource that is not a index buffer");
        return false;
    }

    dm_dx11_index_buffer* internal_buffer = &dx11_renderer->index_buffers[index];

    if(!dm_dx11_update_buffer(internal_buffer->buffer, data, data_size, offset, dx11_renderer->context)) return false;

    return true;
}

bool dm_render_command_backend_update_structured_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_STRUCTURED_READ_BUFFER || type!=DM_RENDER_RESOURCE_TYPE_STRUCTURED_WRITE_BUFFER)
    {
        DM_LOG_FATAL("Trying to update resource that is not a structured buffer");
        return false;
    }

    dm_dx11_structured_buffer* internal_buffer = &dx11_renderer->structured_buffers[index];

    if(!dm_dx11_update_buffer(internal_buffer->buffer, data, data_size, offset, dx11_renderer->context)) return false;

    return true;
}

bool dm_render_command_backend_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER)
    {
        DM_LOG_FATAL("Trying to update resource that is not a constant buffer");
        return false;
    }

    dm_dx11_constant_buffer* internal_buffer = &dx11_renderer->constant_buffers[index];

    if(!dm_dx11_update_buffer(internal_buffer->buffer, data, data_size, offset, dx11_renderer->context)) return false;

    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index               = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_TEXTURE)
    {
        DM_LOG_FATAL("Trying to update resource that is not a texture");
        return false;
    }

    dm_dx11_texture* internal_texture = &dx11_renderer->textures[index];

    // must resize
    if(internal_texture->width != width || internal_texture->height != height)
    {
        D3D11_TEXTURE2D_DESC texture_desc = { 0 };
        ID3D11Texture2D_GetDesc(internal_texture->texture, &texture_desc);
        
        ID3D11Texture2D_Release(internal_texture->texture);

        // new texture
        texture_desc.Width  = width;
        texture_desc.Height = height;

        D3D11_SUBRESOURCE_DATA init_data = { data, sizeof(uint32_t) * width, 0 };

        hr = ID3D11Device_CreateTexture2D(dx11_renderer->device, &texture_desc, &init_data, &internal_texture->texture);
        if(!dm_platform_win32_decode_hresult(hr)) 
        { 
            DM_LOG_FATAL("ID3D11Device_CreateTexture2D failed"); 
            return false; 
        }

        // shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                = texture_desc.Format;
        view_desc.ViewDimension         = D3D11_SRV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipLevels   = texture_desc.MipLevels;

        hr = ID3D11Device_CreateShaderResourceView(dx11_renderer->device, (ID3D11Resource*)internal_texture->texture, &view_desc, &internal_texture->view);
        if(!dm_platform_win32_decode_hresult(hr)) 
        { 
            DM_LOG_FATAL("ID3D11Device_CreateShaderResourceView failed!");
            return false; 
        }

        ID3D11DeviceContext_GenerateMips(dx11_renderer->context, internal_texture->view);

    }

    D3D11_MAPPED_SUBRESOURCE msr = { 0 };
    hr =  ID3D11DeviceContext_Map(dx11_renderer->context, (ID3D11Resource*)internal_texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11DeviceContext_Map failed");
        return false;
    }

    dm_memcpy(msr.pData, data, data_size);

    ID3D11DeviceContext_Unmap(dx11_renderer->context, (ID3D11Resource*)internal_texture->texture, 0);
    
    return true;
}

bool dm_render_command_backend_clear_texture(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    HRESULT hr;

    dm_render_resource_type type = DM_RENDER_HANDLE_GET_TYPE(handle);
    uint16_t index = DM_RENDER_HANDLE_GET_INDEX(handle);

    if(type!=DM_RENDER_RESOURCE_TYPE_TEXTURE)
    {
        DM_LOG_FATAL("Trying to clear resource that is not a texture");
        return false;
    }

    dm_dx11_texture* internal_texture = &dx11_renderer->textures[index];

    D3D11_TEXTURE2D_DESC texture_desc = { 0 };
    ID3D11Texture2D_GetDesc(internal_texture->texture, &texture_desc);

    D3D11_MAPPED_SUBRESOURCE msr = { 0 };
    hr = ID3D11DeviceContext_Map(dx11_renderer->context, (ID3D11Resource*)internal_texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        DM_LOG_FATAL("ID3D11DeviceContext_Map failed");
        return false;
    }

    ZeroMemory(msr.pData, sizeof(uint32_t) * texture_desc.Height * texture_desc.Width);

    ID3D11DeviceContext_Unmap(dx11_renderer->context, (ID3D11Resource*)internal_texture->texture, 0);

    return true;
}


void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;

    ID3D11DeviceContext_Draw(dx11_renderer->context, count, start);
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext_DrawIndexed(dx11_renderer->context, num_indices, index_offset, vertex_offset);
}

void dm_render_command_backend_draw_instanced(uint32_t index_count, uint32_t vertex_count, uint32_t inst_count, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext_DrawIndexedInstanced(dx11_renderer->context, index_count, inst_count, index_offset, vertex_offset, inst_offset);
}

#endif
