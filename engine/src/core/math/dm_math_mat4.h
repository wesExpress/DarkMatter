#ifndef __DM_MATH_MAT4_H__
#define __DM_MATH_MAT4_H__

#include "dm_math_types.h"
#include <stdbool.h>

dm_mat4 dm_mat4_identity();
dm_mat4 dm_mat4_transpose(dm_mat4 mat);
dm_mat4 dm_mat4_mul_mat4(dm_mat4 left, dm_mat4 right);
dm_vec4 dm_mat4_mul_vec4(dm_mat4 mat, dm_vec4 vec);
dm_vec4 dm_mat4_mul_vec3(dm_mat4 mat, dm_vec3 vec);
dm_mat4 dm_mat4_mul_scalar(dm_mat4 mat, float scalar);
dm_mat4 dm_mat4_add_mat4(dm_mat4 left, dm_mat4 right);
dm_mat4 dm_mat4_sub_mat4(dm_mat4 left, dm_mat4 right);
dm_mat4 dm_mat4_from_mat3(dm_mat3 mat);
dm_mat4 dm_mat4_inverse(dm_mat4 mat);

bool dm_mat4_is_equal(dm_mat4 left, dm_mat4 right);

void dm_mat4_identity_inpl(dm_mat4* out);
void dm_mat4_transpose_inpl(dm_mat4 mat, dm_mat4* out);
void dm_mat4_mul_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out);
void dm_mat4_mul_vec4_inpl(dm_mat4 mat, dm_vec4 vec, dm_vec4* out);
void dm_mat4_mul_vec3_inpl(dm_mat4 mat, dm_vec3 vec, dm_vec4* out);
void dm_mat4_mul_scalar_inpl(dm_mat4 mat, float scalar, dm_mat4* out);
void dm_mat4_add_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out);
void dm_mat4_sub_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out);

#endif