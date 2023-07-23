#ifndef DM_RENDERER_DX11_H
#define DM_RENDERER_DX11_H

#include <d3d11_1.h>
#include <dxgi.h>

typedef struct dm_dx11_buffer_t
{
	ID3D11Buffer* buffer;
    D3D11_BIND_FLAG type;
    size_t stride;
} dm_dx11_buffer;

typedef struct dm_dx11_shader_t
{
    ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader*  pixel_shader;
	ID3D11InputLayout*  input_layout;
} dm_dx11_shader;

typedef struct dm_dx11_texture_t
{
	ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* view;
    
    // staging texture
    ID3D11Texture2D* staging;
} dm_dx11_texture;

typedef struct dm_dx11_framebuffer_t
{
    ID3D11Texture2D*          render_target;
    ID3D11ShaderResourceView* shader_view;
    ID3D11RenderTargetView*   view;
    ID3D11DepthStencilState*  depth_stencil_buffer;
} dm_dx11_framebuffer;

typedef enum dm_dx11_pipeline_flag_t
{
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
    bool wireframe, blend, depth, stencil;
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

typedef struct dm_dx11_renderer
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
    
    dm_dx11_buffer      buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_shader      shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_texture     textures[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_framebuffer framebuffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_pipeline    pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
    
    uint32_t buffer_count, shader_count, texture_count, framebuffer_count, pipeline_count;
    
    uint32_t active_pipeline, active_shader;
    
#ifdef DM_DEBUG
    ID3D11Debug* debugger;
#endif
} dm_dx11_renderer;

#ifdef DM_DEBUG
bool dm_dx11_print_errors(dm_dx11_renderer* dx11_renderer);
const char* dm_dx11_decode_category(D3D11_MESSAGE_CATEGORY category);
const char* dm_dx11_decode_severity(D3D11_MESSAGE_SEVERITY severity);
#endif

#define DM_DX11_RELEASE(OBJ) OBJ->lpVtbl->Release(OBJ)

#define DM_DX11_GET_RENDERER dm_dx11_renderer* dx11_renderer = renderer->internal_renderer

#define DM_DX11_INVALID_RESOURCE UINT_MAX

/*********************
 DX11 ENUM CONVERSIONS
***********************/
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

D3D11_BIND_FLAG dm_buffer_type_to_dx11(dm_buffer_type type)
{
	switch (type)
	{
        case DM_BUFFER_TYPE_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
        case DM_BUFFER_TYPE_INDEX:  return D3D11_BIND_INDEX_BUFFER;
        //case DM_BUFFER_TYPE_CONSTANT: return D3D11_BIND_CONSTANT_BUFFER;
        
        default:
		DM_LOG_FATAL("Unknown buffer type!");
		return D3D11_BIND_VIDEO_ENCODER+1;
	}
}

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

DXGI_FORMAT dm_vertex_t_to_dx11_format(dm_vertex_attrib_desc desc)
{
	switch (desc.data_t)
	{
        case DM_VERTEX_DATA_T_BYTE:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R8_SINT;
                case 2: return DXGI_FORMAT_R8G8_SINT;
                case 4: return DXGI_FORMAT_R8G8B8A8_SINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_UBYTE:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R8_UINT;
                case 2: return DXGI_FORMAT_R8G8_UINT;
                case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_SHORT:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R16_SINT;
                case 2: return DXGI_FORMAT_R16G16_SINT;
                case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_USHORT:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R16_UINT;
                case 2: return DXGI_FORMAT_R16G16_UINT;
                case 4: return DXGI_FORMAT_R16G16B16A16_UINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_INT:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R32_SINT;
                case 2: return DXGI_FORMAT_R32G32_SINT;
                case 3: return DXGI_FORMAT_R32G32B32_SINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_UINT:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R32_UINT;
                case 2: return DXGI_FORMAT_R32G32_UINT;
                case 3: return DXGI_FORMAT_R32G32B32_UINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
                default:
                break;
            }
        } break;
        case DM_VERTEX_DATA_T_DOUBLE:
        case DM_VERTEX_DATA_T_FLOAT:
        {
            switch (desc.count)
            {
                case 1: return DXGI_FORMAT_R32_FLOAT;
                case 2: return DXGI_FORMAT_R32G32_FLOAT;
                case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
                case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                default:
                break;
            }
        }
        
        default:
		break;
	}
    
	DM_LOG_FATAL("Unknown vertex format type!");
	return DXGI_FORMAT_UNKNOWN;
}

D3D11_INPUT_CLASSIFICATION dm_vertex_class_to_dx11_class(dm_vertex_attrib_class dm_class)
{
	switch (dm_class)
	{
        case DM_VERTEX_ATTRIB_CLASS_VERTEX:   return D3D11_INPUT_PER_VERTEX_DATA;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE: return D3D11_INPUT_PER_INSTANCE_DATA;
        
        default:
		DM_LOG_FATAL("Unknown vertex attribute input class!");
		return DM_VERTEX_ATTRIB_CLASS_UNKNOWN;
	}
}

D3D11_CULL_MODE dm_cull_to_dx11_cull(dm_cull_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_CULL_FRONT_BACK:
        case DM_CULL_FRONT:      return D3D11_CULL_FRONT;
        case DM_CULL_BACK:       return D3D11_CULL_BACK;
        
        default:
		DM_LOG_FATAL("Unknown cull mode!");
		return D3D11_CULL_NONE;
	}
}

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

dm_comp_to_directx_comp(dm_comparison dm_comp)
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

