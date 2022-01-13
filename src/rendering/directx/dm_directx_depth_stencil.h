#ifndef __DM_DIRECTX_DEPTH_STENCIL_H__
#define __DM_DIRECTX_DEPTH_STENCIL_H__

#include "dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"

bool dm_directx_create_depth_stencil(dm_internal_pipeline* pipeline);
void dm_directx_destroy_depth_stencil(dm_internal_pipeline* pipeline);

#endif //directx check

#endif