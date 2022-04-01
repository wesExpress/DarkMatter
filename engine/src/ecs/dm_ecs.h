#ifndef __DM_ECS_H__
#define __DM_ECS_H__

#include "core/dm_defines.h"
#include "core/dm_hash.h"
#include "dm_components.h"
#include <stdbool.h>

typedef dm_hash dm_entity;

void dm_ecs_init();
void dm_ecs_shutdown();

/*
create an entity
*/
DM_API dm_entity dm_ecs_new_entity();

/*
delete a specified entity
*/
DM_API void dm_ecs_delete_entity(dm_entity entity);

/*
add a component of a specific type (enum) with data
*/
DM_API bool dm_ecs_add_component(dm_entity* entity, dm_component component, void* data);

/*
remove a component of a specific type (enum)
*/
DM_API bool dm_ecs_remove_component(dm_entity entity, dm_component component);

/*
retrieve a specified component
*/
DM_API void* dm_ecs_get_component(dm_entity entity, dm_component component);

/*
check if an entity has a given component
*/
DM_API bool dm_ecs_entity_has_component(dm_entity entity, dm_component component);

#endif //DM_ECS_H
