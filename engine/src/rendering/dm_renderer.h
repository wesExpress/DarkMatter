#ifndef __DM_RENDERER_H__
#define __DM_RENDERER_H__

#include <stdbool.h>
#include "core/dm_engine_types.h"
#include "ecs/dm_components.h"
#include "dm_render_types.h"
#include "dm_colors.h"
#include "dm_camera.h"

#define MAX_RENDER_RESOURCES 100

typedef struct dm_renderer_data
{
	dm_camera camera;
	dm_color clear_color;
	dm_list* render_commands;
	dm_viewport viewport;
    
	dm_render_pipeline* pipeline;
} dm_renderer_data;

/*
mainly a wrapper for the backend renderer initialization the user is not exposed to.
creates and initializes the camera as well.

@param platform_data
@param clear_color - vec4 to specify the initial clear color
*/
bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color);

/*
mainly a wrapper for the backend renderer shutdown the user is not exposed to
*/
void dm_renderer_shutdown();

/*
mainly a wrapper for the backend renderer resizing the user is not exposed to

@param new_width - new window width
@param new_height - new window height
*/
void dm_renderer_resize(int new_width, int new_height);

/*
mainly a wrapper for the backend renderer begin scene the user is not exposed to
*/
bool dm_renderer_begin_frame();

/*
mainly a wrapper for the backend renderer end scene the user is not exposed to
*/
bool dm_renderer_end_frame();

/*
initializes the vertex and index buffers. called after the application has submitted everything.
*/
bool dm_renderer_init_object_data();

/*
default render passes:
- objects
*/
bool dm_renderer_create_default_render_passes();

bool dm_renderer_create_render_pass(dm_shader shader, dm_vertex_layout v_layout, dm_uniform* uniforms, uint32_t num_uniforms, const char* tag);

#endif