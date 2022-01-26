#ifndef __DM_RENDERER_H__
#define __DM_RENDERER_H__

#include <stdbool.h>
#include "core/dm_engine_types.h"
#include "dm_render_types.h"
#include "dm_colors.h"
#include "dm_camera.h"

#define MAX_RENDER_RESOURCES 100

typedef struct dm_renderer_data
{
	dm_camera camera;
	int width, height;
	dm_color clear_color;

	dm_render_pipeline* object_pipeline;
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
bool dm_renderer_resize(int new_width, int new_height);

/*
mainly a wrapper for the backend renderer begin scene the user is not exposed to
*/
bool dm_renderer_begin_scene();

/*
mainly a wrapper for the backend renderer end scene the user is not exposed to
*/
bool dm_renderer_end_scene();

// render object functions
//bool dm_renderer_create_buffer(dm_buffer_desc desc, dm_buffer_handle* handle);
//void dm_renderer_delete_buffer(dm_buffer_handle handle);
//void dm_renderer_bind_buffer(dm_buffer_handle handle);
//bool dm_renderer_create_shader(dm_shader_desc v_desc, dm_shader_desc p_desc, dm_shader_handle* handle);
//void dm_renderer_delete_shader(dm_shader_handle handle);
//void dm_renderer_bind_shader(dm_shader_handle handle);
//
//dm_buffer* dm_renderer_get_buffer(dm_buffer_handle handle);
//dm_shader* dm_renderer_get_shader(dm_shader_handle handle);

#endif