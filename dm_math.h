#ifndef DM_MATH_H
#define DM_MATH_H

/****
VEC2
******/
DM_INLINE
void dm_vec2_add_vec2(dm_vec2 left, dm_vec2 right, dm_vec2 out)
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
}

DM_INLINE
void dm_vec2_sub_vec2(dm_vec2 left, dm_vec2 right, dm_vec2 out)
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
}

DM_INLINE
void dm_vec2_add_scalar(dm_vec2 vec, float scalar, dm_vec2 out)
{
    out[0] = vec[0] + scalar;
    out[1] = vec[1] + scalar;;
}

DM_INLINE
void dm_vec2_sub_scalar(dm_vec2 vec, float scalar, dm_vec2 out)
{
    out[0] = vec[0] - scalar;
    out[1] = vec[1] - scalar;;
}

DM_INLINE
void dm_vec2_scale(dm_vec2 vec, float s, dm_vec2 out)
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
}

DM_INLINE
float dm_vec2_dot(dm_vec2 left, dm_vec2 right)
{
    return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]));
}

DM_INLINE
float dm_vec2_mag(dm_vec2 vec)
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[2]));
}

DM_INLINE
void dm_vec2_norm(dm_vec2 vec, dm_vec2 out)
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
#if 1
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
void dm_vec3_sign(const dm_vec3 vec, dm_vec3 out)
{
    out[0] = DM_SIGNF(vec[0]);
    out[1] = DM_SIGNF(vec[1]);
    out[2] = DM_SIGNF(vec[2]);
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
    float cos_theta, s;
    dm_vec3 neg_v, perp, para;
    
#if 0
    dm_vec3_negate(vec, neg_v);
    cos_theta = dm_vec3_dot(neg_v, n);
    cos_theta = DM_MIN(cos_theta, 1);
    
    dm_vec3_scale(n, cos_theta, perp);
    dm_vec3_add_vec3(perp, vec, perp);
    dm_vec3_scale(perp, e, perp);
    
    s = dm_vec3_mag2(perp);
    s = 1 - s;
    s = dm_fabs(s);
    s = -dm_sqrtf(s);
    dm_vec3_scale(n, s, para);
    
    dm_vec3_add_vec3(perp, para, out);
#else
    float vec_length = dm_vec3_mag(vec);
    float c = -dm_vec3_dot(vec, n) / vec_length;

    float aux = vec_length * (e * c - dm_sqrtf(1 - e * e * (1 - c * c)));
    dm_vec3 nv, nn;
    dm_vec3_scale(vec, e, nv);
    dm_vec3_scale(n, aux, nn);
    dm_vec3_add_vec3(nv, nn, out);
#endif
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
    out[0] = (left[3] * right[0]) + (left[0] * right[3]) + (left[1] * right[2]) - (left[2] * right[1]);
    out[1] = (left[3] * right[1]) - (left[0] * right[2]) + (left[1] * right[3]) + (left[2] * right[0]);
    out[2] = (left[3] * right[2]) + (left[0] * right[1]) - (left[1] * right[0]) + (left[2] * right[3]);
    out[3] = (left[3] * right[3]) - (left[0] * right[0]) - (left[1] * right[1]) - (left[2] * right[2]);
}

DM_INLINE
void dm_quat_cross(const dm_quat left, const dm_quat right, dm_quat out)
{
    dm_quat_mul_quat(left, right, out);
}

DM_INLINE
void dm_vec3_mul_quat(const dm_vec3 v, const dm_quat q, dm_quat out)
{
    dm_quat vq;
    dm_quat_from_vec3(v, vq);
    dm_quat_mul_quat(vq, q, out);
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
    dm_quat_from_axis_angle(axis, dm_deg_to_rad(angle), out);
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
    dm_memzero(mat, DM_MAT3_SIZE);
    
    mat[0][0] = 1;
    mat[1][1] = 1;
    mat[2][2] = 1;
}

