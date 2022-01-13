#include "dm_directx_rendertarget.h"

#ifdef DM_DIRECTX

#include "dm_assert.h"

bool dm_directx_create_rendertarget(dm_internal_pipeline* pipeline)
{
	DM_ASSERT_MSG(pipeline->device, "DirectX device is NULL!");
	DM_ASSERT_MSG(pipeline->swap_chain, "DirectX swap chain is NULL!");

	//dm_directx_device_report_live_objects(renderer);

	HRESULT hr;
	ID3D11RenderTargetView* view = NULL;
	ID3D11Texture2D* back_buffer = NULL;
	ID3D11Device* device = pipeline->device;
	IDXGISwapChain* swap_chain = pipeline->swap_chain;

	DX_ERROR_CHECK(swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)&back_buffer), "IDXGISwapChain::GetBuffer failed!");
	device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)back_buffer, 0, &view);

	//dm_directx_device_report_live_objects(renderer);

	pipeline->render_back_buffer = back_buffer;
	pipeline->render_view = view;

	return true;
}

void dm_directx_destroy_rendertarget(dm_internal_pipeline* pipeline)
{
	ID3D11RenderTargetView* view = pipeline->render_view;
	ID3D11Texture2D* back_buffer = pipeline->render_back_buffer;

	DX_RELEASE(view);
	DX_RELEASE(back_buffer);
}

#endif