#include "dm_renderer.h"
#include "dm_mem.h"
#include "dm_logger.h"

static dm_renderer_data r_data = { 0 };

bool dm_renderer_create_object_pipeline();

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
bool dm_renderer_resize_impl(int new_width, int new_height);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data, dm_render_pipeline* pipeline);
void dm_renderer_delete_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_buffer_impl(dm_buffer* buffer);
bool dm_renderer_create_shader_impl(dm_shader* shader);
void dm_renderer_delete_shader_impl(dm_shader* shader);
void dm_renderer_bind_shader_impl(dm_shader* shader);

void dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline);

void dm_renderer_begin_renderpass_impl();
void dm_renderer_end_rederpass_impl();
bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_bind_vertex_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_index_buffer_impl(dm_buffer* buffer);
void dm_renderer_set_viewport_impl(dm_viewport* viewport);
void dm_renderer_clear_impl(dm_color* clear_color);

void dm_renderer_draw_arrays_impl(int first, size_t count);
void dm_renderer_draw_indexed_impl(int num, int offset);

// renderer resources; buffers, shaders, etc
static dm_render_resources resources;

// test render objects
dm_buffer_handle vb_handle = -1;
dm_buffer_handle ib_handle = -1;
dm_shader_handle s_handle = -1;

dm_command_buffer cb = { 0 };

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

	if (!dm_renderer_create_object_pipeline())
	{
		DM_LOG_FATAL("Failed to create object render pipelien!");
		return false;
	}
		
	// camera init
	dm_camera_init(
		&r_data.camera,(dm_vec3) { 0, 0, 0},
		70.0f,
		platform_data->window_width,
		platform_data->window_height,
		0, 1,
		DM_CAMERA_PERSPECTIVE
	);

	// test rendering
	dm_list_init(&cb.commands, dm_render_command);

	return true;
}

void dm_renderer_shutdown()
{
	// cleanup
	dm_renderer_destroy_render_pipeline_impl(r_data.object_pipeline);
	dm_free(r_data.object_pipeline);

	dm_list_destroy(&cb.commands);

	// backend shutdown
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
	if (cb.commands.size > 0)
	{
		dm_renderer_clear_command_buffer(&cb);
	}
	
	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, NULL, &cb);
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, &r_data.clear_color, &cb);
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, r_data.object_pipeline, &cb);
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, NULL, &cb);

	dm_renderer_begin_scene_impl(&r_data);
}

bool dm_renderer_end_scene()
{
	if (!dm_renderer_submit_command_buffer(&cb)) return false;

	dm_renderer_end_scene_impl(&r_data);

	return true;
}

void dm_renderer_draw_arrays(int first, int count)
{
	dm_renderer_draw_arrays_impl(first, count);
}

void dm_renderer_draw_indexed(int num, int offset)
{
	dm_renderer_draw_indexed_impl(num, offset);
}

bool dm_renderer_create_object_pipeline()
{
	// built-in shader
	dm_shader_desc v_desc = { 0 };
	v_desc.path = "shaders/glsl/object_vertex.glsl";
	v_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc p_desc = { 0 };
	p_desc.path = "shaders/glsl/object_pixel.glsl";
	p_desc.type = DM_SHADER_TYPE_PIXEL;

	dm_shader_handle object_shader_handle = -1;

	if (!dm_renderer_create_shader(v_desc, p_desc, &object_shader_handle))
	{
		DM_LOG_FATAL("Failed to create object shader!");
		return false;
	}

	// raster
	dm_raster_state_desc raster = { 0 };
	raster.cull_mode = DM_CULL_BACK;
	raster.winding_order = DM_WINDING_COUNTER_CLOCK;
	raster.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;
	raster.shader = object_shader_handle;

	// blend
	dm_blend_state_desc blend = { 0 };
	blend.is_enabled = true;
	blend.src = DM_BLEND_FUNC_SRC_ALPHA;
	blend.dest = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;

	// depth
	dm_depth_state_desc depth = { 0 };
	depth.is_enabled = true;
	depth.equation = DM_DEPTH_EQUATION_LESS;

	// stencil
	dm_stencil_state_desc stencil = { 0 };

	// vertex layout
	dm_vertex_attrib_desc v_attribs[] = {
		(dm_vertex_attrib_desc) {"aPos", DM_VERTEX_ATTRIB_POS, 3 * sizeof(float), 0},
	};
	dm_vertex_layout v_layout = { v_attribs, sizeof(v_attribs), 1 };

	// viewport
	dm_viewport viewport = { 0 };
	viewport.y = r_data.height;
	viewport.width = r_data.width;
	viewport.height = r_data.height;
	viewport.max_depth = 1.0f;

	// make pipeline
	r_data.object_pipeline = (dm_render_pipeline*)dm_alloc(sizeof(dm_render_pipeline));
	dm_memzero(r_data.object_pipeline, sizeof(dm_render_pipeline));

	r_data.object_pipeline->raster_desc = raster;
	r_data.object_pipeline->blend_desc = blend;
	r_data.object_pipeline->depth_desc = depth;
	r_data.object_pipeline->stencil_desc = stencil;
	r_data.object_pipeline->vertex_layout = v_layout;
	r_data.object_pipeline->viewport = viewport;

	// internal pipeline
	dm_renderer_create_render_pipeline_impl(r_data.object_pipeline);

	return true;
}

