#include "dm_directx_rendertarget.h"

#ifdef DM_DIRECTX

#include "dm_assert.h"

bool dm_directx_create_rendertarget(dm_internal_pipeline* pipeline)
{
	DM_ASSERT_MSG(pipeline->device, "DirectX device is NULL!");
	DM_ASSERT_MSG(pipeline->swap_chain, "DirectX swap chain is NULL!");

	HRESULT hr;
	ID3D11Device* device = pipeline->device;
	IDXGISwapChain* swap_chain = pipeline->swap_chain;

	pipeline->render_back_buffer = (ID3D11Texture2D*)dm_alloc(sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE);
	pipeline->render_view = (ID3D11RenderTargetView*)dm_alloc(sizeof(ID3D11RenderTargetView), DM_MEM_RENDER_PIPELINE);

	DX_ERROR_CHECK(swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)&(ID3D11Resource*)pipeline->render_back_buffer), "IDXGISwapChain::GetBuffer failed!");
	device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)pipeline->render_back_buffer, 0, &pipeline->render_view);

	return true;
}

void dm_directx_destroy_rendertarget(dm_internal_pipeline* pipeline)
{
	ID3D11RenderTargetView* view = pipeline->render_view;
	ID3D11Texture2D* back_buffer = pipeline->render_back_buffer;

	DX_RELEASE(view);
	DX_RELEASE(back_buffer);

	dm_mem_db_adjust(-sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE);
	dm_mem_db_adjust(-sizeof(ID3D11RenderTargetView), DM_MEM_RENDER_PIPELINE);
}

#endif