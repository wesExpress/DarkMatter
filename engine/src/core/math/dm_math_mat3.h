#ifndef __DM_MATH_MAT3_H__
#define __DM_MATH_MAT3_H__

#include "dm_math_types.h"

dm_mat3 dm_mat3_identity();
dm_mat3 dm_mat3_transpose(dm_mat3 mat);
dm_mat3 dm_mat3_mul_mat3(dm_mat3 left, dm_mat3 right);
dm_vec3 dm_mat3_mul_vec3(dm_mat3 mat, dm_vec3 vec);
dm_mat3 dm_mat3_mul_scalar(dm_mat3 mat, float scalar);
dm_mat3 dm_mat3_add_mat3(dm_mat3 left, dm_mat3 right);
dm_mat3 dm_mat3_sub_mat3(dm_mat3 left, dm_mat3 right);
dm_mat3 dm_mat3_rotation(float radians, dm_vec3 axis);
dm_mat3 dm_mat3_rotation_degrees(float degrees, dm_vec3 axis);

float dm_mat3_det(dm_mat3 mat);

void dm_mat3_identity_inpl(dm_mat3* out);
void dm_mat3_transpose_inpl(dm_mat3 mat, dm_mat3* out);
void dm_mat3_mul_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out);
void dm_mat3_mul_vec3_inpl(dm_mat3 mat, dm_vec3 vec, dm_vec3* out);
void dm_mat3_mul_scalar_inpl(dm_mat3 mat, float scalar, dm_mat3* out);
void dm_mat3_add_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out);
void dm_mat3_sub_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out);
void dm_mat3_rotation_inpl(float radians, dm_vec3 axis, dm_mat3* out);
void dm_mat3_rotation_degrees_inpl(float degrees, dm_vec3 axis, dm_mat3* out);

#endif