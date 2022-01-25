#include "dm_renderer.h"
#include "dm_command_buffer.h"
#include "dm_texture.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include <stddef.h>

static dm_renderer_data r_data = { 0 };

bool dm_renderer_create_object_pipeline();
bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline);
bool dm_renderer_init_object_data();

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline);

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline);

/*
// vertex attributes
*/

// position
dm_vertex_attrib_desc pos_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aPos",
#elif defined DM_DIRECTX
	.name = "POSITION",
#endif
	.data_t =  DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, position),
	.count = 3,
	.normalized = false,
};

// color
dm_vertex_attrib_desc color_attrib_desc = {
#ifdef DM_OPENGL
	.name = "aColor",
#elif defined DM_DIRECTX
	.name = "COLOR",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, color),
	.count = 3,
	.normalized = false,
};

// texture coords
dm_vertex_attrib_desc tex_coord_desc = {
#ifdef DM_OPENGL
	.name = "aTexCoords",
#elif defined DM_DIRECTX
	.name = "TEXCOORD",
#endif
	.data_t = DM_VERTEX_DATA_T_FLOAT,
	.stride = sizeof(dm_vertex_t),
	.offset = offsetof(dm_vertex_t, tex_coords),
	.count = 2,
	.normalized = false
};

/*
// constant buffer data
*/
dm_vec3 offset = { 0, 0, 0 };

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;
	r_data.width = platform_data->window_width;
	r_data.height = platform_data->window_height;
	r_data.object_pipeline = dm_alloc(sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);

#ifdef DM_OPENGL
	char* backend = "OpenGL";
#elif defined DM_DIRECTX
	char* backend = "DirectX 11";
#elif defined DM_METAL
	char* backend = "Metal";
#endif
	DM_LOG_INFO("Initializing %s renderer backed", backend);

	if(!dm_renderer_init_impl(platform_data, &r_data))
	{
		DM_LOG_FATAL("Renderer backend could not be initialized!");
		return false;
	}

	dm_texture_map_init();
	if (!dm_renderer_create_object_pipeline())
	{
		DM_LOG_FATAL("Failed to create object render pipeline!");
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
	dm_renderer_destroy_render_pipeline(r_data.object_pipeline);
	dm_free(r_data.object_pipeline, sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);
	dm_texture_map_destroy();

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
	dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, NULL, r_data.object_pipeline->render_commands);

	return true;
}

void dm_renderer_begin_scene()
{
	dm_renderer_begin_scene_impl(&r_data);

	// commands for object pipeline
	if (!dm_list_is_empty(r_data.object_pipeline->render_commands))
	{
		dm_renderer_clear_command_buffer(r_data.object_pipeline->render_commands);
	}

	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, NULL, r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, &r_data.clear_color, r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, r_data.object_pipeline, r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, NULL, r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, NULL, r_data.object_pipeline->render_commands);
}

bool dm_renderer_end_scene()
{
	if (!dm_renderer_submit_command_buffer(r_data.object_pipeline->render_commands, r_data.object_pipeline)) return false;

	return dm_renderer_end_scene_impl(&r_data);
}

bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline)
{
	// command buffer
	pipeline->render_commands = dm_list_create(sizeof(dm_render_command), 0);

	// rasterizer
	pipeline->raster_desc.shader = dm_alloc(sizeof(dm_shader), DM_MEM_RENDERER_SHADER);

	// render packet
	pipeline->render_packet.vertex_buffer = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	pipeline->render_packet.index_buffer = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	pipeline->render_packet.constant_buffers = dm_list_create(sizeof(dm_constant_buffer), 0);
	pipeline->render_packet.texture_paths = dm_list_create(sizeof(dm_string), 0);

	pipeline->render_packet.count = 0;
	pipeline->render_packet.offset = 0;

	return dm_renderer_create_render_pipeline_impl(pipeline);
}

