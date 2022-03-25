#ifndef __DM_RENDERER_API_H__
#define __DM_RENDERER_API_H__

#include "rendering/dm_render_types.h"
#include "core/dm_defines.h"
#include "core/dm_math_types.h"
#include "ecs/dm_components.h"
#include <stdint.h>
#include <stdbool.h>

//submitting functions

DM_API void dm_renderer_api_submit_vertex_data(const char* tag, dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices);

DM_API bool dm_renderer_api_submit_images(dm_image_desc* image_descs, uint32_t num_desc);

DM_API bool dm_renderer_api_submit_objects(dm_list* objects);

DM_API void dm_renderer_api_set_clear_color(dm_vec3 color);

// updating functions

DM_API void dm_renderer_api_set_camera_pos(dm_vec3 pos);
DM_API void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos);

DM_API void dm_renderer_api_set_camera_forward(dm_vec3 forward);
DM_API void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward);

DM_API void dm_renderer_api_set_camera_euler(dm_vec3 delta_euler);
DM_API void dm_renderer_api_update_camera_euler(dm_vec3 delta_euler);

// getting functions

DM_API dm_vec3 dm_renderer_api_get_camera_forward();
DM_API dm_vec3 dm_renderer_api_get_camera_up();
DM_API dm_vec3 dm_renderer_api_get_camera_pos();

#endif