/*
DXGI_FORMAT dm_image_fmt_to_directx_fmt(dm_texture_format dm_fmt)
{
	switch (dm_fmt)
	{
        case DM_TEXTURE_FORMAT_RGB: 
        case DM_TEXTURE_FORMAT_RGBA: return DXGI_FORMAT_R8G8B8A8_UNORM;
        
        default:
		DM_LOG_FATAL("Unknown texture format!");
		return DXGI_FORMAT_UNKNOWN;
	}
}
*/

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

/****************
DIRECTX11_BUFFER
******************/
bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
	HRESULT hr;
    
	dm_dx11_buffer internal_buffer = {0};
    
	ID3D11Device* device = dx11_renderer->device;
	ID3D11DeviceContext* context = dx11_renderer->context;
    
	D3D11_USAGE usage = dm_buffer_usage_to_dx11(desc.usage);
	if (usage == D3D11_USAGE_STAGING + 1) return false;
	D3D11_BIND_FLAG type = dm_buffer_type_to_dx11(desc.type);
	if (type == D3D11_BIND_VIDEO_ENCODER + 1) return false;
	D3D11_CPU_ACCESS_FLAG cpu_access = dm_buffer_cpu_access_to_dx11(desc.cpu_access);
	if (cpu_access == D3D11_CPU_ACCESS_READ + 1) return false;
    
	D3D11_BUFFER_DESC buffer_desc = { 0 };
	buffer_desc.Usage = usage;
	buffer_desc.BindFlags = type;
	buffer_desc.ByteWidth = desc.buffer_size;
	buffer_desc.StructureByteStride = desc.elem_size;
	if (usage == D3D11_USAGE_DYNAMIC) buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
	if (data) 
    {
        D3D11_SUBRESOURCE_DATA sd = { 0 };
        sd.pSysMem = data;
        
        hr = device->lpVtbl->CreateBuffer(device, &buffer_desc, &sd, &internal_buffer.buffer);
    }
	else
    {
        hr = device->lpVtbl->CreateBuffer(device, &buffer_desc, 0, &internal_buffer.buffer);
    }
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateBuffer failed"); return false; }
    
    internal_buffer.type = type;
    internal_buffer.stride = desc.elem_size;
    
    dm_memcpy(dx11_renderer->buffers + dx11_renderer->buffer_count, &internal_buffer, sizeof(dm_dx11_buffer));
    *handle = dx11_renderer->buffer_count++;
    
	return true;
}

void dm_renderer_backend_destroy_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->buffer_count) { DM_LOG_ERROR("Trying to destroy invalid Directx11 buffer"); return; }
    DM_DX11_RELEASE(dx11_renderer->buffers[handle].buffer);
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    HRESULT hr;
    
	ID3D11Device* device = dx11_renderer->device;
	ID3D11DeviceContext* context = dx11_renderer->context;
    
    D3D11_BUFFER_DESC uni_desc = { 0 };
    uni_desc.Usage = D3D11_USAGE_DYNAMIC;
    uni_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uni_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    uni_desc.ByteWidth = ((size + 15) / 16) * 16;
    uni_desc.StructureByteStride = sizeof(float);
    
    dm_dx11_buffer internal_uniform = { 0 };
    hr = device->lpVtbl->CreateBuffer(device, &uni_desc, 0, &internal_uniform.buffer);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateBuffer failed!"); return false; }
    
    dm_memcpy(dx11_renderer->buffers + dx11_renderer->buffer_count, &internal_uniform, sizeof(dm_dx11_buffer));
    *handle = dx11_renderer->buffer_count++;
    
    return true;
}

void dm_renderer_backend_destroy_uniform(dm_render_handle handle, dm_renderer* renderer)
{
    dm_renderer_backend_destroy_buffer(handle, renderer);
}

/*****************
DIRECTX11_TEXTURE
*******************/
bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_DX11_GET_RENDERER;
    
    HRESULT hr;
    
	dm_dx11_texture internal_texture = {0};
    
    // texture
	D3D11_TEXTURE2D_DESC tex_desc = { 0 };
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.ArraySize = 1;
	tex_desc.SampleDesc.Count = 1;
    tex_desc.MipLevels = 1;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
	hr = dx11_renderer->device->lpVtbl->CreateTexture2D(dx11_renderer->device, &tex_desc, NULL, &internal_texture.texture);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateTexture2D failed!"); return false; }
    
	dx11_renderer->context->lpVtbl->UpdateSubresource(dx11_renderer->context, (ID3D11Resource*)internal_texture.texture, 0, NULL, data, width * 4, 0);
    
    // shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
	view_desc.Format = tex_desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//view_desc.Texture2D.MipLevels = -1;
    view_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    
	hr = dx11_renderer->device->lpVtbl->CreateShaderResourceView(dx11_renderer->device, (ID3D11Resource*)internal_texture.texture, &view_desc, &internal_texture.view);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateShaderResourceView failed!"); return false; }
    
	dx11_renderer->context->lpVtbl->GenerateMips(dx11_renderer->context, internal_texture.view);
    
    // staging texture
    tex_desc.BindFlags = 0;
    tex_desc.Usage = D3D11_USAGE_STAGING;
    tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    tex_desc.MiscFlags = 0;
    
    hr = dx11_renderer->device->lpVtbl->CreateTexture2D(dx11_renderer->device, &tex_desc, NULL, &internal_texture.staging);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateTexture2D failed!"); return false; }
    
    /*
        D3D11_MAPPED_SUBRESOURCE msr;
        ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));
        
        DX11_ERROR_CHECK(dx11_renderer.context->lpVtbl->Map(dx11_renderer.context, (ID3D11Resource*)internal_texture.staging, 0, D3D11_MAP_WRITE, 0, &msr), "ID3D11DeviceContext::Map failed!");
        dm_memcpy(msr.pData, data, width * height * sizeof(uint32_t));
        dx11_renderer.context->lpVtbl->Unmap(dx11_renderer.context, (ID3D11Resource*)internal_texture.staging, 0);
        
        dx11_renderer.context->lpVtbl->CopyResource(dx11_renderer.context, (ID3D11Resource*)internal_texture.texture, (ID3D11Resource*)internal_texture.staging);
        */
    
    dm_memcpy(dx11_renderer->textures + dx11_renderer->texture_count, &internal_texture, sizeof(dm_dx11_texture));
    *handle = dx11_renderer->texture_count++;
    
	return true;
}

