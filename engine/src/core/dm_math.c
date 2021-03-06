#include "dm_math.h"
#include <math.h>

float dm_math_angle_xy(float x, float y)
{
	float theta = 0.0f;
    
	if (x >= 0.0f)
	{
		theta = atanf(y / x);
		if (theta < 0.0f) theta += 2.0f * DM_MATH_PI;
	}
	else theta = atanf(y / x) + DM_MATH_PI;
    
	return theta;
}

float dm_sin(float angle)
{
	return sinf(angle);
}

float dm_cos(float angle)
{
	return cosf(angle);
}


float dm_tan(float angle)
{
	return tanf(angle);
}
