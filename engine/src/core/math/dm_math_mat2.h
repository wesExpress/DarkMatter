#ifndef __DM_MATH_MAT2_H__
#define __DM_MATH_MAT2_H__

#include "dm_math_types.h"

dm_mat2 dm_mat2_identity();
dm_mat2 dm_mat2_transpose(dm_mat2 mat);
dm_mat2 dm_mat2_mul_mat2(dm_mat2 left, dm_mat2 right);
dm_vec2 dm_mat2_mul_vec2(dm_mat2 mat, dm_vec2 vec);
dm_mat2 dm_mat2_mul_scalar(dm_mat2 mat, float scalar);
dm_mat2 dm_mat2_add_mat2(dm_mat2 left, dm_mat2 right);
dm_mat2 dm_mat2_sub_mat2(dm_mat2 left, dm_mat2 right);
dm_mat2 dm_mat2_inverse(dm_mat2 mat);

float dm_mat2_det(dm_mat2 mat);

void dm_mat2_identity_inpl(dm_mat2* out);
void dm_mat2_inpl(dm_mat2 mat, dm_mat2* out);
void dm_mat2_mul_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out);
void dm_mat2_mul_vec2_inpl(dm_mat2 mat, dm_vec2 vec, dm_vec2* out);
void dm_mat2_mul_scalar_inpl(dm_mat2 mat, float scalar, dm_mat2* out);
void dm_mat2_add_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out);
void dm_mat2_sub_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out);
void dm_mat2_inverse_inpl(dm_mat2 mat, dm_mat2* out);

#endif