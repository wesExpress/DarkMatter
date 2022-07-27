#include "dm_renderer.h"
#include "dm_renderer_api.h"
#include "dm_vertex_attribs.h"
#include "dm_image.h"
#include "dm_geometry.h"
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
extern bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, const char* vertex_src, const char* pixel_src, dm_vertex_layout layout, size_t scene_cb_size, size_t object_cb_size);
extern void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass);

// forward declaration of the implementation, or backend, functionality
extern bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_render_pipeline* pipeline);
extern void dm_renderer_shutdown_impl();
extern bool dm_renderer_begin_frame_impl();
extern bool dm_renderer_end_frame_impl();

extern bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data);
extern void dm_renderer_delete_buffer_impl(dm_buffer* buffer);

/*******
GLOBALS
*********/

dm_list* vertices = NULL;
dm_list* indices = NULL;
dm_list* render_commands = NULL;
dm_map* mesh_map = NULL;
dm_map* render_passes = NULL;
dm_viewport default_viewport = {0};
dm_camera camera;

// default data
dm_buffer_data static_buffer = {
    .vertex_buffer = {.desc= { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .usage=DM_BUFFER_USAGE_DEFAULT , .name = "vertex"}},
    .index_buffer = {.desc= { .type = DM_BUFFER_TYPE_INDEX, .data_t = DM_BUFFER_DATA_T_UINT, .usage=DM_BUFFER_USAGE_DEFAULT, .name = "index"}},
    .instance_buffer = {.desc= { .type = DM_BUFFER_TYPE_VERTEX, .data_t = DM_BUFFER_DATA_T_FLOAT, .buffer_size = sizeof(dm_vertex_inst) * DM_MAX_INSTANCES, .elem_size=sizeof(dm_vertex_inst), .usage = DM_BUFFER_USAGE_DYNAMIC, .cpu_access = DM_BUFFER_CPU_WRITE, .name = "instance"}}
};

/***************
RENDER COMMANDS
*****************/

void dm_render_command_shutdown()
{
    dm_list_destroy(render_commands);
}

void dm_renderer_submit_command(dm_render_command_type command_type, dm_byte_buffer* buffer)
{
	dm_render_command command = { 0 };
    command.type = command_type;
    command.buffer = buffer;
    
	dm_list_append(render_commands, &command);
}

void dm_render_command_begin_renderpass(dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &render_pass->internal_index, sizeof(uint32_t));
    dm_byte_buffer_push(buffer, &render_pass->shader.internal_index, sizeof(uint32_t));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, buffer);
}

void dm_render_command_end_renderpass(dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &render_pass->internal_index, sizeof(uint32_t));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, buffer);
}

void dm_render_command_set_viewport(dm_viewport viewport)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &viewport, sizeof(dm_viewport));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, buffer);
}

void dm_render_command_clear(dm_color color)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &color, sizeof(dm_color));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, buffer);
}

void dm_render_command_update_buffer(dm_buffer* buffer, void* data, size_t data_size)
{
    dm_byte_buffer* byte_buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(byte_buffer, data, data_size);
    dm_byte_buffer_push(byte_buffer, &data_size, sizeof(data_size));
    dm_byte_buffer_push(byte_buffer, &(buffer->internal_index), sizeof(buffer->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_BUFFER, byte_buffer);
}

void dm_render_command_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_render_pass* render_pass)
{
    dm_byte_buffer* byte_buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(byte_buffer, &(buffer->internal_index), sizeof(buffer->internal_index));
    dm_byte_buffer_push(byte_buffer, &slot, sizeof(slot));
    dm_byte_buffer_push(byte_buffer, &(buffer->desc.type), sizeof(buffer->desc.type));
    dm_byte_buffer_push(byte_buffer, &(buffer->desc.elem_size), sizeof(buffer->desc.elem_size));
    dm_byte_buffer_push(byte_buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_BUFFER, byte_buffer);
}

