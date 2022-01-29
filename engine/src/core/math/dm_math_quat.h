#ifndef __DM_MATH_QUAT_H__
#define __DM_MATH_QUAT_H__

#include "dm_math_types.h"

dm_quat dm_quat_real(float r);
dm_quat dm_quat_pure(dm_vec3 pure);
dm_quat dm_quat_set(float i, float j, float k, float r);
dm_quat dm_quat_set_imaj(float i, float j, float k);
dm_quat dm_quat_set_from_vec3(dm_vec3 vec);
dm_quat dm_quat_set_from_vec4(dm_vec4 vec);
dm_quat dm_quat_add_quat(dm_quat left, dm_quat right);
dm_quat dm_quat_sub_quat(dm_quat left, dm_quat right);
dm_quat dm_quat_mul_quat(dm_quat left, dm_quat right);
dm_quat dm_quat_scale(dm_quat quat, float scalar);
dm_quat dm_quat_conjugate(dm_quat quat);
dm_quat dm_quat_norm(dm_quat quat);
dm_quat dm_quat_inverse(dm_quat quat);

float dm_quat_mag(dm_quat quat);
float dm_quat_dot(dm_quat left, dm_quat right);
float dm_quat_angle(dm_quat left, dm_quat right);

void dm_quat_real_inpl(float r, dm_quat* out);
void dm_quat_pure_inpl(dm_vec3 pure, dm_quat* out);
void dm_quat_set_inpl(float r, float i, float j, float k, dm_quat* out);
void dm_quat_set_imaj_inpl(float i, float j, float k, dm_quat* out);
void dm_quat_set_from_vec3_inpl(dm_vec3 vec, dm_quat* out);
void dm_quat_set_from_vec4_inpl(dm_vec4 vec, dm_quat* out);
void dm_quat_add_quat_inpl(dm_quat left, dm_quat right, dm_quat* out);
void dm_quat_sub_quat_inpl(dm_quat left, dm_quat right, dm_quat* out);
void dm_quat_mul_quat_inpl(dm_quat left, dm_quat right, dm_quat* out);
void dm_quat_scale_inpl(dm_quat quat, float scalar, dm_quat* out);
void dm_quat_norm_inpl(dm_quat quat, dm_quat* out);
void dm_quat_inverse_inpl(dm_quat quat, dm_quat* out);

#endif