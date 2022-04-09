#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "rendering/dm_renderer.h"
#include "rendering/dm_image.h"

#include "platform/dm_platform_win32.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "platform/dm_platform.h"
#include "structures/dm_list.h"
#include "structures/dm_map.h"

#include <d3d11_1.h>
#include <dxgi.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef DM_DEBUG
bool dm_directx_print_errors();
const char* dm_directx_decode_category(D3D11_MESSAGE_CATEGORY category);
const char* dm_directx_decode_severity(D3D11_MESSAGE_SEVERITY severity);
#endif

#define DX_ERROR_CHECK(HRCALL, ERROR_MSG) hr = HRCALL; if(hr!=S_OK){ DM_LOG_FATAL(ERROR_MSG); DM_LOG_FATAL(dm_get_win32_error_msg(hr)); return false; }
#define DX_RELEASE(OBJ) if(OBJ) { OBJ->lpVtbl->Release(OBJ); }

typedef struct dm_directx_buffer
{
	ID3D11Buffer* buffer;
} dm_directx_buffer;

typedef struct dm_directx_shader
{
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* input_layout;
} dm_directx_shader;

typedef struct dm_directx_texture
{
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* view;
} dm_directx_texture;

typedef struct dm_directx_renderer
{
	HWND hwnd;
	HINSTANCE h_instance;
    
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swap_chain;
    
#if DM_DEBUG
	ID3D11Debug* debugger;
#endif
} dm_directx_renderer;

typedef struct dm_directx_pipeline
{
	ID3D11RenderTargetView* render_view;
	ID3D11Texture2D* render_back_buffer;
	ID3D11DepthStencilView* depth_stencil_view;
	ID3D11Texture2D* depth_stencil_back_buffer;
	ID3D11DepthStencilState* depth_stencil_state;
	dm_list* vertex_buffers;
} dm_directx_pipeline;

typedef struct dm_directx_render_pass
{
	ID3D11RasterizerState* rasterizer_state;
	ID3D11SamplerState* sample_state;
	D3D11_PRIMITIVE_TOPOLOGY topology;
	ID3D11Buffer* constant_buffer;
} dm_directx_render_pass;

static dm_directx_renderer* directx_renderer = NULL;

/*
DIRECTXENUMS
*/

D3D11_CULL_MODE dm_cull_to_directx_cull(dm_cull_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_CULL_FRONT_BACK:
        case DM_CULL_FRONT: return D3D11_CULL_FRONT;
        case DM_CULL_BACK: return D3D11_CULL_BACK;
        default:
		DM_LOG_FATAL("Unknown cull mode!");
		return D3D11_CULL_NONE;
	}
}

D3D11_PRIMITIVE_TOPOLOGY dm_toplogy_to_directx_topology(dm_primitive_topology dm_top)
{
	switch (dm_top)
	{
        case DM_TOPOLOGY_POINT_LIST: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case DM_TOPOLOGY_LINE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case DM_TOPOLOGY_LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case DM_TOPOLOGY_TRIANGLE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case DM_TOPOLOGY_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
		DM_LOG_FATAL("Unknown primitive topology!");
		return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

D3D11_USAGE dm_buffer_usage_to_directx(dm_buffer_usage usage)
{
	switch (usage)
	{
        case DM_BUFFER_USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
        case DM_BUFFER_USAGE_STATIC: return D3D11_USAGE_IMMUTABLE;
        case DM_BUFFER_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
        default:
		DM_LOG_FATAL("Unknown buffer usage!");
		return D3D11_USAGE_STAGING+1;
	}
}

D3D11_BIND_FLAG dm_buffer_type_to_directx(dm_buffer_type type)
{
	switch (type)
	{
        case DM_BUFFER_TYPE_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
        case DM_BUFFER_TYPE_INDEX: return D3D11_BIND_INDEX_BUFFER;
        case DM_BUFFER_TYPE_CONSTANT: return D3D11_BIND_CONSTANT_BUFFER;
        default:
		DM_LOG_FATAL("Unknown buffer type!");
		return D3D11_BIND_VIDEO_ENCODER+1;
	}
}

D3D11_CPU_ACCESS_FLAG dm_buffer_cpu_access_to_directx(dm_buffer_cpu_access access)
{
	switch (access)
	{
        case DM_BUFFER_CPU_READ: return D3D11_CPU_ACCESS_READ;
        case DM_BUFFER_CPU_WRITE: return D3D11_CPU_ACCESS_WRITE;
        default:
		DM_LOG_FATAL("Unknown cpu access!");
		return D3D11_CPU_ACCESS_READ+1;
	}
}

DXGI_FORMAT dm_vertex_t_to_directx_format(dm_vertex_attrib_desc desc)
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
        }
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
        }
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
        }
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
        }
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
        }
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
        }
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