void dm_render_command_update_scene_cb(void* data, size_t data_size, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, data, data_size);
    dm_byte_buffer_push(buffer, &data_size, sizeof(data_size));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_SCENE_CB, buffer);
}

void dm_render_command_update_inst_cb(void* data, size_t data_size, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, data, data_size);
    dm_byte_buffer_push(buffer, &data_size, sizeof(data_size));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_INST_CB, buffer);
}

void dm_render_command_bind_texture(dm_image* image, uint32_t slot, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &(image->internal_index), sizeof(image->internal_index));
    dm_byte_buffer_push(buffer, &slot, sizeof(slot));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_TEXTURE, buffer);
}

void dm_render_command_bind_uniforms(uint32_t slot, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &slot, sizeof(slot));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_UNIFORMS, buffer);
}

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &start, sizeof(start));
    dm_byte_buffer_push(buffer, &count, sizeof(count));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_ARRAYS, buffer);
}

void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &num_indices, sizeof(num_indices));
    dm_byte_buffer_push(buffer, &index_offset, sizeof(index_offset));
    dm_byte_buffer_push(buffer, &vertex_offset, sizeof(vertex_offset));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, buffer);
}

void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
    dm_byte_buffer* buffer = dm_byte_buffer_create();
    
    dm_byte_buffer_push(buffer, &num_indices, sizeof(num_indices));
    dm_byte_buffer_push(buffer, &num_insts, sizeof(num_insts));
    dm_byte_buffer_push(buffer, &index_offset, sizeof(index_offset));
    dm_byte_buffer_push(buffer, &vertex_offset, sizeof(vertex_offset));
    dm_byte_buffer_push(buffer, &inst_offset, sizeof(inst_offset));
    dm_byte_buffer_push(buffer, &(render_pass->internal_index), sizeof(render_pass->internal_index));
    
    dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INSTANCED, buffer);
}

void dm_renderer_clear_command_buffer()
{
    if(render_commands->count > 0) 
    {
        dm_for_list_item(render_commands, dm_render_command, command)
        {
            dm_byte_buffer_destroy(command->buffer);
        }
        dm_list_clear(render_commands, 0);
    }
    else 
    {
        DM_LOG_WARN("Trying to clear a 0 length command buffer.");
    }
}

bool dm_renderer_submit_command_buffer()
{
    return dm_renderer_submit_command_buffer_impl(render_commands);
}

/***********
RENDER PASS
*************/

bool dm_renderer_create_default_render_passes()
{
    if(!dm_renderer_create_default_pass()) return false;
    
    return true;
}

bool dm_renderer_create_render_pass(const char* vertex_src, const char* pixel_src, dm_vertex_layout layout, size_t scene_cb_size, size_t object_cb_size, char* tag)
{
    dm_render_pass render_pass = { 0 };
    
    if(!dm_renderer_create_render_pass_impl(&render_pass, vertex_src, pixel_src, layout, scene_cb_size, object_cb_size)) return false;
    
    dm_map_insert(render_passes, (void*)tag, &render_pass);
    
    return true;
}

void dm_renderer_destroy_render_pass(dm_render_pass* render_pass)
{
    dm_renderer_destroy_render_pass_impl(render_pass);
}

/*************
MAIN RENDERER
***************/
bool dm_renderer_init(dm_platform_data* platform_data, dm_color clear_color)
{
	default_viewport.width = platform_data->window_width;
    default_viewport.height = platform_data->window_height;
    default_viewport.max_depth = 1.0f;
    
	if(!dm_renderer_init_impl(platform_data, &default_pipeline))
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
    
    // render commands
    render_commands = dm_list_create(sizeof(dm_render_command), 0);
    
    // images/textures
    dm_image_map_init();
    
    // camera
    dm_camera_init(&camera, (dm_vec3){0,0,0}, 70.0f, platform_data->window_width, platform_data->window_height, 0.01f, 10000.0f, DM_CAMERA_PERSPECTIVE);
    
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
    
    // buffers
    dm_renderer_delete_buffer_impl(&static_buffer.instance_buffer);
    dm_renderer_delete_buffer_impl(&static_buffer.vertex_buffer);
    dm_renderer_delete_buffer_impl(&static_buffer.index_buffer);
    
    // other globals
    dm_model_loader_shutdown();
    dm_image_map_destroy();
    dm_list_destroy(render_commands);
    
    // backend shutdown
    dm_renderer_shutdown_impl();
}

