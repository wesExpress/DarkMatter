#include "dm_directx_device.h"

#ifdef DM_DIRECTX

#include "core/dm_assert.h"
#include "core/dm_mem.h"

bool dm_directx_create_device(dm_directx_renderer* renderer)
{
	UINT flags = 0;
#if DM_DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL feature_level;

	HRESULT hr;

	// create the device and immediate context
	DX_ERROR_CHECK(
		D3D11CreateDevice(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			flags,
			NULL, 0,
			D3D11_SDK_VERSION,
			&renderer->device,
			&feature_level,
			&renderer->context),
		"D3D11CreateDevice failed!"
	);
	DM_ASSERT_MSG((feature_level == D3D_FEATURE_LEVEL_11_0), "Direct3D Feature Level 11 unsupported!");

	UINT msaa_quality;
	DX_ERROR_CHECK(renderer->device->lpVtbl->CheckMultisampleQualityLevels(renderer->device, DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_quality), "D3D11Device::CheckMultisampleQualityLevels failed!");

	// if in debug, create the debugger to query live objects
#if DM_DEBUG
	DX_ERROR_CHECK(renderer->device->lpVtbl->QueryInterface(renderer->device, &IID_ID3D11Debug, (void**)&(renderer->debugger)), "D3D11Device::QueryInterface failed!");
#endif

	return true;
}

void dm_directx_destroy_device(dm_directx_renderer* renderer)
{
	ID3D11Device* device = renderer->device;
	ID3D11DeviceContext* context = renderer->context;

	DX_RELEASE(context);
#if DM_DEBUG
	dm_directx_device_report_live_objects(renderer);
	DX_RELEASE(renderer->debugger);
#endif

	DX_RELEASE(device);
}

#if DM_DEBUG
void dm_directx_device_report_live_objects(dm_directx_renderer* renderer)
{
	HRESULT hr;
	ID3D11Debug* debugger = renderer->debugger;

	hr = debugger->lpVtbl->ReportLiveDeviceObjects(debugger, D3D11_RLDO_DETAIL);
	if (hr != S_OK) DM_LOG_ERROR("ID3D11Debug::ReportLiveDeviceObjects failed!");
}
#endif

#endif