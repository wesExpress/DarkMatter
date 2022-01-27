#ifndef __DM_MATH_VEC3_H__
#define __DM_MATH_VEC3_H__

#include "dm_math_types.h"

dm_vec3 dm_vec3_set(float x, float y, float z);
dm_vec3 dm_vec3_set_from_vec2(dm_vec2 vec);
dm_vec3 dm_vec3_set_from_vec4(dm_vec4 vec);
dm_vec3 dm_vec3_add_scalar(dm_vec3 vec, float scalar);
dm_vec3 dm_vec3_sub_scalar(dm_vec3 vec, float scalar);
dm_vec3 dm_vec3_scale(dm_vec3 vec, float scale);
dm_vec3 dm_vec3_cross(dm_vec3 left, dm_vec3 right);
dm_vec3 dm_vec3_add_vec3(dm_vec3 left, dm_vec3 right);
dm_vec3 dm_vec3_sub_vec3(dm_vec3 left, dm_vec3 right);
dm_vec3 dm_vec3_norm(dm_vec3 vec);

float dm_vec3_dot(dm_vec3 left, dm_vec3 right);
float dm_vec3_len(dm_vec3 vec);
float dm_vec3_angle(dm_vec3 left, dm_vec3 right);
float dm_vec3_angle_degrees(dm_vec3 left, dm_vec3 right);

void dm_vec3_set_inpl(float x, float y, float z, dm_vec3* out);
void dm_vec3_set_from_vec3_inpl(dm_vec2 vec, dm_vec3* out);
void dm_vec3_set_from_vec4_inpl(dm_vec4 vec, dm_vec3* out);
void dm_vec3_add_scalar_inpl(float scalar, dm_vec3* out);
void dm_vec3_sub_scalar_inpl(float scalar, dm_vec3* out);
void dm_vec3_scale_inpl(float scale, dm_vec3* out);
void dm_vec3_cross_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out);
void dm_vec3_add_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out);
void dm_vec3_sub_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out);
void dm_vec3_norm_inpl(dm_vec3* out);

#endif