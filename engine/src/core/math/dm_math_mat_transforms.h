#ifndef __DM_MATH_MAT_TRANSFORMS_H__
#define __DM_MATH_MAT_TRANSFORMS_H__

#include "dm_math_types.h"

dm_mat4 dm_mat_scale_make(dm_vec3 scale);
dm_mat4 dm_mat_rotation_make(float radians, dm_vec3 axis);
dm_mat4 dm_mat_rotation_degrees_make(float degrees, dm_vec3 axis);
dm_mat4 dm_mat_translate_make(dm_vec3 translation);


dm_mat4 dm_mat_translate(dm_mat4 mat, dm_vec3 translation);
dm_mat4 dm_mat_scale(dm_mat4 mat, dm_vec3 scale);
dm_mat4 dm_mat_rotate(dm_mat4 mat, float radians, dm_vec3 axis);
dm_mat4 dm_mat_rotate_degrees(dm_mat4 mat, float degrees, dm_vec3 axis);

void dm_mat_translate_inpl(dm_mat4 mat, dm_vec3 translation, dm_mat4* out);
void dm_mat_scale_inpl(dm_mat4 mat, dm_vec3 scale, dm_mat4* out);
void dm_mat_rotate_inpl(dm_mat4 mat, float radians, dm_vec3 axis, dm_mat4* out);
void dm_mat_rotate_degrees_inpl(dm_mat4 mat, float degrees, dm_vec3 axis, dm_mat4* out);

// camera

/*
* @param view_origin coordinate of the camera
* @param target coordinate camera is looking at
* @param up up vector for the camera
*/
dm_mat4 dm_mat_view(dm_vec3 view_origin, dm_vec3 target, dm_vec3 up);

/*
* @param fov field of view (radians)
* @param aspect_ratio
* @param n near clipping plane (>0 or else is set to 0.1)
* @param f far clippin plane
*/
dm_mat4 dm_mat_perspective(float fov, float aspect_ratio, float n, float f);

/*
* @param left
* @param right
* @param bottom
* @param top
* @param n near clipping plane (>0 or else is set to 0.1)
* @param f far clippin plane
*/
dm_mat4 dm_mat_ortho(float left, float right, float bottom, float top, float n, float f);

#endif