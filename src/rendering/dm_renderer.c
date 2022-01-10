#include "dm_renderer.h"
#include "dm_mem.h"
#include "dm_logger.h"

static dm_renderer_data r_data = { 0 };

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

void dm_renderer_delete_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_buffer_impl(dm_buffer* buffer);
void dm_renderer_delete_shader_impl(dm_shader* shader);
void dm_renderer_bind_shader_impl(dm_shader* shader);

void dm_renderer_draw_arrays_impl(int first, size_t count);
void dm_renderer_draw_indexed_impl(int num, int offset);

// renderer resources; buffers, shaders, etc
static dm_render_resources resources;

// test render objects
dm_buffer_handle vb_handle = -1;
dm_buffer_handle ib_handle = -1;
dm_shader_handle s_handle = -1;

dm_shader_handle object_shader_handle = -1;

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;
	r_data.width = platform_data->window_width;
	r_data.height = platform_data->window_height;

	if(!dm_renderer_init_impl(&r_data))
	{
		DM_LOG_FATAL("Renderer backend could not be initialized!");
		return false;
	}

	dm_camera_init(
		&r_data.camera,(dm_vec3) { 0, 0, 0},
		70.0f,
		platform_data->window_width,
		platform_data->window_height,
		0, 1,
		DM_CAMERA_PERSPECTIVE
	);

	// test rendering

	return true;
}

void dm_renderer_shutdown()
{
	// cleanup

	dm_renderer_shutdown_impl(&r_data);
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

void dm_renderer_draw_arrays(int first, int count)
{
	dm_renderer_draw_arrays_impl(first, count);
}

void dm_renderer_draw_indexed(int num, int offset)
{
	dm_renderer_draw_indexed_impl(num, offset);
}

void dm_renderer_delete_buffer(dm_buffer_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.buffers[handle])
	{
		dm_renderer_delete_buffer_impl(resources.buffers[handle]);
		dm_free(resources.buffers[handle]);
		resources.buffers[handle] = NULL;
	}
	else
	{
		DM_LOG_ERROR("Trying to delete invalid buffer!");
	}
}

void dm_renderer_bind_buffer(dm_buffer_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.buffers[handle])
	{
		dm_renderer_bind_buffer_impl(resources.buffers[handle]);
	}
	else
	{
		DM_LOG_ERROR("Trying to bind invalid buffer!");
	}
}

void dm_renderer_delete_shader(dm_shader_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.shaders[handle])
	{
		dm_renderer_delete_shader_impl(resources.shaders[handle]);
		dm_free(resources.shaders[handle]);
		resources.shaders[handle] = NULL;
	}
	else
	{
		DM_LOG_ERROR("Trying to delete invalid shader!");
	}
}

void dm_renderer_bind_shader(dm_shader_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.shaders[handle])
	{
		dm_renderer_bind_shader_impl(resources.shaders[handle]);
	}
	else
	{
		DM_LOG_ERROR("Trying to bind invalid shader!");
	}
}