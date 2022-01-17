#include "dm_directx_device.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_assert.h"

bool dm_directx_create_device(dm_internal_pipeline* pipeline)
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
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			flags,
			0, 0,
			D3D11_SDK_VERSION,
			&pipeline->device,
			&feature_level,
			&pipeline->context),
		"D3D11CreateDevice failed!"
	);
	DM_ASSERT_MSG((feature_level == D3D_FEATURE_LEVEL_11_0), "Direct3D Feature Level 11 unsupported!");
	dm_mem_db_adjust(sizeof(ID3D11DeviceContext), DM_MEM_RENDER_PIPELINE);
	dm_mem_db_adjust(sizeof(ID3D11Device), DM_MEM_RENDER_PIPELINE);

	UINT msaa_quality;
	DX_ERROR_CHECK(pipeline->device->lpVtbl->CheckMultisampleQualityLevels(pipeline->device, DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_quality), "D3D11Device::CheckMultisampleQualityLevels failed!");	

	// if in debug, create the debugger to query live objects
#if DM_DEBUG
	DX_ERROR_CHECK(pipeline->device->lpVtbl->QueryInterface(pipeline->device, &IID_ID3D11Debug, (void**)&(pipeline->debugger)), "D3D11Device::QueryInterface failed!");
	dm_mem_db_adjust(sizeof(ID3D11Debug), DM_MEM_RENDER_PIPELINE);
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
	DX_RELEASE(pipeline->debugger);
	dm_mem_db_adjust(-sizeof(ID3D11Debug), DM_MEM_RENDER_PIPELINE);
#endif

	DX_RELEASE(device);

	dm_mem_db_adjust(-sizeof(ID3D11Device), DM_MEM_RENDER_PIPELINE);
	dm_mem_db_adjust(-sizeof(ID3D11DeviceContext), DM_MEM_RENDER_PIPELINE);
}

#if DM_DEBUG
void dm_directx_device_report_live_objects(dm_internal_pipeline* pipeline)
{
	HRESULT hr;
	ID3D11Debug* debugger = pipeline->debugger;

	hr = debugger->lpVtbl->ReportLiveDeviceObjects(debugger, D3D11_RLDO_DETAIL);
	if (hr != S_OK) DM_LOG_ERROR("ID3D11Debug::ReportLiveDeviceObjects failed!");
}
#endif

#endif