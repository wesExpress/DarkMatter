#include "dm_renderer.h"
#include "dm_mem.h"
#include "dm_logger.h"

static dm_renderer_data r_data = { 0 };

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_create_quad_impl(dm_buffer* buffer, void* b_data, int num_v_attribs, dm_vertex_attrib* v_attribs, dm_buffer* i_buffer, void* ib_data, dm_shader* shader);

bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_buffers_impl(dm_renderer_data* renderer_data, dm_buffer_desc v_desc, dm_buffer_desc i_desc);
void dm_renderer_delete_buffers_impl(dm_renderer_data* renderer_data);
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
	//if(!dm_renderer_create_quad(&vb_handle, &ib_handle, &s_handle)) return false;

	return true;
}

void dm_renderer_shutdown()
{
	// cleanup
	//dm_renderer_delete_buffer(vb_handle);
	//dm_renderer_delete_shader(s_handle);

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

	//dm_renderer_bind_shader(s_handle);
	//dm_renderer_bind_buffer(vb_handle);
	//dm_renderer_bind_buffer(ib_handle);

	//dm_renderer_draw_arrays(0, 3);
	//dm_renderer_draw_indexed(6, 0);
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

bool dm_renderer_create_quad(dm_buffer_handle* vb_handle, dm_buffer_handle* ib_handle, dm_shader_handle* qs_handle)
{
	for (dm_buffer_handle h=0; h<MAX_RENDER_RESOURCES; h++)
	{
		if(!resources.buffers[h]) 
		{
			resources.buffers[h] = (dm_buffer*)dm_alloc(sizeof(dm_buffer));
			*vb_handle = h;
			break;
		}
		if(h==MAX_RENDER_RESOURCES-1)
		{
			DM_LOG_FATAL("Can't find valid vertex buffer handle!");
			return false;
		}
	}
	for (dm_buffer_handle h=0; h<MAX_RENDER_RESOURCES; h++)
	{
		if(!resources.buffers[h]) 
		{
			resources.buffers[h] = (dm_buffer*)dm_alloc(sizeof(dm_buffer));
			*ib_handle = h;
			break;
		}
		if(h==MAX_RENDER_RESOURCES-1)
		{
			DM_LOG_FATAL("Can't find valid index buffer handle!");
			return false;
		}
	}

	for(dm_shader_handle h=0; h<MAX_RENDER_RESOURCES; h++)
	{
		if(!resources.shaders[h]) 
		{
			resources.shaders[h] = (dm_shader*)dm_alloc(sizeof(dm_shader));
			*qs_handle =h;
			break;
		}

		if(h==MAX_RENDER_RESOURCES-1)
		{
			DM_LOG_FATAL("Can't find valid shader handle!");
			return false;
		}
	}

	// buffer data
	float vertices[] =
	{
		0.5f,  0.5f, 0.0f,  // top right
		0.5f, -0.5f, 0.0f,  // bottom right
	   -0.5f, -0.5f, 0.0f,  // bottom left
	   -0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] =
	{
		0, 1, 3,
		1, 2, 3
	};

	dm_vertex_attrib v_attribs[] = {
		DM_VERTEX_ATTRIB_POS
	};

	dm_buffer* v_buffer = resources.buffers[*vb_handle];
	dm_buffer_desc vb_desc = {0};
	vb_desc.type = DM_BUFFER_TYPE_VERTEX;
	vb_desc.usage = DM_BUFFER_USAGE_STATIC;
	vb_desc.data_size = sizeof(vertices);

	dm_buffer* i_buffer = resources.buffers[*ib_handle];
	dm_buffer_desc ib_desc = {0};
	ib_desc.type = DM_BUFFER_TYPE_INDEX;
	ib_desc.usage = DM_BUFFER_USAGE_STATIC;
	ib_desc.data_size = sizeof(indices);

	// shader
	dm_shader* shader = resources.shaders[*qs_handle];
	dm_shader_desc v_desc = { 0 };
	v_desc.path = "shaders/glsl/v_basic.glsl";
	v_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc p_desc = { 0 };
	p_desc.path = "shaders/glsl/f_basic.glsl";
	p_desc.type = DM_SHADER_TYPE_PIXEL;

	v_buffer->desc = vb_desc;
	i_buffer->desc = ib_desc;
	shader->vertex_desc = v_desc;
	shader->pixel_desc = p_desc;

	if(!dm_renderer_create_quad_impl(v_buffer, vertices, 1, v_attribs, i_buffer, indices, shader)) return false;

	return true;
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

bool dm_renderer_create_buffers()
{
	dm_buffer_desc v_desc = { 0 };
	v_desc.usage = DM_BUFFER_USAGE_DEFAULT;
	v_desc.type = DM_BUFFER_TYPE_VERTEX;
	v_desc.data_size = sizeof(dm_vertex_3d) * 1024 * 1024;

	dm_buffer_desc i_desc = { 0 };
	i_desc.usage = DM_BUFFER_USAGE_DEFAULT;
	i_desc.type = DM_BUFFER_TYPE_INDEX;
	i_desc.data_size = sizeof(uint32_t) * 1024 * 1024;

	if (!dm_renderer_create_buffers_impl(&r_data, v_desc, i_desc))
	{
		DM_LOG_ERROR("Creating buffers failed!");
		return false;
	}
	return false;
}

void dm_renderer_delete_buffers()
{
	dm_renderer_delete_buffers_impl(&r_data);
}