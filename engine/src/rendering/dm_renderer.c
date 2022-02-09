#include "dm_renderer.h"
#include "dm_vertex_attribs.h"
#include "dm_command_buffer.h"
#include "dm_image.h"
#include "dm_geometry.h"
#include "dm_uniform.h"

#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/math/dm_math.h"

#include <stddef.h>

static dm_renderer_data r_data = { 0 };

bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline);

void dm_renderer_destroy_render_pass(dm_render_pass* render_pass);

// forward declaration of the implementation, or backend, functionality
// if not defined, compiler will be angry

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data);
void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data);
bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data);

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass);

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline);

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline);

// vertex data
dm_list* vertices = NULL;
dm_list* indices = NULL;

// instances
dm_map_t* inst_map = NULL;
dm_list* mesh_tags = NULL;

// render passes
dm_map_t* render_passes = NULL;

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;

	dm_viewport viewport = {
		.x = 0,
		.y = 0,
		.width = platform_data->window_width,
		.height = platform_data->window_height,
		.max_depth = 1.0f
	};
	r_data.viewport = viewport;

	r_data.pipeline = dm_alloc(sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);

	if(!dm_renderer_init_impl(platform_data, &r_data))
	{
		DM_LOG_FATAL("Renderer backend could not be initialized!");
		return false;
	}

	// vertex data
	vertices = dm_list_create(sizeof(dm_vertex_t), 0);
	indices = dm_list_create(sizeof(dm_index_t), 0);

	inst_map = dm_map_create(sizeof(dm_inst_data), 0);
	mesh_tags = dm_list_create(sizeof(dm_string), 0);
	r_data.render_commands = dm_list_create(sizeof(dm_render_command), 0);
	render_passes = dm_map_create(sizeof(dm_render_pass), 0);

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

	if (!dm_renderer_init_render_pipeline(r_data.pipeline))
	{
		DM_LOG_FATAL("Failed to create render pipeline!");
		return false;
	}

	// primitives
	if (!dm_geometry_load_primitives())
	{
		DM_LOG_FATAL("Failed to initialize primitive vertex data!");
		return false;
	}

	return true;
}

void dm_renderer_shutdown()
{
	// vertex data
	dm_list_destroy(vertices);
	dm_list_destroy(indices);

	dm_map_destroy(inst_map);
	dm_list_destroy(mesh_tags);
	for(uint32_t i=0; i<render_passes->capacity;i++)
	{
		if(render_passes->items[i])
		{
			dm_render_pass* render_pass = render_passes->items[i]->value;
			dm_renderer_destroy_render_pass(render_pass);
		}
	}
	dm_map_destroy(render_passes);

	if(r_data.render_commands->count>0) dm_renderer_clear_command_buffer(r_data.render_commands);
	dm_list_destroy(r_data.render_commands);

	dm_renderer_destroy_render_pipeline(r_data.pipeline);
	dm_free(r_data.pipeline, sizeof(dm_render_pipeline), DM_MEM_RENDER_PIPELINE);
	dm_image_map_destroy();

	// backend shutdown
	dm_renderer_shutdown_impl(&r_data);
}

void dm_renderer_resize(int new_width, int new_height)
{
	dm_viewport viewport = {
		.x = 0,
		.y = new_height,
		.width = new_width,
		.height = new_height,
		.max_depth = 1.0f
	};

	r_data.viewport = viewport;
}

