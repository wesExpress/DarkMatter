#ifndef __DM_VEC4_H__
#define __DM_VEC4_H__

#include "dm_math_types.h"

dm_vec4 dm_vec4_set(float x, float y, float z, float w);
dm_vec4 dm_vec4_set_from_vec2(dm_vec2 vec);
dm_vec4 dm_vec4_set_from_vec3(dm_vec3 vec);
dm_vec4 dm_vec4_add_scalar(dm_vec4 vec, float scalar);
dm_vec4 dm_vec4_sub_scalar(dm_vec4 vec, float scalar);
dm_vec4 dm_vec4_scale(dm_vec4 vec, float scale);
dm_vec4 dm_vec4_add_vec4(dm_vec4 left, dm_vec4 right);
dm_vec4 dm_vec4_sub_vec4(dm_vec4 left, dm_vec4 right);
dm_vec4 dm_vec4_norm(dm_vec4 vec);

float dm_vec4_dot(dm_vec4 left, dm_vec4 right);
float dm_vec4_len(dm_vec4 vec);
float dm_vec4_angle(dm_vec4 left, dm_vec4 right);
float dm_vec4_angle_degrees(dm_vec4 left, dm_vec4 right);

void dm_vec4_set_inpl(float x, float y, float z, float w, dm_vec4* out);
void dm_vec4_set_from_vec2_inpl(dm_vec2 vec, dm_vec4* out);
void dm_vec4_set_from_vec3_inpl(dm_vec3 vec, dm_vec4* out);
void dm_vec4_add_scalar_inpl(float scalar, dm_vec4* out);
void dm_vec4_sub_scalar_inpl(float scalar, dm_vec4* out);
void dm_vec4_scale_inpl(float scale, dm_vec4* out);
void dm_vec4_add_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out);
void dm_vec4_sub_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out);
void dm_vec4_norm_inpl(dm_vec4* out);

#endif