dm_comp_to_directx_comp(dm_comparison dm_comp)
{
	switch (dm_comp)
	{
        case DM_COMPARISON_ALWAYS: return D3D11_COMPARISON_ALWAYS;
        case DM_COMPARISON_NEVER: return D3D11_COMPARISON_NEVER;
        case DM_COMPARISON_EQUAL: return D3D11_COMPARISON_EQUAL;
        case DM_COMPARISON_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
        case DM_COMPARISON_LESS: return D3D11_COMPARISON_LESS;
        case DM_COMPARISON_LEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
        case DM_COMPARISON_GREATER: return D3D11_COMPARISON_GREATER;
        case DM_COMPARISON_GEQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
        default:
		DM_LOG_FATAL("Unknown comparison function!");
		return D3D11_COMPARISON_ALWAYS + 1;
	}
}

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

D3D11_FILTER dm_image_filter_to_directx_filter(dm_filter filter)
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

D3D11_TEXTURE_ADDRESS_MODE dm_texture_mode_to_directx_mode(dm_texture_mode dm_mode)
{
	switch (dm_mode)
	{
        case DM_TEXTURE_MODE_WRAP: return D3D11_TEXTURE_ADDRESS_WRAP;
        case DM_TEXTURE_MODE_BORDER: return D3D11_TEXTURE_ADDRESS_BORDER;
        case DM_TEXTURE_MODE_EDGE: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case DM_TEXTURE_MODE_MIRROR_REPEAT: return D3D11_TEXTURE_ADDRESS_MIRROR;
        case DM_TEXTURE_MODE_MIRROR_EDGE: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        default:
		DM_LOG_FATAL("Unknown texture mode!");
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE + 1;
	}
}

D3D11_INPUT_CLASSIFICATION dm_vertex_class_to_directx_class(dm_vertex_attrib_class dm_class)
{
	switch (dm_class)
	{
        case DM_VERTEX_ATTRIB_CLASS_VERTEX: return D3D11_INPUT_PER_VERTEX_DATA;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE: return D3D11_INPUT_PER_INSTANCE_DATA;
        default:
		DM_LOG_FATAL("Unknown vertex attribute input class!");
		return DM_VERTEX_ATTRIB_CLASS_UNKNOWN;
	}
}

/*
BUFFER
*/
bool dm_directx_create_buffer(dm_buffer* buffer, void* data)
{
	HRESULT hr;
    
	buffer->internal_buffer = dm_alloc(sizeof(dm_directx_buffer), DM_MEM_RENDERER_BUFFER);
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;
    
	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
    
	D3D11_USAGE usage = dm_buffer_usage_to_directx(buffer->desc.usage);
	if (usage == D3D11_USAGE_STAGING + 1) return false;
	D3D11_BIND_FLAG type = dm_buffer_type_to_directx(buffer->desc.type);
	if (type == D3D11_BIND_VIDEO_ENCODER + 1) return false;
	D3D11_CPU_ACCESS_FLAG cpu_access = dm_buffer_cpu_access_to_directx(buffer->desc.cpu_access);
	if (cpu_access == D3D11_CPU_ACCESS_READ + 1) return false;
    
	D3D11_BUFFER_DESC desc = { 0 };
	desc.Usage = usage;
	desc.BindFlags = type;
	desc.ByteWidth = buffer->desc.buffer_size;
	desc.StructureByteStride = buffer->desc.elem_size;
	if (usage == D3D11_USAGE_DYNAMIC) desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = data;
    
	if (data)
	{
		DX_ERROR_CHECK(device->lpVtbl->CreateBuffer(device, &desc, &sd, &internal_buffer->buffer), "ID3D11Device::CreateBuffer failed!");
	}
	else
	{
		DX_ERROR_CHECK(device->lpVtbl->CreateBuffer(device, &desc, 0, &internal_buffer->buffer), "ID3D11Device::CreateBuffer failed!");
	}
    
	return true;
}

void dm_directx_delete_buffer(dm_buffer* buffer)
{
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;
    
	DX_RELEASE(internal_buffer->buffer);
	
	dm_free(buffer->internal_buffer, sizeof(dm_directx_buffer), DM_MEM_RENDERER_BUFFER);
}

void dm_directx_bind_buffer(dm_buffer* buffer, uint32_t slot)
{
	ID3D11DeviceContext* context = directx_renderer->context;
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;
    
	UINT stride = buffer->desc.elem_size;
	UINT offset = 0;
    
	switch (buffer->desc.type)
	{
        case DM_BUFFER_TYPE_VERTEX: 
		context->lpVtbl->IASetVertexBuffers(context, slot, 1, &internal_buffer->buffer, &stride, &offset);
		break;
        case DM_BUFFER_TYPE_INDEX: 
		context->lpVtbl->IASetIndexBuffer(context, internal_buffer->buffer, DXGI_FORMAT_R32_UINT, 0);
		break;
        case DM_BUFFER_TYPE_CONSTANT: 
		context->lpVtbl->VSSetConstantBuffers(context, slot, 1, &internal_buffer->buffer);
		break;
        default:
		DM_LOG_WARN("Trying to bind a buffer of unknown type! Shouldn't be here...");
		break;
	}
}

