#include "dm_ecs.h"
#include "core/dm_logger.h"

dm_entity dm_ecs_new_entity()
{
}

void dm_ecs_delete_entity(dm_entity entity)
{
}

bool dm_add_component(dm_entity* entity, dm_component component, void* data)
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
            dm_transform* transform = data;
            
            
        } break;
    }
    
    return true;
}

bool dm_ecs_remove_component(dm_entity* entity, dm_component component)
{
    return true;
}