void dm_renderer_backend_destroy_texture(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->texture_count) { DM_LOG_FATAL("Trying to destroy invalid DirectX11 texture"); return; }
    
	DM_DX11_RELEASE(dx11_renderer->textures[handle].texture);
	DM_DX11_RELEASE(dx11_renderer->textures[handle].view);
    DM_DX11_RELEASE(dx11_renderer->textures[handle].staging);
}

/******************
DIRECTX11_PIPELINE
********************/
bool dm_renderer_backend_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    HRESULT hr;
    
	ID3D11Device* device = dx11_renderer->device;
	ID3D11DeviceContext* context = dx11_renderer->context;
	
    dm_dx11_pipeline internal_pipeline = { 0 };
    
	// blending 
	if (desc.blend)
	{
        internal_pipeline.blend = true;
        
        D3D11_BLEND_DESC blend_desc = { 0 };
        blend_desc.RenderTarget[0].BlendEnable = true;
        blend_desc.RenderTarget[0].SrcBlend = dm_blend_func_to_dx11_func(desc.blend_src_f);
        blend_desc.RenderTarget[0].DestBlend = dm_blend_func_to_dx11_func(desc.blend_dest_f);
        blend_desc.RenderTarget[0].BlendOp = dm_blend_eq_to_dx11_op(desc.blend_eq);
        blend_desc.RenderTarget[0].SrcBlendAlpha = dm_blend_func_to_dx11_func(desc.blend_src_f);
        blend_desc.RenderTarget[0].DestBlendAlpha = dm_blend_func_to_dx11_func(desc.blend_dest_f);
        blend_desc.RenderTarget[0].BlendOpAlpha = dm_blend_eq_to_dx11_op(desc.blend_eq);
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        hr = device->lpVtbl->CreateBlendState(device, &blend_desc, &internal_pipeline.blend_state);
        if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateBlendState failed!"); return false; }
	}
	
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = { 0 };
    
    // depth testing
    if (desc.depth)
	{
        internal_pipeline.depth = true;
        
        depth_stencil_desc.DepthEnable = desc.depth;
        depth_stencil_desc.StencilEnable = desc.stencil;
        
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
	}
    
    // stencil testing
    if(desc.stencil)
    {
        internal_pipeline.stencil = true;
    }
    
	if(desc.depth || desc.stencil) 
    {
        hr = device->lpVtbl->CreateDepthStencilState(device, &depth_stencil_desc, &internal_pipeline.depth_stencil_state);
        if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateDepthStencilState failed!"); return false; }
    }
    
    // rasterizer
    D3D11_RASTERIZER_DESC rd = { 0 };
    D3D11_RASTERIZER_DESC wireframe_rd = { 0 };
    
    rd.FillMode = D3D11_FILL_SOLID;
    wireframe_rd.FillMode = D3D11_FILL_WIREFRAME;
    
    // culling
    rd.CullMode = dm_cull_to_dx11_cull(desc.cull_mode);
    wireframe_rd.CullMode = dm_cull_to_dx11_cull(desc.cull_mode);
    
    rd.DepthClipEnable = true;
    wireframe_rd.DepthClipEnable = true;
    
    internal_pipeline.wireframe = desc.wireframe;
    
    // winding
    switch (desc.winding_order)
    {
        case DM_WINDING_CLOCK:
        {
            rd.FrontCounterClockwise = false;
            wireframe_rd.FrontCounterClockwise = false;
        } break;
        case DM_WINDING_COUNTER_CLOCK:
        {
            rd.FrontCounterClockwise = true;
            wireframe_rd.FrontCounterClockwise = true;
        } break;
        default:
        DM_LOG_FATAL("Unknown winding order!");
        return false;
    }
    
    hr = device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pipeline.rasterizer_state);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateRasterizerState failed!"); return false; }
    hr = device->lpVtbl->CreateRasterizerState(device, &wireframe_rd, &internal_pipeline.wireframe_state);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateRasterizerState failed!"); return false; }
    
    // topology
    internal_pipeline.default_topology = dm_toplogy_to_dx11_topology(desc.primitive_topology);
    
    // sampler state
    D3D11_FILTER filter = dm_image_filter_to_dx11_filter(desc.sampler_filter);
    D3D11_TEXTURE_ADDRESS_MODE u_mode = dm_texture_mode_to_dx11_mode(desc.u_mode);
    D3D11_TEXTURE_ADDRESS_MODE v_mode = dm_texture_mode_to_dx11_mode(desc.v_mode);
    D3D11_TEXTURE_ADDRESS_MODE w_mode = dm_texture_mode_to_dx11_mode(desc.w_mode);
    D3D11_COMPARISON_FUNC comp = dm_comp_to_directx_comp(desc.sampler_comp);
    
    D3D11_SAMPLER_DESC sample_desc = { 0 };
    sample_desc.Filter = filter;
    sample_desc.AddressU = u_mode;
    sample_desc.AddressV = v_mode;
    sample_desc.AddressW = w_mode;
    sample_desc.MaxAnisotropy = 1;
    sample_desc.ComparisonFunc = comp;
    if (filter != D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT) sample_desc.MaxLOD = D3D11_FLOAT32_MAX;
    
    hr = device->lpVtbl->CreateSamplerState(device, &sample_desc, &internal_pipeline.sample_state);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateSamplerState failed!"); return false; }
    
    dm_memcpy(dx11_renderer->pipelines + dx11_renderer->pipeline_count, &internal_pipeline, sizeof(dm_dx11_pipeline));
    *handle = dx11_renderer->pipeline_count++;
    
    return true;
}

