#include "dm_renderer.h"

static dm_renderer_data r_data = { 0 };

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry
bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl();
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;
	r_data.width = platform_data->window_width;
	r_data.height = platform_data->window_height;

	dm_camera_init(
		&r_data.camera,(dm_vec3) { 0, 0, 0},
		70.0f,
		platform_data->window_width,
		platform_data->window_height,
		0, 1,
		DM_CAMERA_PERSPECTIVE
	);

	return dm_renderer_init_impl(&r_data);
}

void dm_renderer_shutdown()
{
	dm_renderer_shutdown_impl();
}

bool dm_renderer_resize(int new_width, int new_height)
{
	r_data.width = new_width;
	r_data.height = new_height;

	return dm_renderer_resize_impl(new_width, new_height);
}

void dm_renderer_begin_scene()
{
	dm_renderer_begin_scene_impl(&r_data);
}

void dm_renderer_end_scene()
{
	dm_renderer_end_scene_impl(&r_data);
}