void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline)
{
	dm_list_destroy(pipeline->render_commands);

	dm_renderer_destroy_render_pipeline_impl(pipeline);

	dm_free(pipeline->render_packet.vertex_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->render_packet.index_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	
	// constant buffers
	dm_list_destroy(pipeline->render_packet.constant_buffers);

	// textures
	for(uint32_t i=0; i<pipeline->render_packet.texture_paths->count; i++)
	{
		dm_string* str = dm_list_at(pipeline->render_packet.texture_paths, i);
		dm_strdel(str->string);
	}
	dm_list_destroy(pipeline->render_packet.texture_paths);

	dm_free(pipeline->raster_desc.shader, sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
}

bool dm_renderer_create_object_pipeline()
{
	// raster
	dm_raster_state_desc raster = { 0 };
	raster.cull_mode = DM_CULL_BACK;
	raster.winding_order = DM_WINDING_COUNTER_CLOCK;
	raster.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;

	// blend
	dm_blend_state_desc blend = { 0 };
	blend.is_enabled = false;
	blend.src = DM_BLEND_FUNC_SRC_ALPHA;
	blend.dest = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;

	// depth
	dm_depth_state_desc depth = { 0 };
	depth.is_enabled = false;
	depth.comparison = DM_COMPARISON_LESS;

	// stencil
	dm_stencil_state_desc stencil = { 0 };

	// sampler
	dm_sampler_desc sampler = { 0 };
	sampler.comparison = DM_COMPARISON_ALWAYS;
	sampler.filter = DM_FILTER_LINEAR;
	sampler.u = DM_TEXTURE_MODE_WRAP;
	sampler.v = DM_TEXTURE_MODE_WRAP;
	sampler.w = DM_TEXTURE_MODE_WRAP;

	// viewport
	dm_viewport viewport = { 0 };
	viewport.y = r_data.height;
	viewport.width = r_data.width;
	viewport.height = r_data.height;
	viewport.max_depth = 1.0f;

	// fill-in pipeline
	r_data.object_pipeline->raster_desc = raster;
	r_data.object_pipeline->blend_desc = blend;
	r_data.object_pipeline->depth_desc = depth;
	r_data.object_pipeline->stencil_desc = stencil;
	r_data.object_pipeline->sampler_desc = sampler;
	r_data.object_pipeline->viewport = viewport;

	// internal pipeline
	dm_renderer_init_render_pipeline(r_data.object_pipeline);

	return true;
}

bool dm_renderer_init_object_data()
{
	// built-in shader
	dm_shader_desc vs_desc = { 0 };
#ifdef DM_OPENGL
	vs_desc.path = "shaders/glsl/object_vertex.glsl";
#elif defined DM_DIRECTX
	vs_desc.path = "shaders/hlsl/object_vertex.fxc";
#endif
	vs_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc ps_desc = { 0 };
#ifdef DM_OPENGL
	ps_desc.path = "shaders/glsl/object_pixel.glsl";
#elif defined DM_DIRECTX
	ps_desc.path = "shaders/hlsl/object_pixel.fxc";
#endif
	ps_desc.type = DM_SHADER_TYPE_PIXEL;

	r_data.object_pipeline->raster_desc.shader->vertex_desc = vs_desc;
	r_data.object_pipeline->raster_desc.shader->pixel_desc = ps_desc;

	// TODO should just be a palceholder for now!
	// likely need to reed in files here in the future

	// triangle data
	dm_vertex_t vertices[] = {
		{ {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
		{ { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
		{ { 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
		{ {-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
	};

#ifdef DM_OPENGL
	dm_index_t indices[] = {
		0, 1, 2,
		2, 3, 0
	};
#elif defined DM_DIRECTX
	dm_index_t indices[] = {
		0, 2, 1,
		2, 0, 3
	};
#endif

	// buffers
	dm_buffer_desc vb_desc = { .type = DM_BUFFER_TYPE_VERTEX, .buffer_size = sizeof(vertices), .elem_size=sizeof(dm_vertex), .usage=DM_BUFFER_USAGE_DEFAULT };
	dm_buffer_desc ib_desc = { .type = DM_BUFFER_TYPE_INDEX, .buffer_size = sizeof(indices), .elem_size=sizeof(dm_index_t), .usage=DM_BUFFER_USAGE_DEFAULT };

	r_data.object_pipeline->render_packet.vertex_buffer->desc = vb_desc;
	r_data.object_pipeline->render_packet.index_buffer->desc = ib_desc;
	r_data.object_pipeline->render_packet.count = ib_desc.buffer_size / ib_desc.elem_size;

	dm_vertex_attrib_desc v_attribs[] = {
		pos_attrib_desc,
		color_attrib_desc,
		tex_coord_desc
	};

	dm_vertex_layout v_layout = {
		.attributes = v_attribs,
		.num = sizeof(v_attribs) / sizeof(dm_vertex_attrib_desc)
	};

	// constant buffers
	dm_buffer cb_buffer = {0};
	cb_buffer.desc.type = DM_BUFFER_TYPE_CONSTANT;
	cb_buffer.desc.usage = DM_BUFFER_USAGE_DYNAMIC;
	cb_buffer.desc.elem_size = sizeof(float);
	cb_buffer.desc.buffer_size = ((sizeof(offset) + 15) / 16) * 16;
	cb_buffer.desc.cpu_access = DM_BUFFER_CPU_WRITE;
	
	dm_constant_buffer cb = { 0 };
	cb.desc.buffer = cb_buffer;
	cb.desc.name = "offset";
	cb.desc.data_t = DM_CONST_BUFFER_T_FLOAT;
	cb.desc.count = 3;
	cb.desc.data = &offset;
	dm_list_append(r_data.object_pipeline->render_packet.constant_buffers, &cb);

	// texture initialization
	dm_image_desc image_desc1 = { 0 };
	image_desc1.path = "assets/container.jpg";
	image_desc1.name = "uTexture1";
	image_desc1.format = DM_TEXTURE_FORMAT_RGB;
	image_desc1.internal_format = DM_TEXTURE_FORMAT_RGB;

	dm_image_desc image_desc2 = { 0 };
	image_desc2.path = "assets/awesomeface.png";
	image_desc2.name = "uTexture2";
	image_desc2.format = DM_TEXTURE_FORMAT_RGBA;
	image_desc2.internal_format = DM_TEXTURE_FORMAT_RGB;
	image_desc2.flip = true;

	dm_image_desc image_descs[] = { image_desc1, image_desc2 };

	if(!dm_textures_load(image_descs, sizeof(image_descs) / sizeof(dm_image_desc))) return false;
	
	for(uint32_t i=0; i<sizeof(image_descs)/sizeof(dm_image_desc);i++)
	{
		dm_string str = {0};
		str.string = dm_strdup(image_descs[i].path);
		str.len = strlen(str.string);
		dm_list_append(r_data.object_pipeline->render_packet.texture_paths, &str);
	}

	return dm_renderer_init_pipeline_data_impl(vertices, indices, v_layout, r_data.object_pipeline);
}