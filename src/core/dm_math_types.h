#ifndef __DM_MATH_TYPES_H__
#define __DM_MATH_TYPES_H__

#define DM_MATH_PI 3.14159265359826
#define DM_MATH_INV_PI 0.31830988618
#define DM_MATH_ANGLE_RAD_TOLERANCE 0.001
#define DM_CLAMP(X, MIN, MAX) (X < MIN) ? MIN : (X > MAX) ? MAX : X

typedef struct dm_vec2
{
	union
	{
		float v[2];
		float xy[2];
		struct
		{
			float x, y;
		};
	};

} dm_vec2;

typedef struct dm_vec3
{
	union
	{
		float v[3];
		float xyz[3];
		struct
		{
			float x, y, z;
		};
	};
} dm_vec3;

typedef struct dm_vec4
{
	union
	{
		float v[4];
		float xyzw[4];
		struct
		{
			float x, y, z, w;
		};
	};
} dm_vec4;

typedef struct dm_mat2
{
	union
	{
		dm_vec2 rows[2];
		float m[2 * 2];
	};
} dm_mat2;

typedef struct dm_mat3
{
	union
	{
		dm_vec3 rows[3];
		float m[3 * 3];
	};

} dm_mat3;

typedef struct dm_mat4
{
	union
	{
		dm_vec4 rows[4];
		float m[4 * 4];
	};

} dm_mat4;

#endif