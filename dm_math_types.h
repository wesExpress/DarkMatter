#ifndef DM_MATH_TYPES_H
#define DM_MATH_TYPES_H

#include "dm_defines.h"

typedef float dm_vec2[2];
typedef float dm_vec3[3];
typedef DM_ALIGN(16) float dm_vec4[4];
typedef DM_ALIGN(16) float dm_quat[4];

typedef float dm_mat2[2][2];
typedef float dm_mat3[3][3];
typedef DM_ALIGN(16) float dm_mat4[4][4];

#define N2 2
#define N3 3
#define N4 4
#define M2 N2 * N2
#define M3 N3 * N3
#define M4 N4 * N4

#define DM_VEC2_SIZE sizeof(float) * N2
#define DM_VEC3_SIZE sizeof(float) * N3
#define DM_VEC4_SIZE sizeof(float) * N4
#define DM_QUAT_SIZE DM_VEC4_SIZE
#define DM_MAT2_SIZE sizeof(float) * N2
#define DM_MAT3_SIZE sizeof(float) * M3
#define DM_MAT4_SIZE sizeof(float) * M4

#define DM_ALIGN_BYTES(SIZE, ALIGNMENT) ((SIZE + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

#endif
