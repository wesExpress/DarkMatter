#include "dm_math_mat3.h"
#include "dm_math_vec3.h"
#include "dm_math_mat2.h"
#include "dm_math_misc.h"
#include "core/dm_logger.h"
#include <math.h>

dm_mat3 dm_mat3_cofactors(dm_mat3 mat);
dm_mat3 dm_mat3_inverse(dm_mat3 mat);
dm_mat2 dm_mat3_minor(dm_mat3 mat, dm_vec2 cols, dm_vec2 rows);

dm_mat3 dm_mat3_identity()
{
	dm_mat3 result = { 0 };

	int N = 3;

	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}
	return result;
}

void dm_mat3_identity_inpl(dm_mat3* out)
{
	*out = dm_mat3_identity();
}

dm_mat3 dm_mat3_transpose(dm_mat3 mat)
{
	dm_mat3 result = { 0 };

	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}

	return result;
}

void dm_mat3_transpose_inpl(dm_mat3 mat, dm_mat3* out)
{
	*out = dm_mat3_transpose(mat);
}

dm_mat3 dm_mat3_mul_mat3(dm_mat3 left, dm_mat3 right)
{
	dm_mat3 result = { 0 };
#if DM_MATH_COL_MAJ
	left = dm_mat3_transpose(left);
#else
	right = dm_mat3_transpose(right);
#endif

	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec3_dot(left.rows[i], right.rows[j]);
		}
	}

#if DM_MATH_COL_MAJ
	result = dm_mat3_transpose(result);
#endif

	return result;
}

void dm_mat3_mul_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_mul_mat3(left, right);
}

dm_vec3 dm_mat3_mul_vec3(dm_mat3 mat, dm_vec3 vec)
{
	dm_vec3 result = { 0 };
#if DM_MATH_COL_MAJ
	mat = dm_mat3_transpose(mat);
#endif

	int N = 3;
	for (int i = 0; i < N; i++)
	{
		result.v[i] = dm_vec3_dot(mat.rows[i], vec);
	}

	return result;
}

void dm_mat3_mul_vec3_inpl(dm_mat3 mat, dm_vec3 vec, dm_vec3* out)
{
	*out = dm_mat3_mul_vec3(mat, vec);
}

dm_mat3 dm_mat3_mul_scalar(dm_mat3 mat, float scalar)
{
	dm_mat3 result = { 0 };

	int N = 3;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}

	return result;
}

void dm_mat3_mul_scalar_inpl(dm_mat3 mat, float scalar, dm_mat3* out)
{
	*out = dm_mat3_mul_scalar(mat, scalar);
}

dm_mat3 dm_mat3_add_mat3(dm_mat3 left, dm_mat3 right)
{
	dm_mat3 result = { 0 };

	int N = 3;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}

	return result;
}

void dm_mat3_add_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_add_mat3(left, right);
}

dm_mat3 dm_mat3_sub_mat3(dm_mat3 left, dm_mat3 right)
{
	return dm_mat3_add_mat3(left, dm_mat3_mul_scalar(right, -1.0f));
}

void dm_mat3_sub_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_sub_mat3(left, right);
}

dm_mat2 dm_mat3_minor(dm_mat3 mat, dm_vec2 cols, dm_vec2 rows)
{
	int N = 3;
	dm_mat2 minor = { 0 };

	for (int i = 0; i < 2; i++)
	{
		int col = (int)cols.v[i];
		for (int j = 0; j < 2; j++)
		{
			int row = (int)rows.v[j];
			minor.m[j * 2 + i] = mat.m[row * N + col];
		}
	}

	return minor;
}

float dm_mat3_det(dm_mat3 mat)
{
#if DM_MATH_COL_MAJ
	mat = dm_mat3_transpose(mat);
#endif

	float det1 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 1, 2 }));
	float det2 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 1, 2 }));
	float det3 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 1, 2 }));

	return mat.m[0] * det1 - mat.m[1] * det2 + mat.m[2] * det3;
}

dm_mat3 dm_mat3_cofactors(dm_mat3 mat)
{
	dm_mat3 result = { 0 };

#if DM_MATH_COL_MAJ
	mat = dm_mat3_transpose(mat);
#endif

	result.m[0] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 1, 2 }));
	result.m[1] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 1, 2 }));
	result.m[2] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 1, 2 }));

	result.m[3] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 0, 2 }));
	result.m[4] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 0, 2 }));
	result.m[5] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 0, 2 }));

	result.m[6] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 0, 1 }));
	result.m[7] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 0, 1 }));
	result.m[8] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 0, 1 }));

	return result;
}

dm_mat3 dm_mat3_inverse(dm_mat3 mat)
{
	dm_mat3 result = { 0 };

	float det = dm_mat3_det(mat);
	if (det == 0)
	{
		DM_LOG_WARN("Trying to invert non-invertible 3x3 matrix! Returning input...");
		return mat;
	}
	det = 1.0f / det;

	// cofactors
	result = dm_mat3_cofactors(mat);
	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = powf(-1.0f, i + j) * result.m[i * N + j];
		}
	}

	// adjugate
	result = dm_mat3_transpose(result);

	return dm_mat3_mul_scalar(result, det);
}

dm_mat3 dm_mat3_rotation(float radians, dm_vec3 axis)
{
	dm_mat3 result = { 0 };

	float C = cosf(radians);
	float S = sinf(radians);
	float t = 1 - C;

	result.m[0] = t * axis.x * axis.x + C;
	result.m[1] = t * axis.x * axis.y - S * axis.z;
	result.m[2] = t * axis.x * axis.z + S * axis.y;

	result.m[3] = t * axis.x * axis.y + S * axis.z;
	result.m[4] = t * axis.y * axis.y + C;
	result.m[5] = t * axis.y * axis.z - S * axis.x;

	result.m[6] = t * axis.x * axis.z - S * axis.y;
	result.m[7] = t * axis.y * axis.z + S * axis.x;
	result.m[8] = t * axis.z * axis.z + C;

#if DM_MATH_COL_MAJ
	result = dm_mat3_transpose(result);
#endif

	return result;
}

void dm_mat3_rotation_inpl(float radians, dm_vec3 axis, dm_mat3* out)
{
	*out = dm_mat3_rotation(radians, axis);
}

dm_mat3 dm_mat3_rotation_degrees(float degrees, dm_vec3 axis)
{
	return dm_mat3_rotation(dm_deg_to_rad(degrees), axis);
}

void dm_mat3_rotation_degrees_inpl(float degrees, dm_vec3 axis, dm_mat3* out)
{
	*out = dm_mat3_rotation_degrees(degrees, axis);
}