#ifndef __DM_MATH_MISC_H__
#define __DM_MATH_MISC_H__

#include "core/dm_defines.h"
#include "dm_math_types.h"

// helper functions

float dm_rad_to_deg(float radians);

float dm_deg_to_rad(float degrees);

float dm_math_angle_xy(float x, float y);

// c math functions
float dm_sin(float angle);
float dm_cos(float angle);
float dm_tan(float angle);

#endif