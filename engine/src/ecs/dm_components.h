#ifndef __DM_COMPONENTS_H__
#define __DM_COMPONENTS_H__

#include "core/math/dm_math_types.h"

typedef struct dm_transform
{
	dm_vec3 position;
	dm_vec3 scale;
} dm_transform;

typedef struct dm_editor_camera
{
	float pitch, yaw, roll;
	float move_velocity, look_sens;
	uint32_t last_x, last_y;
} dm_editor_camera;

typedef struct dm_game_object
{
	dm_transform transform;
	dm_vec3 color;
	const char* texture;
	const char* mesh;
} dm_game_object;

#endif