void dm_directx_bind_vertex_buffers(dm_list* buffers)
{
	ID3D11DeviceContext* context = directx_renderer->context;
	
	ID3D11Buffer** buffer_list = dm_alloc(sizeof(ID3D11Buffer*) * buffers->count, DM_MEM_RENDERER_BUFFER);
	UINT* stride_list = dm_alloc(sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	UINT* offset_list = dm_alloc(sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
    
	for (uint32_t i = 0; i < buffers->count; i++)
	{
		dm_buffer* buffer = dm_list_at(buffers, i);
		dm_directx_buffer* internal_buffer = buffer->internal_buffer;
		buffer_list[i] = internal_buffer->buffer;
		stride_list[i] = buffer->desc.elem_size;
		offset_list[i] = 0;
	}
    
	context->lpVtbl->IASetVertexBuffers(context, 0, buffers->count, buffer_list, stride_list, offset_list);
    
	dm_free(offset_list, sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	dm_free(stride_list, sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	dm_free(buffer_list, sizeof(ID3D11Buffer*) * buffers->count, DM_MEM_RENDERER_BUFFER);
}

/*
SHADER
*/

bool dm_directx_create_input_element(dm_vertex_attrib_desc attrib_desc, D3D11_INPUT_ELEMENT_DESC* element_desc)
{
	DXGI_FORMAT format = dm_vertex_t_to_directx_format(attrib_desc);
	if (format == DXGI_FORMAT_UNKNOWN) return false;
	D3D11_INPUT_CLASSIFICATION input_class = dm_vertex_class_to_directx_class(attrib_desc.attrib_class);
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


bool dm_directx_create_shader(dm_shader* shader, dm_vertex_layout layout)
{
	HRESULT hr;
    
	shader->internal_shader = dm_alloc(sizeof(dm_directx_shader), DM_MEM_RENDERER_SHADER);
	dm_directx_shader* internal_shader = shader->internal_shader;
    
	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
    
	// vertex shader
	wchar_t ws[100];
	swprintf(ws, 100, L"%hs", shader->vertex_desc.path);
    
	ID3DBlob* blob;
    
	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");
    
	DX_ERROR_CHECK(device->lpVtbl->CreateVertexShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->vertex_shader), "ID3D11Device::CreateVertexShader failed!");
	dm_mem_db_adjust(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);
    
	// input layout
	dm_list* desc = dm_list_create(sizeof(D3D11_INPUT_ELEMENT_DESC), 0);
	uint32_t count = 0;
    
	for (uint32_t i = 0; i < layout.num; i++)
	{
		dm_vertex_attrib_desc attrib_desc = layout.attributes[i];
        
		if ((attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT))
		{
			for (uint32_t j = 0; j < attrib_desc.count; j++)
			{
				dm_vertex_attrib_desc sub_desc = attrib_desc;
				if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) sub_desc.data_t = DM_VERTEX_DATA_T_INT;
				else if(attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT) sub_desc.data_t = DM_VERTEX_DATA_T_FLOAT;
				else
				{
					DM_LOG_FATAL("Unknwon vertex data type!");
					return false;
				}
				
				sub_desc.offset = sub_desc.offset + sizeof(float) * j;
				
				D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
				if (!dm_directx_create_input_element(sub_desc, &element_desc)) return false;
				element_desc.SemanticIndex = j;
				element_desc.AlignedByteOffset = sizeof(dm_vec4) * j;
                
				dm_list_append(desc, &element_desc);
				count++;
			}
		}
		else
		{
			D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
			if (!dm_directx_create_input_element(attrib_desc, &element_desc)) return false;
            
			// append the element_desc to the array
			dm_list_append(desc, &element_desc);
			count++;
		}	
	}
    
	DX_ERROR_CHECK(device->lpVtbl->CreateInputLayout(device, desc->data, (UINT)count, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), &internal_shader->input_layout), "ID3D11Device::CreateInputLayout failed!");
	dm_mem_db_adjust(sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);
    
	// pixel shader
	swprintf(ws, 100, L"%hs", shader->pixel_desc.path);
    
	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");
    
	DX_ERROR_CHECK(device->lpVtbl->CreatePixelShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->pixel_shader), "ID3D11Device::CreatePixelShader failed!");
	dm_mem_db_adjust(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);
    
	DX_RELEASE(blob);
	dm_list_destroy(desc);
    
	return true;
}

void dm_directx_delete_shader(dm_shader* shader)
{
	dm_directx_shader* internal_shader = shader->internal_shader;
    
	DX_RELEASE(internal_shader->vertex_shader);
	DX_RELEASE(internal_shader->pixel_shader);
	DX_RELEASE(internal_shader->input_layout);
    
	dm_mem_db_adjust(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);
    
	dm_free(shader->internal_shader, sizeof(dm_directx_shader), DM_MEM_RENDERER_SHADER);
}

/*
TEXTURE
*/

bool dm_directx_create_texture(dm_image* image)
{
	HRESULT hr;
    
	image->internal_texture = dm_alloc(sizeof(dm_directx_texture), DM_MEM_RENDERER_TEXTURE);
	dm_directx_texture* internal_texture = image->internal_texture;
    
	DXGI_FORMAT image_format = dm_image_fmt_to_directx_fmt(image->desc.format);
	if (image_format == DXGI_FORMAT_UNKNOWN) return false;
    
	D3D11_TEXTURE2D_DESC tex_desc = { 0 };
	tex_desc.Width = image->desc.width;
	tex_desc.Height = image->desc.height;
	tex_desc.ArraySize = 1;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.Format = image_format;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
	DX_ERROR_CHECK(directx_renderer->device->lpVtbl->CreateTexture2D(directx_renderer->device, &tex_desc, NULL, &internal_texture->texture), "ID3D11Device::CreateTexture2D failed!");
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);
    
	directx_renderer->context->lpVtbl->UpdateSubresource(directx_renderer->context, (ID3D11Resource*)internal_texture->texture, 0, NULL, image->data, image->desc.width * 4, 0);
    
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
	view_desc.Format = tex_desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels = -1;
    
	DX_ERROR_CHECK(directx_renderer->device->lpVtbl->CreateShaderResourceView(directx_renderer->device, (ID3D11Resource*)internal_texture->texture, &view_desc, &internal_texture->view), "ID3D11Device::CreateShaderResourceView failed!");
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);
    
	directx_renderer->context->lpVtbl->GenerateMips(directx_renderer->context, internal_texture->view);
    
	return true;
}

