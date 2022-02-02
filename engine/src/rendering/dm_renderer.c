#include "dm_renderer.h"
#include "dm_vertex_attribs.h"
#include "dm_command_buffer.h"
#include "dm_image.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/math/dm_math.h"
#include "structures/dm_map.h"
#include <stddef.h>

static dm_renderer_data r_data = { 0 };

bool dm_renderer_create_object_pipeline();
bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline);

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data);
bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline);

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, void* mvp_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline);

// vertex data
dm_list* vertices = NULL;
dm_list* indices = NULL;

// transforms
dm_list* object_transforms = NULL;

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;
	r_data.width = platform_data->window_width;
	r_data.height = platform_data->window_height;
	r_data.object_pipeline = dm_alloc(sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);

	if(!dm_renderer_init_impl(platform_data, &r_data))
	{
		DM_LOG_FATAL("Renderer backend could not be initialized!");
		return false;
	}

	// vertex data
	vertices = dm_list_create(sizeof(dm_vertex_t), 0);
	indices = dm_list_create(sizeof(dm_index_t), 0);
	object_transforms = dm_list_create(sizeof(dm_transform), 0);

	// maps
	dm_image_map_init();

	// camera
	dm_camera_init(
		&r_data.camera, (dm_vec3) { 0, 0, 0 },
		70.0f,
		platform_data->window_width,
		platform_data->window_height,
		0.01, 10000,
		DM_CAMERA_PERSPECTIVE
	);

	// object pipeline
	if (!dm_renderer_create_object_pipeline())
	{
		DM_LOG_FATAL("Failed to create object render pipeline!");
		return false;
	}
	
	return true;
}

void dm_renderer_shutdown()
{
	// vertex data
	dm_list_destroy(vertices);
	dm_list_destroy(indices);

	// transforms
	dm_list_destroy(object_transforms);

	// cleanup
	dm_renderer_destroy_render_pipeline(r_data.object_pipeline);
	dm_free(r_data.object_pipeline, sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);
	dm_image_map_destroy();

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
	dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, NULL, 0, r_data.object_pipeline->render_commands);

	return true;
}

bool dm_renderer_begin_scene()
{
	dm_renderer_begin_scene_impl(&r_data);

	// commands for object pipeline
	if (!dm_list_is_empty(r_data.object_pipeline->render_commands))
	{
		dm_renderer_clear_command_buffer(r_data.object_pipeline->render_commands);
	}

	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, NULL, 0, r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, &r_data.clear_color, sizeof(dm_color), r_data.object_pipeline->render_commands);
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, NULL, 0, r_data.object_pipeline->render_commands);
	
	for (uint32_t i = 0; i < object_transforms->count; i++)
	{
		dm_mat4 mvp = dm_mat4_identity();

		dm_vec3* pos = dm_list_at(object_transforms, i);
		mvp = dm_mat_translate(mvp, *pos);

		mvp = dm_mat4_mul_mat4(mvp, r_data.camera.view_proj);
#ifdef DM_DIRECTX
		mvp = dm_mat4_transpose(mvp);
#endif

		dm_buffer_update_packet buffer_update = { 
			.buffer=r_data.object_pipeline->render_packet.mvp, 
			.data_size=sizeof(dm_mat4), 
			.data=&mvp 
		};
		dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_BUFFER, &buffer_update, sizeof(dm_buffer_update_packet), r_data.object_pipeline->render_commands);
		dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_BUFFER, r_data.object_pipeline->render_packet.mvp, sizeof(dm_buffer), r_data.object_pipeline->render_commands);
		dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, NULL, 0, r_data.object_pipeline->render_commands);
	}
	
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, NULL, 0, r_data.object_pipeline->render_commands);

	return true;
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
	pipeline->render_packet.mvp = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	pipeline->render_packet.image_paths = dm_list_create(sizeof(dm_string), 0);

	pipeline->render_packet.count = 0;
	pipeline->render_packet.offset = 0;

	return dm_renderer_create_render_pipeline_impl(pipeline);
}

