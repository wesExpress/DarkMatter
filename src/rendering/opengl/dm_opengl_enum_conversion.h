#ifndef __DM_OPENGL_ENUM_CONVERSIONS_H__
#define __DM_OPENGL_ENUM_CONVERSIONS_H__

#include "dm_defines.h"

#ifdef DM_OPENGL

#include "dm_opengl_renderer.h"

GLenum dm_buffer_to_opengl_buffer(dm_buffer_type dm_type);
GLenum dm_usage_to_opengl_draw(dm_buffer_usage dm_usage);
GLenum dm_data_to_opengl_data(dm_buffer_data_type dm_data);
GLenum dm_shader_to_opengl_shader(dm_shader_type dm_type);
GLenum dm_vertex_data_t_to_opengl(dm_vertex_data_t dm_type);
GLenum dm_topology_to_opengl_primitive(dm_primitive_topology topology);
GLenum dm_blend_eq_to_opengl_func(dm_blend_equation eq);
GLenum dm_blend_func_to_opengl_func(dm_blend_func func);
GLenum dm_comp_to_opengl_comp(dm_comparison dm_comp);
GLenum dm_cull_to_opengl_cull(dm_cull_mode cull);
GLenum dm_wind_top_opengl_wind(dm_winding_order winding);

#endif

#endif