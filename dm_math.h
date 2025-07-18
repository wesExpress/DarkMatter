#ifndef DM_MATH_H
#define DM_MATH_H

#include "dm_defines.h"
#include "dm_math_defines.h"
#include "dm_math_types.h"

#include "dm_intrinsics.h"

int   dm_abs(int x);
float dm_fabs(float x);
float dm_sqrtf(float x);
float dm_math_angle_xy(float x, float y);
float dm_sin(float angle);
float dm_cos(float angle);
float dm_tan(float angle);
float dm_sind(float angle);
float dm_cosd(float angle);
float dm_asin(float value);
float dm_acos(float value);
float dm_atan(float x, float y);
float dm_smoothstep(float edge0, float edge1, float x);
float dm_smootherstep(float edge0, float edge1, float x);
float dm_exp(float x);
float dm_powf(float x, float y);
float dm_roundf(float x);
int   dm_round(float x);
int   dm_ceil(float x);
int   dm_floor(float x);
float dm_logf(float x);
float dm_log2f(float x);
bool  dm_isnan(float x);
float dm_clamp(float x, float min, float max);

float dm_rad_to_deg(float radians);
float dm_deg_to_rad(float degrees);

/****
VEC2
******/
DM_INLINE
void dm_vec2_set_from_vec2(const dm_vec2 in, dm_vec2 out)
{
    out[0] = in[0];
    out[1] = in[1];
}

DM_INLINE
void dm_vec2_add_vec2(const dm_vec2 left, const dm_vec2 right, dm_vec2 out)
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
}

DM_INLINE
void dm_vec2_sub_vec2(const dm_vec2 left, const dm_vec2 right, dm_vec2 out)
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
}

DM_INLINE
void dm_vec2_add_scalar(const dm_vec2 vec, const float scalar, dm_vec2 out)
{
    out[0] = vec[0] + scalar;
    out[1] = vec[1] + scalar;;
}

DM_INLINE
void dm_vec2_sub_scalar(const dm_vec2 vec, const float scalar, dm_vec2 out)
{
    out[0] = vec[0] - scalar;
    out[1] = vec[1] - scalar;;
}

DM_INLINE
void dm_vec2_scale(const dm_vec2 vec, const float s, dm_vec2 out)
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
}

DM_INLINE
float dm_vec2_dot(const dm_vec2 left, const dm_vec2 right)
{
    return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]));
}

DM_INLINE
float dm_vec2_mag(const dm_vec2 vec)
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[2]));
}

DM_INLINE
void dm_vec2_norm(const dm_vec2 vec, dm_vec2 out)
{
	out[0] = vec[0];
    out[1] = vec[1];
    
    const float mag = dm_vec2_mag(out);
    const float s = 1.0f / mag;
    
    dm_vec2_scale(vec, s, out);
}

/****
VEC3
******/
DM_INLINE
void dm_vec3_from_vec3(const dm_vec3 in, dm_vec3 out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

DM_INLINE
void dm_vec3_from_vec4(const dm_vec4 vec4, dm_vec3 out)
{
    out[0] = vec4[0];
    out[1] = vec4[1];
    out[2] = vec4[2];
}

DM_INLINE
void dm_vec3_add_scalar(const dm_vec3 vec, const float scalar, dm_vec3 out)
{
    out[0] = vec[0] + scalar;
    out[1] = vec[1] + scalar;
    out[2] = vec[2] + scalar;
}

DM_INLINE
void dm_vec3_sub_scalar(const dm_vec3 vec, const float scalar, dm_vec3 out)
{
    out[0] = vec[0] - scalar;
    out[1] = vec[1] - scalar;
    out[2] = vec[2] - scalar;
}

DM_INLINE
void dm_vec3_add_vec3(const dm_vec3 left, const dm_vec3 right, dm_vec3 out)
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[2];
}

DM_INLINE
void dm_vec3_sub_vec3(const dm_vec3 left, const dm_vec3 right, dm_vec3 out)
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
}

DM_INLINE
void dm_vec3_mul_vec3(const dm_vec3 left, const dm_vec3 right, dm_vec3 out)
{
    out[0] = left[0] * right[0];
    out[1] = left[1] * right[1];
    out[2] = left[2] * right[2];
}

DM_INLINE
void dm_vec3_div_vec3(const dm_vec3 left, const dm_vec3 right, dm_vec3 out)
{
    out[0] = left[0] / right[0];
    out[1] = left[1] / right[1];
    out[2] = left[2] / right[2];
}

DM_INLINE
float dm_vec3_dot(const dm_vec3 left, const dm_vec3 right)
{
#if 0
    dm_simd_float x1 = dm_simd_set1_float(left[0]);
    dm_simd_float y1 = dm_simd_set1_float(left[1]);
    dm_simd_float z1 = dm_simd_set1_float(left[2]);
    
    dm_simd_float x2 = dm_simd_set1_float(right[0]);
    dm_simd_float y2 = dm_simd_set1_float(right[1]);
    dm_simd_float z2 = dm_simd_set1_float(right[2]);
    
    dm_simd_float result = dm_simd_mul_float(x1, x2);
    result = dm_simd_fmadd_float(y1, y2, result);
    result = dm_simd_fmadd_float(z1, z2, result);
    
    return dm_simd_extract_float(result);
#else
    float r;
    
    r  = left[0] * right[0];
    r += left[1] * right[1];
    r += left[2] * right[2];
    
    return r;
#endif
}

DM_INLINE
void dm_vec3_cross(const dm_vec3 left, const dm_vec3 right, dm_vec3 out)
{
    out[0] = left[1] * right[2] - left[2] * right[1];
    out[1] = left[2] * right[0] - left[0] * right[2];
    out[2] = left[0] * right[1] - left[1] * right[0];
}