void dm_directx_destroy_texture(dm_image* image)
{
	dm_directx_texture* internal_texture = image->internal_texture;
    
	DX_RELEASE(internal_texture->texture);
	DX_RELEASE(internal_texture->view);
    
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);
    
	dm_free(image->internal_texture, sizeof(dm_directx_texture), DM_MEM_RENDERER_TEXTURE);
}

void dm_directx_bind_texture(dm_image* image, uint32_t slot)
{
	dm_directx_texture* internal_texture = image->internal_texture;
	
	directx_renderer->context->lpVtbl->PSSetShaderResources(directx_renderer->context, slot, 1, &internal_texture->view);
}

/*
DEVICE
*/

#if DM_DEBUG
void dm_directx_device_report_live_objects()
{
	HRESULT hr;
	ID3D11Debug* debugger = directx_renderer->debugger;
    
	hr = debugger->lpVtbl->ReportLiveDeviceObjects(debugger, D3D11_RLDO_DETAIL);
	if (hr != S_OK) DM_LOG_ERROR("ID3D11Debug::ReportLiveDeviceObjects failed!");
}
#endif

bool dm_directx_create_device()
{
	UINT flags = 0;
#if DM_DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL feature_level;
    
	HRESULT hr;
    
	// create the device and immediate context
	DX_ERROR_CHECK(
                   D3D11CreateDevice(
                                     NULL,
                                     D3D_DRIVER_TYPE_HARDWARE,
                                     NULL,
                                     flags,
                                     NULL, 0,
                                     D3D11_SDK_VERSION,
                                     &directx_renderer->device,
                                     &feature_level,
                                     &directx_renderer->context),
                   "D3D11CreateDevice failed!"
                   );
	DM_ASSERT_MSG((feature_level == D3D_FEATURE_LEVEL_11_0), "Direct3D Feature Level 11 unsupported!");
    
	UINT msaa_quality;
	DX_ERROR_CHECK(directx_renderer->device->lpVtbl->CheckMultisampleQualityLevels(directx_renderer->device, DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_quality), "D3D11Device::CheckMultisampleQualityLevels failed!");
    
	// if in debug, create the debugger to query live objects
#if DM_DEBUG
	DX_ERROR_CHECK(directx_renderer->device->lpVtbl->QueryInterface(directx_renderer->device, &IID_ID3D11Debug, (void**)&(directx_renderer->debugger)), "D3D11Device::QueryInterface failed!");
#endif
    
	return true;
}

void dm_directx_destroy_device()
{
	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
    
	DX_RELEASE(context);
#if DM_DEBUG
	dm_directx_device_report_live_objects();
	DX_RELEASE(directx_renderer->debugger);
#endif
    
	DX_RELEASE(device);
}

/*
SWAPCHAIN
*/

bool dm_directx_create_swapchain()
{
	// set up the swap chain pointer to be created in this function
	IDXGISwapChain* swap_chain = NULL;
    
	// make sure the device has been created and then grab it
	DM_ASSERT_MSG(directx_renderer->device, "DirectX device is NULL!");
	ID3D11Device* device = directx_renderer->device;
    
	HRESULT hr;
	RECT client_rect;
	GetClientRect(directx_renderer->hwnd, &client_rect);
    
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
	desc.OutputWindow = directx_renderer->hwnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	//desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
	// obtain factory
	IDXGIDevice* dxgi_device = NULL;
	DX_ERROR_CHECK(device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, &dxgi_device), "D3D11Device::QueryInterface failed!");
    
	IDXGIAdapter* dxgi_adapter = NULL;
	DX_ERROR_CHECK(dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_IDXGIAdapter, (void**)&dxgi_adapter), "IDXGIDevice::GetParent failed!");
    
	IDXGIFactory* dxgi_factory = NULL;
	DX_ERROR_CHECK(dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory, (void**)&dxgi_factory), "IDXGIAdapter::GetParent failed!");
    
	// create the swap chain
	DX_ERROR_CHECK(dxgi_factory->lpVtbl->CreateSwapChain(dxgi_factory, (IUnknown*)device, &desc, &directx_renderer->swap_chain), "IDXGIFactory::CreateSwapChain failed!");
	dm_mem_db_adjust(sizeof(IDXGISwapChain), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_ADD);
    
	// release pack animal directx objects
	DX_RELEASE(dxgi_device);
	DX_RELEASE(dxgi_factory);
	DX_RELEASE(dxgi_adapter);
    
	return true;
}

