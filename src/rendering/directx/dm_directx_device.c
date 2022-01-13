#include "dm_directx_device.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_assert.h"

bool dm_directx_create_device(dm_internal_pipeline* pipeline)
{
	ID3D11Device* device = NULL;
	ID3D11DeviceContext* context = NULL;
	ID3D11Debug* debugger = NULL;

	UINT flags = 0;
#if DM_DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL feature_level;

	HRESULT hr;

	// create the device and immediate context
	DX_ERROR_CHECK(
		D3D11CreateDevice(
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			flags,
			0, 0,
			D3D11_SDK_VERSION,
			&device,
			&feature_level,
			&context),
		"D3D11CreateDevice failed!"
	);
	DM_ASSERT_MSG((feature_level == D3D_FEATURE_LEVEL_11_0), "Direct3D Feature Level 11 unsupported!");

	UINT msaa_quality;
	DX_ERROR_CHECK(
		device->lpVtbl->CheckMultisampleQualityLevels(
			device,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			4,
			&msaa_quality),
		"D3D11Device::CheckMultisampleQualityLevels failed!"
	);

	pipeline->context = context;
	pipeline->device = device;

	// if in debug, create the debugger to query live objects
#if DM_DEBUG
	DX_ERROR_CHECK(device->lpVtbl->QueryInterface(
		device,
		&IID_ID3D11Debug,
		(void**)&(debugger)),
		"D3D11Device::QueryInterface failed!"
	);
	pipeline->debugger = debugger;
#endif

	return true;
}

void dm_directx_destroy_device(dm_internal_pipeline* pipeline)
{
	ID3D11Device* device = pipeline->device;
	ID3D11DeviceContext* context = pipeline->context;

	DX_RELEASE(context);
#if DM_DEBUG
	dm_directx_device_report_live_objects(pipeline);
	ID3D11Debug* debugger = pipeline->debugger;
	DX_RELEASE(debugger);
#endif

	DX_RELEASE(device);
}

#if DM_DEBUG
void dm_directx_device_report_live_objects(dm_internal_pipeline* pipeline)
{
	HRESULT hr;
	ID3D11Debug* debugger = pipeline->debugger;

	hr = debugger->lpVtbl->ReportLiveDeviceObjects(
		debugger,
		D3D11_RLDO_DETAIL
	);
	if (hr != S_OK)
	{
		DM_LOG_ERROR("ID3D11Debug::ReportLiveDeviceObjects failed!");
	}
}
#endif

#endif