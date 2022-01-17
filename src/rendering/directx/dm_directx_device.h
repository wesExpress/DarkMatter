#ifndef __DM_DIRECTX_DEVICE_H__
#define __DM_DIRECTX_DEVICE_H__

#include "dm_defines.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_directx_renderer.h"

bool dm_directx_create_device(dm_internal_renderer* renderer);
void dm_directx_destroy_device(dm_internal_renderer* renderer);
#if DM_DEBUG
void dm_directx_device_report_live_objects(dm_internal_renderer* renderer);
#endif

#endif

#endif