void dm_directx_destroy_swapchain()
{
	// release the directx object
	IDXGISwapChain* swap_chain = directx_renderer->swap_chain;
	DX_RELEASE(swap_chain);
    
	dm_mem_db_adjust(sizeof(IDXGISwapChain), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_SUBTRACT);
}

/*
DEPTHSTENCIL
*/

bool dm_directx_create_depth_stencil(dm_directx_pipeline* pipeline)
{
	DM_ASSERT_MSG(directx_renderer->device, "DirectX device is NULL!");
    
	HRESULT hr;
	RECT client_rect;
	GetClientRect(directx_renderer->hwnd, &client_rect);
	
	ID3D11Device* device = directx_renderer->device;
    
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = client_rect.right;
	desc.Height = client_rect.bottom;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	
	DX_ERROR_CHECK(device->lpVtbl->CreateTexture2D(device, &desc, NULL, &pipeline->depth_stencil_back_buffer), "ID3D11Device::CreateTexture2D failed!");
	DX_ERROR_CHECK(device->lpVtbl->CreateDepthStencilView(device, (ID3D11Resource*)pipeline->depth_stencil_back_buffer, 0, &pipeline->depth_stencil_view), "ID3D11Device::CreateDepthStencilView failed!");
    
	return true;
}

void dm_directx_destroy_depth_stencil(dm_directx_pipeline* pipeline)
{
	ID3D11Texture2D* back_buffer = pipeline->depth_stencil_back_buffer;
	ID3D11DepthStencilView* view = pipeline->depth_stencil_view;
    
	// release the directx objects
	DX_RELEASE(back_buffer);
	DX_RELEASE(view);
}

/*
RENDERTARGET
*/

bool dm_directx_create_rendertarget(dm_directx_pipeline* pipeline)
{
	DM_ASSERT_MSG(directx_renderer->device, "DirectX device is NULL!");
	DM_ASSERT_MSG(directx_renderer->swap_chain, "DirectX swap chain is NULL!");
    
	HRESULT hr;
	ID3D11Device* device = directx_renderer->device;
	IDXGISwapChain* swap_chain = directx_renderer->swap_chain;
	
	DX_ERROR_CHECK(swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)&(ID3D11Resource*)pipeline->render_back_buffer), "IDXGISwapChain::GetBuffer failed!");
	device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)pipeline->render_back_buffer, NULL, &pipeline->render_view);
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_ADD);
	dm_mem_db_adjust(sizeof(ID3D11RenderTargetView), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_ADD);
    
	return true;
}

void dm_directx_destroy_rendertarget(dm_directx_pipeline* pipeline)
{
	ID3D11RenderTargetView* view = pipeline->render_view;
	ID3D11Texture2D* back_buffer = pipeline->render_back_buffer;
    
	DX_RELEASE(view);
	DX_RELEASE(back_buffer);
    
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11RenderTargetView), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_SUBTRACT);
}

/*
RENDERER
*/

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
	DM_LOG_DEBUG("Initializing Directx11 Backend...");
    
	renderer_data->pipeline->internal_pipeline = dm_alloc(sizeof(dm_directx_pipeline), DM_MEM_RENDER_PIPELINE);
	dm_internal_windows_data* internal_data = platform_data->internal_data;
	dm_directx_pipeline* internal_pipe = renderer_data->pipeline->internal_pipeline;
	directx_renderer = dm_alloc(sizeof(dm_directx_renderer), DM_MEM_RENDERER);
    
	directx_renderer->hwnd = internal_data->hwnd;
	directx_renderer->h_instance = internal_data->h_instance;
    
	if (!dm_directx_create_device(directx_renderer)) return false;
	if (!dm_directx_create_swapchain(directx_renderer)) return false;
	
	return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
	dm_directx_destroy_swapchain(directx_renderer);
	dm_directx_destroy_device(directx_renderer);
    
	dm_free(directx_renderer, sizeof(dm_directx_renderer), DM_MEM_RENDERER);
}

bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data)
{
	HRESULT hr;
    
	IDXGISwapChain* swap_chain = directx_renderer->swap_chain;
	ID3D11DeviceContext* context = directx_renderer->context;
    
	if (FAILED(hr = swap_chain->lpVtbl->Present(swap_chain, 1, 0)))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			DM_LOG_FATAL("DirectX Device removed! Exiting...");
		}
		else
		{
			DM_LOG_ERROR("Something bad happened when presenting buffers. Exiting...");
		}
		return false;
	}
    
