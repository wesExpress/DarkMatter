#include "dm_math_mat3.h"
#include "dm_math_mat4.h"
#include "dm_math_vec2.h"
#include "dm_math_vec3.h"
#include "dm_math_vec4.h"
#include "core/dm_logger.h"
#include <math.h>

dm_mat3 dm_mat4_minor(dm_mat4 mat, dm_vec3 cols, dm_vec3 rows);
float dm_mat4_det(dm_mat4 mat);
dm_mat4 dm_mat4_cofactors(dm_mat4 mat);

dm_mat4 dm_mat4_identity()
{
	dm_mat4 result = { 0 };

	int N = 4;

	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}
	return result;
}

void dm_mat4_identity_inpl(dm_mat4* out)
{
	*out = dm_mat4_identity();
}

dm_mat4 dm_mat4_transpose(dm_mat4 mat)
{
	dm_mat4 result = { 0 };

	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}
	return result;
}

void dm_mat4_transpose_inpl(dm_mat4 mat, dm_mat4* out)
{
	*out = dm_mat4_transpose(mat);
}

dm_mat4 dm_mat4_mul_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };

#if DM_MATH_COL_MAJ
	left = dm_mat4_transpose(left);
#else
	right = dm_mat4_transpose(right);
#endif

	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec4_dot(left.rows[i], right.rows[j]);
		}
	}

#if DM_MATH_COL_MAJ
	result = dm_mat4_transpose(result);
#endif

	return result;
}

void dm_mat4_mul_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_mul_mat4(left, right);
}

dm_vec4 dm_mat4_mul_vec4(dm_mat4 mat, dm_vec4 vec)
{
	dm_vec4 result = { 0 };
#if DM_MATH_COL_MAJ
	mat = dm_mat4_transpose(mat);
#endif

	int N = 4;
	for (int i = 0; i < N; i++)
	{
		result.v[i] = dm_vec4_dot(mat.rows[i], vec);
	}

	return result;
}

void dm_mat4_mul_vec4_inpl(dm_mat4 mat, dm_vec4 vec, dm_vec4* out)
{
	*out = dm_mat4_mul_vec4(mat, vec);
}

dm_vec4 dm_mat4_mul_vec3(dm_mat4 mat, dm_vec3 vec)
{
	dm_vec4 new_vec = dm_vec4_set(vec.x, vec.y, vec.z, 1.0f);
	return dm_mat4_mul_vec4(mat, new_vec);
}

void dm_mat4_mul_vec3_inpl(dm_mat4 mat, dm_vec3 vec, dm_vec4* out)
{
	*out = dm_mat4_mul_vec3(mat, vec);
}

dm_mat4 dm_mat4_mul_scalar(dm_mat4 mat, float scalar)
{
	dm_mat4 result = { 0 };

	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}

	return result;
}

void dm_mat4_mul_scalar_inpl(dm_mat4 mat, float scalar, dm_mat4* out)
{
	*out = dm_mat4_mul_scalar(mat, scalar);
}

dm_mat4 dm_mat4_add_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };

	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}

	return result;
}

void dm_mat4_add_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_add_mat4(left, right);
}

dm_mat4 dm_mat4_sub_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };

	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] - right.m[i];
	}

	return result;
}

void dm_mat4_sub_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_sub_mat4(left, right);
}

dm_mat3 dm_mat4_minor(dm_mat4 mat, dm_vec3 cols, dm_vec3 rows)
{
	int N = 4;
	dm_mat3 minor = { 0 };

	for (int i = 0; i < 3; i++)
	{
		int col = (int)cols.v[i];
		for (int j = 0; j < 3; j++)
		{
			int row = (int)rows.v[j];
			minor.m[j * 3 + i] = mat.m[row * N + col];
		}
	}

	return minor;
}

float dm_mat4_det(dm_mat4 mat)
{
#if DM_MATH_COL_MAJ
	mat = dm_mat4_transpose(mat);
#endif

	float det1 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det2 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det3 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det4 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 1, 2, 3 }));

	return mat.m[0] * det1 - mat.m[1] * det2 + mat.m[2] * det3 - mat.m[3] * det4;
}

dm_mat4 dm_mat4_cofactors(dm_mat4 mat)
{
	dm_mat4 result = { 0 };

#if DM_MATH_COL_MAJ
	mat = dm_mat4_transpose(mat);
#endif

	result.m[0] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[1] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[2] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[3] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 1, 2, 3 }));

	result.m[4] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[5] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[6] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[7] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 2, 3 }));

	result.m[8] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[9] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[10] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[11] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 1, 3 }));

	result.m[12] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[13] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[14] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[15] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 1, 2 }));

	return result;
}

dm_mat4 dm_mat4_inverse(dm_mat4 mat)
{
	dm_mat4 result = { 0 };

	float det = dm_mat4_det(mat);
	if (det == 0)
	{
		DM_LOG_WARN("Trying to invert non-invertible 4x4 matrix! Returning input...");
		return mat;
	}
	det = 1.0f / det;

	// cofactors
	result = dm_mat4_cofactors(mat);
	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = powf(-1.0f, i + j) * result.m[i * N + j];
		}
	}

	// adjugate
	result = dm_mat4_transpose(result);

	return dm_mat4_mul_scalar(result, det);
}

bool dm_mat4_is_equal(dm_mat4 left, dm_mat4 right)
{
	int N = 4;

	for (int i = 0; i < N * N; i++)
	{
		if (left.m[i] != right.m[i]) return false;
	}

	return true;
}

dm_mat4 dm_mat4_from_mat3(dm_mat3 mat)
{
	dm_mat4 result = { 0 };

	for (int i = 0; i < 3; i++)
	{
		result.rows[i] = dm_vec4_set_from_vec3(mat.rows[i]);
	}

	return result;
}