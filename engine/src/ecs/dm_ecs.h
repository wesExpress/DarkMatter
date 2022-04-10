#ifndef __DM_ECS_H__
#define __DM_ECS_H__

#include "dm_components.h"

#include "core/dm_defines.h"
#include "core/dm_hash.h"
#include <stdbool.h>
#include <stdint.h>

enum dm_component;

typedef uint32_t dm_entity;

void dm_ecs_init();
void dm_ecs_shutdown();

/*
create an entity
*/
DM_API dm_entity dm_ecs_create_entity();

/*
delete a specified entity
*/
DM_API void dm_ecs_delete_entity(dm_entity entity);

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

/*
get the entity register for a component
*/
DM_API dm_list* dm_ecs_get_entity_registry(dm_component component);

// all of the actual adding of components
DM_API bool dm_ecs_add_transform(dm_entity entity, dm_transform_component* transform);
DM_API bool dm_ecs_add_mesh(dm_entity entity, dm_mesh_component* mesh);
DM_API bool dm_ecs_add_texture(dm_entity entity, dm_texture_component* texture);
DM_API bool dm_ecs_add_material(dm_entity entity, dm_material_component* material);
DM_API bool dm_ecs_add_color(dm_entity entity, dm_color_component* color);
DM_API bool dm_ecs_add_light_src(dm_entity entity, dm_light_src_component* light_src);
DM_API bool dm_ecs_add_editor_camera(dm_entity entity, dm_editor_camera* camera);

#endif //DM_ECS_H