#ifdef DM_DEBUG
	dm_directx_print_errors();
#endif
    
	return true;
}

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
	directx_renderer->context->lpVtbl->Draw(directx_renderer->context, count, start);
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
	directx_renderer->context->lpVtbl->DrawIndexed(directx_renderer->context, num_indices, index_offset, vertex_offset);
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
	directx_renderer->context->lpVtbl->DrawIndexedInstanced(directx_renderer->context, num_indices, num_insts, index_offset, vertex_offset, inst_offset);
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	HRESULT hr;
    
	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
	dm_directx_pipeline* internal_pipe = pipeline->internal_pipeline;
	ID3D11RenderTargetView* render_view = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_view = internal_pipe->depth_stencil_view;
    
	/*
	// Create the render target and depth stencil
	*/
	if (!dm_directx_create_rendertarget(internal_pipe)) return false;
	if (!dm_directx_create_depth_stencil(internal_pipe)) return false;
    
	/*
	// blending 
	*/
	if (pipeline->blend_desc.is_enabled)
	{
        
	}
	else
	{
        
	}
    
	/*
	// depth and stencil testing
	*/
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = { 0 };
	depth_stencil_desc.DepthEnable = pipeline->depth_desc.is_enabled;
	if (pipeline->depth_desc.is_enabled)
	{
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		D3D11_COMPARISON_FUNC comp = dm_comp_to_directx_comp(pipeline->depth_desc.comparison);
		if (comp == D3D11_COMPARISON_ALWAYS + 1) return false;
		depth_stencil_desc.DepthFunc = comp;
	}
    
	depth_stencil_desc.StencilEnable = pipeline->stencil_desc.is_enabled;
	if (pipeline->stencil_desc.is_enabled)
	{
		
	}
    
	DX_ERROR_CHECK(device->lpVtbl->CreateDepthStencilState(device, &depth_stencil_desc, &internal_pipe->depth_stencil_state), "ID3D11Device::CreateDepthStencilState failed!");
    
	return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_directx_pipeline* internal_pipe = pipeline->internal_pipeline;
    
	dm_directx_delete_buffer(pipeline->vertex_buffer);
	dm_directx_delete_buffer(pipeline->index_buffer);
	dm_directx_delete_buffer(pipeline->inst_buffer);
    
	DX_RELEASE(internal_pipe->depth_stencil_state);
    
	dm_directx_destroy_depth_stencil(internal_pipe);
	dm_directx_destroy_rendertarget(internal_pipe);
	dm_list_destroy(internal_pipe->vertex_buffers);
    
	dm_free(pipeline->internal_pipeline, sizeof(dm_directx_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline)
{
	dm_directx_pipeline* internal_pipe = pipeline->internal_pipeline;
    
	/*
	// buffers
	*/
	if (!dm_directx_create_buffer(pipeline->vertex_buffer, vb_data)) return false;
	if (!dm_directx_create_buffer(pipeline->index_buffer, ib_data)) return false;
	if (!dm_directx_create_buffer(pipeline->inst_buffer, 0)) return false;
    
	internal_pipe->vertex_buffers = dm_list_create(sizeof(dm_buffer), 0);
	dm_list_append(internal_pipe->vertex_buffers, pipeline->vertex_buffer);
	dm_list_append(internal_pipe->vertex_buffers, pipeline->inst_buffer);
    
	return true;
}

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
	HRESULT hr;
    
	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
    
	render_pass->internal_render_pass = dm_alloc(sizeof(dm_directx_render_pass), DM_MEM_RENDER_PASS);
	dm_directx_render_pass* internal_pass = render_pass->internal_render_pass;
    
	// shader
	if (!dm_directx_create_shader(render_pass->shader, v_layout)) return false;
    
	// rasterizer
	D3D11_RASTERIZER_DESC rd = { 0 };
	// wireframe
	if (render_pass->wireframe)
	{
		rd.FillMode = D3D11_FILL_WIREFRAME;
	}
	else
	{
		rd.FillMode = D3D11_FILL_SOLID;
	}
    
	// culling
	rd.CullMode = dm_cull_to_directx_cull(render_pass->raster_desc.cull_mode);
	if (rd.CullMode == D3D11_CULL_NONE) return false;
    
	rd.DepthClipEnable = pipeline->depth_desc.is_enabled;
	// winding
	switch (render_pass->raster_desc.winding_order)
	{
        case DM_WINDING_CLOCK:
        {
            rd.FrontCounterClockwise = true;
        } break;
        case DM_WINDING_COUNTER_CLOCK:
        {
            rd.FrontCounterClockwise = false;
        } break;
        default:
		DM_LOG_FATAL("Unknown winding order!");
		return false;
	}
    
	DX_ERROR_CHECK(device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pass->rasterizer_state), "ID3D11Device::CreateRasterizerState failed!");
    
	/*
	// topology
	*/
	internal_pass->topology = dm_toplogy_to_directx_topology(render_pass->raster_desc.primitive_topology);
	if (internal_pass->topology == D3D11_PRIMITIVE_UNDEFINED) return false;
    
	/*
	sampler state
	*/
	D3D11_FILTER filter = dm_image_filter_to_directx_filter(render_pass->sampler_desc.filter);
	if (filter == D3D11_FILTER_MAXIMUM_ANISOTROPIC + 1) return false;
	D3D11_TEXTURE_ADDRESS_MODE u_mode = dm_texture_mode_to_directx_mode(render_pass->sampler_desc.u);
	if (u_mode == D3D11_TEXTURE_ADDRESS_MIRROR_ONCE + 1) return false;
	D3D11_TEXTURE_ADDRESS_MODE v_mode = dm_texture_mode_to_directx_mode(render_pass->sampler_desc.v);
	if (v_mode == D3D11_TEXTURE_ADDRESS_MIRROR_ONCE + 1) return false;
	D3D11_TEXTURE_ADDRESS_MODE w_mode = dm_texture_mode_to_directx_mode(render_pass->sampler_desc.w);
	if (w_mode == D3D11_TEXTURE_ADDRESS_MIRROR_ONCE + 1) return false;
	D3D11_COMPARISON_FUNC comp = dm_comp_to_directx_comp(render_pass->sampler_desc.comparison);
	if (comp == D3D11_COMPARISON_ALWAYS + 1) return false;
    
	D3D11_SAMPLER_DESC sample_desc = { 0 };
	sample_desc.Filter = filter;
	sample_desc.AddressU = u_mode;
	sample_desc.AddressV = v_mode;
	sample_desc.AddressW = w_mode;
	sample_desc.MaxAnisotropy = 1;
	sample_desc.ComparisonFunc = comp;
	if (filter != D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT) sample_desc.MaxLOD = D3D11_FLOAT32_MAX;
    
	DX_ERROR_CHECK(device->lpVtbl->CreateSamplerState(device, &sample_desc, &internal_pass->sample_state), "ID3D11Device::CreateSamplerState failed!");
    
	// uniforms
	size_t buffer_size = 0;
	void* buffer_data = NULL;
	dm_for_map_item(render_pass->uniforms)
	{
		dm_uniform* uniform = item->value;
        
		buffer_data = dm_realloc(buffer_data, buffer_size + uniform->desc.data_size);
		void* dest = (char*)buffer_data + buffer_size;
		dm_memcpy(dest, uniform->data, uniform->desc.data_size);
		buffer_size += uniform->desc.data_size;
	}
    
	D3D11_USAGE usage = D3D11_USAGE_DYNAMIC;
	D3D11_BIND_FLAG type = D3D11_BIND_CONSTANT_BUFFER;
	D3D11_CPU_ACCESS_FLAG cpu_flag = D3D11_CPU_ACCESS_WRITE;
    
	D3D11_BUFFER_DESC cb_desc = { 0 };
	cb_desc.Usage = usage;
	cb_desc.BindFlags = type;
	cb_desc.CPUAccessFlags = cpu_flag;
	cb_desc.ByteWidth = ((buffer_size + 15) / 16) * 16;
	cb_desc.StructureByteStride = sizeof(float);
    
	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = buffer_data;
    
	DX_ERROR_CHECK(device->lpVtbl->CreateBuffer(device, &cb_desc, &sd, &internal_pass->constant_buffer), "ID3D11Device::CreateBuffer failed!");
    
	free(buffer_data);
    
	return true;
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
	dm_directx_render_pass* internal_pass = render_pass->internal_render_pass;
    
	dm_directx_delete_shader(render_pass->shader);
    
	DX_RELEASE(internal_pass->sample_state);
	DX_RELEASE(internal_pass->rasterizer_state);	
	DX_RELEASE(internal_pass->constant_buffer);
    
	dm_free(internal_pass, sizeof(dm_directx_render_pass), DM_MEM_RENDER_PASS);
}

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size)
{
	HRESULT hr;
    
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;
    
	D3D11_MAPPED_SUBRESOURCE msr;
	ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));
    
	DX_ERROR_CHECK(directx_renderer->context->lpVtbl->Map(directx_renderer->context, (ID3D11Resource*)internal_buffer->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr), "ID3D11DeviceContext::Map failed!");
	dm_memcpy(msr.pData, data, data_size);
	directx_renderer->context->lpVtbl->Unmap(directx_renderer->context, (ID3D11Resource*)internal_buffer->buffer, 0);
    
	return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot)
{
	dm_directx_bind_buffer(buffer, slot);
    
	return true;
}

bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot)
{
    dm_directx_bind_texture(image, slot);
    
    return true;
}

bool dm_create_texture_impl(dm_image* image)
{
    return dm_directx_create_texture(image);
}

void dm_destroy_texture_impl(dm_image* image)
{
    dm_directx_destroy_texture(image);
}

bool dm_renderer_bind_uniform_impl(dm_uniform* uniform)
{
    //return dm_directx_bind_uniform(uniform);
    return true;
}

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
	HRESULT hr;
    
	ID3D11DeviceContext* context = directx_renderer->context;
    
	dm_directx_render_pass* internal_pass = render_pass->internal_render_pass;
    
	ID3D11RasterizerState* raster_state = internal_pass->rasterizer_state;
	dm_directx_shader* internal_shader = render_pass->shader->internal_shader;
    
	// raster state
	context->lpVtbl->RSSetState(context, raster_state);
	context->lpVtbl->IASetPrimitiveTopology(context, internal_pass->topology);
    
	// sampler
	context->lpVtbl->PSSetSamplers(context, 0, 1, &internal_pass->sample_state);
    
	// shader
	context->lpVtbl->VSSetShader(context, internal_shader->vertex_shader, NULL, 0);
	context->lpVtbl->PSSetShader(context, internal_shader->pixel_shader, NULL, 0);
	context->lpVtbl->IASetInputLayout(context, internal_shader->input_layout);
    
	// uniforms
	size_t buffer_size = 0;
	void* buffer_data = NULL;
	dm_for_map_item(render_pass->uniforms)
	{
		dm_uniform* uniform = item->value;
        
		buffer_data = dm_realloc(buffer_data, buffer_size + uniform->desc.data_size);
		void* dest = (char*)buffer_data + buffer_size;
		dm_memcpy(dest, uniform->data, uniform->desc.data_size);
		buffer_size += uniform->desc.data_size;
	}
    
	D3D11_MAPPED_SUBRESOURCE msr;
	ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));
    
	DX_ERROR_CHECK(context->lpVtbl->Map(context, (ID3D11Resource*)internal_pass->constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr), "ID3D11DeviceContext::Map failed!");
	dm_memcpy(msr.pData, buffer_data, buffer_size);
	context->lpVtbl->Unmap(context, (ID3D11Resource*)internal_pass->constant_buffer, 0);
    
	free(buffer_data);
    
	// constant buffer
	context->lpVtbl->VSSetConstantBuffers(context, 0, 1, &internal_pass->constant_buffer);
	context->lpVtbl->PSSetConstantBuffers(context, 0, 1, &internal_pass->constant_buffer);
    
    return true;
}

