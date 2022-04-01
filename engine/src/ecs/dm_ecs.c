#include "dm_ecs.h"
#include "core/dm_logger.h"
#include "structures/dm_map.h"
#include "core/dm_random.h"

dm_map* transforms = NULL;

void dm_ecs_init()
{
    transforms = dm_map_create(DM_MAP_KEY_UINT32, sizeof(dm_transform), 0);
}

void dm_ecs_shutdown()
{
    dm_map_destroy(transforms);
}

dm_entity dm_ecs_create_entity()
{
    return dm_random_uint32();
}

void dm_ecs_delete_entity(dm_entity entity)
{
    
}

bool dm_ecs_add_component(dm_entity entity, dm_component component, void* data)
{
    if(component == DM_COMPONENT_UNKNOWN)
    {
        DM_LOG_FATAL("Trying to add unknown component!;");
        return false;
    }
    
    switch(component)
    {
        case DM_COMPONENT_TRANSFORM:
        {
            dm_transform transform;
            
            if (data)
            {
                transform = *(dm_transform*)data;
            }
            else
            {
                transform = (dm_transform){
                    .position = {0,0,0},
                    .scale = {1,1,1}
                };
            }
            
            dm_map_insert(transforms, &entity, &transform); 
            
        } break;
        
    }
    
    return true;
}


bool dm_ecs_remove_component(dm_entity entity, dm_component component)
{
    return true;
}

void* dm_ecs_get_component(dm_entity entity, dm_component component)
{
    if (dm_ecs_entity_has_component(entity, component))
    {
        return dm_map_get(transforms, &entity);
    }
    
    return NULL;
}

bool dm_ecs_entity_has_component(dm_entity entity, dm_component component)
{
    switch(component)
    {
        case DM_COMPONENT_TRANSFORM:
        {
            return dm_map_exists(transforms, &entity);
        } break;
        default:
        {
            DM_LOG_ERROR("Uknown component!");
            return false;
        }
    }
}