#include "dm_renderer.h"
#include "dm_renderer_api.h"
#include "dm_vertex_attribs.h"
#include "dm_command_buffer.h"
#include "dm_image.h"
#include "dm_geometry.h"
#include "dm_uniform.h"

#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_math.h"

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

// meshes
dm_map* mesh_map = NULL;

// camera
dm_editor_camera* camera = NULL;

// render passes
dm_map* render_passes = NULL;

bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	r_data.clear_color = clear_color;
    
	dm_viewport viewport = {
		.x = 0, .y = 0,
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
    
	//inst_map = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_inst_data), 0);
    
    render_passes = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_render_pass), 0);
	
    // mesh
	mesh_map = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_mesh), 0);
    
    
    //mesh_tags = dm_list_create(sizeof(dm_string), 0);
    r_data.render_commands = dm_list_create(sizeof(dm_render_command), 0);
    
    // maps
    dm_image_map_init();
    
    // camera
    dm_camera_init(
                   &r_data.camera, (dm_vec3) { 0, 0, 0 },
                   70.0f,
                   platform_data->window_width,
                   platform_data->window_height,
                   0.01f, 10000,
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
    
	//dm_map_destroy(inst_map);
	dm_for_map_item(mesh_map)
    {
        dm_mesh* mesh = item->value;
        dm_list_destroy(mesh->entities);
    }
    
    dm_map_destroy(mesh_map);
    
    //dm_list_destroy(mesh_tags);
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

bool dm_renderer_render_objects()
{
    dm_render_pass* obj_pass = dm_map_get(render_passes, "object");
	if (!obj_pass)
	{
		DM_LOG_FATAL("Object render pass is null!");
		return false;
	}
    
	// update the uniforms
	for (uint32_t i = 0; i < obj_pass->uniforms->count; i++)
	{
		dm_uniform* uniform = dm_list_at(obj_pass->uniforms, i);
        
		if (strcmp(uniform->name, "view_proj") == 0)
		{
#ifdef DM_DIRECTX
			dm_mat4 new_view_proj = dm_mat4_transpose(r_data.camera.view_proj);
			dm_memcpy(uniform->data, &new_view_proj, sizeof(new_view_proj));
#else
			dm_memcpy(uniform->data, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
#endif
		}
		else if (strcmp(uniform->name, "light_pos") == 0)
		{
            dm_list* lights = dm_ecs_get_entity_registry(DM_COMPONENT_LIGHT_SRC);
            
            for(uint32_t i=0; i<lights->count; i++)
            {
                uint32_t entity_id = *(uint32_t*)dm_list_at(lights, i);
                dm_transform_component* transform = dm_ecs_get_component(entity_id, DM_COMPONENT_TRANSFORM);
                
                dm_memcpy(uniform->data, &transform->position, sizeof(transform->position));
            }
		}
		else if (strcmp(uniform->name, "view_pos") == 0)
		{
			dm_memcpy(uniform->data, &r_data.camera.pos, sizeof(r_data.camera.pos));
		}
	}
    
    dm_for_map_item(mesh_map)
    {
        dm_mesh* mesh = item->value;
        //DM_LOG_TRACE("%s", item->key);
        
        dm_list* buffer_data = dm_list_create(sizeof(dm_vertex_inst), 0);
        uint32_t count = 0;
        
        for(uint32_t i=0; i<mesh->entities->count; i++)
        {
            uint32_t entity_id = *(uint32_t*)dm_list_at(mesh->entities, i);
            dm_transform_component* transform = dm_ecs_get_component(entity_id, DM_COMPONENT_TRANSFORM);
            dm_color_component* color = dm_ecs_get_component(entity_id, DM_COMPONENT_COLOR);
            bool is_light = (dm_ecs_get_component(entity_id, DM_COMPONENT_LIGHT_SRC) != NULL);
            
            if(!is_light)
            {
                dm_vertex_inst inst = {0};
                
                inst.model = dm_mat4_identity();
                inst.model = dm_mat_translate(inst.model, transform->position);
                inst.model = dm_mat_scale(inst.model, transform->scale);
#ifdef DM_DIRECTX
                inst.model = dm_mat4_transpose(inst.model);
#endif
                
                inst.color = dm_vec3_set_from_vec4(color->color);
                
                dm_list_append(buffer_data, &inst);
                
                count++;
            }
        }
        
        dm_render_command_begin_renderpass(obj_pass, r_data.render_commands);
		dm_render_command_update_buffer(r_data.pipeline->inst_buffer, buffer_data->data, buffer_data->count * buffer_data->element_size, r_data.render_commands);
		dm_render_command_draw_instanced(mesh->index_count, count, mesh->index_offset, mesh->vertex_offset, 0, obj_pass, r_data.render_commands);
		dm_render_command_end_renderpass(obj_pass, r_data.render_commands);
        
        dm_list_destroy(buffer_data);
    }
    
	if (!dm_renderer_submit_command_buffer(r_data.render_commands, r_data.pipeline)) return false;
	dm_renderer_clear_command_buffer(r_data.render_commands);
    
    return true;
}

bool dm_render_light_sources()
{
    dm_render_pass* lsrc_pass = dm_map_get(render_passes, "light_src");
	if (!lsrc_pass)
	{
		DM_LOG_FATAL("Light source render pass is NULL!");
		return false;
	}
	// update the uniforms
	for (uint32_t i = 0; i < lsrc_pass->uniforms->count; i++)
	{
		dm_uniform* uniform = dm_list_at(lsrc_pass->uniforms, i);
        
		if (strcmp(uniform->name, "view_proj") == 0)
		{
#ifdef DM_DIRECTX
			dm_mat4 new_view_proj = dm_mat4_transpose(r_data.camera.view_proj);
			dm_memcpy(uniform->data, &new_view_proj, sizeof(new_view_proj));
#else
			dm_memcpy(uniform->data, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
#endif
		}
	}
    
    dm_render_command_begin_renderpass(lsrc_pass, r_data.render_commands);
    
    dm_list* lights = dm_ecs_get_entity_registry(DM_COMPONENT_LIGHT_SRC);
    
    for(uint32_t i = 0; i<lights->count; i++)
    {
        dm_list* buffer_data = dm_list_create(sizeof(dm_vertex_inst), 0);
        
        uint32_t entity_id = *(uint32_t*)dm_list_at(lights, i);
        dm_mesh_component* mesh_c = dm_ecs_get_component(entity_id, DM_COMPONENT_MESH);
        dm_mesh* mesh = dm_map_get(mesh_map, mesh_c->name);
        
        dm_transform_component* transform = dm_ecs_get_component(entity_id, DM_COMPONENT_TRANSFORM);
        dm_light_src_component* light_src = dm_ecs_get_component(entity_id, DM_COMPONENT_LIGHT_SRC);
        
        dm_vertex_inst inst = {0};
        
        inst.model = dm_mat4_identity();
        inst.model = dm_mat_translate(inst.model, transform->position);
        inst.model = dm_mat_scale(inst.model, transform->scale);
#ifdef DM_DIRECTX
        inst.model = dm_mat4_transpose(inst.model);
#endif
        
        inst.color = dm_vec3_set_from_vec4(light_src->color);
        
        dm_list_append(buffer_data, &inst);
        
        dm_render_command_update_buffer(r_data.pipeline->inst_buffer, buffer_data->data, buffer_data->count * buffer_data->element_size, r_data.render_commands);
        dm_render_command_draw_indexed(mesh->index_count, mesh->index_offset, mesh->vertex_offset, lsrc_pass, r_data.render_commands);
        
        dm_list_destroy(buffer_data);
    }
    
    dm_render_command_end_renderpass(lsrc_pass, r_data.render_commands);
    
	if (!dm_renderer_submit_command_buffer(r_data.render_commands, r_data.pipeline)) return false;
	dm_renderer_clear_command_buffer(r_data.render_commands);
    
    return true;
}

bool dm_renderer_begin_frame()
{
	dm_render_command_clear(&r_data.clear_color, r_data.render_commands);
	dm_render_command_set_viewport(&r_data.viewport, r_data.render_commands);
	dm_render_command_bind_pipeline(r_data.pipeline, r_data.render_commands);
    
	/************************
	    object render pass
	*******************************/
    dm_renderer_render_objects();
	
	return true;
}

bool dm_renderer_end_frame()
{
	/**********************
	  light source render pass
	****************************/
	dm_render_light_sources();
    
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
    
	// object uniforms
	dm_uniform obj_vp = dm_create_uniform("view_proj", mat4_uni_desc, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
	dm_uniform light_color = dm_create_uniform("light_color", vec3_uni_desc, &(dm_vec3){1, 1, 1}, sizeof(dm_vec3));
	dm_uniform ambient_str = dm_create_uniform("ambient_str", float_uni_desc, &(float){0.1f}, sizeof(float));
	dm_uniform light_pos = dm_create_uniform("light_pos", vec3_uni_desc, &(dm_vec3){0,0,0}, sizeof(dm_vec3));
	dm_uniform view_pos = dm_create_uniform("view_pos", vec3_uni_desc, &(dm_vec3){0,0,0}, sizeof(dm_vec3));
    
	// light source uniforms
	dm_uniform lsrc_vp_uni = dm_create_uniform("view_proj", mat4_uni_desc, &r_data.camera.view_proj, sizeof(r_data.camera.view_proj));
    
	dm_shader obj_shader = {0};
	obj_shader.vertex_desc.type = DM_SHADER_TYPE_VERTEX;
	obj_shader.pixel_desc.type = DM_SHADER_TYPE_PIXEL;
	obj_shader.name = "object";
    
#ifdef DM_METAL
	obj_shader.single_file = true;
	obj_shader.file_name = "shaders/metal/object_shader.metallib";
#endif
    
	// shaders
#ifdef DM_OPENGL
	obj_shader.vertex_desc.path = "shaders/glsl/object_vertex.glsl";
#elif defined DM_DIRECTX
	obj_shader.vertex_desc.path = "shaders/hlsl/object_vertex.fxc";
#elif defined DM_METAL
	obj_shader.vertex_desc.path = "vertex_main";
#endif
    
#ifdef DM_OPENGL
	obj_shader.pixel_desc.path = "shaders/glsl/object_pixel.glsl";
#elif defined DM_DIRECTX
	obj_shader.pixel_desc.path = "shaders/hlsl/object_pixel.fxc";
#elif defined DM_METAL
	obj_shader.pixel_desc.path = "fragment_main";
#endif
    
	dm_shader lsrc_shader = {0};
	lsrc_shader.vertex_desc.type = DM_SHADER_TYPE_VERTEX;
	lsrc_shader.pixel_desc.type = DM_SHADER_TYPE_PIXEL;
	lsrc_shader.name = "light_src";
    
#ifdef DM_METAL
	lsrc_shader.single_file = true;
	lsrc_shader.file_name = "shaders/metal/light_src_shader.metallib";
#endif
    
#ifdef DM_OPENGL
	lsrc_shader.vertex_desc.path = "shaders/glsl/light_src_vertex.glsl";
#elif defined DM_DIRECTX
	lsrc_shader.vertex_desc.path = "shaders/hlsl/light_src_vertex.fxc";
#elif defined DM_METAL
	lsrc_shader.vertex_desc.path = "vertex_main";
#endif
    
#ifdef DM_OPENGL
	lsrc_shader.pixel_desc.path = "shaders/glsl/light_src_pixel.glsl";
#elif defined DM_DIRECTX
	lsrc_shader.pixel_desc.path = "shaders/hlsl/light_src_pixel.fxc";
#elif defined DM_METAL
	lsrc_shader.pixel_desc.path = "light_src_main";
#endif
    
	// object render pass
	dm_list* obj_uni_list = dm_list_create(sizeof(dm_uniform), 0);
	dm_list_append(obj_uni_list, &obj_vp);
	dm_list_append(obj_uni_list, &light_color);
	dm_list_append(obj_uni_list, &ambient_str);
	dm_list_append(obj_uni_list, &light_pos);
	dm_list_append(obj_uni_list, &view_pos);
    
	if (!dm_renderer_create_render_pass(obj_shader, obj_v_layout, obj_uni_list, obj_shader.name))
	{
		DM_LOG_FATAL("Could not create default object render pass!");
		return false;
	}
    
	//dm_list_destroy(obj_uni_list);
    
	// light src render pass
	dm_list* lsrc_uni_list = dm_list_create(sizeof(dm_uniform), 0);
	dm_list_append(lsrc_uni_list, &lsrc_vp_uni);
    
	if (!dm_renderer_create_render_pass(lsrc_shader, obj_v_layout, lsrc_uni_list, lsrc_shader.name))
	{
		DM_LOG_FATAL("Could not create default light src render pass!");
		return false;
	}
	
	//dm_list_destroy(lsrc_uni_list);
    
	return true;
}

bool dm_renderer_create_render_pass(dm_shader shader, dm_vertex_layout v_layout, dm_list* uniforms, const char* tag)
{
	dm_render_pass* render_pass = dm_alloc(sizeof(dm_render_pass), DM_MEM_RENDER_PASS);
    
	render_pass->name = tag;
    
	// shader
	render_pass->shader = dm_alloc(sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
	dm_memcpy(render_pass->shader, &shader, sizeof(dm_shader));
    
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
	render_pass->uniforms = uniforms;
    
	if(!dm_renderer_create_render_pass_impl(render_pass, v_layout, r_data.pipeline)) return false;
    
	dm_map_insert(render_passes, (void*)tag, render_pass);
    
	dm_free(render_pass, sizeof(dm_render_pass), DM_MEM_RENDER_PASS);
    
	return true;
}

void dm_renderer_destroy_render_pass(dm_render_pass* render_pass)
{
	dm_renderer_destroy_render_pass_impl(render_pass);
    
	for (uint32_t i = 0; i < render_pass->uniforms->count; i++)
	{
		
		dm_uniform* uniform = dm_list_at(render_pass->uniforms, i);
		dm_destroy_uniform(uniform);
	}
	dm_list_destroy(render_pass->uniforms);
    
	dm_free(render_pass->shader, sizeof(dm_shader), DM_MEM_RENDERER_SHADER);
}

/*
RENDERER API FUNCTIONS
*/
void dm_renderer_api_submit_vertex_data(const char* tag, dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices)
{
    dm_mesh mesh = {0};
    mesh.index_count = num_indices;
    mesh.vertex_offset = vertices->count;
    mesh.index_offset = indices->count;
    mesh.entities = dm_list_create(sizeof(uint32_t), 0);
    
    dm_map_insert(mesh_map, tag, &mesh);
    
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

bool dm_renderer_api_submit_images(dm_image_desc* image_descs, uint32_t num_descs)
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

void dm_renderer_api_set_clear_color(dm_vec3 color)
{
	r_data.clear_color = dm_vec4_set_from_vec3(color);
}

/*
to simplify rendering, we simply store what entities are using what mesh
we also keep track of the index each entity is at in the mesh's list
*/
bool dm_renderer_api_register_mesh(dm_entity* entity, dm_mesh_component* component)
{
    dm_mesh* mesh = dm_map_get(mesh_map, component->name);
    
    component->index = mesh->entities->count;
    
    dm_list_append(mesh->entities, &entity->id);
    
    return true;
}

bool dm_renderer_api_deregister_mesh(dm_entity* entity)
{
    dm_mesh_component* mesh_c = dm_ecs_get_component(entity->id, DM_COMPONENT_MESH);
    
    dm_mesh* mesh = dm_map_get(mesh_map, mesh_c->name);
    
    for(uint32_t i=mesh_c->index; i<mesh->entities->count; i++)
    {
        uint32_t id = *(uint32_t*)dm_list_at(mesh->entities, i);
        dm_mesh_component* c = dm_ecs_get_component(id, DM_COMPONENT_MESH);
        c->index -= 1;
    }
    
    dm_list_pop_at(mesh->entities, mesh_c->index);
    
    return true;
}

bool dm_renderer_api_register_camera(dm_entity* entity, dm_editor_camera* component)
{
    camera = dm_ecs_get_component(entity->id, DM_COMPONENT_EDITOR_CAMERA);
    
    return true;
}

void dm_renderer_api_set_camera_pos(dm_vec3 pos)
{
	dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos)
{
	dm_vec3 pos = dm_vec3_add_vec3(r_data.camera.pos, delta_pos);
	dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_api_set_camera_forward(dm_vec3 forward)
{
	dm_camera_set_forward(&r_data.camera, forward);
}

void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward)
{
	dm_vec3 forward = dm_vec3_add_vec3(r_data.camera.forward, delta_forward);
	forward.x = DM_CLAMP(forward.x, -89, 89);
	forward.y = DM_CLAMP(forward.y, -89, 89);
	forward.z = DM_CLAMP(forward.z, -89, 89);
	dm_camera_set_forward(&r_data.camera, forward);
}

dm_vec3 dm_renderer_api_get_camera_forward()
{
	return r_data.camera.forward;
}

dm_vec3 dm_renderer_api_get_camera_up()
{
	return r_data.camera.up;
}

dm_vec3 dm_renderer_api_get_camera_pos()
{
	return r_data.camera.pos;
}