void dm_renderer_end_rederpass_impl()
{
    
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_directx_pipeline* internal_pipe = pipeline->internal_pipeline;
    
	ID3D11DeviceContext* context = directx_renderer->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
    
	/*
	// render target
	*/
	context->lpVtbl->OMSetRenderTargets(context, 1u, &render_target, depth_stencil);
    
	/*
	// depth stencil state
	*/
	context->lpVtbl->OMSetDepthStencilState(context, internal_pipe->depth_stencil_state, 1);
    
	/*
	// buffers
	*/
	dm_directx_bind_buffer(pipeline->index_buffer, 0);
	dm_directx_bind_vertex_buffers(internal_pipe->vertex_buffers);
    
	return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
{
	ID3D11DeviceContext* context = directx_renderer->context;
    
	D3D11_VIEWPORT new_viewport = { 0 };
	new_viewport.Width = (FLOAT)viewport.width;
	new_viewport.Height = (FLOAT)viewport.height;
	new_viewport.MaxDepth = 1.0f;
	
	context->lpVtbl->RSSetViewports(context, 1, &new_viewport);
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
	dm_directx_pipeline* internal_pipe = pipeline->internal_pipeline;
    
	ID3D11DeviceContext* context = directx_renderer->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	
	// clear framebuffer
	context->lpVtbl->ClearRenderTargetView(context, render_target, &(clear_color->v[0]));
	context->lpVtbl->ClearDepthStencilView(context, depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

#ifdef DM_DEBUG
bool dm_directx_print_errors()
{
	HRESULT hr;
    
	ID3D11InfoQueue* info_queue;
	hr = directx_renderer->device->lpVtbl->QueryInterface(directx_renderer->device, &IID_ID3D11InfoQueue, (void**)&info_queue);
	if (hr != S_OK)
	{
		DM_LOG_ERROR("ID3D11Device::QueryInterface failed!");
		return false;
	}
    
	UINT64 message_count = info_queue->lpVtbl->GetNumStoredMessages(info_queue);
    
	for (UINT64 i = 0; i < message_count; i++)
	{
		SIZE_T message_size = 0;
		info_queue->lpVtbl->GetMessage(info_queue, i, NULL, &message_size);
        
		D3D11_MESSAGE* message = dm_alloc(message_size, DM_MEM_RENDER_PIPELINE);
		info_queue->lpVtbl->GetMessage(info_queue, i, message, &message_size);
        
		const char* category = dm_directx_decode_category(message->Category);
		const char* severity = dm_directx_decode_severity(message->Severity);
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
        
		dm_free(message, message_size, DM_MEM_RENDER_PIPELINE);
	}
    
	DX_RELEASE(info_queue);
    
    return true;
}

const char* dm_directx_decode_category(D3D11_MESSAGE_CATEGORY category)
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

const char* dm_directx_decode_severity(D3D11_MESSAGE_SEVERITY severity)
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

#endif