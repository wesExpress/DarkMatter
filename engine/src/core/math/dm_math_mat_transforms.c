#include "dm_math_mat_transforms.h"
#include "dm_math_vec3.h"
#include "dm_math_vec4.h"
#include "dm_math_mat3.h"
#include "dm_math_mat4.h"

dm_mat4 dm_mat_scale_make(dm_vec3 scale)
{
	dm_mat4 result = dm_mat4_identity();

	result.m[0] = scale.x;
	result.m[5] = scale.y;
	result.m[10] = scale.z;

	return result;
}

dm_mat4 dm_mat_rotation_make(float radians, dm_vec3 axis)
{
	if (dm_vec3_len(axis) != 1)
	{
		axis = dm_vec3_norm(axis);
	}

	dm_mat4 result = { 0 };
	dm_mat3 rotation = dm_mat3_rotation(radians, axis);

	result = dm_mat4_from_mat3(rotation);
	result.m[15] = 1;

	return result;
}

dm_mat4 dm_mat_rotation_degrees_make(float degrees, dm_vec3 axis)
{
	return dm_mat_rotation_make(dm_deg_to_rad(degrees), axis);
}

dm_mat4 dm_mat_translate_make(dm_vec3 translation)
{
	dm_mat4 result = dm_mat4_identity();

	result.rows[3] = dm_vec4_set(translation.x, translation.y, translation.z, 1.0f);

	return result;
}


dm_mat4 dm_mat_translate(dm_mat4 mat, dm_vec3 translation)
{
	dm_mat4 result = mat;

	result.rows[3].x = translation.x;
	result.rows[3].y = translation.y;
	result.rows[3].z = translation.z;

	return result;
}

void dm_mat_translate_inpl(dm_mat4 mat, dm_vec3 translation, dm_mat4* out)
{
	*out = dm_mat_translate(mat, translation);
}

dm_mat4 dm_mat_scale(dm_mat4 mat, dm_vec3 scale)
{
	dm_mat4 result = mat;

	result.rows[0] = dm_vec4_scale(result.rows[0], scale.x);
	result.rows[1] = dm_vec4_scale(result.rows[1], scale.y);
	result.rows[2] = dm_vec4_scale(result.rows[2], scale.z);

	return result;
}

void dm_mat_scale_inpl(dm_mat4 mat, dm_vec3 scale, dm_mat4* out)
{
	*out = dm_mat_scale(mat, scale);
}

dm_mat4 dm_mat_rotate(dm_mat4 mat, float radians, dm_vec3 axis)
{
	dm_mat4 rotation = dm_mat_rotation_make(radians, axis);

	return dm_mat4_mul_mat4(mat, rotation);
}

void dm_mat_rotate_inpl(dm_mat4 mat, float radians, dm_vec3 axis, dm_mat4* out)
{
	*out = dm_mat_rotate(mat, radians, axis);
}

dm_mat4 dm_mat_rotate_degrees(dm_mat4 mat, float degrees, dm_vec3 axis)
{
	return dm_mat_rotate(mat, dm_deg_to_rad(degrees), axis);
}

void dm_mat_rotate_degrees_inpl(dm_mat4 mat, float degrees, dm_vec3 axis, dm_mat4* out)
{
	*out = dm_mat_rotate_degrees(mat, degrees, axis);
}

dm_mat4 dm_mat_view(dm_vec3 view_origin, dm_vec3 target, dm_vec3 up)
{
	//if (dm_vec3_len(up) != 1) up = dm_vec3_norm(up);

	dm_vec3 w = dm_vec3_sub_vec3(target, view_origin);
	w = dm_vec3_norm(w);

	dm_vec3 u = dm_vec3_cross(w, up);
	u = dm_vec3_norm(u);

	dm_vec3 v = dm_vec3_cross(u, w);

	dm_mat4 view = { 0 };

	view.rows[0] = (dm_vec4){ u.x, v.x, -w.x, 0 };
	view.rows[1] = (dm_vec4){ u.y, v.y, -w.y, 0 };
	view.rows[2] = (dm_vec4){ u.z, v.z, -w.z, 0 };
	view.rows[3] = (dm_vec4){
		-dm_vec3_dot(view_origin, u),
		-dm_vec3_dot(view_origin, v),
		dm_vec3_dot(view_origin, w),
		1
	};

	return view;
}

dm_mat4 dm_mat_perspective(float fov, float aspect_ratio, float n, float f)
{
	dm_mat4 projection = { 0 };

	float t = 1.0f / tanf(fov * 0.5f);
	float fn = 1.0f / (n - f);
	projection.m[0] = t / aspect_ratio;
	projection.m[5] = t;
	projection.m[10] = (f + n) * fn;
	projection.m[11] = -1.0f;
	projection.m[14] = 2.0f * n * f * fn;

	return projection;
}

dm_mat4 dm_mat_ortho(float left, float right, float bottom, float top, float n, float f)
{
	dm_mat4 ortho = { 0 };

	ortho.m[0] = 2.0f / (right - left);
	ortho.m[5] = 2.0f / (top - bottom);
	ortho.m[10] = -2.0f / (f - n);
	ortho.m[12] = -(right + left) / (right - left);
	ortho.m[13] = -(top + bottom) / (top - bottom);
	ortho.m[14] = -(f + n) / (f - n);
	ortho.m[15] = 1.0f;

	return ortho;
}