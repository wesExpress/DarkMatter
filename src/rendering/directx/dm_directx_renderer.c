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
	renderer_data->object_pipeline->interal_pipeline = (dm_internal_pipeline*)dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
	windows_internal_data* internal_data = (windows_internal_data*)platform_data->internal_data;
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)renderer_data->object_pipeline->interal_pipeline;

	internal_pipe->hwnd = internal_data->hwnd;
	internal_pipe->h_instance = internal_data->h_instance;

	if (!dm_directx_create_device(internal_pipe)) return false;
	if (!dm_directx_create_swapchain(internal_pipe)) return false;
	if (!dm_directx_create_rendertarget(internal_pipe)) return false;
	if (!dm_directx_create_depth_stencil(internal_pipe)) return false;

	ID3D11DeviceContext* context = internal_pipe->context;

	context->lpVtbl->OMSetRenderTargets(context, 1, &internal_pipe->render_view, internal_pipe->depth_stencil_view);
	
	D3D11_VIEWPORT viewport = { 0 };
	viewport.Width = renderer_data->object_pipeline->viewport.width;
	viewport.Height = renderer_data->object_pipeline->viewport.height;
	viewport.MaxDepth = 1.0f;
	context->lpVtbl->RSSetViewports(context, 1, &viewport);

	internal_pipe->viewport = viewport;

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

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
	dm_render_packet render_packet = pipeline->render_packet;

	ID3D11DeviceContext* context = internal_pipe->context;

	context->lpVtbl->DrawIndexed(context, render_packet.count, 0, render_packet.offset);
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
	dm_mem_db_adjust(sizeof(ID3D11DepthStencilState), DM_MEM_RENDER_PIPELINE);

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

	rd.CullMode = D3D11_CULL_NONE;
	rd.FrontCounterClockwise = false;
	rd.DepthClipEnable = true;

	DX_ERROR_CHECK(device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pipe->rasterizer_state), "ID3D11Device::CreateRasterizerState failed!");
	dm_mem_db_adjust(sizeof(ID3D11RasterizerState), DM_MEM_RENDER_PIPELINE);

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

	DX_RELEASE(internal_pipe->rasterizer_state);
	DX_RELEASE(internal_pipe->depth_stencil_state);
	dm_mem_db_adjust(-sizeof(ID3D11RasterizerState), DM_MEM_RENDER_PIPELINE);
	dm_mem_db_adjust(-sizeof(ID3D11DepthStencilState), DM_MEM_RENDER_PIPELINE);

	dm_directx_destroy_depth_stencil(internal_pipe);
	dm_directx_destroy_rendertarget(internal_pipe);
	dm_directx_destroy_swapchain(internal_pipe);
	dm_directx_destroy_device(internal_pipe);

	dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(dm_buffer_desc vb_desc, void* vb_data, dm_buffer_desc ib_desc, void* ib_data, dm_shader_desc vs_desc, dm_shader_desc ps_desc, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	/*
	// shader
	*/
	pipeline->raster_desc.shader->vertex_desc = vs_desc;
	pipeline->raster_desc.shader->pixel_desc = ps_desc;

	if (!dm_directx_create_shader(pipeline->raster_desc.shader, v_layout, pipeline)) return false;

	/*
	// buffers
	*/
	pipeline->render_packet.vertex_buffer->desc = vb_desc;
	pipeline->render_packet.index_buffer->desc = ib_desc;

	if (!dm_directx_create_buffer(pipeline->render_packet.vertex_buffer, vb_data, internal_pipe)) return false;
	if (!dm_directx_create_buffer(pipeline->render_packet.index_buffer, ib_data, internal_pipe)) return false;

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

	/*
	// viewport
	*/
	context->lpVtbl->RSSetViewports(context, 1, &internal_pipe->viewport);

	/*
	// raster state
	*/
	context->lpVtbl->RSSetState(context, raster_state);
	

	/*
	// depth stencil state
	*/
	context->lpVtbl->OMSetDepthStencilState(context, internal_pipe->depth_stencil_state, 1);

	/*
	// render target
	*/
	context->lpVtbl->OMSetRenderTargets(context, 1u, &render_target, depth_stencil);

	/*
	// buffers
	*/
	dm_directx_bind_buffer(pipeline->render_packet.vertex_buffer, pipeline->interal_pipeline);
	dm_directx_bind_buffer(pipeline->render_packet.index_buffer, pipeline->interal_pipeline);

	/*
	// shader
	*/
	context->lpVtbl->VSSetShader(context, internal_shader->vertex_shader, NULL, 0);
	context->lpVtbl->PSSetShader(context, internal_shader->pixel_shader, NULL, 0);
	context->lpVtbl->IASetInputLayout(context, internal_shader->input_layout);

	context->lpVtbl->IASetPrimitiveTopology(context, internal_pipe->topology);

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

	ID3D11DeviceContext* context = internal_pipe->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	
	// clear framebuffer
	context->lpVtbl->ClearRenderTargetView(context, render_target, &(clear_color->v[0]));
	context->lpVtbl->ClearDepthStencilView(context, depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

#endif