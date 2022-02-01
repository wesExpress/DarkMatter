#include "dm_math_quat.h"
#include "dm_math_vec3.h"
#include "dm_math_vec4.h"
#include <math.h>

dm_vec3 dm_vec3_from_quat(dm_quat quat);
dm_vec4 dm_vec4_from_quat(dm_quat quat);

dm_quat dm_quat_real(float r)
{
	return (dm_quat) { 0, 0, 0, r };
}

void dm_quat_real_inpl(float r, dm_quat* out)
{
	*out = dm_quat_real(r);
}

dm_quat dm_quat_pure(dm_vec3 pure)
{
	return (dm_quat) { pure.x, pure.y, pure.z, 0 };
}

void dm_quat_pure_inpl(dm_vec3 pure, dm_quat* out)
{
	*out = dm_quat_pure(pure);
}

dm_quat dm_quat_set(float i, float j, float k, float r)
{
	return (dm_quat) { i, j, k, r };
}

void dm_quat_set_inpl(float r, float i, float j, float k, dm_quat* out)
{
	*out = dm_quat_set(i, j, k, r);
}

dm_quat dm_quat_set_imaj(float i, float j, float k)
{
	return (dm_quat) { i, j, k, 1.0 };
}

void dm_quat_set_imaj_inpl(float i, float j, float k, dm_quat* out)
{
	*out = dm_quat_set_imaj(i, j, k);
}

dm_quat dm_quat_set_from_vec3(dm_vec3 vec)
{
	return (dm_quat) { vec.x, vec.y, vec.z };
}

void dm_quat_set_from_vec3_inpl(dm_vec3 vec, dm_quat* out)
{
	*out = dm_quat_set_from_vec3(vec);
}

dm_quat dm_quat_set_from_vec4(dm_vec4 vec)
{
	return (dm_quat) { vec.x, vec.y, vec.z, vec.w };
}

void dm_quat_set_from_vec4_inpl(dm_vec4 vec, dm_quat* out)
{
	*out = dm_quat_set_from_vec4(vec);
}

dm_quat dm_quat_add_quat(dm_quat left, dm_quat right)
{
	return (dm_quat) { (left.i + right.i), (left.j + right.j), (left.k + right.k), (left.r + right.r) };
}

void dm_quat_add_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_add_quat(left, right);
}

dm_quat dm_quat_sub_quat(dm_quat left, dm_quat right)
{
	return (dm_quat) { (left.i - right.i), (left.j - right.j), (left.k - right.k), (left.r - right.r) };
}

void dm_quat_sub_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_sub_quat(left, right);
}

dm_quat dm_quat_mul_quat(dm_quat left, dm_quat right)
{
	dm_vec3 a = dm_vec3_from_quat(left);
	dm_vec3 b = dm_vec3_from_quat(right);
	float ab_dot = dm_vec3_dot(a, b);
	float real = left.r * right.r - ab_dot;

	if (ab_dot) return (dm_quat) { 0, 0, 0, real };

	dm_vec3 imaj = dm_vec3_scale(b, left.r);
	imaj = dm_vec3_add_vec3(imaj, dm_vec3_scale(a, right.r));
	imaj = dm_vec3_add_vec3(imaj, dm_vec3_cross(a, b));

	return (dm_quat) { imaj.x, imaj.y, imaj.z, real };
}

void dm_quat_mul_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_mul_quat(left, right);
}

dm_vec3 dm_vec3_from_quat(dm_quat quat)
{
	return (dm_vec3){ quat.i, quat.j, quat.k };
}

dm_vec4 dm_vec4_from_quat(dm_quat quat)
{
	return (dm_vec4) { quat.i, quat.j, quat.k, quat.r };
}

dm_quat dm_quat_scale(dm_quat quat, float scalar)
{
	return (dm_quat) { quat.i* scalar, quat.j* scalar, quat.k* scalar, quat.r* scalar };
}

void dm_quat_scale_inpl(dm_quat quat, float scalar, dm_quat* out)
{
	*out = dm_quat_scale(quat, scalar);
}

dm_quat dm_quat_conjugate(dm_quat quat)
{
	return (dm_quat) { -quat.i, -quat.j, -quat.k, quat.r };
}

dm_quat dm_quat_norm(dm_quat quat)
{
	float mag = dm_quat_mag(quat);

	if (mag > 0) return dm_quat_scale(quat, 1.0f / mag);
	return quat;
}

void dm_quat_norm_inpl(dm_quat quat, dm_quat* out)
{
	*out = dm_quat_norm(quat);
}

float dm_quat_mag(dm_quat quat)
{
	return sqrt((quat.i * quat.i) + (quat.j * quat.j) + (quat.k * quat.k) + (quat.r * quat.r));
}

dm_quat dm_quat_inverse(dm_quat quat)
{
	dm_quat conj = dm_quat_conjugate(quat);

	if (dm_quat_mag(conj) == 1) return conj;

	float mag = dm_quat_mag(quat);

	return dm_quat_scale(conj, mag * mag);
}

void dm_quat_inverse_inpl(dm_quat quat, dm_quat* out)
{
	*out = dm_quat_inverse(quat);
}

float dm_quat_dot(dm_quat left, dm_quat right)
{
	return (left.i * right.i) + (left.j * right.j) + (left.k * right.k) + (left.r + right.r);
}

float dm_quat_angle(dm_quat left, dm_quat right)
{
	float mag = dm_quat_mag(left) * dm_quat_mag(right);

	if (mag == 0) return 0;

	float angle = dm_quat_dot(left, right) / mag;
	return acos(angle);
}