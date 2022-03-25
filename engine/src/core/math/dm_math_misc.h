#ifndef __DM_MATH_MISC_H__
#define __DM_MATH_MISC_H__

#include "core/dm_defines.h"
#include "dm_math_types.h"

// helper functions

DM_API float dm_rad_to_deg(float radians);

DM_API float dm_deg_to_rad(float degrees);

DM_API float dm_math_angle_xy(float x, float y);

// c math functions
DM_API float dm_sin(float angle);
DM_API float dm_cos(float angle);
DM_API float dm_tan(float angle);

#endif