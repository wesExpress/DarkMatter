#ifndef __DM_CAMERA_H__
#define __DM_CAMERA_H__

#include "core/dm_defines.h"
#include "core/dm_math_types.h"

typedef enum dm_camera_type
{
	DM_CAMERA_PERSPECTIVE,
	DM_CAMERA_ORTHOGRAPHIC,
	DM_CAMERA_UNKNOWN
} dm_camera_type;

typedef struct dm_camera
{
	dm_mat4 view;
	dm_mat4 projection;
	dm_mat4 view_proj;
    
	dm_vec3 euler;
	dm_vec3 forward;
	dm_vec3 up;
    
	dm_vec3 pos;
	float fov, aspect_ratio, nZ, fZ;
    
	dm_camera_type type;
} dm_camera;

void dm_camera_init(dm_camera* camera, dm_vec3 pos, float fov, int width, int height, float n, float f, dm_camera_type type);

void dm_camera_update_view_proj(dm_camera* camera);
void dm_camera_set_pos(dm_camera* camera, dm_vec3 pos);
void dm_camera_set_forward(dm_camera* camera, dm_vec3 forward);
void dm_camera_set_view(dm_camera* camera, dm_vec3 view_origin, dm_vec3 target, dm_vec3 up);
void dm_camera_set_projection(dm_camera* camera, float fov, int width, int height, float n, float f);

#endif