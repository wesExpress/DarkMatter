#include "dm_math_quat.h"

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
	return (dm_quat) { (left.r - right.r), (left.i - right.r), (left.j - right.j), (left.k - right.k) };
}

void dm_quat_sub_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_sub_quat(left, right);
}

dm_quat dm_quat_mul_quat(dm_quat left, dm_quat right)
{

}

void dm_quat_mul_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{

}