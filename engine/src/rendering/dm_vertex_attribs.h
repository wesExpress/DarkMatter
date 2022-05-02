#ifndef __DM_VERTEX_ATTRIBS_H__
#define __DM_VERTEX_ATTRIBS_H__

#include "core/dm_defines.h"
#include "dm_render_types.h"
#include <stddef.h>

// position
dm_vertex_attrib_desc pos_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aPos",
#elif defined DM_DIRECTX
	.name = "POSITION",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_VERTEX,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, position),
	.count = 3,
	.normalized = false,
};

dm_vertex_attrib_desc norm_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aNormal",
#else
	.name = "NORMAL",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_VERTEX,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, normal),
	.count = 3,
	.normalized = false
};

// texture coords
dm_vertex_attrib_desc tex_coord_desc = {
#ifdef DM_OPENGL
	.name = "aTexCoords",
#elif defined DM_DIRECTX
	.name = "TEXCOORD",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_VERTEX,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, tex_coords),
	.count = 2,
	.normalized = false
};

/*
instance attributes
*/

// model
dm_vertex_attrib_desc model_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aModel",
#elif defined DM_DIRECTX
	.name = "MODEL",
#endif
	.data_t = DM_VERTEX_DATA_T_MATRIX_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_INSTANCE,
	.stride = sizeof(dm_vertex_inst_t),
	.offset = offsetof(dm_vertex_inst_t, model),
	.count = 4,
	.normalized = false
};


// diffuse
dm_vertex_attrib_desc diffuse_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aDiffuse",
#elif defined DM_DIRECTX
	.name = "COLOR",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_INSTANCE,
	.stride = sizeof(dm_vertex_inst_t),
	.offset = offsetof(dm_vertex_inst_t, diffuse),
	.count = 3,
	.normalized = false,
};

dm_vertex_attrib_desc specular_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aSpecular",
#elif defined DM_DIRECTX
	.name = "COLOR",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.attrib_class = DM_VERTEX_ATTRIB_CLASS_INSTANCE,
	.stride = sizeof(dm_vertex_inst_t),
	.offset = offsetof(dm_vertex_inst_t, specular),
	.count = 3,
    .index = 1,
	.normalized = false,
};

#endif