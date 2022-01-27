#ifndef __DM_VERTEX_ATTRIBS_H__
#define __DM_VERTEX_ATTRIBS_H__

#include "core/dm_defines.h"
#include "dm_render_types.h"

// position
dm_vertex_attrib_desc pos_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aPos",
#elif defined DM_DIRECTX
	.name = "POSITION",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, position),
	.count = 3,
	.normalized = false,
};

// color
dm_vertex_attrib_desc color_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aColor",
#elif defined DM_DIRECTX
	.name = "COLOR",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, color),
	.count = 3,
	.normalized = false,
};

// texture coords
dm_vertex_attrib_desc tex_coord_desc = {
#ifdef DM_OPENGL
	.name = "aTexCoords",
#elif defined DM_DIRECTX
	.name = "TEXCOORD",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, tex_coords),
	.count = 2,
	.normalized = false
};

#endif