DM_INLINE
void dm_vec3_cross_cross(const dm_vec3 first, const dm_vec3 second, const dm_vec3 third, dm_vec3 out)
{
    dm_vec3_cross(first, second, out);
    dm_vec3_cross(out,   third, out);
}

DM_INLINE
void dm_vec3_scale(const dm_vec3 vec, float s, dm_vec3 out)
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
    out[2] = vec[2] * s;
}

DM_INLINE
float dm_vec3_mag(const dm_vec3 vec)
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]));
}

DM_INLINE
float dm_vec3_mag2(const dm_vec3 vec)
{
    return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
}

DM_INLINE
void dm_vec3_norm(const dm_vec3 vec, dm_vec3 out)
{
    float mag = dm_vec3_mag(vec);
	
    float s = 1.0f / mag;
    dm_vec3_scale(vec, s, out);
}

DM_INLINE
void dm_vec3_rotate(const dm_vec3 vec, const dm_quat quat, dm_vec3 out)
{
    dm_vec3 qv = { quat[0],quat[1],quat[2] };
    dm_vec3 t  = { 0 };
    dm_vec3 a  = { 0 };
    
    dm_vec3_cross(qv, vec, t);
    dm_vec3_scale(t, 2, t);
    
    dm_vec3_cross(qv, t, a);
    dm_vec3_scale(t, quat[3], t);
    dm_vec3_add_vec3(t, a, a);
    dm_vec3_add_vec3(vec, a, out);
}

DM_INLINE
void dm_vec3_negate(const dm_vec3 vec, dm_vec3 out)
{
    out[0] = -vec[0];
    out[1] = -vec[1];
    out[2] = -vec[2];
}

DM_INLINE
bool dm_vec3_same_direction(const dm_vec3 left, const dm_vec3 right)
{
    return dm_vec3_dot(left,right) > 0;
}

DM_INLINE
bool dm_vec3_equals_vec3(const dm_vec3 left, const dm_vec3 right)
{
    return ((left[0]==right[0]) && (left[1]==right[1]) && (left[2]==right[2]));
}

DM_INLINE
bool dm_vec3_lt_vec3(const dm_vec3 left, const dm_vec3 right)
{
    return (left[0]<right[0]) && (left[1]<right[1]) && (left[2]<right[2]);
}

DM_INLINE
bool dm_vec3_leq_vec3(const dm_vec3 left, const dm_vec3 right)
{
    return (left[0]<=right[0]) && (left[1]<=right[1]) && (left[2]<=right[2]);
}

DM_INLINE
bool dm_vec3_gt_vec3(const dm_vec3 left, const dm_vec3 right)
{
    return (left[0]>right[0]) && (left[1]>right[1]) && (left[2]>right[2]);
}

DM_INLINE
bool dm_vec3_geq_vec3(const dm_vec3 left, const dm_vec3 right)
{
    return (left[0]>=right[0]) && (left[1]>=right[1]) && (left[2]>=right[2]);
}

DM_INLINE
void dm_vec3_sign(const dm_vec3 vec, dm_vec3 out)
{
    out[0] = DM_SIGNF(vec[0]);
    out[1] = DM_SIGNF(vec[1]);
    out[2] = DM_SIGNF(vec[2]);
}

DM_INLINE
void dm_vec3_fabs(const dm_vec3 vec, dm_vec3 out)
{
    out[0] = dm_fabs(vec[0]);
    out[1] = dm_fabs(vec[1]);
    out[2] = dm_fabs(vec[2]);
}

DM_INLINE
void dm_vec3_reflect(const dm_vec3 vec, const dm_vec3 normal, dm_vec3 out)
{
    float s;
    dm_vec3 n, v;
    
    dm_vec3_norm(vec, v);
    s = dm_vec3_dot(vec, normal);
    s *= 2.f;
    dm_vec3_scale(normal, s, n);
    
    dm_vec3_sub_vec3(vec, n, out);
}

DM_INLINE
void dm_vec3_refract(const dm_vec3 vec, const dm_vec3 n, const float e, dm_vec3 out)
{
    float vec_length = dm_vec3_mag(vec);
    float c = -dm_vec3_dot(vec, n) / vec_length;
    
    float aux = vec_length * (e * c - dm_sqrtf(1 - e * e * (1 - c * c)));
    dm_vec3 nv, nn;
    dm_vec3_scale(vec, e, nv);
    dm_vec3_scale(n, aux, nn);
    dm_vec3_add_vec3(nv, nn, out);
}

/****
VEC4
******/
DM_INLINE
void dm_vec4_set_from_floats(float x, float y, float z, float w, dm_vec4 out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

DM_INLINE
void dm_vec4_set_from_vec4(const dm_vec4 in, dm_vec4 out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3];
}

DM_INLINE
void dm_vec4_from_vec3(const dm_vec3 vec3, dm_vec4 out)
{
    out[0] = vec3[0];
    out[1] = vec3[1];
    out[2] = vec3[2];
    out[3] = 0;
}

DM_INLINE
void dm_vec4_add_vec4(const dm_vec4 left, const dm_vec4 right, dm_vec4 out)
{
    dm_simd_float l, r, o;
    
    l = dm_simd_load_float(left);
    r = dm_simd_load_float(right);
    o = dm_simd_add_float(l, r);
    
    dm_simd_store_float(out, o);
}

DM_INLINE
void dm_vec4_sub_vec4(const dm_vec4 left, const dm_vec4 right, dm_vec4 out)
{
    dm_simd_float l, r, o;
    
    l = dm_simd_load_float(left);
    r = dm_simd_load_float(right);
    o = dm_simd_sub_float(l, r);
    
    dm_simd_store_float(out, o);
}

