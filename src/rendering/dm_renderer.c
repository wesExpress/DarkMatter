#include "dm_renderer.h"
#include "dm_command_buffer.h"
#include "dm_mem.h"
#include "dm_logger.h"

static dm_renderer_data r_data = { 0 };

bool dm_renderer_create_object_pipeline();
void dm_renderer_destroy_object_pipeline();
bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline);

bool dm_renderer_init_object_data();
bool dm_renderer_init_object_data_impl(void* vertex_data, void* index_data, dm_render_pipeline* pipeline);

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data, dm_render_pipeline* pipeline);
void dm_renderer_delete_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_buffer_impl(dm_buffer* buffer);
bool dm_renderer_create_shader_impl(dm_shader* shader);
void dm_renderer_delete_shader_impl(dm_shader* shader);
void dm_renderer_bind_shader_impl(dm_shader* shader);

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline);

// renderer resources; buffers, shaders, etc
static dm_render_resources resources;

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

	if (!dm_renderer_init_object_data())
	{
		DM_LOG_FATAL("Could not initialize object data!");
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

	return true;
}

void dm_renderer_shutdown()
{
	// cleanup
	dm_renderer_destroy_object_pipeline();

	// backend shutdown
	dm_renderer_shutdown_impl(&r_data);
}

bool dm_renderer_resize(int new_width, int new_height)
{
	r_data.width = new_width;
	r_data.height = new_height;

	dm_viewport viewport = {
		.x = 0,
		.y = new_height,
		.width = new_width,
		.height = new_height,
		.max_depth = 1.0f
	};
	r_data.object_pipeline->viewport = viewport;
	dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, NULL, &r_data.object_pipeline->command_buffer);

	return true;
}

void dm_renderer_begin_scene()
{
	if (r_data.object_pipeline->command_buffer.commands.size > 0)
	{
		dm_renderer_clear_command_buffer(&r_data.object_pipeline->command_buffer);
	}
	
	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, NULL, &r_data.object_pipeline->command_buffer);
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, &r_data.clear_color, &r_data.object_pipeline->command_buffer);
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, r_data.object_pipeline, &r_data.object_pipeline->command_buffer);
	
	// TODO REMOVE
	// test rendering
	dm_draw_indexed_params params = { .count = 6, .offset = 0 };
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, &params, &r_data.object_pipeline->command_buffer);

	dm_renderer_begin_scene_impl(&r_data);
}

bool dm_renderer_end_scene()
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, NULL, &r_data.object_pipeline->command_buffer);

	if (!dm_renderer_submit_command_buffer(&r_data.object_pipeline->command_buffer, r_data.object_pipeline)) return false;

	dm_renderer_end_scene_impl(&r_data);

	return true;
}

bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline)
{
	// command buffer
	dm_list_init(&pipeline->command_buffer.commands, dm_render_command);

	return dm_renderer_create_render_pipeline_impl(pipeline);
}

void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline)
{
	dm_list_destroy(&pipeline->command_buffer.commands);

	dm_renderer_destroy_render_pipeline_impl(pipeline);
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
	blend.is_enabled = false;
	blend.src = DM_BLEND_FUNC_SRC_ALPHA;
	blend.dest = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;

	// depth
	dm_depth_state_desc depth = { 0 };
	depth.is_enabled = false;
	depth.equation = DM_DEPTH_EQUATION_LESS;

	// stencil
	dm_stencil_state_desc stencil = { 0 };

	// viewport
	dm_viewport viewport = { 0 };
	viewport.y = r_data.height;
	viewport.width = r_data.width;
	viewport.height = r_data.height;
	viewport.max_depth = 1.0f;

	// make pipeline
	r_data.object_pipeline = (dm_render_pipeline*)dm_alloc(sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);

	r_data.object_pipeline->raster_desc = raster;
	r_data.object_pipeline->blend_desc = blend;
	r_data.object_pipeline->depth_desc = depth;
	r_data.object_pipeline->stencil_desc = stencil;
	r_data.object_pipeline->viewport = viewport;

	// internal pipeline
	dm_renderer_init_render_pipeline(r_data.object_pipeline);

	return true;
}

void dm_renderer_destroy_object_pipeline()
{
	dm_renderer_delete_shader(r_data.object_pipeline->raster_desc.shader);
	dm_renderer_delete_buffer(r_data.object_pipeline->render_packet.vertex_buffer);
	dm_renderer_delete_buffer(r_data.object_pipeline->render_packet.index_buffer);
	dm_renderer_destroy_render_pipeline(r_data.object_pipeline);

	dm_free(r_data.object_pipeline, sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_object_data()
{
	// TODO should just be a palceholder for now!
	// likely need to reed in files here in the future

	dm_vertex_t vertices[] = {
		{0.5f,   0.5f, 0.0f},
		{0.5f,  -0.5f, 0.0f},
		{-0.5f, -0.5f, 0.0f},
		{-0.5f,  0.5f, 0.0f}
	};
	
	dm_index_t indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	// buffers
	dm_buffer_desc v_desc = { .type = DM_BUFFER_TYPE_VERTEX, .size = sizeof(vertices), .usage=DM_BUFFER_USAGE_STATIC };
	dm_buffer_desc i_desc = { .type = DM_BUFFER_TYPE_INDEX, .size = sizeof(indices), .usage=DM_BUFFER_USAGE_STATIC };

	if (!dm_renderer_create_buffer(v_desc, &r_data.object_pipeline->render_packet.vertex_buffer)) return false;
	if (!dm_renderer_create_buffer(i_desc, &r_data.object_pipeline->render_packet.index_buffer)) return false;

	// vertex layout
	dm_vertex_attrib_desc v_attribs[] = {
		(dm_vertex_attrib_desc) {
		.name = "aPos",
		.data_t = DM_VERTEX_DATA_T_FLOAT,
		.size = 3,
		.stride = sizeof(dm_vertex),
		.offset = offsetof(dm_vertex, position),
		.normalized = false},
	};

	dm_vertex_layout v_layout = {
		.attributes = v_attribs,
		.num = 1
	};

	r_data.object_pipeline->vertex_layout = v_layout;

	return dm_renderer_init_object_data_impl(vertices, indices, r_data.object_pipeline);
}

bool dm_renderer_create_buffer(dm_buffer_desc desc, dm_buffer_handle* handle)
{
	for (dm_buffer_handle h=0; h < MAX_RENDER_RESOURCES; h++)
	{
		if (!resources.buffers[h])
		{
			*handle = h;
			resources.buffers[h] = (dm_buffer*)dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
			resources.buffers[h]->desc = desc;
			return true;
			//return dm_renderer_create_buffer_impl(resources.buffers[h], data, r_data.object_pipeline);
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
		dm_free(resources.buffers[handle], sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
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
			resources.shaders[h] = (dm_shader*)dm_alloc(sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
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
		dm_free(resources.shaders[handle], sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
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