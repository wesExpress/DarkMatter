#include "dm_directx_swapchain.h"

#ifdef DM_DIRECTX

#include "dm_assert.h"

bool dm_directx_create_swapchain(dm_internal_pipeline* pipeline)
{
	// set up the swap chain pointer to be created in this function
	IDXGISwapChain* swap_chain = NULL;

	// make sure the device has been created and then grab it
	DM_ASSERT_MSG(pipeline->device, "DirectX device is NULL!");
	ID3D11Device* device = pipeline->device;

	HRESULT hr;
	RECT client_rect;
	GetClientRect(pipeline->hwnd, &client_rect);

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
	desc.OutputWindow = pipeline->hwnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// obtain factory
	IDXGIDevice* dxgi_device = NULL;
	DX_ERROR_CHECK(device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, &dxgi_device), "D3D11Device::QueryInterface failed!");

	IDXGIAdapter* dxgi_adapter = NULL;
	DX_ERROR_CHECK(dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_IDXGIAdapter, (void**)&dxgi_adapter), "IDXGIDevice::GetParent failed!");

	IDXGIFactory* dxgi_factory = NULL;
	DX_ERROR_CHECK(dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory, (void**)&dxgi_factory), "IDXGIAdapter::GetParent failed!");

	// create the swap chain
	DX_ERROR_CHECK(dxgi_factory->lpVtbl->CreateSwapChain(dxgi_factory, (IUnknown*)device, &desc, &swap_chain), "IDXGIFactory::CreateSwapChain failed!");

	// release pack animal directx objects
	DX_RELEASE(dxgi_device);
	DX_RELEASE(dxgi_factory);
	DX_RELEASE(dxgi_adapter);

	pipeline->swap_chain = (IDXGISwapChain*)dm_alloc(sizeof(IDXGISwapChain), DM_MEM_RENDER_PIPELINE);

	// point our swap chain instance to our newly created swap chain
	pipeline->swap_chain = swap_chain;

	return true;
}

void dm_directx_destroy_swapchain(dm_internal_pipeline* pipeline)
{
	// release the directx object
	IDXGISwapChain* swap_chain = pipeline->swap_chain;
	DX_RELEASE(swap_chain);

	dm_mem_db_adjust(-sizeof(IDXGISwapChain), DM_MEM_RENDER_PIPELINE);
}

#endif // directx check