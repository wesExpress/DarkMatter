#ifndef __DM_DIRECTX_ENUM_CONVERSION_H__
#define __DM_DIRECTX_ENUM_CONVERSION_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include "../dm_render_types.h"

D3D11_CULL_MODE dm_cull_to_directx_cull(dm_cull_mode dm_mode);
D3D11_PRIMITIVE_TOPOLOGY dm_toplogy_to_directx_topology(dm_primitive_topology dm_top);
D3D11_USAGE dm_buffer_usage_to_directx(dm_buffer_usage usage);
D3D11_BIND_FLAG dm_buffer_type_to_directx(dm_buffer_type type);
D3D11_CPU_ACCESS_FLAG dm_buffer_cpu_access_to_directx(dm_buffer_cpu_access access);
DXGI_FORMAT dm_vertex_t_to_directx_format(dm_vertex_attrib_desc desc);
D3D11_COMPARISON_FUNC dm_comp_to_directx_comp(dm_comparison dm_comp);
DXGI_FORMAT dm_image_fmt_to_directx_fmt(dm_texture_format dm_fmt);
D3D11_FILTER dm_image_filter_to_directx_filter(dm_filter filter);
D3D11_TEXTURE_ADDRESS_MODE dm_texture_mode_to_directx_mode(dm_texture_mode dm_mode);

#endif

#endif