void dm_renderer_backend_destroy_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->pipeline_count) { DM_LOG_FATAL("Trying to destroy invalid DirectX11 pipeline"); return; }
    
    dm_dx11_pipeline* internal_pipeline = &dx11_renderer->pipelines[handle];
    
    if(internal_pipeline->depth || internal_pipeline->stencil) DM_DX11_RELEASE(internal_pipeline->depth_stencil_state);
    if(internal_pipeline->blend) DM_DX11_RELEASE(internal_pipeline->blend_state);
    DM_DX11_RELEASE(internal_pipeline->sample_state);
    DM_DX11_RELEASE(internal_pipeline->rasterizer_state);
    DM_DX11_RELEASE(internal_pipeline->wireframe_state);
}

/****************
DIRECTX11_SHADER
******************/
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

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    HRESULT hr;
    
    ID3D11Device* device = dx11_renderer->device;
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    dm_dx11_shader internal_shader = { 0 };
    
    // vertex shader
    wchar_t ws[100];
    swprintf(ws, 100, L"%hs", shader_desc.vertex);
    
    ID3DBlob* blob;
    hr = D3DReadFileToBlob(ws, &blob); 
    if(hr!=S_OK) { DM_LOG_FATAL("D3DReadFileToBlob failed!"); return false; }
    
    hr = device->lpVtbl->CreateVertexShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader.vertex_shader);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateVertexShader failed!"); return false; }
    
    // input layout
    D3D11_INPUT_ELEMENT_DESC* desc = dm_alloc(num_attribs * 4 * sizeof(D3D11_INPUT_ELEMENT_DESC));
    uint32_t count = 0;
    
    for(uint32_t i=0; i<num_attribs; i++)
    {
        dm_vertex_attrib_desc attrib_desc = attrib_descs[i];
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
                
                desc[count++] = element_desc;
            }
        }
        else
        {
            D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
            if (!dm_dx11_create_input_element(attrib_desc, &element_desc)) return false;
            
            // append the element_desc to the array
            desc[count++] = element_desc;
        }	
    }
    
    hr = device->lpVtbl->CreateInputLayout(device, desc, (UINT)count, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), &internal_shader.input_layout);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateInputLayout failed!"); return false; }
    
    dm_free(desc);
    
    // pixel shader
    swprintf(ws, 100, L"%hs", shader_desc.pixel);
    
    hr = D3DReadFileToBlob(ws, &blob);
    if(hr!=S_OK) { DM_LOG_FATAL("D3DReadFileToBlob failed!"); return false; }
    
    hr = device->lpVtbl->CreatePixelShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader.pixel_shader);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreatePixelShader failed!"); return false; }
    
    DM_DX11_RELEASE(blob);
    
    dm_memcpy(dx11_renderer->shaders + dx11_renderer->shader_count, &internal_shader, sizeof(dm_dx11_shader));
    *shader_handle = dx11_renderer->shader_count++;
    
    if(!dm_renderer_backend_create_pipeline(pipe_desc, pipe_handle, renderer)) return false;
    
    return true;
}

void dm_renderer_backend_destroy_shader(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->shader_count) { DM_LOG_FATAL("Trying to destroy invalid DirectX shader"); return; }
    
    DM_DX11_RELEASE(dx11_renderer->shaders[handle].vertex_shader);
    DM_DX11_RELEASE(dx11_renderer->shaders[handle].pixel_shader);
    DM_DX11_RELEASE(dx11_renderer->shaders[handle].input_layout);
}

/******
DEVICE
********/
#if DM_DEBUG
void dm_dx11_device_report_live_objects(dm_dx11_renderer* dx11_renderer)
{
    HRESULT hr;
    ID3D11Debug* debugger = dx11_renderer->debugger;
    
    hr = debugger->lpVtbl->ReportLiveDeviceObjects(debugger, D3D11_RLDO_DETAIL);
    if (hr != S_OK) DM_LOG_ERROR("ID3D11Debug::ReportLiveDeviceObjects failed!");
}
#endif

bool dm_dx11_create_device(dm_dx11_renderer* dx11_renderer)
{
    UINT flags = 0;
#if DM_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL feature_level;
    
    HRESULT hr;
    
    // create the device and immediate context
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION,
                           &dx11_renderer->device, &feature_level, &dx11_renderer->context);
    if(hr!=S_OK) { DM_LOG_FATAL("D3D11CreateDevice failed!"); return false; }
    if(feature_level != D3D_FEATURE_LEVEL_11_0) { DM_LOG_FATAL("Direct3D Feature Level 11 unsupported!"); return false; }
    
    UINT msaa_quality;
    hr = dx11_renderer->device->lpVtbl->CheckMultisampleQualityLevels(dx11_renderer->device, DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_quality);
    if(hr!=S_OK) { DM_LOG_FATAL("D3D11Device::CheckMultisampleQualityLevels failed!"); return false; }
    
    // if in debug, create the debugger to query live objects
