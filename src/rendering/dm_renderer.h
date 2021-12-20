#ifndef __DM_RENDERER_H__
#define __DM_RENDERER_H__

#include <stdbool.h>
#include "dm_engine_types.h"
#include "dm_render_types.h"
#include "dm_colors.h"
#include "dm_camera.h"

typedef struct dm_renderer_data
{
	dm_camera camera;
	int width, height;
	dm_color clear_color;
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
void dm_renderer_begin_scene();

/*
mainly a wrapper for the backend renderer end scene the user is not exposed to
*/
void dm_renderer_end_scene();

#endif