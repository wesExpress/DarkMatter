#include "dm_directx_renderer.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_directx_device.h"
#include "dm_directx_swapchain.h"
#include "dm_directx_rendertarget.h"
#include "dm_directx_depth_stencil.h"
#include "dm_directx_buffer.h"
#include "dm_directx_shader.h"
#include "dm_directx_enum_conversion.h"

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
#include "structures/dm_list.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct windows_internal_data
{
	HINSTANCE h_instance;
	HWND hwnd;
} windows_internal_data;

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
	HRESULT hr;

	renderer_data->object_pipeline->interal_pipeline = (dm_internal_pipeline*)dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
	windows_internal_data* internal_data = (windows_internal_data*)platform_data->internal_data;
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)renderer_data->object_pipeline->interal_pipeline;

	internal_pipe->hwnd = internal_data->hwnd;
	internal_pipe->h_instance = internal_data->h_instance;

	if (!dm_directx_create_device(internal_pipe)) return false;
	if (!dm_directx_create_swapchain(internal_pipe)) return false;
	if (!dm_directx_create_rendertarget(internal_pipe)) return false;
	if (!dm_directx_create_depth_stencil(internal_pipe)) return false;

	return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
	
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{

}

bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
	HRESULT hr;

	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)renderer_data->object_pipeline->interal_pipeline;
	
	IDXGISwapChain* swap_chain = internal_pipe->swap_chain;
	ID3D11DeviceContext* context = internal_pipe->context;

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

void dm_renderer_draw_arrays_impl(int first, size_t count)
{
	
}

void dm_renderer_draw_indexed_impl(uint32_t count, uint32_t offset, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = internal_pipe->context;

	context->lpVtbl->DrawIndexed(context, count, 0, offset);
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	HRESULT hr;

	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
	ID3D11Device* device = internal_pipe->device;
	ID3D11DeviceContext* context = internal_pipe->context;
	ID3D11RenderTargetView* render_view = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_view = internal_pipe->depth_stencil_view;

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
	// depth testing
	*/
	if (pipeline->depth_desc.is_enabled)
	{

	}
	else
	{

	}

	/*
	// stencil testing
	*/
	if (pipeline->stencil_desc.is_enabled)
	{

	}
	else
	{

	}

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
	D3D11_CULL_MODE cull = dm_cull_to_directx_cull(pipeline->raster_desc.cull_mode);
	if (cull == D3D11_CULL_NONE) return false;
	rd.CullMode = cull;
	rd.DepthClipEnable = pipeline->depth_desc.is_enabled;
	// winding
	switch (pipeline->raster_desc.winding_order)
	{
	case DM_WINDING_CLOCK:
	{
		rd.FrontCounterClockwise = false;
	} break;
	case DM_WINDING_COUNTER_CLOCK:
	{
		rd.FrontCounterClockwise = true;
	} break;
	default:
		DM_LOG_FATAL("Unknown winding order!");
		return false;
	}

	/*
	// topology
	*/
	D3D11_PRIMITIVE_TOPOLOGY topology = dm_toplogy_to_directx_topology(pipeline->raster_desc.primitive_topology);
	if (topology == D3D11_PRIMITIVE_UNDEFINED) return false;
	internal_pipe->topology = topology;

	/*
	* // finally fill in all the members
	*/
	
	DX_ERROR_CHECK(device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pipe->rasterizer_state), "ID3D11Device::CreateRasterizerState failed!");

	return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* interanl_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	dm_directx_delete_buffer(pipeline->render_packet.vertex_buffer, pipeline->interal_pipeline);
	dm_directx_delete_buffer(pipeline->render_packet.index_buffer, pipeline->interal_pipeline);
	dm_directx_delete_shader(pipeline->raster_desc.shader, pipeline->interal_pipeline);

	dm_free(pipeline->render_packet.vertex_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->render_packet.index_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->raster_desc.shader, sizeof(dm_shader), DM_MEM_RENDERER_SHADER);

	dm_directx_destroy_depth_stencil(interanl_pipe);
	dm_directx_destroy_rendertarget(interanl_pipe);
	dm_directx_destroy_swapchain(interanl_pipe);
	dm_directx_destroy_device(interanl_pipe);

	dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(dm_buffer_desc vb_desc, void* vb_data, dm_buffer_desc ib_desc, void* ib_data, dm_shader_desc vs_desc, dm_shader_desc ps_desc, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	/*
	// shader
	*/
	dm_shader* shader = (dm_shader*)dm_alloc(sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
	shader->vertex_desc = vs_desc;
	shader->pixel_desc = ps_desc;

	if (!dm_directx_create_shader(shader, v_layout, pipeline)) return false;
	pipeline->raster_desc.shader = shader;

	/*
	// buffers
	*/
	dm_buffer* vertex_buffer = (dm_buffer*)dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_buffer* index_buffer = (dm_buffer*)dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);

	vertex_buffer->desc = vb_desc;
	index_buffer->desc = ib_desc;

	if (!dm_directx_create_buffer(vertex_buffer, vb_data, internal_pipe)) return false;
	if (!dm_directx_create_buffer(index_buffer, ib_data, internal_pipe)) return false;

	pipeline->render_packet.vertex_buffer = vertex_buffer;
	pipeline->render_packet.index_buffer = index_buffer;

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

	ID3D11DeviceContext* context = internal_pipe->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	ID3D11RasterizerState* raster_state = internal_pipe->rasterizer_state;
	dm_internal_shader* internal_shader = (dm_internal_shader*)pipeline->raster_desc.shader->internal_shader;
	
	D3D11_VIEWPORT viewport = { 0 };
	viewport.Width = pipeline->viewport.width;
	viewport.Height = pipeline->viewport.height;
	viewport.MaxDepth = 1.0f;
	context->lpVtbl->RSSetViewports(context, 1, &viewport);

	/*
	// raster state
	*/
	context->lpVtbl->RSSetState(context, raster_state);
	context->lpVtbl->IASetPrimitiveTopology(context, internal_pipe->topology);

	/*
	// shader
	*/
	context->lpVtbl->VSSetShader(context, internal_shader->vertex_shader, NULL, 0);
	context->lpVtbl->PSSetShader(context, internal_shader->pixel_shader, NULL, 0);
	context->lpVtbl->IASetInputLayout(context, internal_shader->input_layout);

	/*
	// buffers
	*/
	dm_directx_bind_buffer(pipeline->render_packet.vertex_buffer, pipeline->interal_pipeline);
	dm_directx_bind_buffer(pipeline->render_packet.index_buffer, pipeline->interal_pipeline);

	return true;
}

void dm_renderer_set_viewport_impl(dm_viewport* viewport)
{

}

void dm_renderer_clear_impl(dm_render_pipeline* pipeline, dm_color* clear_color)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = internal_pipe->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	
	// clear framebuffer
	context->lpVtbl->ClearRenderTargetView(context, render_target, &(clear_color->v[0]));
	//context->lpVtbl->ClearDepthStencilView(context, depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

#endif