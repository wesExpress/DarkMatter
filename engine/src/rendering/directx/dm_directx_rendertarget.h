#ifndef __DM_DIRECTX_RENDERTARGET_H__
#define __DM_DIRECTX_RENDERTARGET_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"

bool dm_directx_create_rendertarget(dm_directx_renderer* renderer, dm_directx_pipeline* pipeline);
void dm_directx_destroy_rendertarget(dm_directx_pipeline* pipeline);

#endif // directx check

#endif