void dm_renderer_resize(int new_width, int new_height)
{
	dm_viewport new_viewport = {
		.x = 0,
		.y = new_height,
		.width = new_width,
		.height = new_height,
		.max_depth = 1.0f
	};
    
    default_viewport = new_viewport;
}

bool dm_default_pass()
{
    dm_render_pass* default_pass = dm_map_get(render_passes, "default");
    if(!default_pass) return false;
    
    dm_render_command_begin_renderpass(default_pass);
    
    dm_default_pass_scene_cb scene_cb = { 0 };
#ifndef DM_DIRECTX
    scene_cb.view_proj = camera.view_proj;
#else
    scene_cb.view_proj = dm_mat4_transpose(camera.view_proj);
#endif
    
    scene_cb.view_pos = camera.pos;
    
    dm_list* lights = dm_ecs_get_entity_registry(DM_COMPONENT_LIGHT_SRC);
    dm_entity* light_id = dm_list_at(lights, 0);
    dm_light_src_component* light = dm_ecs_get_component(*light_id, DM_COMPONENT_LIGHT_SRC);
    dm_transform_component* light_trans = dm_ecs_get_component(*light_id, DM_COMPONENT_TRANSFORM);
    
    scene_cb.light_ambient = light->ambient;
    scene_cb.light_diffuse = light->diffuse;
    scene_cb.light_specular = light->specular;
    scene_cb.light_pos = light_trans->position;
    
    // for each mesh
    dm_for_map_item(mesh_map)
    {
        dm_mesh* mesh = item->value;
        uint32_t count = 0;
        dm_list* instance_buffer = dm_list_create(sizeof(dm_vertex_inst), 0);
        dm_list* instance_array_buffer = dm_list_create(sizeof(dm_default_pass_inst_cb), 0);
        
        // materials first
        dm_for_list_item(mesh->entities, dm_entity, entity)
        {
            dm_transform_component* transform = dm_ecs_get_component(*entity, DM_COMPONENT_TRANSFORM);
            
            dm_vertex_inst inst = { 0 };
            dm_default_pass_inst_cb inst_cb = { 0 };
            
            inst.model = dm_mat4_identity();
            inst.model = dm_mat_translate(inst.model, transform->position);
            inst.model = dm_mat_scale(inst.model, transform->scale);
#ifdef DM_DIRECTX
            inst.model = dm_mat4_transpose(inst.model);
#endif
            
            if(dm_ecs_entity_has_component(*entity, DM_COMPONENT_MATERIAL))
            {
                dm_material_component* material = dm_ecs_get_component(*entity, DM_COMPONENT_MATERIAL);
                
                inst_cb.has_texture = 1;
                inst_cb.shininess = material->shininess;
                char* test = material->diffuse_map;
                size_t len = strlen(test) + 1;
                dm_image* diffuse_map = dm_image_get(material->diffuse_map);
                if(!diffuse_map)
                {
                    DM_LOG_FATAL("Could not retrieve diffuse map: %s", material->diffuse_map);
                    return false;
                }
                dm_image* specular_map = dm_image_get(material->specular_map);
                if(!specular_map)
                {
                    DM_LOG_FATAL("Could not retrieve specular map: %s", material->specular_map);
                    return false;
                }
                dm_render_command_bind_texture(diffuse_map, 0, default_pass);
                dm_render_command_bind_texture(specular_map, 1, default_pass);
            }
            else if(dm_ecs_entity_has_component(*entity, DM_COMPONENT_COLOR))
            {
                dm_color_component* color = dm_ecs_get_component(*entity, DM_COMPONENT_COLOR);
                
                inst.diffuse = color->diffuse;
                inst.specular = color->specular;
                inst_cb.shininess = color->shininess;
            }
            else if(dm_ecs_entity_has_component(*entity, DM_COMPONENT_LIGHT_SRC))
            {
                dm_light_src_component* light_src = dm_ecs_get_component(*entity, DM_COMPONENT_LIGHT_SRC);
                
                inst_cb.is_light = 1;
                inst.diffuse = light_src->diffuse;
            }
            
            dm_list_append(instance_buffer, &inst);
            dm_list_append(instance_array_buffer, &inst_cb);
            
            count++;
        }
        
        dm_render_command_update_buffer(&static_buffer.instance_buffer, instance_buffer->data, instance_buffer->element_size * instance_buffer->count);
        dm_render_command_update_scene_cb(&scene_cb, sizeof(scene_cb), default_pass);
        dm_render_command_update_inst_cb(&instance_array_buffer->data, sizeof(dm_default_pass_inst_cb) * instance_array_buffer->count, default_pass);
        dm_render_command_bind_buffer(&static_buffer.vertex_buffer, 0, default_pass);
        dm_render_command_bind_buffer(&static_buffer.index_buffer, 0, default_pass);
        dm_render_command_bind_buffer(&static_buffer.instance_buffer, 1, default_pass);
        dm_render_command_bind_uniforms(0, default_pass);
        dm_render_command_draw_instanced(mesh->index_count, count, mesh->index_offset, mesh->vertex_offset, 0, default_pass);
        
        dm_list_destroy(instance_buffer);
        dm_list_destroy(instance_array_buffer);
    }
    
    dm_render_command_end_renderpass(default_pass);
    
    return true;
}

