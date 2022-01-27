#ifndef __DM_MATH_MISC_H__
#define __DM_MATH_MISC_H__

#include "core/dm_defines.h"
#include "dm_math_types.h"

// helper functions
DM_INLINE
float dm_rad_to_deg(float radians)
{
	return  (float)(radians * (180.0f / DM_MATH_PI));
}
DM_INLINE
float dm_deg_to_rad(float degrees)
{
	return (float)(degrees * (DM_MATH_PI / 180.0f));
}

float dm_math_angle_xy(float x, float y);

#endif