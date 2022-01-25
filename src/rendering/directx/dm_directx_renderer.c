#include "dm_directx_renderer.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_directx_device.h"
#include "dm_directx_swapchain.h"
#include "dm_directx_rendertarget.h"
#include "dm_directx_depth_stencil.h"
#include "dm_directx_buffer.h"
#include "dm_directx_shader.h"
#include "dm_directx_texture.h"
#include "dm_directx_enum_conversion.h"
#include "rendering/dm_texture.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "platform/dm_platform.h"
#include "structures/dm_list.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct windows_internal_data
{
	HINSTANCE h_instance;
	HWND hwnd;
} windows_internal_data;

static dm_internal_renderer* directx_renderer = NULL;

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
	renderer_data->object_pipeline->interal_pipeline = (dm_internal_pipeline*)dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
	windows_internal_data* internal_data = (windows_internal_data*)platform_data->internal_data;
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)renderer_data->object_pipeline->interal_pipeline;
	directx_renderer = (dm_internal_renderer*)dm_alloc(sizeof(dm_internal_renderer), DM_MEM_RENDERER);

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

	dm_free(directx_renderer, sizeof(dm_internal_renderer), DM_MEM_RENDERER);
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{
	
}

bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
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

	return true;
}

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{
	dm_render_packet render_packet = pipeline->render_packet;

	// index count, start index, base index
	// base index is the value added to each index before reading a vertex from the buffer
	directx_renderer->context->lpVtbl->DrawIndexed(directx_renderer->context, render_packet.count, 0, render_packet.offset);
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	HRESULT hr;

	ID3D11Device* device = directx_renderer->device;
	ID3D11DeviceContext* context = directx_renderer->context;
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
	ID3D11RenderTargetView* render_view = internal_pipe->render_view;
	//ID3D11DepthStencilView* depth_view = internal_pipe->depth_stencil_view;

	/*
	// Create the render target and depth stencil
	*/
	if (!dm_directx_create_rendertarget(directx_renderer, internal_pipe)) return false;
	if (!dm_directx_create_depth_stencil(directx_renderer, internal_pipe)) return false;

	/*
	// viewport
	*/
	internal_pipe->viewport.Width = pipeline->viewport.width;
	internal_pipe->viewport.Height = pipeline->viewport.height;
	internal_pipe->viewport.MaxDepth = pipeline->viewport.max_depth;

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

	//DX_ERROR_CHECK(device->lpVtbl->CreateDepthStencilState(device, &depth_stencil_desc, &internal_pipe->depth_stencil_state), "ID3D11Device::CreateDepthStencilState failed!");
	dm_mem_db_adjust(sizeof(ID3D11DepthStencilState), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_ADD);

	/*
	// rasterizer
	*/
	
	D3D11_RASTERIZER_DESC rd = { 0 };
	// wireframe
	if (pipeline->wireframe)
	{
		rd.FillMode = D3D11_FILL_WIREFRAME;
	}
	else
	{
		rd.FillMode = D3D11_FILL_SOLID;
	}

	// culling
	rd.CullMode = dm_cull_to_directx_cull(pipeline->raster_desc.cull_mode);
	if (rd.CullMode == D3D11_CULL_NONE) return false;
	
	rd.DepthClipEnable = pipeline->depth_desc.is_enabled;
	// winding
	switch (pipeline->raster_desc.winding_order)
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

	//DX_ERROR_CHECK(device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pipe->rasterizer_state), "ID3D11Device::CreateRasterizerState failed!");
	dm_mem_db_adjust(sizeof(ID3D11RasterizerState), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_ADD);

	/*
	// topology
	*/
	internal_pipe->topology = dm_toplogy_to_directx_topology(pipeline->raster_desc.primitive_topology);
	if (internal_pipe->topology == D3D11_PRIMITIVE_UNDEFINED) return false;

	return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	dm_directx_delete_buffer(pipeline->render_packet.vertex_buffer, pipeline->interal_pipeline);
	dm_directx_delete_buffer(pipeline->render_packet.index_buffer, pipeline->interal_pipeline);
	dm_directx_delete_shader(pipeline->raster_desc.shader, pipeline->interal_pipeline);

	// constant buffers
	for(uint32_t i=0; i<pipeline->render_packet.constant_buffers->count; i++)
	{
		dm_constant_buffer* cb = dm_list_at(pipeline->render_packet.constant_buffers, i);
	
		dm_directx_delete_buffer(&cb->desc.buffer, pipeline->interal_pipeline);
	}

	/*
	texture
	*/
	for (uint32_t i = 0; i < pipeline->render_packet.texture_paths->count; i++)
	{
		dm_string* key = dm_list_at(pipeline->render_packet.texture_paths, i);
		dm_texture* texture = dm_texture_get(key->string);

		dm_directx_destroy_texture(texture);
	}

	//DX_RELEASE(internal_pipe->rasterizer_state);
	//DX_RELEASE(internal_pipe->depth_stencil_state);
	dm_mem_db_adjust(sizeof(ID3D11RasterizerState), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11DepthStencilState), DM_MEM_RENDER_PIPELINE, DM_MEM_ADJUST_SUBTRACT);

	dm_directx_destroy_depth_stencil(internal_pipe);
	dm_directx_destroy_rendertarget(internal_pipe);

	dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	/*
	// shader
	*/

	if (!dm_directx_create_shader(pipeline->raster_desc.shader, v_layout, directx_renderer, pipeline)) return false;

	/*
	// buffers
	*/

	if (!dm_directx_create_buffer(pipeline->render_packet.vertex_buffer, vb_data, directx_renderer, internal_pipe)) return false;
	if (!dm_directx_create_buffer(pipeline->render_packet.index_buffer, ib_data, directx_renderer, internal_pipe)) return false;

	/*
	// constant buffer(s)
	*/
	for(uint32_t i=0; i<pipeline->render_packet.constant_buffers->count; i++)
	{
		dm_constant_buffer* cb = dm_list_at(pipeline->render_packet.constant_buffers, i);

		if (!dm_directx_create_buffer(&cb->desc.buffer, cb->desc.data, directx_renderer, internal_pipe)) return false;
	}

	/*
	textures
	*/
	for (uint32_t i = 0; i < pipeline->render_packet.texture_paths->count; i++ )
	{
		dm_string* key = dm_list_at(pipeline->render_packet.texture_paths, i);
		dm_texture* texture = dm_texture_get(key->string);

		if(!dm_directx_create_texture(texture, directx_renderer)) return false;
	}

	return true;
}

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline)
{

}