DM_INLINE
void dm_mat3_transpose(const dm_mat3 mat, dm_mat3 out)
{
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
    
    dm_memcpy(out, d, sizeof(d));
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
    
    dm_memcpy(out, d, sizeof(d));
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
	dm_mat3_rotation(dm_deg_to_rad(degrees), axis, out);
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
    out[0][1] = 2 * (xy + zw);
    out[0][2] = 2 * (xz - yw);
    
    out[1][0] = 2 * (xy - zw);
    out[1][1] = 1 - 2 * (xx + zz);
    out[1][2] = 2 * (yz + xw);
    
    out[2][0] = 2 * (xz + yw);
    out[2][1] = 2 * (yz - xw);
    out[2][2] = 1 - 2 * (xx + yy);
}

/****
MAT4
*****/
DM_INLINE
void dm_mat4_identity(dm_mat4 mat)
{
    dm_memzero(mat, DM_MAT4_SIZE);
    
	mat[0][0] = 1;
    mat[1][1] = 1;
    mat[2][2] = 1;
    mat[3][3] = 1;
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
    dm_mat4 right_t;
    dm_mat4_transpose(right, right_t);
    
    dm_simd_float row_1, row_2, row_3, row_4;
    dm_simd_float left_r, right_r;
    
    left_r  = dm_simd_load_float(left[0]);
    right_r = dm_simd_load_float(right_t[0]);
    row_1   = dm_simd_mul_float(left_r, right_r);
    
    left_r  = dm_simd_load_float(left[1]);
    right_r = dm_simd_load_float(right_t[1]);
    row_2   = dm_simd_mul_float(left_r, right_r);
    
    left_r  = dm_simd_load_float(left[2]);
    right_r = dm_simd_load_float(right_t[2]);
    row_3   = dm_simd_mul_float(left_r, right_r);
    
    left_r  = dm_simd_load_float(left[3]);
    right_r = dm_simd_load_float(right_t[3]);
    row_4   = dm_simd_mul_float(left_r, right_r);
    
    dm_simd_store_float(out[0], row_1);
    dm_simd_store_float(out[1], row_2);
    dm_simd_store_float(out[2], row_3);
    dm_simd_store_float(out[3], row_4);
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
bool dm_mat4_is_equal(const dm_mat4 left, const dm_mat4 right)
{
	for (int y = 0; y < 4; y++)
	{
        for(int x=0; x<4; x++)
        {
            if (left[y] != right[x]) return false;
        }
	}
    
	return true;
}

DM_INLINE
void dm_mat4_from_mat3(dm_mat3 mat, dm_mat4 out)
{
    dm_memzero(out, DM_MAT4_SIZE);
    
    out[0][0] = mat[0][0];
    out[0][1] = mat[0][1];
    out[0][2] = mat[0][2];
    
    out[1][0] = mat[1][0];
    out[1][1] = mat[1][1];
    out[1][2] = mat[1][2];
    
    out[2][0] = mat[2][0];
    out[2][1] = mat[2][1];
    out[2][2] = mat[2][2];
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
    dm_vec3_scale(mat[0], scale[0], out[0]);
    dm_vec3_scale(mat[1], scale[1], out[1]);
    dm_vec3_scale(mat[2], scale[2], out[2]);
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
	dm_mat_rotation_make(dm_deg_to_rad(degrees), axis, out);
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
    out[3][0] = translation[0];
    out[3][1] = translation[1];
    out[3][2] = translation[2];
}

DM_INLINE
void dm_mat_rotate(const dm_mat4 mat, float radians, const dm_vec3 axis, dm_mat4 out)
{
    dm_mat4 rotation;
    
    dm_mat_rotation_make(radians, axis, rotation);
	dm_mat4_mul_mat4(mat, rotation, out);
}

DM_INLINE
void dm_mat_rotate_degrees(const dm_mat4 mat, float degrees, const dm_vec3 axis, dm_mat4 out)
{
	dm_mat_rotate(mat, dm_deg_to_rad(degrees), axis, out);
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
	float t  = 1.0f / dm_tan(fov * 0.5f);
	float fn = 1.0f / (n - f);
	
    dm_memzero(out, DM_MAT4_SIZE);
    
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
    dm_memzero(out, sizeof(float) * M4);
    
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
#endif
    
	out[3][3] = 1.0f;
}

#endif //DM_MATH_H
