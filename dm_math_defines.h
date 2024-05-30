#ifndef DM_MATH_DEFINES_H
#define DM_MATH_DEFINES_H

// various constants
#define DM_MATH_PI                  3.14159265359826f
#define DM_MATH_INV_PI              0.31830988618f
#define DM_MATH_2PI                 6.2831853071795f
#define DM_MATH_INV_2PI             0.159154943091f
#define DM_MATH_4PI                 12.566370614359173f
#define DM_MATH_INV_4PI             0.0795774715459f
#define DM_MATH_INV_12              0.0833333f

#define DM_MATH_ANGLE_RAD_TOLERANCE 0.001f
#define DM_MATH_SQRT2               1.41421356237309f
#define DM_MATH_INV_SQRT2           0.70710678118654f
#define DM_MATH_SQRT3               1.73205080756887f
#define DM_MATH_INV_SQRT3           0.57735026918962f

#define DM_MATH_DEG_TO_RAD          0.0174533f
#define DM_MATH_RAD_TO_DEG          57.2958f

#define DM_MATH_1024_INV            0.0009765625f

// math macros
#define DM_MAX(X, Y)          (X > Y ? X : Y)
#define DM_MIN(X, Y)          (X < Y ? X : Y)
#define DM_SIGN(X)            ((0 < X) - (X < 0))
#define DM_SIGNF(X)           (float)((0 < X) - (X < 0))
#define DM_CLAMP(X, MIN, MAX) DM_MIN(DM_MAX(X, MIN), MAX)
#define DM_BIT_SHIFT(X)       (1 << X)

#endif
