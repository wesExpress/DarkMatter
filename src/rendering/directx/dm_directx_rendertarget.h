#ifndef __DM_DIRECTX_RENDERTARGET_H__
#define __DM_DIRECTX_RENDERTARGET_H__

#include "dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"

bool dm_directx_create_rendertarget(dm_internal_renderer* renderer, dm_internal_pipeline* pipeline);
void dm_directx_destroy_rendertarget(dm_internal_pipeline* pipeline);

#endif // directx check

#endif