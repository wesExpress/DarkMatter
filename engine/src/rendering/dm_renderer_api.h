#ifndef __DM_RENDERER_API_H__
#define __DM_RENDERER_API_H__

#include "rendering/dm_render_types.h"
#include "core/dm_defines.h"
#include "core/math/dm_math_types.h"
#include <stdint.h>
#include <stdbool.h>

//submitting functions

DM_API void dm_renderer_api_submit_vertex_data(dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices);

DM_API bool dm_renderer_api_submit_textures(dm_image_desc* image_descs, uint32_t num_desc);

// updating functions

DM_API void dm_renderer_api_set_camera_pos(dm_vec3 pos);
DM_API void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos);

DM_API void dm_renderer_api_set_camera_forward(dm_vec3 forward);
DM_API void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward);

DM_API void dm_renderer_api_set_camera_euler(dm_vec3 delta_euler);
DM_API void dm_renderer_api_update_camera_euler(dm_vec3 delta_euler);

#endif