#if DM_DEBUG
    hr = dx11_renderer->device->lpVtbl->QueryInterface(dx11_renderer->device, &IID_ID3D11Debug, (void**)&(dx11_renderer->debugger));
    if(hr!=S_OK) { DM_LOG_FATAL("D3D11Device::QueryInterface failed!"); return false; }
#endif
    
    return true;
}

void dm_dx11_destroy_device(dm_dx11_renderer* dx11_renderer)
{
    ID3D11Device* device = dx11_renderer->device;
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    DM_DX11_RELEASE(context);
#if DM_DEBUG
    dm_dx11_device_report_live_objects(dx11_renderer);
    DM_DX11_RELEASE(dx11_renderer->debugger);
#endif
    
    DM_DX11_RELEASE(device);
}

/*********
SWAPCHAIN
***********/
bool dm_dx11_create_swapchain(dm_dx11_renderer* dx11_renderer)
{
    // set up the swap chain pointer to be created in this function
    IDXGISwapChain* swap_chain = NULL;
    
    // make sure the device has been created and then grab it
    if(!dx11_renderer->device) { DM_LOG_FATAL("DirectX device is NULL!"); return false; }
    ID3D11Device* device = dx11_renderer->device;
    
    HRESULT hr;
    RECT client_rect;
    GetClientRect(dx11_renderer->hwnd, &client_rect);
    
    struct DXGI_SWAP_CHAIN_DESC desc = { 0 };
    desc.BufferDesc.Width = client_rect.right;
    desc.BufferDesc.Height = client_rect.bottom;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = dx11_renderer->hwnd;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    //desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    // obtain factory
    IDXGIDevice* dxgi_device = NULL;
    hr = device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, &dxgi_device);
    if(hr!=S_OK) { DM_LOG_FATAL("D3D11Device::QueryInterface failed!"); return false; }
    
    IDXGIAdapter* dxgi_adapter = NULL;
    hr = dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_IDXGIAdapter, (void**)&dxgi_adapter);
    if(hr!=S_OK) { DM_LOG_FATAL("IDXGIDevice::GetParent failed!"); return false; }
    
    IDXGIFactory* dxgi_factory = NULL;
    hr = dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory, (void**)&dxgi_factory);
    if(hr!=S_OK) { DM_LOG_FATAL("IDXGIAdapter::GetParent failed!"); return false; }
    
    // create the swap chain
    hr = dxgi_factory->lpVtbl->CreateSwapChain(dxgi_factory, (IUnknown*)device, &desc, &dx11_renderer->swap_chain);
    if(hr!=S_OK) { DM_LOG_FATAL("IDXGIFactory::CreateSwapChain failed!"); return false; }
    
    // release pack animal directx objects
    DM_DX11_RELEASE(dxgi_device);
    DM_DX11_RELEASE(dxgi_factory);
    DM_DX11_RELEASE(dxgi_adapter);
    
    return true;
}

/************
DEPTHSTENCIL
**************/
bool dm_dx11_create_depth_stencil(dm_dx11_renderer* dx11_renderer)
{
    if(!dx11_renderer->device) { DM_LOG_FATAL("DirectX device is NULL!"); return false; }
    
    HRESULT hr;
    RECT client_rect;
    GetClientRect(dx11_renderer->hwnd, &client_rect);
    
    ID3D11Device* device = dx11_renderer->device;
    
    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = client_rect.right;
    desc.Height = client_rect.bottom;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    
    hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &dx11_renderer->depth_stencil_back_buffer);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateTexture2D failed!"); return false; }
    hr = device->lpVtbl->CreateDepthStencilView(device, (ID3D11Resource*)dx11_renderer->depth_stencil_back_buffer, 0, &dx11_renderer->depth_stencil_view);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11Device::CreateDepthStencilView failed!"); return false; }
    
    return true;
}

/**********************
DIRECTX11 RENDERTARGET
************************/
bool dm_dx11_create_rendertarget(dm_dx11_renderer* dx11_renderer)
{
    if(!dx11_renderer->device)     { DM_LOG_FATAL("DirectX11 device is NULL!"); return false; }
    if(!dx11_renderer->swap_chain) { DM_LOG_FATAL("DirectX11 swap chain is NULL!"); return false; }
    
    HRESULT hr;
    ID3D11Device* device = dx11_renderer->device;
    IDXGISwapChain* swap_chain = dx11_renderer->swap_chain;
    
    hr = swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)&(ID3D11Resource*)dx11_renderer->render_back_buffer);
    if(hr!=S_OK) { DM_LOG_FATAL("IDXGISwapChain::GetBuffer failed!"); return false; }
    device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)dx11_renderer->render_back_buffer, NULL, &dx11_renderer->render_view);
    return true;
}

/*************************
DIRECTX11_UPDATE_RESOURCE
***************************/
bool dm_dx11_update_resource(void* resource, void* data, size_t data_size, dm_dx11_renderer* dx11_renderer)
{
    HRESULT hr;
    
    D3D11_MAPPED_SUBRESOURCE msr;
    ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));
    
    hr = dx11_renderer->context->lpVtbl->Map(dx11_renderer->context, (ID3D11Resource*)resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if(hr!=S_OK) { DM_LOG_FATAL("ID3D11DeviceContext::Map failed!"); return false; }
    
    dm_memcpy(msr.pData, data, data_size);
    dx11_renderer->context->lpVtbl->Unmap(dx11_renderer->context, (ID3D11Resource*)resource, 0);
    
    return true;
}

