#include "dm_renderer.h"
#include "dm_mem.h"
#include "dm_logger.h"

static dm_renderer_data r_data = { 0 };

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry
bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl();
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

void dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data);
void dm_renderer_delete_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_buffer_impl(dm_buffer* buffer);
void dm_renderer_create_shader_impl(dm_shader* shader, dm_vertex_layout_type vertex_layout);
void dm_renderer_delete_shader_impl(dm_shader* shader);
void dm_renderer_bind_shader_impl(dm_shader* shader);

void dm_renderer_draw_arrays_impl(int first, size_t count);

// renderer resources; buffers, shaders, etc
static dm_render_resources resources;

// test render objects
dm_buffer_handle b_handle = -1;
dm_shader_handle s_handle = -1;

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
	float vertices[] =
	{
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	dm_buffer_desc b_desc = { 0 };
	b_desc.type = DM_BUFFER_TYPE_VERTEX;
	b_desc.usage = DM_BUFFER_USAGE_STATIC;
	b_desc.data_type = DM_BUFFER_DATA_FLOAT;
	b_desc.num_v_elements = 3;
	b_desc.elem_size = sizeof(float);
	b_desc.data_size = sizeof(vertices);
	dm_renderer_create_buffer(b_desc, vertices, &b_handle);

	// shader
	dm_shader_desc v_desc = { 0 };
	v_desc.path = "shaders/glsl/v_basic.glsl";
	v_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc p_desc = { 0 };
	p_desc.path = "shaders/glsl/f_basic.glsl";
	p_desc.type = DM_SHADER_TYPE_PIXEL;

	dm_renderer_create_shader(v_desc, p_desc, 0, &s_handle);

	return true;
}

void dm_renderer_shutdown()
{
	// cleanup
	dm_renderer_delete_buffer(b_handle);
	dm_renderer_delete_shader(s_handle);

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

	dm_renderer_bind_shader(s_handle);
	dm_renderer_bind_buffer(b_handle);

	dm_renderer_draw_arrays(0, 3);
}

void dm_renderer_end_scene()
{
	dm_renderer_end_scene_impl(&r_data);
}

void dm_renderer_draw_arrays(int first, int count)
{
	dm_renderer_draw_arrays_impl(first, count);
}

void dm_renderer_create_buffer(dm_buffer_desc desc, void* data, dm_buffer_handle* handle)
{
	for (dm_buffer_handle h=0; h < MAX_RENDER_RESOURCES; h++)
	{
		if (!resources.buffers[h])
		{
			*handle = h;

			dm_buffer* buffer = (dm_buffer*)dm_alloc(sizeof(dm_buffer));
			buffer->desc = desc;

			dm_renderer_create_buffer_impl(buffer, data);

			resources.buffers[h] = buffer;
			break;
		}
	}
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

void dm_renderer_create_shader(dm_shader_desc v_desc, dm_shader_desc p_desc, dm_vertex_layout_type vertex_layout, dm_shader_handle* handle)
{
	for (dm_shader_handle h = 0; h < MAX_RENDER_RESOURCES; h++)
	{
		if (!resources.shaders[h])
		{
			*handle = h;

			dm_shader* shader = (dm_shader*)dm_alloc(sizeof(dm_shader));

			shader->vertex_desc = v_desc;
			shader->pixel_desc = p_desc;

			dm_renderer_create_shader_impl(shader, vertex_layout);

			resources.shaders[h] = shader;
			break;
		}
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