DM_INLINE
float dm_vec4_dot(const dm_vec4 left, const dm_vec4 right)
{
    dm_simd_float l, r;
    
    l = dm_simd_load_float(left);
    r = dm_simd_load_float(right);
    
    return dm_simd_dot_float(l, r);
}

DM_INLINE
void dm_vec4_scale(const dm_vec4 vec, float s, dm_vec4 out)
{
    dm_simd_float v, scalar;
    
    scalar = dm_simd_set1_float(s);
    v      = dm_simd_load_float(vec);
    v      = dm_simd_mul_float(v, scalar);
    
    dm_simd_store_float(out, v);
}

DM_INLINE
float dm_vec4_mag(const dm_vec4 vec)
{
    dm_simd_float v = dm_simd_load_float(vec);
    return dm_simd_dot_float(v,v);
}

DM_INLINE
void dm_vec4_norm(const dm_vec4 vec, dm_vec4 out)
{
    dm_simd_float v    = dm_simd_load_float(vec);
    dm_simd_float v_sq = dm_simd_mul_float(v, v);
    dm_simd_float mag  = dm_simd_hadd_fast_float(v_sq);
    
    mag = dm_simd_sqrt_float(mag);
    
    v = dm_simd_div_float(v, mag);
    
    dm_simd_store_float(out, v);
}

/**********
QUATERNION
************/
DM_INLINE
void dm_quat_from_quat(const dm_quat in, dm_quat out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3];
}

DM_INLINE
void dm_quat_from_vec4(const dm_vec4 vec, dm_quat out)
{
    out[0] = vec[0];
    out[1] = vec[1];
    out[2] = vec[2];
    out[3] = vec[3];
}

DM_INLINE
void dm_quat_from_vec3(const dm_vec3 vec, dm_quat out)
{
    out[0] = vec[0];
    out[1] = vec[1];
    out[2] = vec[2];
    out[3] = 0;
}

DM_INLINE
void dm_quat_add_quat(const dm_quat left, const dm_quat right, dm_quat out)
{
    dm_vec4_add_vec4(left, right, out);
}

DM_INLINE
void dm_quat_sub_quat(const dm_quat left, const dm_quat right, dm_quat out)
{
    dm_vec4_sub_vec4(left, right, out);
}

DM_INLINE
void dm_quat_mul_quat(const dm_quat left, const dm_quat right, dm_quat out)
{
    //
    const float l0 = left[0];
    const float l1 = left[1];
    const float l2 = left[2];
    const float l3 = left[3];
    
    //
    const float r0 = right[0];
    const float r1 = right[1];
    const float r2 = right[2];
    const float r3 = right[3];
    
    //
    out[0] = (l3 * r0) + (l0 * r3) + (l1 * r2) - (l2 * r1);
    out[1] = (l3 * r1) - (l0 * r2) + (l1 * r3) + (l2 * r0);
    out[2] = (l3 * r2) + (l0 * r1) - (l1 * r0) + (l2 * r3);
    out[3] = (l3 * r3) - (l0 * r0) - (l1 * r1) - (l2 * r2);
}

DM_INLINE
void dm_quat_cross(const dm_quat left, const dm_quat right, dm_quat out)
{
    dm_quat_mul_quat(left, right, out);
}

DM_INLINE
void dm_vec3_mul_quat(const dm_vec3 v, const dm_quat q, dm_quat out)
{
    const float v0 = v[0];
    const float v1 = v[1];
    const float v2 = v[2];
    
    const float q0 = q[0];
    const float q1 = q[1];
    const float q2 = q[2];
    const float q3 = q[3];
    
    out[0] =  (v0 * q3) + (v1 * q2) - (v2 * q1);
    out[1] = -(v0 * q2) + (v1 * q3) + (v2 * q0);
    out[2] =  (v0 * q1) - (v1 * q0) + (v2 * q3);
    out[3] = -(v0 * q0) - (v1 * q1) - (v2 * q2);
}

DM_INLINE
float dm_quat_mag(const dm_quat quat)
{
    return dm_vec4_mag(quat);
}

DM_INLINE
void dm_quat_scale(const dm_quat quat, float s, dm_quat out)
{
    dm_vec4_scale(quat, s, out);
}

DM_INLINE
void dm_quat_norm(const dm_quat quat, dm_quat out)
{
	dm_vec4_norm(quat, out);
}

DM_INLINE
void dm_quat_conjugate(const dm_quat quat, dm_quat out)
{
    out[0] = -quat[0];
    out[1] = -quat[1];
    out[2] = -quat[2];
    out[3] =  quat[3];
}

DM_INLINE
void dm_quat_inverse(const dm_quat quat, dm_quat out)
{
    dm_quat_conjugate(quat,out);
    
	float mag = dm_quat_mag(quat);
    mag *= mag;
    
    dm_quat_scale(out, mag, out);
}

DM_INLINE
float dm_quat_dot(const dm_quat left, const dm_quat right)
{
	return dm_vec4_dot(left, right);
}

DM_INLINE
float dm_quat_angle(const dm_quat left, const dm_quat right)
{
	float mag = dm_quat_mag(left) * dm_quat_mag(right);
    
	if (mag == 0) return 0;
    
	float angle = dm_quat_dot(left, right) / mag;
	return dm_acos(angle);
}

DM_INLINE
void dm_quat_from_axis_angle(const dm_quat axis, float angle, dm_quat out)
{
    const float half_a = angle * 0.5f;
    const float s      = dm_sin(half_a);
    
    out[0] = axis[0] * s;
    out[1] = axis[1] * s; 
    out[2] = axis[2] * s;
    out[3] = dm_cos(half_a);
    
    dm_quat_norm(out,out);
}

