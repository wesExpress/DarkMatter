#include "dm_math_vec2.h"
#include "dm_math_misc.h"
#include "core/dm_assert.h"
#include <math.h>

dm_vec2 dm_vec2_set(float x, float y)
{
	return (dm_vec2) { x, y };
}

void dm_vec2_set_inpl(float x, float y, dm_vec2* out)
{
	*out = dm_vec2_set(x, y);
}

dm_vec2 dm_vec2_set_from_vec3(dm_vec3 vec)
{
	return (dm_vec2) { vec.x, vec.y };
}

void dm_vec2_set_from_vec3_inpl(dm_vec3 vec, dm_vec2* out)
{
	*out = dm_vec2_set_from_vec3(vec);
}

dm_vec2 dm_vec2_set_from_vec4(dm_vec4 vec)
{
	return (dm_vec2) { vec.x, vec.y };
}

void dm_vec2_set_from_vec4_inpl(dm_vec4 vec, dm_vec2* out)
{
	*out = dm_vec2_set_from_vec4(vec);
}

dm_vec2 dm_vec2_add_scalar(dm_vec2 vec, float scalar)
{
	return (dm_vec2) { vec.x + scalar, vec.y + scalar };
}

void dm_vec2_add_scalar_inpl(dm_vec2 vec, float scalar, dm_vec2* out)
{
	*out = dm_vec2_add_scalar(vec, scalar);
}

dm_vec2 dm_vec2_sub_scalar(dm_vec2 vec, float scalar)
{
	return dm_vec2_add_scalar(vec, -scalar);
}

void dm_vec2_sub_scalar_inpl(dm_vec2 vec, float scalar, dm_vec2* out)
{
	*out = dm_vec2_sub_scalar(vec, scalar);
}

dm_vec2 dm_vec2_scale(dm_vec2 vec, float scale)
{
	return (dm_vec2) { vec.x* scale, vec.y* scale };
}

void dm_vec2_scale_inpl(float scale, dm_vec2* out)
{
	*out = dm_vec2_scale(*out, scale);
}

float dm_vec2_dot(dm_vec2 left, dm_vec2 right)
{
	return (float)(left.x * right.x + left.y * right.y);
}

float dm_vec2_len(dm_vec2 vec)
{
	float len = (float)sqrt(dm_vec2_dot(vec, vec));
	//if (len == 0) DM_WARN("Vec2 has length 0!");
	return len;
}

dm_vec2 dm_vec2_add_vec2(dm_vec2 left, dm_vec2 right)
{
	return (dm_vec2) { left.x + right.x, left.y + right.y };
}

void dm_vec2_add_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out)
{
	*out = dm_vec2_add_vec2(left, right);
}

dm_vec2 dm_vec2_sub_vec2(dm_vec2 left, dm_vec2 right)
{
	return (dm_vec2) { left.x - right.x, left.y - right.y };
}

void dm_vec2_sub_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out)
{
	*out = dm_vec2_sub_vec2(left, right);
}

dm_vec2 dm_vec2_norm(dm_vec2 vec)
{
	float len = dm_vec2_len(vec);
	if (len == 0) return (dm_vec2) { 0, 0 };
	return dm_vec2_scale(vec, 1.0f / len);
}

void dm_vec2_norm_inpl(dm_vec2* out)
{
	float len = dm_vec2_len(*out);
	if (len != 0) dm_vec2_scale_inpl(1.0f / len, out);
}

float dm_vec2_angle(dm_vec2 left, dm_vec2 right)
{
	float llen = dm_vec2_len(left);
	float rlen = dm_vec2_len(right);
	DM_ASSERT_MSG((llen > 0 && rlen > 0), "Trying to get an angle between vec2s and at least one has length 0!");
	return (float)acosf(DM_CLAMP(dm_vec2_dot(left, right) / (llen * rlen),-1.0f, 1.0f));
}

float dm_vec2_angle_degrees(dm_vec2 left, dm_vec2 right)
{
	return dm_deg_to_rad(dm_vec2_angle(left, right));
}