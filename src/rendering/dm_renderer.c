#include "dm_renderer.h"

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry
bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_color clear_color);
void dm_renderer_shutdown_impl();
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl();
void dm_renderer_end_scene_impl();

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	return dm_renderer_init_impl(platform_data, clear_color);
}

void dm_renderer_shutdown()
{
	dm_renderer_shutdown_impl();
}

bool dm_renderer_resize(int new_width, int new_height)
{
	return dm_renderer_resize_impl(new_width, new_height);
}

void dm_renderer_begin_scene()
{
	dm_renderer_begin_scene_impl();
}

void dm_renderer_end_scene()
{
	dm_renderer_end_scene_impl();
}