DM_INLINE
void dm_quat_from_axis_angle_deg(const dm_vec3 axis, float angle, dm_quat out)
{
    dm_quat_from_axis_angle(axis, DM_MATH_DEG_TO_RAD * angle, out);
}

DM_INLINE
void dm_quat_from_vectors(const dm_vec3 vec1, const dm_vec3 vec2, dm_quat out)
{
    dm_vec3 u    = { 0 };
    dm_vec3 w    = { 0 }; 
    dm_vec3 axis = { 0 };
    
    dm_vec3_norm(vec1, u);
    dm_vec3_norm(vec2, w);
    
    float angle = dm_acos(dm_vec3_dot(u,w));
    dm_vec3_cross(u,w,axis);
    dm_vec3_norm(axis, axis);
    
    dm_quat_from_axis_angle(axis, angle, out);
}

DM_INLINE
void dm_quat_negate(const dm_quat quat, dm_quat out)
{
    dm_quat_scale(quat, -1, out);
}

DM_INLINE
void dm_quat_nlerp(const dm_quat quat_a, const dm_quat quat_b, float t, dm_quat out)
{
    dm_quat lerp; 
    dm_quat s;
    
    dm_quat_sub_quat(quat_b, quat_a,s);
    dm_quat_scale(s, t, s);
    dm_quat_add_quat(quat_a, s, lerp);
    dm_quat_norm(lerp, lerp);
}

/****
MAT3
******/
DM_INLINE
void dm_mat3_identity(dm_mat3 mat)
{
    mat[0][0] = 1;
    mat[1][1] = 1;
    mat[2][2] = 1;

    mat[0][1] = 0;
    mat[0][2] = 0;
    mat[1][0] = 0;
    mat[1][2] = 0;
    mat[2][0] = 0;
    mat[2][1] = 0;
}

DM_INLINE
void dm_mat3_transpose(const dm_mat3 mat, dm_mat3 out)
{
    // need a dummy matrix since we could be writing to the same one we are transposing
    dm_mat3 d;
    
    // diagonals the same
    d[0][0] = mat[0][0];
    d[1][1] = mat[1][1];
    d[2][2] = mat[2][2];
    
    // swap rest
    d[0][1] = mat[1][0];
    d[0][2] = mat[2][0];
    
    d[1][0] = mat[0][1];
    d[2][0] = mat[0][2];
    
    d[1][2] = mat[2][1];
    
    d[2][1] = mat[1][2];

    // write back out
    out[0][0] = d[0][0];
    out[0][1] = d[0][1];
    out[0][2] = d[0][2];

    out[1][0] = d[1][0];
    out[1][1] = d[1][1];
    out[1][2] = d[1][2];

    out[2][0] = d[2][0];
    out[2][1] = d[2][1];
    out[2][2] = d[2][2];
}

DM_INLINE
void dm_mat3_mul_mat3(dm_mat3 left, dm_mat3 right, dm_mat3 out)
{
	dm_mat3 d;
    dm_mat3 r_t;
    dm_mat3_transpose(right, r_t);
    
    d[0][0] = dm_vec3_dot(left[0], r_t[0]);
    d[0][1] = dm_vec3_dot(left[0], r_t[1]);
    d[0][2] = dm_vec3_dot(left[0], r_t[2]);
    
    d[1][0] = dm_vec3_dot(left[1], r_t[0]);
    d[1][1] = dm_vec3_dot(left[1], r_t[1]);
    d[1][2] = dm_vec3_dot(left[1], r_t[2]);
    
    d[2][0] = dm_vec3_dot(left[2], r_t[0]);
    d[2][1] = dm_vec3_dot(left[2], r_t[1]);
    d[2][2] = dm_vec3_dot(left[2], r_t[2]);
    
    //dm_memcpy(out, d, sizeof(d));
    out[0][0] = d[0][0];
    out[0][1] = d[0][1];
    out[0][2] = d[0][2];

    out[1][0] = d[1][0];
    out[1][1] = d[1][1];
    out[1][2] = d[1][2];

    out[2][0] = d[2][0];
    out[2][1] = d[2][1];
    out[2][2] = d[2][2];

}

DM_INLINE
void dm_mat3_mul_vec3(dm_mat3 mat, dm_vec3 vec, dm_vec3 out)
{
    out[0] = dm_vec3_dot(mat[0], vec);
    out[1] = dm_vec3_dot(mat[1], vec);
    out[2] = dm_vec3_dot(mat[2], vec);
}

DM_INLINE
void dm_mat3_mul_scalar(dm_mat3 mat, float scalar, dm_mat3 out)
{
    dm_vec3_scale(mat[0], scalar, out[0]);
    dm_vec3_scale(mat[1], scalar, out[1]);
    dm_vec3_scale(mat[2], scalar, out[2]);
}

DM_INLINE
void dm_mat3_add_mat3(dm_mat3 left, dm_mat3 right, dm_mat3 out)
{
    dm_vec3_add_vec3(left[0], right[0], out[0]);
    dm_vec3_add_vec3(left[1], right[1], out[1]);
    dm_vec3_add_vec3(left[2], right[2], out[2]);
}

DM_INLINE
void dm_mat3_sub_mat3(dm_mat3 left, dm_mat3 right, dm_mat3 out)
{
    dm_vec3_sub_vec3(left[0], right[0], out[0]);
    dm_vec3_sub_vec3(left[1], right[1], out[1]);
    dm_vec3_sub_vec3(left[2], right[2], out[2]);
}

