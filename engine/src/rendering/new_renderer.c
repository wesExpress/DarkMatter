#include "dm_renderer.h"
#include "dm_renderer_api.h"
#include "dm_vertex_attribs.h"
#include "dm_command_buffer.h"
#include "dm_image.h"
#include "dm_geometry.h"
#include "dm_uniform.h"
#include "dm_model.h"
#include "dm_default_render_passes.h"

#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_math.h"

#include <stddef.h>

#define DM_MAX_INSTANCES 10000

// buffer data struct
typedef struct dm_buffer_data
{
    dm_buffer vertex_buffer;
    dm_buffer index_buffer;
    dm_buffer instance_buffer;
} dm_buffer_data;

// render pass and pipeline
extern bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout layout);
extern void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass);

// forward declaration of the implementation, or backend, functionality
extern bool dm_renderer_init_impl(dm_platform_data* platform_data);
extern void dm_renderer_shutdown_impl();
extern bool dm_renderer_end_frame_impl();

extern bool dm_renderer_create_pipeline_impl(dm_render_pipeline_state* pipeline);
extern void dm_renderer_destroy_pipeline_impl(dm_render_pipeline_state* pipeline);
extern bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data);

// vertex data
dm_list* vertices = NULL;
dm_list* indices = NULL;

// meshes
dm_map* mesh_map = NULL;

// render passes and pipelines
dm_map* render_passes = NULL;
dm_map* pipelines = NULL;

dm_buffer_data static_buffer = {
    .vertex_buffer = {.desc= { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .usage=DM_BUFFER_USAGE_DEFAULT , .name = "vertex"}},
    .index_buffer = {.desc= { .type = DM_BUFFER_TYPE_INDEX, .data_t = DM_BUFFER_DATA_T_UINT, .usage=DM_BUFFER_USAGE_DEFAULT, .name = "index"}},
    .instance_buffer = {.desc= { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .buffer_size = sizeof(dm_inst_data) * DM_MAX_INSTANCES, .elem_size=sizeof(dm_vertex_inst), .usage = DM_BUFFER_USAGE_DYNAMIC, .cpu_access = DM_BUFFER_CPU_WRITE, .name = "instance"}}
};

/*********************
RENDER PIPELINE STATE
***********************/

bool dm_renderer_create_pipeline_state()
{
    return dm_renderer_create_pipeline_impl(NULL);
}

void dm_renderer_destroy_pipeline(dm_render_pipeline_state* pipeline)
{
	dm_renderer_destroy_pipeline_impl(pipeline);
}

/***********
RENDER PASS
*************/

bool dm_renderer_create_default_render_passes()
{
    if(!dm_renderer_create_material_pass()) return false;
    if(!dm_renderer_create_material_color_pass()) return false;
    if(!dm_renderer_create_light_src_pass()) return false;
    
    return true;
}

bool dm_renderer_create_render_pass(dm_shader shader, dm_vertex_layout layout, dm_uniform* uniforms, uint32_t num_uniforms,
                                    dm_render_pipeline_state pipeline_state, char* tag)
{
    dm_render_pass render_pass = { 0 };
    render_pass.name = tag;
    
    render_pass.pipeline_state = pipeline_state;
    render_pass.shader = shader;
    render_pass.shader.pass = tag;
    
    render_pass.uniforms = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_uniform), 0);
    
    for(uint32_t i=0; i<num_uniforms; i++)
    {
        dm_map_insert(render_pass.uniforms, uniforms[i].name, &uniforms[i]);
    }
    
    if(!dm_renderer_create_render_pass_impl(&render_pass, layout)) return false;
    
    dm_map_insert(render_passes, (void*)tag, &render_pass);
    
    return true;
}

void dm_renderer_destroy_render_pass(dm_render_pass* render_pass)
{
    dm_renderer_destroy_render_pass_impl(render_pass);
    
    dm_for_map_item(render_pass->uniforms)
    {
        dm_uniform* uniform = item->value;
        dm_destroy_uniform(uniform);
    }
    dm_map_destroy(render_pass->uniforms);
}

/*************
MAIN RENDERER
***************/
bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	//r_data.clear_color = clear_color;
    
	dm_viewport viewport = {
		.x = 0, .y = 0,
		.width = platform_data->window_width,
		.height = platform_data->window_height,
		.max_depth = 1.0f
	};
    
	if(!dm_renderer_init_impl(platform_data))
	{
		DM_LOG_FATAL("Renderer backend could not be initialized!");
		return false;
	}
    
    // vertex data
	vertices = dm_list_create(sizeof(dm_vertex_t), 0);
	indices = dm_list_create(sizeof(dm_index_t), 0);
    
    // render passes
    render_passes = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_render_pass), 0);
    
    // mesh
	mesh_map = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_mesh), 0);
    
    // pipelines
    pipelines = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_render_pipeline_state), 0);
    
    dm_render_command_init();
    
    // images/textures
    dm_image_map_init();
    
    // camera
    /*
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
    */
    // primitives
    dm_model_loader_init();
    
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
    
    // meshes
	dm_for_map_item(mesh_map)
    {
        dm_mesh* mesh = item->value;
        dm_list_destroy(mesh->entities);
    }
    dm_map_destroy(mesh_map);
    
    // render passes
    dm_for_map_item(render_passes)
    {
        dm_render_pass* render_pass = item->value;
        dm_renderer_destroy_render_pass(render_pass);
    }
    dm_map_destroy(render_passes);
    
    // pipelines
    dm_for_map_item(pipelines)
    {
        dm_render_pipeline_state* pipeline = item->value;
        dm_renderer_destroy_pipeline(pipeline);
    }
    dm_map_destroy(pipelines);
    
    dm_model_loader_shutdown();
    
    dm_render_command_shutdown();
    
    dm_image_map_destroy();
    
    // backend shutdown
    dm_renderer_shutdown_impl();
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
    
    dm_render_command_set_viewport(&viewport);
}