bool dm_renderer_create_buffer(dm_buffer_desc desc, void* data, dm_buffer_handle* handle)
{
	for (dm_buffer_handle h=0; h < MAX_RENDER_RESOURCES; h++)
	{
		if (!resources.buffers[h])
		{
			*handle = h;
			resources.buffers[h] = (dm_buffer*)dm_alloc(sizeof(dm_buffer));
			dm_memzero(resources.buffers[h], sizeof(dm_buffer));
			resources.buffers[h]->desc = desc;
			return dm_renderer_create_buffer_impl(resources.buffers[h], data, r_data.object_pipeline);
		}
	}
	DM_LOG_ERROR("Failed to find valid buffer handle!");
	return false;
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

dm_buffer* dm_renderer_get_buffer(dm_buffer_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.buffers[handle])
	{
		return resources.buffers[handle];
	}

	DM_LOG_ERROR("Trying to retreive invalid buffer!");
	return NULL;
}

bool dm_renderer_create_shader(dm_shader_desc v_desc, dm_shader_desc p_desc, dm_shader_handle* handle)
{
	for (dm_shader_handle h=0; h < MAX_RENDER_RESOURCES; h++)
	{
		if (!resources.shaders[h])
		{
			*handle = h;
			resources.shaders[h] = (dm_shader*)dm_alloc(sizeof(dm_shader));
			dm_memzero(resources.shaders[h], sizeof(dm_shader));
			resources.shaders[h]->vertex_desc = v_desc;
			resources.shaders[h]->pixel_desc = p_desc;
			return dm_renderer_create_shader_impl(resources.shaders[h]);
		}
	}
	DM_LOG_ERROR("Failed to find valid shader handle!");
	return false;
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

dm_shader* dm_renderer_get_shader(dm_shader_handle handle)
{
	if (handle >= 0 && handle < MAX_RENDER_RESOURCES && resources.shaders[handle])
	{
		return resources.shaders[handle];
	}
	else
	{
		DM_LOG_ERROR("Trying to access invalid shader!");
		return NULL;
	}
}

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, dm_command_buffer* command_buffer)
{
	dm_render_command command = { .command = command_type, .data = data };
	dm_list_append(&command_buffer->commands, command);
}

void dm_renderer_clear_command_buffer(dm_command_buffer* command_buffer)
{
	dm_list_clear(&command_buffer->commands);
}

void dm_renderer_destroy_command_buffer(dm_command_buffer* command_buffer)
{
	dm_list_destroy(&command_buffer->commands);
}

bool dm_renderer_submit_command_buffer(dm_command_buffer* command_buffer)
{
	dm_list_for_range(command_buffer->commands)
	{
		dm_render_command command = command_buffer->commands.array[i];

		switch (command.command)
		{
		// TODO flesh out
		case DM_RENDER_COMMAND_BEGIN_RENDER_PASS:
		{
			continue;
		} break;
		case DM_RENDER_COMMAND_END_RENDER_PASS:
		{
			dm_renderer_end_rederpass_impl();
		} break;
		case DM_RENDER_COMMAND_SET_VIEWPORT:
		{
			dm_renderer_set_viewport_impl((dm_viewport*)command.data);
		} break;
		case DM_RENDER_COMMAND_CLEAR:
		{
			dm_renderer_clear_impl((dm_color*)command.data);
		} break;
		case DM_RENDER_COMMAND_BIND_PIPELINE:
		{
			if (!dm_renderer_bind_pipeline_impl((dm_render_pipeline*)command.data)) return false;
		} break;
		case DM_RENDER_COMMAND_SUBMIT_VERTEX_BUFFER:
		{} break;
		case DM_RENDER_COMMAND_SUBMIT_INDEX_BUFFER:
		{} break;
		case DM_RENDER_COMMAND_BIND_SHADER:
		{} break;
		case DM_RENDER_COMMAND_DRAW_INDEXED:
		{} break;
		case DM_RENDER_COMMAND_DRAW_INSTANCED:
		{} break;
		}
	}

	return true;
}