// https://github.com/recp/cglm/blob/cdd4d0e83e9ee79f73aeb0a4fb60b4abd8ecf947/include/cglm/mat3.h#L341
DM_INLINE
void dm_mat3_inverse(dm_mat3 mat, dm_mat3 dest)
{
	float det;
    float a = mat[0][0], b = mat[0][1], c = mat[0][2],
    d = mat[1][0], e = mat[1][1], f = mat[1][2],
    g = mat[2][0], h = mat[2][1], i = mat[2][2];
    
    dest[0][0] =   e * i - f * h;
    dest[0][1] = -(b * i - h * c);
    dest[0][2] =   b * f - e * c;
    
    dest[1][0] = -(d * i - g * f);
    dest[1][1] =   a * i - c * g;
    dest[1][2] = -(a * f - d * c);
    
    dest[2][0] =   d * h - g * e;
    dest[2][1] = -(a * h - g * b);
    dest[2][2] =   a * e - b * d;
    
    det = 1.0f / (a * dest[0][0] + b * dest[1][0] + c * dest[2][0]);
    
    dm_mat3_mul_scalar(dest, det, dest);
}

DM_INLINE
void dm_mat3_rotation(float radians, const dm_vec3 axis, dm_mat3 out)
{
	float C = dm_cos(radians);
	float S = dm_sin(radians);
	float t = 1 - C;
    
	out[0][0] = t * axis[0] * axis[0] + C;
	out[0][1] = t * axis[0] * axis[1] - S * axis[2];
	out[0][2] = t * axis[0] * axis[2] + S * axis[1];
    
	out[1][0] = t * axis[0] * axis[1] + S * axis[2];
	out[1][1] = t * axis[0] * axis[1] + C;
	out[1][2] = t * axis[0] * axis[2] - S * axis[1];
    
	out[2][0] = t * axis[0] * axis[2] - S * axis[1];
	out[2][1] = t * axis[0] * axis[2] + S * axis[0];
	out[2][2] = t * axis[0] * axis[2] + C;
}

DM_INLINE
void dm_mat3_rotation_degrees(float degrees, const dm_vec3 axis, dm_mat3 out)
{
	dm_mat3_rotation(DM_MATH_DEG_TO_RAD * degrees, axis, out);
}

DM_INLINE
void dm_mat3_rotate_from_quat(const dm_quat quat, dm_mat3 out)
{
    float xx = quat[0] * quat[0];
    float yy = quat[1] * quat[1];
    float zz = quat[2] * quat[2];
    
    float xy = quat[0] * quat[1];
    float xz = quat[0] * quat[2];
    float xw = quat[0] * quat[3];
    float yz = quat[1] * quat[2];
    float yw = quat[1] * quat[3];
    float zw = quat[2] * quat[3];
    
    out[0][0] = 1 - 2 * (yy + zz);
    out[0][1] = 2 * (xy - zw);
    out[0][2] = 2 * (xz + yw);
    
    out[1][0] = 2 * (xy + zw);
    out[1][1] = 1 - 2 * (xx + zz);
    out[1][2] = 2 * (yz - xw);
    
    out[2][0] = 2 * (xz - yw);
    out[2][1] = 2 * (yz + xw);
    out[2][2] = 1 - 2 * (xx + yy);
}

/****
MAT4
*****/
DM_INLINE
void dm_mat4_identity(dm_mat4 mat)
{
	mat[0][0] = 1;
    mat[1][1] = 1;
    mat[2][2] = 1;
    mat[3][3] = 1;

    mat[0][1] = 0;
    mat[0][2] = 0;
    mat[0][3] = 0;
    
    mat[1][0] = 0;
    mat[1][2] = 0;
    mat[1][3] = 0;

    mat[2][0] = 0;
    mat[2][1] = 0;
    mat[2][3] = 0;

    mat[3][0] = 0;
    mat[3][1] = 0;
    mat[3][2] = 0;
}

DM_INLINE
void dm_mat4_transpose(const dm_mat4 mat, dm_mat4 out)
{
#ifdef DM_SIMD_X86
    dm_simd_float row1 = dm_simd_load_float(mat[0]);
    dm_simd_float row2 = dm_simd_load_float(mat[1]);
    dm_simd_float row3 = dm_simd_load_float(mat[2]);
    dm_simd_float row4 = dm_simd_load_float(mat[3]);
    
    dm_simd_transpose_mat4(&row1, &row2, &row3, &row4);
    
    dm_simd_store_float(out[0], row1);
    dm_simd_store_float(out[1], row2);
    dm_simd_store_float(out[2], row3);
    dm_simd_store_float(out[3], row4);
#else
    dm_mat4 d = { 0 };
    
    d[0][0] = mat[0][0];
    d[1][1] = mat[1][1];
    d[2][2] = mat[2][2];
    d[3][3] = mat[3][3];
    
    d[0][1] = mat[1][0];
    d[0][2] = mat[2][0];
    d[0][3] = mat[3][0];
    
    d[1][0] = mat[0][1];
    d[1][2] = mat[2][1];
    d[1][3] = mat[3][1];
    
    d[2][0] = mat[0][2];
    d[2][1] = mat[1][2];
    d[2][3] = mat[3][2];
    
    d[3][0] = mat[0][3];
    d[3][1] = mat[1][3];
    d[3][2] = mat[2][3];
    
    dm_memcpy(out, d, sizeof(d));
#endif
}