bool dm_renderer_begin_frame()
{
	//dm_render_command_clear(&r_data.clear_color, r_data.render_commands);
	//dm_render_command_set_viewport(&r_data.viewport, r_data.render_commands);
	//dm_render_command_bind_pipeline(r_data.pipeline, r_data.render_commands);
    
	/************************
	    material render passes
	*******************************/
    //if(!dm_renderer_render_materials()) return false;
    //if(!dm_renderer_render_colored_materials()) return false;
	
	return true;
}

bool dm_renderer_end_frame()
{
	/**********************
	  light source render pass
	****************************/
	//dm_render_light_sources();
    
	return dm_renderer_end_frame_impl();
}

bool dm_renderer_init_buffer_data()
{
    static_buffer.vertex_buffer.desc.buffer_size = vertices->count * vertices->element_size;
    static_buffer.vertex_buffer.desc.elem_size=vertices->element_size;
    
    static_buffer.index_buffer.desc.buffer_size = indices->count * indices->element_size;
    static_buffer.index_buffer.desc.elem_size=indices->element_size;
    
    if(!dm_renderer_init_buffer_data_impl( &static_buffer.vertex_buffer, vertices->data)) return false;
    
    if(!dm_renderer_init_buffer_data_impl( &static_buffer.index_buffer, indices->data)) return false;
    
    if(!dm_renderer_init_buffer_data_impl( &static_buffer.vertex_buffer, NULL)) return false;
    
    return true;
}

/***
API
****/

void dm_renderer_api_submit_vertex_data(const char* tag, dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices, bool is_indexed)
{
    dm_mesh mesh = {0};
    mesh.index_count = num_indices;
    mesh.vertex_offset = vertices->count;
    mesh.index_offset = indices->count;
    mesh.is_indexed = is_indexed;
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
        uint32_t offset_index = index_data[i] + mesh.index_offset;
		dm_list_append(indices, &offset_index);
	}
}

bool dm_renderer_api_register_image(dm_image_desc desc)
{
    return dm_load_image(desc);
}

void dm_renderer_api_set_clear_color(dm_vec3 color)
{
	//r_data.clear_color = dm_vec4_set_from_vec3(color);
}

/*
to simplify rendering, we simply store what entities are using what mesh
we also keep track of the index each entity is at in the mesh's list
*/

bool dm_renderer_api_register_mesh(dm_entity entity, dm_mesh_component* component)
{
    dm_mesh* mesh = dm_map_get(mesh_map, component->name);
    
    if(!mesh)
    {
        DM_LOG_FATAL("Trying to access a non-existent mesh: %s", component->name);
        return false;
    }
    
    component->index = mesh->entities->count;
    
    dm_list_append(mesh->entities, &entity);
    
    return true;
}

bool dm_renderer_api_deregister_mesh(dm_entity entity)
{
    dm_mesh_component* mesh_c = dm_ecs_get_component(entity, DM_COMPONENT_MESH);
    
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

bool dm_renderer_api_register_camera(dm_entity entity, dm_editor_camera* component)
{
    //camera = dm_ecs_get_component(entity, DM_COMPONENT_EDITOR_CAMERA);
    
    return true;
}

void dm_renderer_api_set_camera_pos(dm_vec3 pos)
{
	//dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos)
{
	//dm_vec3 pos = dm_vec3_add_vec3(r_data.camera.pos, delta_pos);
	//dm_camera_set_pos(&r_data.camera, pos);
}

void dm_renderer_api_set_camera_forward(dm_vec3 forward)
{
	//dm_camera_set_forward(&r_data.camera, forward);
}

void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward)
{
    /*
	dm_vec3 forward = dm_vec3_add_vec3(r_data.camera.forward, delta_forward);
	forward.x = DM_CLAMP(forward.x, -89, 89);
	forward.y = DM_CLAMP(forward.y, -89, 89);
	forward.z = DM_CLAMP(forward.z, -89, 89);
	dm_camera_set_forward(&r_data.camera, forward);
*/
}

dm_vec3 dm_renderer_api_get_camera_forward()
{
	//return r_data.camera.forward;
}

dm_vec3 dm_renderer_api_get_camera_up()
{
	//return r_data.camera.up;
}

dm_vec3 dm_renderer_api_get_camera_pos()
{
	//return r_data.camera.pos;
}