#include "dm_directx_renderer.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_directx_device.h"
#include "dm_directx_swapchain.h"
#include "dm_directx_rendertarget.h"
#include "dm_directx_depth_stencil.h"
#include "dm_directx_enum_conversion.h"

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
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
	dm_internal_pipeline* internal_pipeline = (dm_internal_pipeline*)renderer_data->object_pipeline->interal_pipeline;

	internal_pipeline->hwnd = internal_data->hwnd;

	if (!dm_directx_create_device(renderer_data->object_pipeline->interal_pipeline)) return false;
	if (!dm_directx_create_swapchain(renderer_data->object_pipeline->interal_pipeline)) return false;
	if (!dm_directx_create_rendertarget(renderer_data->object_pipeline->interal_pipeline)) return false;
	if (!dm_directx_create_depth_stencil(renderer_data->object_pipeline->interal_pipeline)) return false;

	return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
	
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{

}

void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
	
}

void dm_renderer_draw_arrays_impl(int first, size_t count)
{
	
}

void dm_renderer_draw_indexed_impl(dm_draw_indexed_params* params, dm_render_pipeline* pipeline)
{
	
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

	rd.DepthClipEnable = pipeline->depth_desc.is_enabled;

	/*
	// topology
	*/

	D3D11_PRIMITIVE_TOPOLOGY topology = dm_toplogy_to_directx_topology(pipeline->raster_desc.primitive_topology);
	if (topology == D3D11_PRIMITIVE_UNDEFINED) return false;

	/*
	* // finally fill in all the members
	*/
	
	DX_ERROR_CHECK(
		device->lpVtbl->CreateRasterizerState(
			device,
			&rd,
			&internal_pipe->rasterizer_state),
		"ID3D11Device::CreateRasterizerState failed!"
	);

	context->lpVtbl->IASetPrimitiveTopology(
		context,
		topology
	);

	return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* interanl_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	dm_directx_destroy_depth_stencil(interanl_pipe);
	dm_directx_destroy_rendertarget(interanl_pipe);
	dm_directx_destroy_swapchain(interanl_pipe);
	dm_directx_destroy_device(interanl_pipe);

	dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vertex_data, void* index_data, dm_render_pipeline* pipeline)
{
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
	ID3D11RasterizerState* raster_state = internal_pipe->rasterizer_state;
	
	// clear framebuffer
	context->lpVtbl->ClearRenderTargetView(
		context,
		render_target,
		&(clear_color->v[0])
	);
	//context->lpVtbl->ClearDepthStencilView(
	//	context,
	//	depth_stencil,
	//	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
	//	1.0f, 0
	//);
	context->lpVtbl->RSSetState(
		context,
		raster_state
	);
}

void dm_renderer_delete_buffer_impl(dm_buffer* buffer){}
void dm_renderer_bind_buffer_impl(dm_buffer* buffer){}
bool dm_renderer_create_shader_impl(dm_shader* shader) { return true; }
void dm_renderer_delete_shader_impl(dm_shader* shader){}
void dm_renderer_bind_shader_impl(dm_shader* shader){}

#endif