bool dm_renderer_begin_frame()
{
    if(!dm_renderer_begin_frame_impl()) return false;
    
    dm_render_command_clear((dm_color){0,0,0,1});
    dm_render_command_set_viewport(default_viewport);
    
    /************
        default pass
    **************/
    if(!dm_default_pass()) return false;
    
    return true;
}

bool dm_renderer_end_frame()
{
    dm_renderer_submit_command_buffer();
    dm_renderer_clear_command_buffer();
    
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
    
    if(!dm_renderer_init_buffer_data_impl( &static_buffer.instance_buffer, NULL)) return false;
    
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
    
    dm_map_insert(mesh_map, (void*)tag, &mesh);
    
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

bool dm_renderer_api_register_image(const char* path, const char* name, bool flip, dm_texture_format format, dm_texture_format internal_format)
{
    return dm_load_image(path, name, flip, format, internal_format);
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

void dm_renderer_api_set_camera_pos(dm_vec3 pos)
{
    dm_camera_set_pos(&camera, pos);
}

void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos)
{
    dm_vec3 pos = dm_vec3_add_vec3(camera.pos, delta_pos);
    dm_camera_set_pos(&camera, pos);
}

void dm_renderer_api_set_camera_forward(dm_vec3 forward)
{
    dm_camera_set_forward(&camera, forward);
}

void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward)
{
    dm_vec3 forward = dm_vec3_add_vec3(camera.forward, delta_forward);
    forward.x = DM_CLAMP(forward.x, -89, 89);
    forward.y = DM_CLAMP(forward.y, -89, 89);
    forward.z = DM_CLAMP(forward.z, -89, 89);
    dm_camera_set_forward(&camera, forward);
}

dm_vec3 dm_renderer_api_get_camera_forward()
{
    return camera.forward;
}

dm_vec3 dm_renderer_api_get_camera_up()
{
    return camera.up;
}

dm_vec3 dm_renderer_api_get_camera_pos()
{
    return camera.pos;
}

dm_render_pass* dm_renderer_get_render_pass(char* tag)
{
    return dm_map_get(render_passes, tag);
}