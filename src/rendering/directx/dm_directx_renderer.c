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

void dm_renderer_draw_indexed_impl(dm_draw_indexed_params* params, dm_render_pipeline* pipeline)
{
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = internal_pipe->context;

	context->lpVtbl->DrawIndexed(context, params->count, 0, params->offset);
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
	
	DX_ERROR_CHECK(device->lpVtbl->CreateRasterizerState(device, &rd, &internal_pipe->rasterizer_state), "ID3D11Device::CreateRasterizerState failed!");

	context->lpVtbl->IASetPrimitiveTopology(context, topology);

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
	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	ID3D11DeviceContext* context = internal_pipe->context;
	ID3D11RenderTargetView* render_target = internal_pipe->render_view;
	ID3D11DepthStencilView* depth_stencil = internal_pipe->depth_stencil_view;
	ID3D11RasterizerState* raster_state = internal_pipe->rasterizer_state;
	dm_internal_shader* internal_shader = (dm_internal_shader *)dm_renderer_get_shader(pipeline->raster_desc.shader)->internal_shader;
	
	/*
	// raster state
	*/
	context->lpVtbl->RSSetState(context, raster_state);

	/*
	// shader
	*/
	context->lpVtbl->VSSetShader(context, internal_shader->vertex_shader, NULL, 0);
	context->lpVtbl->PSSetShader(context, internal_shader->pixel_shader, NULL, 0);
	context->lpVtbl->IASetInputLayout(context, internal_shader->input_layout);

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

void dm_renderer_delete_buffer_impl(dm_buffer* buffer){}
void dm_renderer_bind_buffer_impl(dm_buffer* buffer){}

bool dm_renderer_create_shader_impl(dm_shader* shader, dm_render_pipeline* pipeline)
{ 
	HRESULT hr;

	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	shader->internal_shader = (dm_internal_shader*)dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
	dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;
	internal_shader->vertex_shader = (ID3D11VertexShader*)dm_alloc(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER);
	internal_shader->pixel_shader = (ID3D11PixelShader*)dm_alloc(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER);

	ID3D11Device* device = internal_pipe->device;
	ID3D11DeviceContext* context = internal_pipe->context;

	// vertex shader
	wchar_t ws[100];
	swprintf(ws, 100, L"%hs", shader->vertex_desc.path);

	ID3DBlob* blob = NULL;

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	DX_ERROR_CHECK(device->lpVtbl->CreateVertexShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->vertex_shader), "ID3D11Device::CreateVertexShader failed!");

	dm_list(D3D11_INPUT_ELEMENT_DESC) desc;
	dm_list_init(&desc, D3D11_INPUT_ELEMENT_DESC);

	for (int i = 0; i < pipeline->vertex_layout.num; i++)
	{
		dm_vertex_attrib_desc attrib_desc = pipeline->vertex_layout.attributes[i];

		DXGI_FORMAT format = dm_vertex_t_to_directx_format(attrib_desc);
		if (format == DXGI_FORMAT_UNKNOWN) return false;

		D3D11_INPUT_ELEMENT_DESC element_desc = {
			.SemanticName = attrib_desc.name,
			.SemanticIndex = 0,
			.Format = format,
			.AlignedByteOffset = attrib_desc.offset,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		};
		
		// append the element_desc to the array
		dm_list_append(&desc, element_desc);
	}

	DX_ERROR_CHECK(device->lpVtbl->CreateInputLayout(device, desc.array, (UINT)pipeline->vertex_layout.num, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), &internal_shader->input_layout), "ID3D11Device::CreateInputLayout failed!");

	// pixel shader
	swprintf(ws, 100, L"%hs", shader->pixel_desc.path);

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	DX_ERROR_CHECK(device->lpVtbl->CreatePixelShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->pixel_shader), "ID3D11Device::CreatePixelShader failed!");

	DX_RELEASE(blob);
	dm_list_destroy(&desc);

	return true; 
}

void dm_renderer_delete_shader_impl(dm_shader* shader){}
void dm_renderer_bind_shader_impl(dm_shader* shader){}

#endif