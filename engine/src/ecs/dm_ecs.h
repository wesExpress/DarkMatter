#ifndef __DM_ECS_H__
#define __DM_ECS_H__

#include "dm_components.h"

#include "core/dm_defines.h"
#include "core/dm_hash.h"
#include <stdbool.h>
#include <stdint.h>

enum dm_component;

typedef struct dm_entity
{
    uint32_t id;
    uint32_t component_mask;
} dm_entity;

void dm_ecs_init();
void dm_ecs_shutdown();

/*
create an entity
*/
DM_API dm_entity dm_ecs_create_entity();

/*
delete a specified entity
*/
DM_API void dm_ecs_delete_entity(dm_entity* entity);

/*
add a component of a specific type (enum) with data
*/
DM_API bool dm_ecs_add_component(dm_entity* entity, dm_component component, void* data);

/*
remove a component of a specific type (enum)
*/
DM_API bool dm_ecs_remove_component(dm_entity* entity, dm_component component);

/*
retrieve a specified component
*/
DM_API void* dm_ecs_get_component(uint32_t entity_id, dm_component component);

/*
check if an entity has a given component
*/
DM_API bool dm_ecs_entity_has_component(dm_entity* entity, dm_component component);

/*
get the entity register for a component
*/
DM_API dm_list* dm_ecs_get_entity_registry(dm_component component);

#endif //DM_ECS_H
