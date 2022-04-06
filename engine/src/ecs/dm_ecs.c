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

void dm_insert_component(dm_component component, dm_entity* entity, void* data);
bool dm_add_transform(dm_entity* entity, void* data);
bool dm_add_mesh(dm_entity* entity, void* data);
bool dm_add_texture(dm_entity* entity, void* data);
bool dm_add_color(dm_entity* entity, void* data);
bool dm_add_light_src(dm_entity* entity, void* data);
bool dm_add_editor_camera(dm_entity* entity, void* data);

typedef struct dm_ecs_manager
{
    dm_map* component_registry[DM_COMPONENT_UNKNOWN];
    dm_list* entity_registry[DM_COMPONENT_UNKNOWN];
} dm_ecs_manager;

dm_ecs_manager manager = {0};

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

bool dm_ecs_add_component(dm_entity* entity, dm_component component, void* data)
{
    if(component == DM_COMPONENT_UNKNOWN)
    {
        DM_LOG_FATAL("Trying to add unknown component!;");
        return false;
    }
    
    if(dm_ecs_entity_has_component(entity, component))
    {
        DM_LOG_FATAL("Trying to add an existing component to entiy!");
        return false;
    }
    
    entity->component_mask |= BIT_SHIFT(component);
    
    switch(component)
    {
        case DM_COMPONENT_TRANSFORM:
        {
            if(!dm_add_transform(entity, data)) return false;
        } break;
        case DM_COMPONENT_MESH:
        {
            if(!dm_add_mesh(entity, data)) return false;
        } break;
        case DM_COMPONENT_COLOR:
        {
            if(!dm_add_color(entity, data)) return false;
        } break;
        case DM_COMPONENT_LIGHT_SRC:
        {
            if(!dm_add_light_src(entity, data)) return false;
        } break;
        case DM_COMPONENT_EDITOR_CAMERA:
        {
            if(!dm_add_editor_camera(entity, data)) return false;
        } break;
        default:
        {
            DM_LOG_ERROR("Component not handled yet.");
        }
    }
    
    dm_list_append(manager.entity_registry[component], &(entity->id));
    
    return true;
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

bool dm_add_transform(dm_entity* entity, void* data)
{
    dm_transform_component transform = {0};
    
    if (data)
    {
        transform = *(dm_transform_component*)data;
    }
    else
    {
        transform.scale = dm_vec3_set(1,1,1);
    }
    
    dm_insert_component(DM_COMPONENT_TRANSFORM, entity, &transform);
    
    return true;
}

bool dm_add_mesh(dm_entity* entity, void* data)
{
    dm_mesh_component mesh = {0};
    
    mesh.name = data;
    dm_renderer_api_register_mesh(entity, &mesh);
    
    dm_insert_component(DM_COMPONENT_MESH, entity, &mesh);
    
    return true;
}

bool dm_add_texture(dm_entity* entity, void* data)
{
    dm_texture_component texture = {0};
    
    texture.name = data;
    
    dm_insert_component(DM_COMPONENT_TEXTURE, entity, &texture);
    return true;
}

bool dm_add_color(dm_entity* entity, void* data)
{
    dm_color_component color = {0};
    
    color = *(dm_color_component*)data;
    
    dm_insert_component(DM_COMPONENT_COLOR, entity, &color);
    
    return true;
}

bool dm_add_light_src(dm_entity* entity, void* data)
{
    dm_light_src_component light = {0};
    
    light = *(dm_light_src_component*)data;
    
    if(light.sync_to_obj_color)
    {
        if(dm_ecs_entity_has_component(entity, DM_COMPONENT_COLOR))
        {
            dm_color_component color = *(dm_color_component*)dm_ecs_get_component(entity->id, DM_COMPONENT_COLOR);
            light.color = color.color;
        }
        else
        {
            DM_LOG_ERROR("Can't sync light source color to non-existant object color. Setting to false.");
            light.sync_to_obj_color = false;
        }
    }
    
    dm_insert_component(DM_COMPONENT_LIGHT_SRC, entity, &light);
    
    return true;
}

bool dm_add_editor_camera(dm_entity* entity, void* data)
{
    dm_editor_camera camera = {0};
    
    if(data)
    {
        camera = *(dm_editor_camera*)data;
    }
    else
    {
        camera.pos = dm_vec3_set(1,1,1);
        camera.forward = dm_vec3_set(0,0,0);
        camera.up = dm_vec3_set(0,1,0);
        
        camera.pitch= 0;
        camera.yaw = -90;
        camera.roll = 0;
        
        camera.move_velocity = 2.5f;
        camera.look_sens = 0.1f;
    }
    
    dm_insert_component(DM_COMPONENT_EDITOR_CAMERA, entity, &camera);
    
    return true;
}

void dm_insert_component(dm_component component, dm_entity* entity, void* data)
{
    dm_map_insert(manager.component_registry[component], &(entity->id), data);
}

dm_list* dm_ecs_get_entity_registry(dm_component component)
{
    return manager.entity_registry[component];
}