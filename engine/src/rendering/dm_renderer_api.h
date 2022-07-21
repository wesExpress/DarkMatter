#ifndef __DM_RENDERER_API_H__
#define __DM_RENDERER_API_H__

#include "rendering/dm_render_types.h"
#include "core/dm_defines.h"
#include "core/dm_math_types.h"
#include "ecs/dm_ecs.h"
#include "ecs/dm_components.h"

#include <stdint.h>
#include <stdbool.h>

//submitting functions

DM_API void dm_renderer_api_submit_vertex_data(const char* tag, dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices, bool is_indexed);

DM_API void dm_renderer_api_set_clear_color(dm_vec3 color);

DM_API bool dm_renderer_api_register_mesh(dm_entity entity, dm_mesh_component* component);
DM_API bool dm_renderer_api_deregister_mesh(dm_entity entity);

DM_API bool dm_renderer_api_register_image(const char* path, const char* name, bool flip, dm_texture_format format, dm_texture_format internal_format);

// updating functions

DM_API void dm_renderer_api_set_camera_pos(dm_vec3 pos);
DM_API void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos);

DM_API void dm_renderer_api_set_camera_forward(dm_vec3 forward);
DM_API void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward);

// getting functions

DM_API dm_vec3 dm_renderer_api_get_camera_forward();
DM_API dm_vec3 dm_renderer_api_get_camera_up();
DM_API dm_vec3 dm_renderer_api_get_camera_pos();

#endif