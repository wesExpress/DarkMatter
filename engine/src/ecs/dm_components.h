#ifndef __DM_COMPONENTS_H__
#define __DM_COMPONENTS_H__

#include "core/dm_math_types.h"
#include "rendering/dm_render_types.h"
#include <stdint.h>

typedef enum dm_component
{
    DM_COMPONENT_TRANSFORM,
    DM_COMPONENT_MESH,
    DM_COMPONENT_TEXTURE,
    DM_COMPONENT_MATERIAL,
    DM_COMPONENT_COLOR,
    DM_COMPONENT_LIGHT_SRC,
    DM_COMPONENT_EDITOR_CAMERA,
    DM_COMPONENT_UNKNOWN
} dm_component;

typedef struct dm_transform_component
{
	dm_vec3 position;
	dm_vec3 scale;
} dm_transform_component;

typedef struct dm_mesh_component
{
    char* name;
    uint32_t index;
} dm_mesh_component;

typedef struct dm_texture_component
{
    char* name;
} dm_texture_component;

typedef struct dm_material_component
{
    char* diffuse_map;
    char* specular_map;
    float shininess;
} dm_material_component;

typedef struct dm_color_component
{
    dm_color diffuse;
    dm_color specular;
    float shininess;
} dm_color_component;

typedef struct dm_light_src_component
{
    dm_color ambient;
    dm_color diffuse;
    dm_color specular;
    float strength;
} dm_light_src_component;

typedef struct dm_editor_camera
{
    dm_vec3 pos;
	float pitch, yaw, roll;
	float move_velocity, look_sens;
	uint32_t last_x, last_y;
} dm_editor_camera;

typedef struct dm_game_object
{
	dm_transform_component transform;
	dm_vec3 color;
	const char* texture;
	const char* mesh;
	const char* render_pass;
} dm_game_object;

#endif