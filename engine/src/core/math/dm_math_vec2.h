#ifndef __DM_MATH_VEC2_H__
#define __DM_MATH_VEC2_H__

#include "dm_math_types.h"

dm_vec2 dm_vec2_set(float x, float y);
dm_vec2 dm_vec2_set_from_vec3(dm_vec3 vec);
dm_vec2 dm_vec2_set_from_vec4(dm_vec4 vec);
dm_vec2 dm_vec2_add_scalar(dm_vec2 vec, float scalar);
dm_vec2 dm_vec2_sub_scalar(dm_vec2 vec, float scalar);
dm_vec2 dm_vec2_scale(dm_vec2 vec, float scale);
dm_vec2 dm_vec2_add_vec2(dm_vec2 left, dm_vec2 right);
dm_vec2 dm_vec2_sub_vec2(dm_vec2 left, dm_vec2 right);
dm_vec2 dm_vec2_norm(dm_vec2 vec);

float dm_vec2_dot(dm_vec2 left, dm_vec2 right);
float dm_vec2_len(dm_vec2 vec);
float dm_vec2_angle(dm_vec2 left, dm_vec2 right);
float dm_vec2_angle_degrees(dm_vec2 left, dm_vec2 right);

void dm_vec2_set_inpl(float x, float y, dm_vec2* out);
void dm_vec2_set_from_vec3_inpl(dm_vec3 vec, dm_vec2* out);
void dm_vec2_set_from_vec4_inpl(dm_vec4 vec, dm_vec2* out);
void dm_vec2_add_scalar_inpl(dm_vec2 vec, float scalar, dm_vec2* out);
void dm_vec2_sub_scalar_inpl(dm_vec2 vec, float scalar, dm_vec2* out);
void dm_vec2_scale_inpl(float scale, dm_vec2* out);
void dm_vec2_add_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out);
void dm_vec2_sub_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out);
void dm_vec2_norm_inpl(dm_vec2* out);

#endif