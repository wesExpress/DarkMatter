#include "dm_math_vec3.h"
#include "dm_math_misc.h"
#include "core/dm_assert.h"
#include <math.h>

dm_vec3 dm_vec3_set(float x, float y, float z)
{
	return (dm_vec3) { x, y, z };
}

void dm_vec3_set_inpl(float x, float y, float z, dm_vec3* out)
{
	*out = dm_vec3_set(x, y, z);
}

dm_vec3 dm_vec3_set_from_vec2(dm_vec2 vec)
{
	return (dm_vec3) { vec.x, vec.y, 0.0f };
}

void dm_vec3_set_from_vec3_inpl(dm_vec2 vec, dm_vec3* out)
{
	*out = dm_vec3_set_from_vec2(vec);
}

dm_vec3 dm_vec3_set_from_vec4(dm_vec4 vec)
{
	return (dm_vec3) { vec.x, vec.y, vec.z };
}

void dm_vec3_set_from_vec4_inpl(dm_vec4 vec, dm_vec3* out)
{
	*out = dm_vec3_set_from_vec4(vec);
}

dm_vec3 dm_vec3_add_scalar(dm_vec3 vec, float scalar)
{
	return (dm_vec3) { vec.x + scalar, vec.y + scalar, vec.z + scalar };
}

void dm_vec3_add_scalar_inpl(float scalar, dm_vec3* out)
{
	*out = dm_vec3_add_scalar(*out, scalar);
}

dm_vec3 dm_vec3_sub_scalar(dm_vec3 vec, float scalar)
{
	return dm_vec3_add_scalar(vec, -scalar);
}

void dm_vec3_sub_scalar_inpl(float scalar, dm_vec3* out)
{
	*out = dm_vec3_sub_scalar(*out, scalar);
}

dm_vec3 dm_vec3_scale(dm_vec3 vec, float scale)
{
	return (dm_vec3) { vec.x* scale, vec.y* scale, vec.z* scale };
}

void dm_vec3_scale_inpl(float scale, dm_vec3* out)
{
	*out = dm_vec3_scale(*out, scale);
}

float dm_vec3_dot(dm_vec3 left, dm_vec3 right)
{
	return (float)(left.x * right.x + left.y * right.y + left.z * right.z);
}

dm_vec3 dm_vec3_cross(dm_vec3 left, dm_vec3 right)
{
	return (dm_vec3) {
		left.y* right.z - right.y * left.z,
			-(left.x * right.z - right.x * left.z),
			left.x* right.y - right.x * left.y
	};
}

void dm_vec3_cross_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_cross(left, right);
}

float dm_vec3_len(dm_vec3 vec)
{
	return (float)sqrt(dm_vec3_dot(vec, vec));
}

dm_vec3 dm_vec3_add_vec3(dm_vec3 left, dm_vec3 right)
{
	return (dm_vec3) { left.x + right.x, left.y + right.y, left.z + right.z };
}

void dm_vec3_add_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_add_vec3(left, right);
}

dm_vec3 dm_vec3_sub_vec3(dm_vec3 left, dm_vec3 right)
{
	return (dm_vec3) { left.x - right.x, left.y - right.y, left.z - right.z };
}

void dm_vec3_sub_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_sub_vec3(left, right);
}

dm_vec3 dm_vec3_norm(dm_vec3 vec)
{
	float len = dm_vec3_len(vec);
	if (len == 0)
	{
		return (dm_vec3) { 0, 0, 0 };
	}
	return dm_vec3_scale(vec, 1.0f / len);
}

void dm_vec3_norm_inpl(dm_vec3* out)
{
	float len = dm_vec3_len(*out);
	if (len != 0) dm_vec3_scale_inpl(1.0f / len, out);
}

float dm_vec3_angle(dm_vec3 left, dm_vec3 right)
{
	float llen = dm_vec3_len(left);
	float rlen = dm_vec3_len(right);
	DM_ASSERT_MSG((llen > 0 && rlen > 0), "Trying to get an angle between vec3s and at least one has length 0!");
	return (float)acosf(DM_CLAMP(dm_vec3_dot(left, right) / (llen * rlen),-1.0f, 1.0f));
}

float dm_vec3_angle_degrees(dm_vec3 left, dm_vec3 right)
{
	return dm_deg_to_rad(dm_vec3_angle(left, right));
}