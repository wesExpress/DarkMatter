#include "dm_math_mat2.h"
#include "dm_math_vec2.h"
#include "core/dm_logger.h"

dm_mat2 dm_mat2_identity()
{
	dm_mat2 result = { 0 };

	int N = 2;

	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}

	return result;
}

void dm_mat2_identity_inpl(dm_mat2* out)
{
	*out = dm_mat2_identity();
}

dm_mat2 dm_mat2_transpose(dm_mat2 mat)
{
	dm_mat2 result = { 0 };

	int N = 2;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}
	return result;
}

void dm_mat2_inpl(dm_mat2 mat, dm_mat2* out)
{
	*out = dm_mat2_transpose(mat);
}

dm_mat2 dm_mat2_mul_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };

#if DM_MATH_COL_MAJ
	left = dm_mat2_transpose(left);
#else
	right = dm_mat2_transpose(right);
#endif

	int N = 2;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec2_dot(left.rows[i], right.rows[j]);
		}
	}

#if DM_MATH_COL_MAJ
	result = dm_mat2_transpose(result);
#endif

	return result;
}

void dm_mat2_mul_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_mul_mat2(left, right);
}

dm_vec2 dm_mat2_mul_vec2(dm_mat2 mat, dm_vec2 vec)
{
	dm_vec2 result = { 0 };
#if DM_MATH_COL_MAJ
	mat = dm_mat2_transpose(mat);
#endif

	int N = 2;
	for (int i = 0; i < N; i++)
	{
		result.v[i] = dm_vec2_dot(mat.rows[i], vec);
	}

	return result;
}

void dm_mat2_mul_vec2_inpl(dm_mat2 mat, dm_vec2 vec, dm_vec2* out)
{
	*out = dm_mat2_mul_vec2(mat, vec);
}

dm_mat2 dm_mat2_mul_scalar(dm_mat2 mat, float scalar)
{
	dm_mat2 result = { 0 };

	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}
	return result;
}

void dm_mat2_mul_scalar_inpl(dm_mat2 mat, float scalar, dm_mat2* out)
{
	*out = dm_mat2_mul_scalar(mat, scalar);
}

dm_mat2 dm_mat2_add_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };

	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}

	return result;
}

void dm_mat2_add_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_add_mat2(left, right);
}

dm_mat2 dm_mat2_sub_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };

	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] - right.m[i];
	}

	return result;
}

void dm_mat2_sub_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_sub_mat2(left, right);
}

dm_mat2 dm_mat2_inverse(dm_mat2 mat)
{
	dm_mat2 result = { 0 };

	float det = mat.m[0] * mat.m[3] - mat.m[1] * mat.m[2];
	if (det == 0)
	{
		DM_LOG_WARN("Trying to invert non-invertible 2x2 matrix (determinant is zero)! Returning input matrix...");
		return mat;
	}

	det = 1.0f / det;
	int N = 2;
	for (int i = 0; i < N; i++)
	{
		result.m[i] = mat.m[N - i];
	}

	return dm_mat2_mul_scalar(mat, det);
}

void dm_mat2_inverse_inpl(dm_mat2 mat, dm_mat2* out)
{
	*out = dm_mat2_inverse(mat);
}

float dm_mat2_det(dm_mat2 mat)
{
	return (float)(mat.m[0] * mat.m[3] - mat.m[1] * mat.m[2]);
}