DM_INLINE
void dm_mat4_mul_mat4(const dm_mat4 left, const dm_mat4 right, dm_mat4 out)
{
    //
    const float l00 = left[0][0];
    const float l01 = left[0][1];
    const float l02 = left[0][2];
    const float l03 = left[0][3];
    
    const float l10 = left[1][0];
    const float l11 = left[1][1];
    const float l12 = left[1][2];
    const float l13 = left[1][3];
    
    const float l20 = left[2][0];
    const float l21 = left[2][1];
    const float l22 = left[2][2];
    const float l23 = left[2][3];
    
    const float l30 = left[3][0];
    const float l31 = left[3][1];
    const float l32 = left[3][2];
    const float l33 = left[3][3];
    
    //
    const float r00 = right[0][0];
    const float r01 = right[0][1];
    const float r02 = right[0][2];
    const float r03 = right[0][3];
    
    const float r10 = right[1][0];
    const float r11 = right[1][1];
    const float r12 = right[1][2];
    const float r13 = right[1][3];
    
    const float r20 = right[2][0];
    const float r21 = right[2][1];
    const float r22 = right[2][2];
    const float r23 = right[2][3];
    
    const float r30 = right[3][0];
    const float r31 = right[3][1];
    const float r32 = right[3][2];
    const float r33 = right[3][3];
    
    //
    out[0][0] = (l00 * r00) + (l01 * r10) + (l02 * r20) + (l03 * r30);
    out[0][1] = (l00 * r01) + (l01 * r11) + (l02 * r21) + (l03 * r31);
    out[0][2] = (l00 * r02) + (l01 * r12) + (l02 * r22) + (l03 * r32);
    out[0][3] = (l00 * r03) + (l01 * r13) + (l02 * r23) + (l03 * r33);
    
    out[1][0] = (l10 * r00) + (l11 * r10) + (l12 * r20) + (l13 * r30);
    out[1][1] = (l10 * r01) + (l11 * r11) + (l12 * r21) + (l13 * r31);
    out[1][2] = (l10 * r02) + (l11 * r12) + (l12 * r22) + (l13 * r32);
    out[1][3] = (l10 * r03) + (l11 * r13) + (l12 * r23) + (l13 * r33);
    
    out[2][0] = (l20 * r00) + (l21 * r10) + (l22 * r20) + (l23 * r30);
    out[2][1] = (l20 * r01) + (l21 * r11) + (l22 * r21) + (l23 * r31);
    out[2][2] = (l20 * r02) + (l21 * r12) + (l22 * r22) + (l23 * r32);
    out[2][3] = (l20 * r03) + (l21 * r13) + (l22 * r23) + (l23 * r33);
    
    out[3][0] = (l30 * r00) + (l31 * r10) + (l32 * r20) + (l33 * r30);
    out[3][1] = (l30 * r01) + (l31 * r11) + (l32 * r21) + (l33 * r31);
    out[3][2] = (l30 * r02) + (l31 * r12) + (l32 * r22) + (l33 * r32);
    out[3][3] = (l30 * r03) + (l31 * r13) + (l32 * r23) + (l33 * r33);
}

DM_INLINE
void dm_mat4_mul_vec4(const dm_mat4 mat, const dm_vec4 vec, dm_vec4 out)
{
    dm_mat4 mat_t;
    dm_mat4_transpose(mat, mat_t);
    
    const dm_simd_float v_x = dm_simd_set1_float(vec[0]);
    const dm_simd_float v_y = dm_simd_set1_float(vec[1]);
    const dm_simd_float v_z = dm_simd_set1_float(vec[2]);
    const dm_simd_float v_w = dm_simd_set1_float(vec[3]);
    
    const dm_simd_float m1 = dm_simd_load_float(mat_t[0]);
    const dm_simd_float m2 = dm_simd_load_float(mat_t[1]);
    const dm_simd_float m3 = dm_simd_load_float(mat_t[2]);
    const dm_simd_float m4 = dm_simd_load_float(mat_t[3]);
    
    dm_simd_float v;
    v = dm_simd_mul_float(m1, v_x);
    v = dm_simd_fmadd_float(m2,v_y, v);
    v = dm_simd_fmadd_float(m3,v_z, v);
    v = dm_simd_fmadd_float(m4,v_w, v);
    
    dm_simd_store_float(out, v);
}

DM_INLINE
void dm_mat4_mul_vec3(const dm_mat4 mat, const dm_vec3 vec, const float val_4, dm_vec4 out)
{
    dm_vec4 new_vec = { vec[0], vec[1], vec[2], val_4 };
	dm_mat4_mul_vec4(mat, new_vec, out);
}

DM_INLINE
void dm_mat4_mul_scalar(const dm_mat4 mat, float scalar, dm_mat4 out)
{
    const dm_simd_float s = dm_simd_set1_float(scalar);
    dm_simd_float row_1, row_2, row_3, row_4;
    
    row_1 = dm_simd_load_float(mat[0]);
    row_1 = dm_simd_mul_float(row_1, s);
    
    row_2 = dm_simd_load_float(mat[1]);
    row_2 = dm_simd_mul_float(row_2, s);
    
    row_3 = dm_simd_load_float(mat[2]);
    row_3 = dm_simd_mul_float(row_3, s);
    
    row_4 = dm_simd_load_float(mat[3]);
    row_4 = dm_simd_mul_float(row_4, s);
    
    dm_simd_store_float(out[0], row_1);
    dm_simd_store_float(out[1], row_2);
    dm_simd_store_float(out[2], row_3);
    dm_simd_store_float(out[3], row_4);
}

DM_INLINE
void dm_mat4_add_mat4(const dm_mat4 left, const dm_mat4 right, dm_mat4 out)
{
    dm_simd_float l, r;
    dm_simd_float row_1, row_2, row_3, row_4;
    
    l     = dm_simd_load_float(left[0]);
    r     = dm_simd_load_float(right[0]);
    row_1 = dm_simd_add_float(l, r);
    
    l     = dm_simd_load_float(left[1]);
    r     = dm_simd_load_float(right[1]);
    row_2 = dm_simd_add_float(l, r);
    
    l     = dm_simd_load_float(left[2]);
    r     = dm_simd_load_float(right[2]);
    row_3 = dm_simd_add_float(l, r);
    
    l     = dm_simd_load_float(left[3]);
    r     = dm_simd_load_float(right[3]);
    row_4 = dm_simd_add_float(l, r);
    
    dm_simd_store_float(out[0], row_1);
    dm_simd_store_float(out[1], row_2);
    dm_simd_store_float(out[2], row_3);
    dm_simd_store_float(out[3], row_4);
}

