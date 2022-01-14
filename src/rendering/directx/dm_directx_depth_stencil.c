#include "dm_directx_depth_stencil.h"

#ifdef DM_DIRECTX

#include "dm_assert.h"

bool dm_directx_create_depth_stencil(dm_internal_pipeline* pipeline)
{
	DM_ASSERT_MSG(pipeline->device, "DirectX device is NULL!");

	HRESULT hr;
	RECT client_rect;
	GetClientRect(pipeline->hwnd, &client_rect);

	ID3D11DepthStencilView* view = NULL;
	ID3D11Texture2D* back_buffer = NULL;
	ID3D11Device* device = pipeline->device;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = client_rect.right;
	desc.Height = client_rect.bottom;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	DX_ERROR_CHECK(device->lpVtbl->CreateTexture2D(device, &desc, 0, &back_buffer), "ID3D11Device::CreateTexture2D failed!");
	DX_ERROR_CHECK(device->lpVtbl->CreateDepthStencilView(device, (ID3D11Resource*)back_buffer, 0, &view), "ID3D11Device::CreateDepthStencilView failed!");

	pipeline->depth_stencil_back_buffer = (ID3D11Texture2D*)dm_alloc(sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE);
	pipeline->depth_stencil_view = (ID3D11DepthStencilView*)dm_alloc(sizeof(ID3D11DepthStencilView), DM_MEM_RENDER_PIPELINE);

	// assign the new pointers to locations in directx renderer
	pipeline->depth_stencil_back_buffer = back_buffer;
	pipeline->depth_stencil_view = view;

	return true;
}

void dm_directx_destroy_depth_stencil(dm_internal_pipeline* pipeline)
{
	ID3D11Texture2D* back_buffer = pipeline->depth_stencil_back_buffer;
	ID3D11DepthStencilView* view = pipeline->depth_stencil_view;

	// release the directx objects
	DX_RELEASE(back_buffer);
	DX_RELEASE(view);

	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDER_PIPELINE);
	dm_mem_db_adjust(sizeof(ID3D11DepthStencilView), DM_MEM_RENDER_PIPELINE);
}

#endif