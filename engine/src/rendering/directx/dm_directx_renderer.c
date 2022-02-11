#include "dm_directx_renderer.h"

#ifdef DM_DIRECTX

#include "dm_directx_device.h"
#include "dm_directx_swapchain.h"
#include "dm_directx_rendertarget.h"
#include "dm_directx_depth_stencil.h"
#include "dm_directx_buffer.h"
#include "dm_directx_shader.h"
#include "dm_directx_texture.h"
#include "dm_directx_enum_conversion.h"
#include "rendering/dm_image.h"

#include "platform/dm_platform_win32.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "platform/dm_platform.h"
#include "structures/dm_list.h"
#include "structures/dm_map.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DM_DEBUG
void dm_directx_print_errors();
const char* dm_directx_decode_category(D3D11_MESSAGE_CATEGORY category);
const char* dm_directx_decode_severity(D3D11_MESSAGE_SEVERITY severity);
#endif

static dm_directx_renderer* directx_renderer = NULL;

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
	if (!dm_directx_create_rendertarget(directx_renderer, internal_pipe)) return false;
	if (!dm_directx_create_depth_stencil(directx_renderer, internal_pipe)) return false;

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

	/*
	texture
	*/
	for (uint32_t i = 0; i < pipeline->render_packet.image_paths->count; i++)
	{
		dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
		dm_image* image = dm_image_get(key->string);

		dm_directx_destroy_texture(image);
	}

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
	if (!dm_directx_create_buffer(pipeline->vertex_buffer, vb_data, directx_renderer)) return false;
	if (!dm_directx_create_buffer(pipeline->index_buffer, ib_data, directx_renderer)) return false;
	if (!dm_directx_create_buffer(pipeline->inst_buffer, 0, directx_renderer)) return false;

	internal_pipe->vertex_buffers = dm_list_create(sizeof(dm_buffer), 0);
	dm_list_append(internal_pipe->vertex_buffers, pipeline->vertex_buffer);
	dm_list_append(internal_pipe->vertex_buffers, pipeline->inst_buffer);

	/*
	textures
	*/
	for (uint32_t i = 0; i < pipeline->render_packet.image_paths->count; i++ )
	{
		dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
		dm_image* image = dm_image_get(key->string);

		if(!dm_directx_create_texture(image, directx_renderer)) return false;
	}

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
	if (!dm_directx_create_shader(render_pass->shader, v_layout, directx_renderer)) return false;

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
	for (uint32_t i = 0; i < render_pass->uniforms->count; i++)
	{
		dm_uniform* uniform = dm_list_at(render_pass->uniforms, i);

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
	dm_directx_bind_buffer(buffer, slot, directx_renderer);

	return true;
}

void dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
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
	for (uint32_t i = 0; i < render_pass->uniforms->count; i++)
	{
		dm_uniform* uniform = dm_list_at(render_pass->uniforms, i);

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
	dm_directx_bind_buffer(pipeline->index_buffer, 0, directx_renderer);
	dm_directx_bind_vertex_buffers(internal_pipe->vertex_buffers, directx_renderer);

	/*
	textures
	*/
	for (uint32_t i = 0; i < pipeline->render_packet.image_paths->count; i++)
	{
		dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
		dm_image* image = dm_image_get(key->string);
		dm_directx_bind_texture(image, i, directx_renderer);
	}

	return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
{
	ID3D11DeviceContext* context = directx_renderer->context;

	D3D11_VIEWPORT new_viewport = { 0 };
	new_viewport.Width = viewport.width;
	new_viewport.Height = viewport.height;
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
void dm_directx_print_errors()
{
	HRESULT hr;

	ID3D11InfoQueue* info_queue;
	hr = directx_renderer->device->lpVtbl->QueryInterface(directx_renderer->device, &IID_ID3D11InfoQueue, (void**)&info_queue);
	if (hr != S_OK)
	{
		DM_LOG_ERROR("ID3D11Device::QueryInterface failed!");
		return;
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
		case D3D11_MESSAGE_SEVERITY_CORRUPTION: DM_LOG_FATAL("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); break;
		case D3D11_MESSAGE_SEVERITY_ERROR: DM_LOG_ERROR("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); break;
		case D3D11_MESSAGE_SEVERITY_WARNING: DM_LOG_WARN("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); break;
		case D3D11_MESSAGE_SEVERITY_INFO: DM_LOG_INFO("\n    [DirectX11 %s]: (%d) %s", severity, (int)id, message->pDescription); break;
		case D3D11_MESSAGE_SEVERITY_MESSAGE: DM_LOG_TRACE("\n    [DirectX11 %s]: (%d) %s", severity, id, message->pDescription); break;
		}

		dm_free(message, message_size, DM_MEM_RENDER_PIPELINE);
	}

	DX_RELEASE(info_queue);
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