void dm_renderer_end_rederpass_impl()
{

}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = directx_renderer->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	//ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	//ID3D11RasterizerState* raster_state = internal_pipe->rasterizer_state;
	dm_internal_shader* internal_shader = (dm_internal_shader*)pipeline->raster_desc.shader->internal_shader;

	/*
	// viewport
	*/
	context->lpVtbl->RSSetViewports(context, 1, &internal_pipe->viewport);

	/*
	// render target
	*/
	context->lpVtbl->OMSetRenderTargets(context, 1u, &render_target, NULL);

	/*
	// raster state
	*/
	//context->lpVtbl->RSSetState(context, raster_state);
	context->lpVtbl->IASetPrimitiveTopology(context, internal_pipe->topology);

	/*
	// depth stencil state
	*/
	//context->lpVtbl->OMSetDepthStencilState(context, internal_pipe->depth_stencil_state, 1);

	/*
	// shader
	*/
	context->lpVtbl->VSSetShader(context, internal_shader->vertex_shader, NULL, 0);
	context->lpVtbl->PSSetShader(context, internal_shader->pixel_shader, NULL, 0);
	context->lpVtbl->IASetInputLayout(context, internal_shader->input_layout);

	/*
	// buffers
	*/
	dm_directx_bind_buffer(pipeline->render_packet.vertex_buffer, directx_renderer, internal_pipe);
	dm_directx_bind_buffer(pipeline->render_packet.index_buffer, directx_renderer, internal_pipe);

	/*
	// constant buffers
	*/
	for(uint32_t i=0; i<pipeline->render_packet.constant_buffers->count; i++)
	{
		dm_constant_buffer* cb = dm_list_at(pipeline->render_packet.constant_buffers, i);
		dm_directx_bind_buffer(&cb->desc.buffer, directx_renderer, internal_pipe);
	}

	return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	D3D11_VIEWPORT new_viewport = { 0 };
	new_viewport.Width = viewport.width;
	new_viewport.Height = viewport.height;
	new_viewport.MaxDepth = 1.0f;
	internal_pipe->viewport = new_viewport;
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = directx_renderer->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	//ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	
	// clear framebuffer
	context->lpVtbl->ClearRenderTargetView(context, render_target, &(clear_color->v[0]));
	//context->lpVtbl->ClearDepthStencilView(context, depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

#endif