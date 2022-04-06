#include "dm_ecs.h"

#include "core/dm_logger.h"
#include "core/dm_random.h"
#include "core/dm_math.h"

#include "structures/dm_map.h"

#include "rendering/dm_renderer_api.h"

#define BIT_SHIFT(X) 1 << X

size_t component_sizes[] = {
    sizeof(dm_transform_component),
    sizeof(dm_mesh_component),
    sizeof(dm_texture_component),
    sizeof(dm_color_component),
    sizeof(dm_light_src_component),
    sizeof(dm_editor_camera)
};

typedef struct dm_ecs_manager
{
    dm_map* component_registry[DM_COMPONENT_UNKNOWN];
    dm_list* entity_registry[DM_COMPONENT_UNKNOWN];
} dm_ecs_manager;

bool dm_ecs_insert_component(dm_entity* entity, dm_component component, void* data);

dm_ecs_manager manager = {0};

//

void dm_ecs_init()
{
    for(uint32_t i=0; i<DM_COMPONENT_UNKNOWN; i++)
    {
        manager.component_registry[i] = dm_map_create(DM_MAP_KEY_UINT32, component_sizes[i], 0);
        manager.entity_registry[i] = dm_list_create(sizeof(uint32_t), 0);
    }
}

void dm_ecs_shutdown()
{
    for (uint32_t i=0; i<DM_COMPONENT_UNKNOWN; i++)
    {
        dm_map_destroy(manager.component_registry[i]);
        dm_list_destroy(manager.entity_registry[i]);
    }
}

dm_entity dm_ecs_create_entity()
{
    dm_entity entity = {0};
    
    entity.id = dm_random_uint32();
    
    return entity;
}

void dm_ecs_delete_entity(dm_entity* entity)
{
    for(uint32_t i=0; i<DM_COMPONENT_UNKNOWN; i++)
    {
        if(dm_map_exists(manager.component_registry[i], entity))
        {
            dm_map_delete_elem(manager.component_registry[i], entity);
        }
    }
}

bool dm_ecs_remove_component(dm_entity* entity, dm_component component)
{
    if(dm_ecs_entity_has_component(entity, component))
    {
        switch(component)
        {
            case DM_COMPONENT_MESH:
            {
                dm_renderer_api_deregister_mesh(entity);
            } break;
            default: break;
        }
        
        entity->component_mask &= ~BIT_SHIFT(component);
        
        return true;
    }
    
    DM_LOG_FATAL("Trying to remove a non-existent component from entity!");
    return false;
}

void* dm_ecs_get_component(uint32_t entity_id, dm_component component)
{
    return dm_map_get(manager.component_registry[component], &entity_id);
}

bool dm_ecs_entity_has_component(dm_entity* entity, dm_component component)
{
    return (entity->component_mask & BIT_SHIFT(component));
}

bool dm_ecs_add_transform(dm_entity* entity, dm_transform_component* transform)
{
    if(!transform)
    {
        transform->scale = dm_vec3_set(1,1,1);
    }
    
    return dm_ecs_insert_component(entity, DM_COMPONENT_TRANSFORM, transform);
}

bool dm_ecs_add_mesh(dm_entity* entity, dm_mesh_component* mesh)
{
    dm_renderer_api_register_mesh(entity, mesh);
    
    return dm_ecs_insert_component(entity, DM_COMPONENT_MESH, mesh);
}

bool dm_ecs_add_texture(dm_entity* entity, dm_texture_component* texture)
{
    return dm_ecs_insert_component(entity, DM_COMPONENT_TEXTURE, texture);
}

bool dm_ecs_add_color(dm_entity* entity, dm_color_component* color)
{
    return dm_ecs_insert_component(entity, DM_COMPONENT_COLOR, color);
}

bool dm_ecs_add_light_src(dm_entity* entity, dm_light_src_component* light_src)
{
    if(light_src->sync_to_obj_color)
    {
        if(dm_ecs_entity_has_component(entity, DM_COMPONENT_COLOR))
        {
            dm_color_component color = *(dm_color_component*)dm_ecs_get_component(entity->id, DM_COMPONENT_COLOR);
            light_src->color = color.color;
        }
        else
        {
            DM_LOG_ERROR("Can't sync light source color to non-existant object color. Setting to false.");
            light_src->sync_to_obj_color = false;
        }
    }
    
    return dm_ecs_insert_component(entity, DM_COMPONENT_LIGHT_SRC, light_src);
}

bool dm_ecs_add_editor_camera(dm_entity* entity, dm_editor_camera* camera)
{
    if(!camera)
    {
        dm_editor_camera default_camera = {0};
        
        default_camera.pos = dm_vec3_set(1,1,1);
        default_camera.up = dm_vec3_set(0,1,0);
        
        default_camera.yaw = -90;
        
        default_camera.move_velocity = 2.5f;
        default_camera.look_sens = 0.1f;
        
        return dm_ecs_insert_component(entity, DM_COMPONENT_EDITOR_CAMERA, &default_camera);
    }
    
    return dm_ecs_insert_component(entity, DM_COMPONENT_EDITOR_CAMERA, camera);
}

bool dm_ecs_insert_component(dm_entity* entity, dm_component component, void* data)
{
    if(dm_ecs_entity_has_component(entity, component))
    {
        DM_LOG_FATAL("Trying to add already existing component to entity!");
        return false;
    }
    
    dm_map_insert(manager.component_registry[component], &entity->id, data);
    dm_list_append(manager.entity_registry[component], &entity->id);
    entity->component_mask |= BIT_SHIFT(component);
    
    return true;
}

dm_list* dm_ecs_get_entity_registry(dm_component component)
{
    return manager.entity_registry[component];
}