void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline)
{
	for(uint32_t i=0; i<pipeline->render_commands->count;i++)
	{
		dm_render_command* command = dm_list_at(pipeline->render_commands, i);
		if(command->data)
		{
			dm_free(command->data, command->data_size, DM_MEM_RENDER_COMMAND);
		}
	}
	dm_list_destroy(pipeline->render_commands);

	dm_renderer_destroy_render_pipeline_impl(pipeline);

	dm_free(pipeline->render_packet.vertex_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->render_packet.index_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->render_packet.mvp, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);

	// images
	for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
	{
		dm_string* str = dm_list_at(pipeline->render_packet.image_paths, i);
		dm_strdel(str->string);
	}
	dm_list_destroy(pipeline->render_packet.image_paths);

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
	depth.is_enabled = true;
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
#elif defined DM_METAL
	vs_desc.path = "vertex_main";
#endif
	vs_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc ps_desc = { 0 };
#ifdef DM_OPENGL
	ps_desc.path = "shaders/glsl/object_pixel.glsl";
#elif defined DM_DIRECTX
	ps_desc.path = "shaders/hlsl/object_pixel.fxc";
#elif defined DM_METAL
	ps_desc.path = "fragment_main";
#endif
	ps_desc.type = DM_SHADER_TYPE_PIXEL;

	r_data.object_pipeline->raster_desc.shader->vertex_desc = vs_desc;
	r_data.object_pipeline->raster_desc.shader->pixel_desc = ps_desc;

	// buffers
	dm_buffer_desc vb_desc = { .type = DM_BUFFER_TYPE_VERTEX, .buffer_size = vertices->count * vertices->element_size, .elem_size=vertices->element_size, .usage=DM_BUFFER_USAGE_DEFAULT };
	dm_buffer_desc ib_desc = { .type = DM_BUFFER_TYPE_INDEX, .buffer_size = indices->count * indices->element_size, .elem_size=indices->element_size, .usage=DM_BUFFER_USAGE_DEFAULT };

	r_data.object_pipeline->render_packet.vertex_buffer->desc = vb_desc;
	r_data.object_pipeline->render_packet.index_buffer->desc = ib_desc;
	r_data.object_pipeline->render_packet.count = ib_desc.buffer_size / ib_desc.elem_size;

	dm_vertex_attrib_desc v_attribs[] = {
		pos_attrib_desc,
		tex_coord_desc
	};

	dm_vertex_layout v_layout = {
		.attributes = v_attribs,
		.num = sizeof(v_attribs) / sizeof(dm_vertex_attrib_desc)
	};

	/*
	// constant buffers
	*/
	dm_buffer_desc vp_desc = { 0 };
	vp_desc.type = DM_BUFFER_TYPE_CONSTANT;
	vp_desc.usage = DM_BUFFER_USAGE_DYNAMIC;
	vp_desc.cpu_access = DM_BUFFER_CPU_WRITE;
	vp_desc.data_t = DM_BUFFER_DATA_T_MATRIX;
	vp_desc.elem_size = sizeof(float);
	vp_desc.count = 4;
	vp_desc.buffer_size = ((sizeof(dm_mat4) + 15) / 16) * 16;
	vp_desc.name = "mvp";

	r_data.object_pipeline->render_packet.mvp->desc = vp_desc;

	dm_mat4 model = dm_mat4_identity();
	model = dm_mat4_mul_mat4(model, r_data.camera.view_proj);

	if(!dm_renderer_init_pipeline_data_impl(vertices->data, indices->data, &model, v_layout, r_data.object_pipeline)) return false;

	return true;
}

void dm_renderer_submit_vertex_data(dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices)
{
	for (uint32_t i = 0; i < num_vertices; i++)
	{
		dm_list_append(vertices, &vertex_data[i]);
	}

#ifdef DM_DIRECTX
	for (uint32_t i = 0; i < num_indices; )
	{
		dm_index_t swap = index_data[i + 2];
		index_data[i + 2] = index_data[i + 1];
		index_data[i + 1] = swap;

		i += 3;
	}
#endif

	for (uint32_t i = 0; i < num_indices; i++)
	{
		dm_list_append(indices, &index_data[i]);
	}
}

void dm_renderer_submit_object_transforms(dm_transform* transforms, uint32_t num_transforms)
{
	for (uint32_t i = 0; i < num_transforms; i++)
	{
		dm_list_append(object_transforms, &transforms[i]);
	}
}

void dm_renderer_update_object_transforms(dm_transform* transforms, uint32_t num_transforms)
{
	for (uint32_t i = 0; i < num_transforms; i++)
	{
		dm_list_set(object_transforms, &transforms[i], i);
	}
}

bool dm_renderer_submit_images(dm_image_desc* image_descs, uint32_t num_descs)
{
	if (!dm_images_load(image_descs, num_descs)) return false;

	for (uint32_t i = 0; i < num_descs; i++)
	{
		dm_string str = { 0 };
		str.string = dm_strdup(image_descs[i].path);
		str.len = strlen(str.string);
		dm_list_append(r_data.object_pipeline->render_packet.image_paths, &str);
	}

	return true;
}

void dm_renderer_set_camera_pos(dm_vec3 pos)
{
	dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_update_camera_pos(dm_vec3 delta_pos)
{
	dm_vec3 pos = dm_vec3_add_vec3(r_data.camera.pos, delta_pos);
	dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_set_camera_forward(dm_vec3 forward)
{
	dm_camera_set_forward(&r_data.camera, forward);
}

void dm_renderer_update_camera_forward(dm_vec3 delta_forward)
{
	dm_vec3 forward = dm_vec3_add_vec3(r_data.camera.forward, delta_forward);
	forward.x = DM_CLAMP(forward.x, -89, 89);
	forward.y = DM_CLAMP(forward.y, -89, 89);
	forward.z = DM_CLAMP(forward.z, -89, 89);
	dm_camera_set_forward(&r_data.camera, forward);
}