/*******
BACKEND
*********/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Directx11 Backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_dx11_renderer));
    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;
    
    dm_internal_w32_data* w32_data = context->platform_data.internal_data;
    
    dx11_renderer->hwnd = w32_data->hwnd;
    dx11_renderer->h_instance = w32_data->h_instance;
    
    if (!dm_dx11_create_device(dx11_renderer)) return false;
    if (!dm_dx11_create_swapchain(dx11_renderer)) return false;
    
    if (!dm_dx11_create_rendertarget(dx11_renderer)) return false;
    if (!dm_dx11_create_depth_stencil(dx11_renderer)) return false;
    
    dx11_renderer->active_pipeline = DM_DX11_INVALID_RESOURCE;
    dx11_renderer->active_shader   = DM_DX11_INVALID_RESOURCE;
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;
    
    // buffers
    for(uint32_t i=0; i<dx11_renderer->buffer_count; i++)
    {
        dm_renderer_backend_destroy_buffer(i, &context->renderer);
    }
    
    // shaders
    for(uint32_t i=0; i<dx11_renderer->shader_count; i++)
    {
        dm_renderer_backend_destroy_shader(i, &context->renderer);
    }
    
    // textures
    for(uint32_t i=0; i<dx11_renderer->texture_count; i++)
    {
        dm_renderer_backend_destroy_texture(i, &context->renderer);
    }
    
    /*
    // framebuffers
    for(uint32_t i=0; i<dx11_renderer->framebuffer_count; i++)
    {
        dm_renderer_backend_destroy_framebuffer(i, &context->renderer);
    }
    */
    
    // pipelines
    for(uint32_t i=0; i<dx11_renderer->pipeline_count; i++)
    {
        dm_renderer_backend_destroy_pipeline(i, &context->renderer);
    }
    
    DM_DX11_RELEASE(dx11_renderer->render_view);
    DM_DX11_RELEASE(dx11_renderer->render_back_buffer);
    DM_DX11_RELEASE(dx11_renderer->swap_chain);
    DM_DX11_RELEASE(dx11_renderer->depth_stencil_view);
    DM_DX11_RELEASE(dx11_renderer->depth_stencil_back_buffer);
    DM_DX11_RELEASE(dx11_renderer->context);
#ifdef DM_DEBUG
    dm_dx11_device_report_live_objects(dx11_renderer);
    DM_DX11_RELEASE(dx11_renderer->debugger);
#endif
    DM_DX11_RELEASE(dx11_renderer->device);
    
    dm_free(context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context =          dx11_renderer->context;
    ID3D11RenderTargetView* render_target = dx11_renderer->render_view;
    ID3D11DepthStencilView* depth_stencil = dx11_renderer->depth_stencil_view;
    
    // render target
    context->lpVtbl->OMSetRenderTargets(context, 1u, &render_target, depth_stencil);
    
    return true;
}

bool dm_renderer_backend_end_frame(bool vsync, dm_context* context)
{
    dm_dx11_renderer* dx11_renderer = context->renderer.internal_renderer;
    
    HRESULT hr;
    
    IDXGISwapChain* swap_chain = dx11_renderer->swap_chain;
    
    uint32_t v = vsync ? 1 : 0;
    if (FAILED(hr = swap_chain->lpVtbl->Present(swap_chain, v, 0)))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) DM_LOG_FATAL("DirectX Device removed! Exiting...");
        else DM_LOG_ERROR("Something bad happened when presenting buffers. Exiting...");
        return false;
    }
    
    dx11_renderer->active_pipeline = DM_DX11_INVALID_RESOURCE;
    dx11_renderer->active_shader   = DM_DX11_INVALID_RESOURCE;
    
#ifdef DM_DEBUG
    dm_dx11_print_errors(dx11_renderer);
#endif
    
    return true;
}

