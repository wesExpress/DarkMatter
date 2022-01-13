#ifndef __DM_DIRECTX_ENUM_CONVERSION_H__
#define __DM_DIRECTX_ENUM_CONVERSION_H__

#include "dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include "../dm_render_types.h"

D3D11_CULL_MODE dm_cull_to_directx_cull(dm_cull_mode dm_mode);
D3D11_PRIMITIVE_TOPOLOGY dm_toplogy_to_directx_topology(dm_primitive_topology dm_top);

#endif

#endif