bool dm_renderer_begin_frame()
{
	dm_render_command_clear(&r_data.clear_color, r_data.render_commands);
	dm_render_command_set_viewport(&r_data.viewport, r_data.render_commands);
	dm_render_command_bind_pipeline(r_data.pipeline, r_data.render_commands);

	/************************
	    object render pass
	*******************************/

	dm_render_pass* obj_pass = dm_map_get(render_passes, "object");
	if (!obj_pass)
	{
		DM_LOG_FATAL("Object render pass is null!");
		return false;
	}

	dm_uniform* view_proj = dm_map_get(obj_pass->uniforms, "view_proj");
#ifdef DM_DIRECTX
	dm_mat4 new_view_proj = dm_mat4_transpose(r_data.camera.view_proj);
	dm_memcpy(view_proj->data, &new_view_proj, sizeof(new_view_proj));
#else
	dm_memcpy(view_proj->data, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
#endif

	dm_render_pass* light_pass = dm_map_get(render_passes, "light_src");
	if (!light_pass)
	{
		DM_LOG_FATAL("Light source render pass is NULL!");
		return false;
	}

	dm_list* lights = dm_map_get(light_pass->objects, "cube");
	dm_game_object* light = dm_list_at(lights, 0);
	if (!light)
	{
		DM_LOG_FATAL("Light is NULL!");
		return false;
	}
	dm_uniform* lpos = dm_map_get(obj_pass->uniforms, "light_pos");
	dm_memcpy(lpos->data, &light->transform.position, sizeof(light->transform.position));
		
	dm_uniform* view_pos = dm_map_get(obj_pass->uniforms, "view_pos");
	dm_memcpy(view_pos->data, &r_data.camera.pos, sizeof(r_data.camera.pos));

	for(uint32_t i=0; i< mesh_tags->count;i++)
	{
		dm_string* key = dm_list_at(mesh_tags, i);
		dm_inst_data* inst_data = dm_map_get(inst_map, key->string);
		dm_list* objs = dm_map_get(obj_pass->objects, key->string);
		dm_list* buffer_data = dm_list_create(sizeof(dm_vertex_inst), 0);

		for (uint32_t j = 0; j < objs->count; j++)
		{
			dm_game_object* obj = dm_list_at(objs, j);
			dm_vertex_inst inst = { 0 };

			inst.model = dm_mat4_identity();
			inst.model = dm_mat_translate(inst.model, obj->transform.position);
			inst.model = dm_mat_scale(inst.model, obj->transform.scale);
#ifdef DM_DIRECTX
			inst.model = dm_mat4_transpose(inst.model);
#endif
			inst.color = obj->color;

			dm_list_append(buffer_data, &inst);
		}

		dm_render_command_begin_renderpass(obj_pass, r_data.render_commands);
		dm_render_command_update_buffer(r_data.pipeline->inst_buffer, buffer_data->data, buffer_data->count * buffer_data->element_size, r_data.render_commands);
		dm_render_command_draw_instanced(inst_data->index_count, objs->count, inst_data->index_offset, inst_data->vertex_offset, 0, obj_pass, r_data.render_commands);
		dm_render_command_end_renderpass(obj_pass, r_data.render_commands);

		dm_list_destroy(buffer_data);
	}

	if (!dm_renderer_submit_command_buffer(r_data.render_commands, r_data.pipeline)) return false;
	dm_renderer_clear_command_buffer(r_data.render_commands);
	
	return true;
}

bool dm_renderer_end_frame()
{
	/**********************
	  light source render pass
	****************************/

	dm_render_pass* lsrc_pass = dm_map_get(render_passes, "light_src");
	if (!lsrc_pass)
	{
		DM_LOG_FATAL("Light source render pass is NULL!");
		return false;
	}

	dm_uniform* view_proj = dm_map_get(lsrc_pass->uniforms, "view_proj");
#ifdef DM_DIRECTX
	dm_mat4 new_view_proj = dm_mat4_transpose(r_data.camera.view_proj);
	dm_memcpy(view_proj->data, &new_view_proj, sizeof(new_view_proj));
#else
	dm_memcpy(view_proj->data, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
#endif

	for (uint32_t i = 0; i < mesh_tags->count; i++)
	{
		dm_string* key = dm_list_at(mesh_tags, i);
		dm_inst_data* inst_data = dm_map_get(inst_map, key->string);
		dm_list* objs = dm_map_get(lsrc_pass->objects, key->string);
		dm_list* buffer_data = dm_list_create(sizeof(dm_vertex_inst), 0);

		for (uint32_t j = 0; j < objs->count; j++)
		{
			dm_game_object* obj = dm_list_at(objs, j);
			dm_vertex_inst inst = { 0 };

			inst.model = dm_mat4_identity();
			inst.model = dm_mat_translate(inst.model, obj->transform.position);
			inst.model = dm_mat_scale(inst.model, obj->transform.scale);
#ifdef DM_DIRECTX
			inst.model = dm_mat4_transpose(inst.model);
#endif
			inst.color = obj->color;

			dm_list_append(buffer_data, &inst);
		}

		dm_render_command_begin_renderpass(lsrc_pass, r_data.render_commands);
		dm_render_command_update_buffer(r_data.pipeline->inst_buffer, buffer_data->data, buffer_data->count * buffer_data->element_size, r_data.render_commands);
		dm_render_command_draw_instanced(inst_data->index_count, objs->count, inst_data->index_offset, inst_data->vertex_offset, 0, lsrc_pass, r_data.render_commands);
		dm_render_command_end_renderpass(lsrc_pass, r_data.render_commands);

		dm_list_destroy(buffer_data);
	}

	if (!dm_renderer_submit_command_buffer(r_data.render_commands, r_data.pipeline)) return false;
	dm_renderer_clear_command_buffer(r_data.render_commands);

	return dm_renderer_end_frame_impl(&r_data);
}

bool dm_renderer_init_render_pipeline(dm_render_pipeline* pipeline)
{
	// blend
	dm_blend_state_desc blend = { 0 };
	blend.is_enabled = true;
	blend.src = DM_BLEND_FUNC_SRC_ALPHA;
	blend.dest = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;

	// depth
	dm_depth_state_desc depth = { 0 };
	depth.is_enabled = true;
	depth.comparison = DM_COMPARISON_LESS;

	// stencil
	dm_stencil_state_desc stencil = { 0 };

	// fill-in pipeline
	pipeline->blend_desc = blend;
	pipeline->depth_desc = depth;
	pipeline->stencil_desc = stencil;

	// render packet
	pipeline->vertex_buffer = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	pipeline->index_buffer = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	pipeline->inst_buffer = dm_alloc(sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);

	pipeline->render_packet.image_paths = dm_list_create(sizeof(dm_string), 0);

	return dm_renderer_create_render_pipeline_impl(pipeline);
}

void dm_renderer_destroy_render_pipeline(dm_render_pipeline* pipeline)
{
	dm_renderer_destroy_render_pipeline_impl(pipeline);

	dm_free(pipeline->vertex_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->index_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);
	dm_free(pipeline->inst_buffer, sizeof(dm_buffer), DM_MEM_RENDERER_BUFFER);

	// images
	for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
	{
		dm_string* str = dm_list_at(pipeline->render_packet.image_paths, i);
		dm_strdel(str->string);
	}

	dm_list_destroy(pipeline->render_packet.image_paths);
}

bool dm_renderer_init_object_data()
{
	// buffers
	dm_buffer_desc vb_desc = { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .buffer_size = vertices->count * vertices->element_size, .elem_size=vertices->element_size, .usage=DM_BUFFER_USAGE_DEFAULT , .name = "vertex"};
	dm_buffer_desc ib_desc = { .type = DM_BUFFER_TYPE_INDEX, .data_t = DM_BUFFER_DATA_T_UINT, .buffer_size = indices->count * indices->element_size, .elem_size=indices->element_size, .usage=DM_BUFFER_USAGE_DEFAULT, .name = "index"};
	dm_buffer_desc inst_desc = { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .buffer_size = sizeof(dm_inst_data) * DM_MAX_INSTANCES, .elem_size=sizeof(dm_vertex_inst), .usage = DM_BUFFER_USAGE_DYNAMIC, .cpu_access = DM_BUFFER_CPU_WRITE, .name = "instance" };

	r_data.pipeline->vertex_buffer->desc = vb_desc;
	r_data.pipeline->index_buffer->desc = ib_desc;
	r_data.pipeline->inst_buffer->desc = inst_desc;
		
	if(!dm_renderer_init_pipeline_data_impl(vertices->data, indices->data, r_data.pipeline)) return false;

	return true;
}

bool dm_renderer_create_default_render_passes()
{
	// vertex attributes
	dm_vertex_attrib_desc obj_v_attribs[] = {
		pos_attrib_desc,
		norm_attrib_desc,
		tex_coord_desc,
		model_attrib_desc,
		color_attrib_desc
	};

	dm_vertex_layout obj_v_layout = {
		.attributes = obj_v_attribs,
		.num = sizeof(obj_v_attribs) / sizeof(obj_v_attribs[0])
	};

	// uniforms
	dm_vec3 global_light_color = { 1,1,1 };
	float ambient = 0.1;

	// object uniforms
	dm_uniform obj_vp = dm_create_uniform("view_proj", mat4_uni_desc, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
	dm_uniform global_light = dm_create_uniform("global_light", vec3_uni_desc, &global_light_color, sizeof(global_light_color));
	dm_uniform ambient_light = dm_create_uniform("ambient", float_uni_desc, &ambient, sizeof(ambient));
	dm_uniform light_pos = dm_create_uniform("light_pos", vec3_uni_desc, &(dm_vec3){0,0,0}, sizeof(dm_vec3));
	dm_uniform view_pos = dm_create_uniform("view_pos", vec3_uni_desc, &(dm_vec3){0,0,0}, sizeof(dm_vec3));

	// light source uniforms
	dm_uniform lsrc_vp_uni = dm_create_uniform("view_proj", mat4_uni_desc, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));

	// shaders
#ifdef DM_OPENGL
	const char* object_vertex_shader = "shaders/glsl/object_vertex.glsl";
#elif defined DM_DIRECTX
	const char* object_vertex_shader = "shaders/hlsl/object_vertex.fxc";
#elif defined DM_METAL
	const char* object_vertex_shader = "vertex_main";
#endif

#ifdef DM_OPENGL
	const char* object_pixel_shader = "shaders/glsl/object_pixel.glsl";
#elif defined DM_DIRECTX
	const char* object_pixel_shader = "shaders/hlsl/object_pixel.fxc";
#elif defined DM_METAL
	const char* object_pixel_shader = "fragment_main";
#endif

#ifdef DM_OPENGL
	const char* light_src_vertex_shader = "shaders/glsl/light_src_vertex.glsl";
#elif defined DM_DIRECTX
	const char* light_src_vertex_shader = "shaders/hlsl/light_src_vertex.fxc";
#elif defined DM_METAL
	const char* light_src_vertex_shader = "vertex_main";
#endif

#ifdef DM_OPENGL
	const char* light_src_pixel_shader = "shaders/glsl/light_src_pixel.glsl";
#elif defined DM_DIRECTX
	const char* light_src_pixel_shader = "shaders/hlsl/light_src_pixel.fxc";
#elif defined DM_METAL
	const char* light_src_pixel_shader = "light_src_main";
#endif

	// object render pass
	dm_list* obj_uni_list = dm_list_create(sizeof(dm_uniform), 0);
	dm_list_append(obj_uni_list, &obj_vp);
	dm_list_append(obj_uni_list, &global_light);
	dm_list_append(obj_uni_list, &ambient_light);
	dm_list_append(obj_uni_list, &light_pos);
	dm_list_append(obj_uni_list, &view_pos);

	if (!dm_renderer_create_render_pass(object_vertex_shader, object_pixel_shader, obj_v_layout, obj_uni_list, "object"))
	{
		DM_LOG_FATAL("Could not create default object render pass!");
		return false;
	}

	dm_list_destroy(obj_uni_list);

	// light src render pass
	dm_list* lsrc_uni_list = dm_list_create(sizeof(dm_uniform), 0);
	dm_list_append(lsrc_uni_list, &lsrc_vp_uni);

	if (!dm_renderer_create_render_pass(light_src_vertex_shader, light_src_pixel_shader, obj_v_layout, lsrc_uni_list, "light_src"))
	{
		DM_LOG_FATAL("Could not create default light src render pass!");
		return false;
	}
	
	dm_list_destroy(lsrc_uni_list);

	return true;
}

bool dm_renderer_create_render_pass(const char* vertex_shader, const char* pixel_shader, dm_vertex_layout v_layout, dm_list* uniforms, const char* tag)
{
	dm_render_pass* render_pass = dm_alloc(sizeof(dm_render_pass), DM_MEM_RENDER_PASS);

	render_pass->name = tag;

	// shader
	render_pass->shader = dm_alloc(sizeof(dm_shader), DM_MEM_RENDERER_SHADER);

	render_pass->shader->vertex_desc.path = vertex_shader;
	render_pass->shader->vertex_desc.type = DM_SHADER_TYPE_VERTEX;

	render_pass->shader->pixel_desc.path = pixel_shader;
	render_pass->shader->pixel_desc.type = DM_SHADER_TYPE_PIXEL;

	// raster
	dm_raster_state_desc raster = { 0 };
	raster.cull_mode = DM_CULL_BACK;
	raster.winding_order = DM_WINDING_COUNTER_CLOCK;
	raster.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;

	render_pass->raster_desc = raster;

	// sampler
	dm_sampler_desc sampler = { 0 };
	sampler.comparison = DM_COMPARISON_ALWAYS;
	sampler.filter = DM_FILTER_LINEAR;
	sampler.u = DM_TEXTURE_MODE_WRAP;
	sampler.v = DM_TEXTURE_MODE_WRAP;
	sampler.w = DM_TEXTURE_MODE_WRAP;

	render_pass->sampler_desc = sampler;

	// uniforms
	render_pass->uniforms = dm_map_create(sizeof(dm_uniform), 0);

	for (uint32_t i = 0; i < uniforms->count; i++)
	{
		dm_uniform* uniform = dm_list_at(uniforms, i);

		dm_map_insert(render_pass->uniforms, uniform->name, uniform);
	}

	render_pass->objects = dm_map_create(sizeof(dm_list), 0);

	if(!dm_renderer_create_render_pass_impl(render_pass, v_layout, r_data.pipeline)) return false;

	dm_map_insert(render_passes, tag, render_pass);

	dm_free(render_pass, sizeof(dm_render_pass), DM_MEM_RENDER_PASS);

	return true;
}

void dm_renderer_destroy_render_pass(dm_render_pass* render_pass)
{
	dm_renderer_destroy_render_pass_impl(render_pass);

	for (uint32_t i = 0; i < render_pass->uniforms->capacity; i++)
	{
		if (render_pass->uniforms->items[i])
		{
			dm_uniform* uniform = render_pass->uniforms->items[i]->value;
			dm_destroy_uniform(uniform);
		}
	}
	dm_map_destroy(render_pass->uniforms);

	dm_free(render_pass->shader, sizeof(dm_shader), DM_MEM_RENDERER_SHADER);

	dm_map_list_destroy(render_pass->objects);
}

void dm_renderer_submit_vertex_data(dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices, const char* tag)
{
	dm_string obj_tag = {
		.string = tag,
		.len = strlen(tag)
	};
	dm_list_append(mesh_tags, &obj_tag);
	
	dm_inst_data inst_data = { 0 };
	inst_data.index_count = num_indices;
	inst_data.index_offset = indices->count;
	inst_data.vertex_offset = vertices->count;
	dm_map_insert(inst_map, tag, &inst_data);

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

bool dm_renderer_submit_objects(dm_list* objects)
{
	for (uint32_t i = 0; i < objects->count; i++)
	{
		dm_game_object* object = dm_list_at(objects, i);
		dm_render_pass* render_pass = dm_map_get(render_passes, object->render_pass);

		if (render_pass)
		{
			dm_list* obj_list = dm_map_get(render_pass->objects, object->mesh);

			if (!obj_list)
			{
				obj_list = dm_list_create(sizeof(dm_game_object), 0);
				dm_list_append(obj_list, object);
				dm_map_insert_list(render_pass->objects, object->mesh, obj_list);
				dm_list_destroy(obj_list);
			}
			else
			{
				dm_list_append(obj_list, object);
			}
		}
		else
		{
			DM_LOG_FATAL("Trying to submit an object to an unknown render pass: %s", object->render_pass);
			return false;
		}
	}

	return true;
}

bool dm_renderer_submit_images(dm_image_desc* image_descs, uint32_t num_descs)
{
	if (!dm_images_load(image_descs, num_descs)) return false;

	for (uint32_t i = 0; i < num_descs; i++)
	{
		dm_string str = { 0 };
		str.string = dm_strdup(image_descs[i].path);
		str.len = strlen(str.string);
		dm_list_append(r_data.pipeline->render_packet.image_paths, &str);
	}

	return true;
}

void dm_renderer_set_clear_color(dm_vec3 color)
{
	r_data.clear_color = dm_vec4_set_from_vec3(color);
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

dm_vec3 dm_renderer_get_camera_forward()
{
	return r_data.camera.forward;
}

dm_vec3 dm_renderer_get_camera_up()
{
	return r_data.camera.up;
}

dm_vec3 dm_renderer_get_camera_pos()
{
	return r_data.camera.pos;
}