/********
COMMANDS
**********/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    float c[] = { r,g,b,a };
    ID3D11DeviceContext* context = dx11_renderer->context;
    ID3D11RenderTargetView* render_target = dx11_renderer->render_view;
    ID3D11DepthStencilView* depth_stencil = dx11_renderer->depth_stencil_view;
    
    // clear framebuffer
    context->lpVtbl->ClearRenderTargetView(context, render_target, c);
    context->lpVtbl->ClearDepthStencilView(context, depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    D3D11_VIEWPORT new_viewport = { 0 };
    new_viewport.Width = (FLOAT)width;
    new_viewport.Height = (FLOAT)height;
    new_viewport.MaxDepth = 1.0f;
    
    context->lpVtbl->RSSetViewports(context, 1, &new_viewport);
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->pipeline_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 pipeline"); return false; }
    
    dm_dx11_pipeline internal_pipeline = dx11_renderer->pipelines[handle];
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    // raster state
    if(!internal_pipeline.wireframe) context->lpVtbl->RSSetState(context, internal_pipeline.rasterizer_state);
    else context->lpVtbl->RSSetState(context, internal_pipeline.wireframe_state);
    context->lpVtbl->IASetPrimitiveTopology(context, internal_pipeline.default_topology);
    
    // sampler
    context->lpVtbl->PSSetSamplers(context, 0, 1, &internal_pipeline.sample_state);
    
    // blend state
    if(internal_pipeline.blend)
    {
        static FLOAT blends[] = {0,0,0,0};
        context->lpVtbl->OMSetBlendState(context, internal_pipeline.blend_state, blends, 0xffffffff);
    }
    
    // depth stencil state
    if(internal_pipeline.depth || internal_pipeline.stencil) 
    {
        context->lpVtbl->OMSetDepthStencilState(context, internal_pipeline.depth_stencil_state, 1);
    }
    
    dx11_renderer->active_pipeline = handle;
    
    return true;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    D3D11_PRIMITIVE_TOPOLOGY new_topology = dm_toplogy_to_dx11_topology(topology);
    if(new_topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) return false;
    
    context->lpVtbl->IASetPrimitiveTopology(context, new_topology);
    
    return true;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->shader_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 shader"); return false; }
    
    dm_dx11_shader internal_shader = dx11_renderer->shaders[handle];
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    context->lpVtbl->VSSetShader(context, internal_shader.vertex_shader, NULL, 0);
    context->lpVtbl->PSSetShader(context, internal_shader.pixel_shader, NULL, 0);
    context->lpVtbl->IASetInputLayout(context, internal_shader.input_layout);
    
    return true;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 buffer"); return false; }
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    dm_dx11_buffer internal_buffer = dx11_renderer->buffers[handle];
    
    UINT offset = 0;
    uint32_t new_stride = (uint32_t)internal_buffer.stride;
    
    switch(internal_buffer.type)
    {
        case D3D11_BIND_VERTEX_BUFFER:
        {
            context->lpVtbl->IASetVertexBuffers(context, slot, 1, &internal_buffer.buffer, &new_stride, &offset);
        } break;
        case D3D11_BIND_INDEX_BUFFER:
        {
            context->lpVtbl->IASetIndexBuffer(context, internal_buffer.buffer, DXGI_FORMAT_R32_UINT, 0);
        } break;
        default:
        DM_LOG_FATAL("Unknown DirectX11 buffer! Shouldn't be here...");
        return false;
    }
    
    return true;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid Directx11 buffer"); return false; }
    
    return dm_dx11_update_resource(dx11_renderer->buffers[handle].buffer, data, data_size, dx11_renderer);
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid DirectX11 buffer"); return false; }
    
    dm_dx11_buffer internal_buffer = dx11_renderer->buffers[handle];
    
    if(stage!=DM_UNIFORM_STAGE_PIXEL) dx11_renderer->context->lpVtbl->VSSetConstantBuffers(dx11_renderer->context, slot, 1, &internal_buffer.buffer);
    if(stage!=DM_UNIFORM_STAGE_VERTEX) dx11_renderer->context->lpVtbl->PSSetConstantBuffers(dx11_renderer->context, slot, 1, &internal_buffer.buffer);
    
    return true;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid Directx11 buffer"); return false;}
    
    dm_dx11_buffer internal_uniform = dx11_renderer->buffers[handle];
    
    return dm_dx11_update_resource(internal_uniform.buffer, data, data_size, dx11_renderer);
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->texture_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 texture"); return false; }
    
    dx11_renderer->context->lpVtbl->PSSetShaderResources(dx11_renderer->context, slot, 1, &dx11_renderer->textures[handle].view);
    
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    /*
    dm_dx11_texture* internal_texture = DM_DX11_GET_RESOURCE(DM_DX11_RESOURCE_TEXTURE, internal_index);
    if(!internal_texture) { DM_LOG_FATAL("Trying to update invalid DirectX11 texture"); return false; }
    
    HRESULT hr;
    
    D3D11_MAPPED_SUBRESOURCE msr;
    ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));
    
    DX11_ERROR_CHECK(dx11_renderer.context->lpVtbl->Map(dx11_renderer.context, (ID3D11Resource*)internal_texture->staging, 0, D3D11_MAP_WRITE, 0, &msr), "ID3D11DeviceContext::Map failed!");
    dm_memcpy(msr.pData, data, data_size);
    dx11_renderer.context->lpVtbl->Unmap(dx11_renderer.context, (ID3D11Resource*)internal_texture->staging, 0);
    
    dx11_renderer.context->lpVtbl->CopyResource(dx11_renderer.context, (ID3D11Resource*)internal_texture->texture, (ID3D11Resource*)internal_texture->staging);
    */
    DM_LOG_ERROR("Not supported: DirectX11 Update Texture");
    return true;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    ID3D11RenderTargetView* render_target = dx11_renderer->render_view;
    ID3D11DepthStencilView* depth_stencil = dx11_renderer->depth_stencil_view;
    
    if(!context) { DM_LOG_FATAL("DX11 context is NULL"); return false; }
    if(!render_target) { DM_LOG_FATAL("Default render target is NULL"); return false; }
    if(!depth_stencil) { DM_LOG_FATAL("Default depth stencil is NULL"); return false; }
    
    context->lpVtbl->OMSetRenderTargets(context, 1u, &render_target, depth_stencil);
    
    return true;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    if(handle > dx11_renderer->framebuffer_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 framebuffer"); return false; }
    
    dm_dx11_framebuffer internal_framebuffer = dx11_renderer->framebuffers[handle];
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    ID3D11DepthStencilView* depth_stencil = dx11_renderer->depth_stencil_view;
    
    if(!context) { DM_LOG_FATAL("DX11 context is NULL"); return false; }
    if(!depth_stencil) { DM_LOG_FATAL("Default depth stencil is NULL"); return false; }
    
    float c[] = { 0,0,0,1 };
    
    context->lpVtbl->OMSetRenderTargets(context, 1u, &internal_framebuffer.view, depth_stencil);
    context->lpVtbl->ClearRenderTargetView(context, internal_framebuffer.view, c);
    
    return true;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    if(!context) { DM_LOG_FATAL("DX11 context is NULL"); return false; }
    
    if(handle > dx11_renderer->framebuffer_count) { DM_LOG_FATAL("Trying to bind invalid Directx11 framebuffer"); return false; }
    
    dm_dx11_framebuffer internal_framebuffer = dx11_renderer->framebuffers[handle];
    
    if(!internal_framebuffer.shader_view) { DM_LOG_FATAL("DirectX11 framebuffer has invalid shader view"); return false; }
    
    context->lpVtbl->PSSetShaderResources(context, slot, 1, &internal_framebuffer.shader_view);
    
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    dx11_renderer->context->lpVtbl->Draw(dx11_renderer->context, count, start);
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    dx11_renderer->context->lpVtbl->DrawIndexed(dx11_renderer->context, num_indices, index_offset, vertex_offset);
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    dx11_renderer->context->lpVtbl->DrawIndexedInstanced(dx11_renderer->context, num_indices, num_insts, index_offset, vertex_offset, inst_offset);
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    DM_DX11_GET_RENDERER;
    
    ID3D11DeviceContext* context = dx11_renderer->context;
    
    dm_dx11_pipeline active_pipeline = dx11_renderer->pipelines[dx11_renderer->active_pipeline];
    
    if(wireframe) context->lpVtbl->RSSetState(context, active_pipeline.wireframe_state);
    else context->lpVtbl->RSSetState(context, active_pipeline.rasterizer_state);
}

/*****************
DIRECTX DEBUGGING
*******************/
#ifdef DM_DEBUG
bool dm_dx11_print_errors(dm_dx11_renderer* dx11_renderer)
{
    HRESULT hr;
    
    ID3D11InfoQueue* info_queue;
    hr = dx11_renderer->device->lpVtbl->QueryInterface(dx11_renderer->device, &IID_ID3D11InfoQueue, (void**)&info_queue);
    if (hr != S_OK) { DM_LOG_ERROR("ID3D11Device::QueryInterface failed!"); return false; }
    
    UINT64 message_count = info_queue->lpVtbl->GetNumStoredMessages(info_queue);
    
    for (UINT64 i = 0; i < message_count; i++)
    {
        SIZE_T message_size = 0;
        info_queue->lpVtbl->GetMessage(info_queue, i, NULL, &message_size);
        
        D3D11_MESSAGE* message = dm_alloc(message_size);
        info_queue->lpVtbl->GetMessage(info_queue, i, message, &message_size);
        
        const char* category = dm_dx11_decode_category(message->Category);
        const char* severity = dm_dx11_decode_severity(message->Severity);
        D3D11_MESSAGE_ID id = message->ID;
        
        switch (message->Severity)
        {
            case D3D11_MESSAGE_SEVERITY_CORRUPTION:
            {
                DM_LOG_FATAL("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); 
                return false;
            } break;
            case D3D11_MESSAGE_SEVERITY_ERROR:
            {
                DM_LOG_ERROR("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); 
                return false;
            } break;
            case D3D11_MESSAGE_SEVERITY_WARNING:
            {
                DM_LOG_WARN("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); 
                return false;
            } break;
            case D3D11_MESSAGE_SEVERITY_INFO: 
            {
                DM_LOG_INFO("\n    [DirectX11 %s]: (%d) %s", severity, (int)id, message->pDescription); 
                return false;
            } break;
            case D3D11_MESSAGE_SEVERITY_MESSAGE: 
            {
                DM_LOG_TRACE("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); 
                return false;
            } break;
        }
        
        dm_free(message);
    }
    
    DM_DX11_RELEASE(info_queue);
    
    return true;
}

const char* dm_dx11_decode_category(D3D11_MESSAGE_CATEGORY category)
{
    switch (category)
    {
        case D3D11_MESSAGE_CATEGORY_APPLICATION_DEFINED: return "APPLICATION_DEFINED";
        case D3D11_MESSAGE_CATEGORY_MISCELLANEOUS: return "MISCELLANEOUS";
        case D3D11_MESSAGE_CATEGORY_INITIALIZATION: return "INITIALIZATION";
        case D3D11_MESSAGE_CATEGORY_CLEANUP: return "CLEANUP";
        case D3D11_MESSAGE_CATEGORY_COMPILATION: return "COMPILATION";
        case D3D11_MESSAGE_CATEGORY_STATE_CREATION: return "STATE_CREATION";
        case D3D11_MESSAGE_CATEGORY_STATE_SETTING: return "STATE_SETTING";
        case D3D11_MESSAGE_CATEGORY_STATE_GETTING: return "STATE_GETTING";
        case D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: return "RESOURCE_MANIPULATION";
        case D3D11_MESSAGE_CATEGORY_EXECUTION: return "EXECUTION";
        case D3D11_MESSAGE_CATEGORY_SHADER: return "SHADER";
        default:
        DM_LOG_FATAL("Unknown D3D11_MESSAGE_CATEGORY! Shouldn't be here...");
        return "Unknown category";
    }
}

const char* dm_dx11_decode_severity(D3D11_MESSAGE_SEVERITY severity)
{
    switch (severity)
    {
        case D3D11_MESSAGE_SEVERITY_CORRUPTION: return "CORRUPTION";
        case D3D11_MESSAGE_SEVERITY_ERROR: return "ERROR";
        case D3D11_MESSAGE_SEVERITY_WARNING: return "WARNING";
        case D3D11_MESSAGE_SEVERITY_INFO: return "INFO";
        case D3D11_MESSAGE_SEVERITY_MESSAGE: return "MESSAGE";
        default:
        DM_LOG_FATAL("Unknwon D3D11_MESSAGE_SEVERITY! Shouldn't be here...");
        return "Unknown severity";
    }
}
#endif

#endif //DM_RENDERER_DX11_H