DM_INLINE
void dm_mat4_sub_mat4(const dm_mat4 left, const dm_mat4 right, dm_mat4 out)
{
	dm_simd_float l, r;
    dm_simd_float row_1, row_2, row_3, row_4;
    
    l     = dm_simd_load_float(left[0]);
    r     = dm_simd_load_float(right[0]);
    row_1 = dm_simd_sub_float(l, r);
    
    l     = dm_simd_load_float(left[1]);
    r     = dm_simd_load_float(right[1]);
    row_2 = dm_simd_sub_float(l, r);
    
    l     = dm_simd_load_float(left[2]);
    r     = dm_simd_load_float(right[2]);
    row_3 = dm_simd_sub_float(l, r);
    
    l     = dm_simd_load_float(left[3]);
    r     = dm_simd_load_float(right[3]);
    row_4 = dm_simd_sub_float(l, r);
    
    dm_simd_store_float(out[0], row_1);
    dm_simd_store_float(out[1], row_2);
    dm_simd_store_float(out[2], row_3);
    dm_simd_store_float(out[3], row_4);
}

// https://github.com/recp/cglm/blob/cdd4d0e83e9ee79f73aeb0a4fb60b4abd8ecf947/include/cglm/mat4.h#L640
DM_INLINE
void dm_mat4_inverse(const dm_mat4 mat, dm_mat4 dest)
{
	float t[6];
    float det;
    float a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3],
    e = mat[1][0], f = mat[1][1], g = mat[1][2], h = mat[1][3],
    i = mat[2][0], j = mat[2][1], k = mat[2][2], l = mat[2][3],
    m = mat[3][0], n = mat[3][1], o = mat[3][2], p = mat[3][3];
    
    t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
    t[3] = i * p - m * l; t[4] = i * o - m * k; t[5] = i * n - m * j;
    
    dest[0][0] =  f * t[0] - g * t[1] + h * t[2];
    dest[1][0] =-(e * t[0] - g * t[3] + h * t[4]);
    dest[2][0] =  e * t[1] - f * t[3] + h * t[5];
    dest[3][0] =-(e * t[2] - f * t[4] + g * t[5]);
    
    dest[0][1] =-(b * t[0] - c * t[1] + d * t[2]);
    dest[1][1] =  a * t[0] - c * t[3] + d * t[4];
    dest[2][1] =-(a * t[1] - b * t[3] + d * t[5]);
    dest[3][1] =  a * t[2] - b * t[4] + c * t[5];
    
    t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
    t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;
    
    dest[0][2] =  b * t[0] - c * t[1] + d * t[2];
    dest[1][2] =-(a * t[0] - c * t[3] + d * t[4]);
    dest[2][2] =  a * t[1] - b * t[3] + d * t[5];
    dest[3][2] =-(a * t[2] - b * t[4] + c * t[5]);
    
    t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
    t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;
    
    dest[0][3] =-(b * t[0] - c * t[1] + d * t[2]);
    dest[1][3] =  a * t[0] - c * t[3] + d * t[4];
    dest[2][3] =-(a * t[1] - b * t[3] + d * t[5]);
    dest[3][3] =  a * t[2] - b * t[4] + c * t[5];
    
    det = 1.0f / (a * dest[0][0] + b * dest[1][0]
                  + c * dest[2][0] + d * dest[3][0]);
    
    dm_mat4_mul_scalar(dest, det, dest);
}

DM_INLINE
int dm_mat4_is_equal(const dm_mat4 left, const dm_mat4 right)
{
	for (int y = 0; y < 4; y++)
	{
        for(int x=0; x<4; x++)
        {
            if (left[y] != right[x]) return 0;
        }
	}
    
	return 1;
}

DM_INLINE
void dm_mat4_from_mat3(dm_mat3 mat, dm_mat4 out)
{
    out[0][0] = mat[0][0];
    out[0][1] = mat[0][1];
    out[0][2] = mat[0][2];
    out[0][3] = 0;

    out[1][0] = mat[1][0];
    out[1][1] = mat[1][1];
    out[1][2] = mat[1][2];
    out[1][3] = 0;

    out[2][0] = mat[2][0];
    out[2][1] = mat[2][1];
    out[2][2] = mat[2][2];
    out[2][3] = 0;

    out[3][0] = 0;
    out[3][1] = 0;
    out[3][2] = 0;
    out[3][3] = 0;
}

DM_INLINE
void dm_mat4_rotate_from_quat(const dm_quat quat, dm_mat4 out)
{
    dm_mat3 rot;
    dm_mat3_rotate_from_quat(quat, rot);
    
    dm_mat4_from_mat3(rot, out);
    out[3][3] = 1;
}

/*****************
MATRIX TRANSFORMS
*******************/
DM_INLINE
void dm_mat_scale_make(const dm_vec3 scale, dm_mat4 out)
{
	dm_mat4_identity(out);
    
    out[0][0] = scale[0];
	out[1][1] = scale[1];
	out[2][2] = scale[2];
}

DM_INLINE
void dm_mat_scale(const dm_mat4 mat, const dm_vec3 scale, dm_mat4 out)
{
    dm_vec4_scale(mat[0], scale[0], out[0]);
    dm_vec4_scale(mat[1], scale[1], out[1]);
    dm_vec4_scale(mat[2], scale[2], out[2]);

    out[3][0] = mat[3][0];
    out[3][1] = mat[3][1];
    out[3][2] = mat[3][2];
    out[3][3] = mat[3][3];
}

