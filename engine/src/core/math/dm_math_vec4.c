#include "dm_math_vec4.h"
#include "dm_math_misc.h"
#include "core/dm_assert.h"
#include <math.h>

dm_vec4 dm_vec4_set(float x, float y, float z, float w)
{
	return (dm_vec4) { x, y, z, w };
}

void dm_vec4_set_inpl(float x, float y, float z, float w, dm_vec4* out)
{
	*out = dm_vec4_set(x, y, z, w);
}

dm_vec4 dm_vec4_set_from_vec2(dm_vec2 vec)
{
	return (dm_vec4) { vec.x, vec.y, 0.0f, 0.0f };
}

void dm_vec4_set_from_vec2_inpl(dm_vec2 vec, dm_vec4* out)
{
	*out = dm_vec4_set_from_vec2(vec);
}

dm_vec4 dm_vec4_set_from_vec3(dm_vec3 vec)
{
	return (dm_vec4) { vec.x, vec.y, vec.z, 0.0f };
}

void dm_vec4_set_from_vec3_inpl(dm_vec3 vec, dm_vec4* out)
{
	*out = dm_vec4_set_from_vec3(vec);
}

dm_vec4 dm_vec4_add_scalar(dm_vec4 vec, float scalar)
{
	return (dm_vec4) { vec.x + scalar, vec.y + scalar, vec.z + scalar, vec.w* scalar };
}

void dm_vec4_add_scalar_inpl(float scalar, dm_vec4* out)
{
	*out = dm_vec4_add_scalar(*out, scalar);
}

dm_vec4 dm_vec4_sub_scalar(dm_vec4 vec, float scalar)
{
	return dm_vec4_add_scalar(vec, -scalar);
}

void dm_vec4_sub_scalar_inpl(float scalar, dm_vec4* out)
{
	*out = dm_vec4_sub_scalar(*out, scalar);
}

dm_vec4 dm_vec4_scale(dm_vec4 vec, float scale)
{
	return (dm_vec4) { vec.x* scale, vec.y* scale, vec.z* scale, vec.w* scale };
}

void dm_vec4_scale_inpl(float scale, dm_vec4* out)
{
	*out = dm_vec4_scale(*out, scale);
}

float dm_vec4_dot(dm_vec4 left, dm_vec4 right)
{
	return (float)(left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w);
}

float dm_vec4_len(dm_vec4 vec)
{
	float len = (float)sqrt(dm_vec4_dot(vec, vec));
	//if (len == 0) DM_WARN("Vec4 has length 0!");
	return len;
}

dm_vec4 dm_vec4_add_vec4(dm_vec4 left, dm_vec4 right)
{
	return (dm_vec4) { left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w };
}

void dm_vec4_add_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out)
{
	*out = dm_vec4_add_vec4(left, right);
}

dm_vec4 dm_vec4_sub_vec4(dm_vec4 left, dm_vec4 right)
{
	return (dm_vec4) { left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w };
}

void dm_vec4_sub_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out)
{
	*out = dm_vec4_sub_vec4(left, right);
}

dm_vec4 dm_vec4_norm(dm_vec4 vec)
{
	float len = dm_vec4_len(vec);
	if (len == 0)
	{
		return (dm_vec4) { 0, 0, 0, 0 };
	}
	return dm_vec4_scale(vec, 1.0f / len);
}

void dm_vec4_norm_inpl(dm_vec4* out)
{
	float len = dm_vec4_len(*out);
	if (len != 0) dm_vec4_scale_inpl(1.0f / len, out);
}

float dm_vec4_angle(dm_vec4 left, dm_vec4 right)
{
	float llen = dm_vec4_len(left);
	float rlen = dm_vec4_len(right);
	DM_ASSERT_MSG((llen > 0 && rlen > 0), "Trying to get an angle between vec3s and at least one has length 0!");
	return (float)acosf(DM_CLAMP(dm_vec4_dot(left, right) / (llen * rlen), -1.0f, 1.0f));
}

float dm_vec4_angle_degrees(dm_vec4 left, dm_vec4 right)
{
	return dm_deg_to_rad(dm_vec4_angle(left, right));
}