DM_INLINE
void dm_mat_rotation_make(float radians, const dm_vec3 axis, dm_mat4 out)
{
    dm_mat3 rotation;
    dm_mat3_rotation(radians, axis, rotation);
    
	dm_mat4_from_mat3(rotation, out);
	out[3][3] = 1;
}

DM_INLINE
void dm_mat_rotation_degrees_make(float degrees, const dm_vec3 axis, dm_mat4 out)
{
	dm_mat_rotation_make(DM_MATH_DEG_TO_RAD * degrees, axis, out);
}

DM_INLINE
void dm_mat_translate_make(const dm_vec3 translation, dm_mat4 out)
{
    dm_mat4_identity(out);
    
    out[3][0] = translation[0];
    out[3][1] = translation[1];
    out[3][2] = translation[2];
}

DM_INLINE
void dm_mat_translate(const dm_mat4 mat, const dm_vec3 translation, dm_mat4 out)
{
    out[0][0] = mat[0][0];
    out[0][1] = mat[0][1];
    out[0][2] = mat[0][2];
    out[0][3] = mat[0][3];

    out[1][0] = mat[1][0];
    out[1][1] = mat[1][1];
    out[1][2] = mat[1][2];
    out[1][3] = mat[1][3];

    out[2][0] = mat[2][0];
    out[2][1] = mat[2][1];
    out[2][2] = mat[2][2];
    out[2][3] = mat[2][3];

    out[3][0] += translation[0];
    out[3][1] += translation[1];
    out[3][2] += translation[2];
    out[3][3] = mat[3][3];
}

DM_INLINE
void dm_mat_rotate(const dm_mat4 mat, float radians, const dm_vec3 axis, dm_mat4 out)
{
    dm_mat4 rotation;
    
    dm_mat_rotation_make(radians, axis, rotation);
	dm_mat4_mul_mat4(mat, rotation, out);
}

DM_INLINE
void dm_mat_rotate_quat(const dm_mat4 mat, const dm_vec4 quat, dm_mat4 out)
{
    dm_mat4 rotation;
    dm_mat4_rotate_from_quat(quat, rotation);

    dm_mat4_mul_mat4(mat, rotation, out);
}

DM_INLINE
void dm_mat_rotate_degrees(const dm_mat4 mat, float degrees, const dm_vec3 axis, dm_mat4 out)
{
	dm_mat_rotate(mat, DM_MATH_DEG_TO_RAD * degrees, axis, out);
}

/***************
CAMERA MATRICES
*****************/
DM_INLINE
void dm_mat_view(const dm_vec3 view_origin, const dm_vec3 target, const dm_vec3 up, dm_mat4 out)
{
	dm_vec3 w;
    dm_vec3 u;
    dm_vec3 v;
    
    dm_vec3_sub_vec3(target, view_origin, w);
	dm_vec3_norm(w, w);
    
	dm_vec3_cross(w, up, u);
	dm_vec3_norm(u, u);
    
	dm_vec3_cross(u, w, v);
    
    out[0][0] =  u[0];
    out[0][1] =  v[0];
    out[0][2] = -w[0];
    
    out[1][0] =  u[1];
    out[1][1] =  v[1];
    out[1][2] = -w[1];
    
    out[2][0] =  u[2];
    out[2][1] =  v[2];
    out[2][2] = -w[2];
    
    out[0][3] =  0;
    out[1][3] =  0;
    out[2][3] =  0;
    
    out[3][0] = -dm_vec3_dot(view_origin, u);
    out[3][1] = -dm_vec3_dot(view_origin, v);
    out[3][2] =  dm_vec3_dot(view_origin, w);
    out[3][3] = 1;
}

DM_INLINE
void dm_mat_perspective(float fov, float aspect_ratio, float n, float f, dm_mat4 out)
{
	const float t  = 1.0f / dm_tan(fov * 0.5f);
	const float fn = 1.0f / (n - f);
	
    out[0][1] = 0;
    out[0][2] = 0;
    out[0][3] = 0;

    out[1][0] = 0;
    out[1][2] = 0;
    out[1][3] = 0;

    out[2][0] = 0;
    out[2][1] = 0;

    out[3][0] = 0;
    out[3][1] = 0;
    out[3][3] = 0;

    out[0][0]  = t / aspect_ratio;
	out[1][1]  = t;
#ifdef DM_METAL
    // metal has z clip space [0,1], so needs special treatment
	out[2][2] = f * fn;
    out[3][2] = n * f * fn;
#else
    out[2][2] = (f + n) * fn;
	out[3][2] = 2.0f * n * f * fn;
#endif
    out[2][3] = -1.0f;
}

DM_INLINE
void dm_mat_ortho(float left, float right, float bottom, float top, float n, float f, dm_mat4 out)
{
    out[0][1] = 0;
    out[0][2] = 0;
    out[0][3] = 0;

    out[1][0] = 0;
    out[1][2] = 0;
    out[1][3] = 0;

    out[2][0] = 0;
    out[2][1] = 0;
    out[2][3] = 0;

	out[0][0] = 2.0f / (right - left);
	out[1][1] = 2.0f / (top - bottom);
#ifdef DM_METAL
    out[2][2] = 1.0f / (f - n);
    
    out[3][0] = (right + left) / (left - right);
    out[3][1] = (top + bottom) / (bottom - top);
    out[3][2] = n / (n - f);
#else
	out[2][2] = 2.0f / (n - f);
    
    out[3][0] = -(right + left) / (right - left);
    out[3][1] = -(top + bottom) / (top - bottom);
    out[3][2] = 0;
#endif
    
	out[3][3] = 1.0f;
}

#endif //DM_MATH_H
