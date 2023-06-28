#ifndef DM_MATH_H
#define DM_MATH_H

// various
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

/****
VEC2
******/
DM_INLINE
dm_vec2 dm_vec2_set(float x, float y)
{
	return (dm_vec2) { x, y };
}

DM_INLINE
void dm_vec2_set_inpl(float x, float y, dm_vec2* out)
{
	*out = dm_vec2_set(x, y);
}

DM_INLINE
dm_vec2 dm_vec2_set_from_vec3(dm_vec3 vec)
{
	return dm_vec2_set(vec.x, vec.y);
}

DM_INLINE
void dm_vec2_set_from_vec3_inpl(dm_vec3 vec, dm_vec2* out)
{
	*out = dm_vec2_set_from_vec3(vec);
}

DM_INLINE
dm_vec2 dm_vec2_set_from_vec4(dm_vec4 vec)
{
	return dm_vec2_set(vec.x, vec.y);
}

DM_INLINE
void dm_vec2_set_from_vec4_inpl(dm_vec4 vec, dm_vec2* out)
{
	*out = dm_vec2_set_from_vec4(vec);
}

DM_INLINE
dm_vec2 dm_vec2_add_scalar(dm_vec2 vec, float scalar)
{
    return dm_vec2_set(vec.x + scalar, vec.y + scalar);
}

DM_INLINE
void dm_vec2_add_scalar_inpl(dm_vec2 vec, float scalar, dm_vec2* out)
{
	*out = dm_vec2_add_scalar(vec, scalar);
}

DM_INLINE
dm_vec2 dm_vec2_sub_scalar(dm_vec2 vec, float scalar)
{
	return dm_vec2_add_scalar(vec, -scalar);
}

DM_INLINE
void dm_vec2_sub_scalar_inpl(float scalar, dm_vec2* out)
{
	*out = dm_vec2_sub_scalar(*out, scalar);
}

DM_INLINE
dm_vec2 dm_vec2_scale(dm_vec2 vec, float scale)
{
    return dm_vec2_set(vec.x * scale, vec.y * scale);
}

DM_INLINE
void dm_vec2_scale_inpl(float scale, dm_vec2* out)
{
	*out = dm_vec2_scale(*out, scale);
}

DM_INLINE
dm_vec2 dm_vec2_negate(dm_vec2 vec)
{
    return dm_vec2_scale(vec, -1);
}

DM_INLINE
void dm_vec2_negate_inpl(dm_vec2* vec)
{
    *vec = dm_vec2_negate(*vec);
}

DM_INLINE
float dm_vec2_dot(dm_vec2 left, dm_vec2 right)
{
	return (left.x * right.x + left.y * right.y);
}

DM_INLINE
float dm_vec2_len(dm_vec2 vec)
{
	return dm_sqrtf(dm_vec2_dot(vec, vec));;
}

DM_INLINE
dm_vec2 dm_vec2_add_vec2(dm_vec2 left, dm_vec2 right)
{
	return dm_vec2_set(left.x + right.x, left.y + right.y);
}

DM_INLINE
void dm_vec2_add_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out)
{
	*out = dm_vec2_add_vec2(left, right);
}

DM_INLINE
dm_vec2 dm_vec2_sub_vec2(dm_vec2 left, dm_vec2 right)
{
	return dm_vec2_set(left.x - right.x, left.y - right.y);
}

DM_INLINE
void dm_vec2_sub_vec2_inpl(dm_vec2 left, dm_vec2 right, dm_vec2* out)
{
	*out = dm_vec2_sub_vec2(left, right);
}

DM_INLINE
dm_vec2 dm_vec2_norm(dm_vec2 vec)
{
	float len = dm_vec2_len(vec);
	if (len == 0) return (dm_vec2) { 0, 0 };
	return dm_vec2_scale(vec, 1.0f / len);
}

DM_INLINE
void dm_vec2_norm_inpl(dm_vec2* out)
{
	float len = dm_vec2_len(*out);
	if (len != 0) dm_vec2_scale_inpl(1.0f / len, out);
}

DM_INLINE
float dm_vec2_angle(dm_vec2 left, dm_vec2 right)
{
	float llen = dm_vec2_len(left);
	float rlen = dm_vec2_len(right);
	
    if(llen > 0 && rlen > 0) return dm_acos(DM_CLAMP(dm_vec2_dot(left, right) / (llen * rlen),-1.0f, 1.0f));
    
	return 0.0f;
}

DM_INLINE
float dm_vec2_angle_degrees(dm_vec2 left, dm_vec2 right)
{
	return dm_deg_to_rad(dm_vec2_angle(left, right));
}

DM_INLINE
float dm_vec2_distance(dm_vec2 left, dm_vec2 right)
{
    return dm_vec2_len(dm_vec2_sub_vec2(left, right));
}

/****
VEC3
******/
DM_INLINE
dm_vec3 dm_vec3_set(float x, float y, float z)
{
	return (dm_vec3) { x, y, z };
}

DM_INLINE
void dm_vec3_set_inpl(float x, float y, float z, dm_vec3* out)
{
	*out = dm_vec3_set(x, y, z);
}

DM_INLINE
dm_vec3 dm_vec3_set_from_float(float x)
{
    return dm_vec3_set(x,x,x);
}

DM_INLINE
dm_vec3 dm_vec3_set_from_vec2(dm_vec2 vec)
{
	return dm_vec3_set(vec.x, vec.y, 0.0f);
}

DM_INLINE
void dm_vec3_set_from_vec3_inpl(dm_vec2 vec, dm_vec3* out)
{
	*out = dm_vec3_set_from_vec2(vec);
}

DM_INLINE
dm_vec3 dm_vec3_set_from_vec4(dm_vec4 vec)
{
	return dm_vec3_set(vec.x, vec.y, vec.z);
}

DM_INLINE
void dm_vec3_set_from_vec4_inpl(dm_vec4 vec, dm_vec3* out)
{
	*out = dm_vec3_set_from_vec4(vec);
}

DM_INLINE
dm_vec3 dm_vec3_add_scalar(dm_vec3 vec, float scalar)
{
	return dm_vec3_set(vec.x + scalar, vec.y + scalar, vec.z + scalar);
}

DM_INLINE
void dm_vec3_add_scalar_inpl(float scalar, dm_vec3* out)
{
	*out = dm_vec3_add_scalar(*out, scalar);
}

DM_INLINE
dm_vec3 dm_vec3_sub_scalar(dm_vec3 vec, float scalar)
{
	return dm_vec3_add_scalar(vec, -scalar);
}

DM_INLINE
void dm_vec3_sub_scalar_inpl(float scalar, dm_vec3* out)
{
	*out = dm_vec3_sub_scalar(*out, scalar);
}

DM_INLINE
dm_vec3 dm_vec3_scale(dm_vec3 vec, float scale)
{
	return dm_vec3_set(vec.x * scale, vec.y * scale, vec.z * scale);
}

DM_INLINE
void dm_vec3_scale_inpl(float scale, dm_vec3* out)
{
	*out = dm_vec3_scale(*out, scale);
}

DM_INLINE
dm_vec3 dm_vec3_negate(dm_vec3 vec)
{
    return dm_vec3_scale(vec, -1);
}

DM_INLINE
void dm_vec3_negate_inpl(dm_vec3* vec)
{
    *vec = dm_vec3_negate(*vec);
}

DM_INLINE
float dm_vec3_dot(dm_vec3 left, dm_vec3 right)
{
	return (left.x * right.x + left.y * right.y + left.z * right.z);
}

DM_INLINE
bool dm_vec3_same_direction(dm_vec3 left, dm_vec3 right)
{
    return dm_vec3_dot(left, right) > 0;
}

DM_INLINE
dm_vec3 dm_vec3_cross(dm_vec3 left, dm_vec3 right)
{
	return dm_vec3_set(left.y* right.z - right.y * left.z,
                       -(left.x * right.z - right.x * left.z),
                       left.x* right.y - right.x * left.y);
}

DM_INLINE
void dm_vec3_cross_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_cross(left, right);
}

DM_INLINE
float dm_vec3_len(dm_vec3 vec)
{
	return dm_sqrtf(dm_vec3_dot(vec, vec));
}

DM_INLINE
float dm_vec3_len2(dm_vec3 vec)
{
    return dm_vec3_dot(vec, vec);
}

DM_INLINE
dm_vec3 dm_vec3_add_vec3(dm_vec3 left, dm_vec3 right)
{
	return dm_vec3_set(left.x + right.x, left.y + right.y, left.z + right.z);
}

DM_INLINE
void dm_vec3_add_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_add_vec3(left, right);
}

DM_INLINE
dm_vec3 dm_vec3_sub_vec3(dm_vec3 left, dm_vec3 right)
{
	return dm_vec3_set(left.x - right.x, left.y - right.y, left.z - right.z);
}

DM_INLINE
void dm_vec3_sub_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
	*out = dm_vec3_sub_vec3(left, right);
    
}

DM_INLINE
dm_vec3 dm_vec3_mul_vec3(dm_vec3 left, dm_vec3 right)
{
    return dm_vec3_set(left.x * right.x, left.y * right.y, left.z * right.z);
}

DM_INLINE
void dm_vec3_mul_vec3_inpl(dm_vec3 left, dm_vec3 right, dm_vec3* out)
{
    *out = dm_vec3_mul_vec3(left, right);
}

DM_INLINE
dm_vec3 dm_vec3_norm(dm_vec3 vec)
{
	float len = dm_vec3_len(vec);
	if (len == 0)
	{
		return (dm_vec3) { 0, 0, 0 };
	}
	return dm_vec3_scale(vec, 1.0f / len);
}

DM_INLINE
void dm_vec3_norm_inpl(dm_vec3* out)
{
	float len = dm_vec3_len(*out);
	if (len != 0) dm_vec3_scale_inpl(1.0f / len, out);
}

DM_INLINE
float dm_vec3_angle(dm_vec3 left, dm_vec3 right)
{
	float llen = dm_vec3_len(left);
	float rlen = dm_vec3_len(right);
	return dm_acos(DM_CLAMP(dm_vec3_dot(left, right) / (llen * rlen),-1.0f, 1.0f));
}

DM_INLINE
float dm_vec3_angle_degrees(dm_vec3 left, dm_vec3 right)
{
	return dm_deg_to_rad(dm_vec3_angle(left, right));
}

DM_INLINE
dm_vec3 dm_vec3_rotate(dm_vec3 v, dm_quat q)
{
    /*
    dm_vec3 qi = dm_vec3_set(q.i, q.j, q.k);
    dm_vec3 qc = dm_vec3_cross(qi, v);
    qc = dm_vec3_add_vec3(qc, dm_vec3_scale(v, q.r));
    qc = dm_vec3_cross(qi, qc);
    
    return dm_vec3_add_vec3(v, dm_vec3_scale(qc, 2));
*/
    
    dm_vec3 qv = dm_vec3_set(q.i, q.j, q.k);
    dm_vec3 t = dm_vec3_scale(dm_vec3_cross(qv, v), 2.0f);
    dm_vec3 a = dm_vec3_add_vec3(dm_vec3_scale(t, q.r), dm_vec3_cross(qv, t));
    
    return dm_vec3_add_vec3(v,a);
}

DM_INLINE
void dm_vec3_rotate_inpl(dm_quat q, dm_vec3* out)
{
    *out = dm_vec3_rotate(*out, q);
}

DM_INLINE
dm_vec3 dm_vec3_abs(dm_vec3 vec)
{
    return dm_vec3_set(dm_fabs(vec.x), dm_fabs(vec.y), dm_fabs(vec.z));
}

DM_INLINE
bool dm_vec3_equals_vec3(dm_vec3 left, dm_vec3 right)
{
    return ( (left.x==right.x) && (left.y==right.y) && (left.z==right.z) );
}

DM_INLINE
bool dm_vec3_is_zero(dm_vec3 v)
{
    return (v.x==0 && v.y==0 && v.z==0);
}

DM_INLINE
bool dm_vec3_gt_vec3(dm_vec3 left, dm_vec3 right)
{
    return ( (left.x > right.x) && (left.y > right.y) && (left.z > right.z) );
}

DM_INLINE
bool dm_vec3_geq_vec3(dm_vec3 left, dm_vec3 right)
{
    return ( (left.x >= right.x) && (left.y >= right.y) && (left.z >= right.z) );
}

DM_INLINE
bool dm_vec3_lt_vec3(dm_vec3 left, dm_vec3 right)
{
    return ( (left.x < right.x) && (left.y < right.y) && (left.z < right.z) );
}

DM_INLINE
bool dm_vec3_leq_vec3(dm_vec3 left, dm_vec3 right)
{
    return ( (left.x <= right.x) && (left.y <= right.y) && (left.z <= right.z) );
}

DM_INLINE
float dm_vec3_sum_elements(dm_vec3 vec)
{
    return (vec.x + vec.y + vec.z);
}

/****
VEC4
******/
DM_INLINE
dm_vec4 dm_vec4_set(float x, float y, float z, float w)
{
	return (dm_vec4) { x, y, z, w };
}

DM_INLINE
void dm_vec4_set_inpl(float x, float y, float z, float w, dm_vec4* out)
{
	*out = dm_vec4_set(x, y, z, w);
}

DM_INLINE
dm_vec4 dm_vec4_set_from_float(float f)
{
    return dm_vec4_set(f, f, f, f);
}

DM_INLINE
void dm_vec4_set_from_float_inpl(float f, dm_vec4* out)
{
    *out = dm_vec4_set_from_float(f);
}

DM_INLINE
dm_vec4 dm_vec4_set_from_vec2(dm_vec2 vec)
{
    return dm_vec4_set(vec.x, vec.y, 0.0f, 0.0f);
}

DM_INLINE
void dm_vec4_set_from_vec2_inpl(dm_vec2 vec, dm_vec4* out)
{
	*out = dm_vec4_set_from_vec2(vec);
}

DM_INLINE
dm_vec4 dm_vec4_set_from_vec3(dm_vec3 vec)
{
    return dm_vec4_set(vec.x, vec.y, vec.z, 0.0f);
}

DM_INLINE
void dm_vec4_set_from_vec3_inpl(dm_vec3 vec, dm_vec4* out)
{
	*out = dm_vec4_set_from_vec3(vec);
}

DM_INLINE
dm_vec4 dm_vec4_add_scalar(dm_vec4 vec, float scalar)
{
	return dm_vec4_set(vec.x + scalar, vec.y + scalar, vec.z + scalar, vec.w + scalar);
}

DM_INLINE
void dm_vec4_add_scalar_inpl(float scalar, dm_vec4* out)
{
	*out = dm_vec4_add_scalar(*out, scalar);
}

DM_INLINE
dm_vec4 dm_vec4_sub_scalar(dm_vec4 vec, float scalar)
{
	return dm_vec4_add_scalar(vec, -scalar);
}

DM_INLINE
void dm_vec4_sub_scalar_inpl(float scalar, dm_vec4* out)
{
	*out = dm_vec4_sub_scalar(*out, scalar);
}

DM_INLINE
dm_vec4 dm_vec4_scale(dm_vec4 vec, float scale)
{
	return dm_vec4_set(vec.x * scale, vec.y * scale, vec.z * scale, vec.w * scale);
}

DM_INLINE
dm_vec4 dm_vec4_negate(dm_vec4 vec)
{
    return dm_vec4_scale(vec, -1);
}

DM_INLINE
void dm_vec4_negate_inpl(dm_vec4* vec)
{
    *vec = dm_vec4_negate(*vec);
}

DM_INLINE
void dm_vec4_scale_inpl(float scale, dm_vec4* out)
{
	*out = dm_vec4_scale(*out, scale);
}

DM_INLINE
float dm_vec4_dot(dm_vec4 left, dm_vec4 right)
{
	return (left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w);
}

DM_INLINE
float dm_vec4_len(dm_vec4 vec)
{
	float len = dm_sqrtf(dm_vec4_dot(vec, vec));
	return len;
}

DM_INLINE
dm_vec4 dm_vec4_add_vec4(dm_vec4 left, dm_vec4 right)
{
	return dm_vec4_set(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
}

DM_INLINE
void dm_vec4_add_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out)
{
	*out = dm_vec4_add_vec4(left, right);
}

DM_INLINE
dm_vec4 dm_vec4_sub_vec4(dm_vec4 left, dm_vec4 right)
{
	return dm_vec4_set(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
}

DM_INLINE
void dm_vec4_sub_vec4_inpl(dm_vec4 left, dm_vec4 right, dm_vec4* out)
{
	*out = dm_vec4_sub_vec4(left, right);
}

DM_INLINE
dm_vec4 dm_vec4_norm(dm_vec4 vec)
{
	float len = dm_vec4_len(vec);
	if (len == 0)
	{
		return (dm_vec4) { 0, 0, 0, 0 };
	}
	return dm_vec4_scale(vec, 1.0f / len);
}

DM_INLINE
void dm_vec4_norm_inpl(dm_vec4* out)
{
	float len = dm_vec4_len(*out);
	if (len != 0) dm_vec4_scale_inpl(1.0f / len, out);
}

DM_INLINE
float dm_vec4_angle(dm_vec4 left, dm_vec4 right)
{
	float llen = dm_vec4_len(left);
	float rlen = dm_vec4_len(right);
	return dm_acos(DM_CLAMP(dm_vec4_dot(left, right) / (llen * rlen), -1.0f, 1.0f));
}

DM_INLINE
float dm_vec4_angle_degrees(dm_vec4 left, dm_vec4 right)
{
	return dm_deg_to_rad(dm_vec4_angle(left, right));
}

/***********
QUATERNIONS
*************/
DM_INLINE
dm_quat dm_quat_set(float i, float j, float k, float r)
{
	return (dm_quat) { i, j, k, r };
}

DM_INLINE
void dm_quat_set_inpl(float r, float i, float j, float k, dm_quat* out)
{
	*out = dm_quat_set(i, j, k, r);
}

DM_INLINE
dm_quat dm_quat_set_real(float r)
{
	return dm_quat_set(0, 0, 0, r);
}

DM_INLINE
void dm_quat_set_real_inpl(float r, dm_quat* out)
{
	*out = dm_quat_set_real(r);
}

DM_INLINE
dm_quat dm_quat_set_pure(float i, float j, float k)
{
    return dm_quat_set(i, j, k, 0);
}

DM_INLINE 
void dm_quat_set_pure_inpl(float i, float j, float k, dm_quat* out)
{
    *out = dm_quat_set_pure(i, j, k);
}

DM_INLINE
dm_quat dm_quat_set_pure_v(dm_vec3 pure)
{
    return dm_quat_set_pure(pure.x, pure.y, pure.z);
}

DM_INLINE
void dm_quat_set_pure_v_inpl(dm_vec3 pure, dm_quat* out)
{
	*out = dm_quat_set_pure_v(pure);
}

DM_INLINE
dm_quat dm_quat_set_imaj(float i, float j, float k)
{
    return dm_quat_set(i, j, k, 1);
}

DM_INLINE
void dm_quat_set_imaj_inpl(float i, float j, float k, dm_quat* out)
{
	*out = dm_quat_set_imaj(i, j, k);
}

DM_INLINE
dm_quat dm_quat_set_from_vec3(dm_vec3 vec)
{
	return dm_quat_set(vec.x, vec.y, vec.z, 1);
}

DM_INLINE
void dm_quat_set_from_vec3_inpl(dm_vec3 vec, dm_quat* out)
{
	*out = dm_quat_set_from_vec3(vec);
}

DM_INLINE
dm_quat dm_quat_set_from_vec4(dm_vec4 vec)
{
	return dm_quat_set(vec.x, vec.y, vec.z, vec.w);
}

DM_INLINE
void dm_quat_set_from_vec4_inpl(dm_vec4 vec, dm_quat* out)
{
	*out = dm_quat_set_from_vec4(vec);
}

DM_INLINE
dm_quat dm_quat_add_quat(dm_quat left, dm_quat right)
{
	return dm_quat_set(left.i + right.i, left.j + right.j, left.k + right.k, left.r + right.r);
}

DM_INLINE
void dm_quat_add_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_add_quat(left, right);
}

DM_INLINE
dm_quat dm_quat_sub_quat(dm_quat left, dm_quat right)
{
	return dm_quat_set(left.i - right.i, left.j - right.j, left.k - right.k, left.r - right.r);
}

DM_INLINE
void dm_quat_sub_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_sub_quat(left, right);
}

DM_INLINE
dm_vec3 dm_vec3_from_quat(dm_quat quat)
{
	return dm_vec3_set(quat.i, quat.j, quat.k);
}

DM_INLINE
dm_quat dm_quat_cross(dm_quat left, dm_quat right)
{
    float qi = (left.r * right.i) + (left.i * right.r) + (left.j * right.k) - (left.k * right.j);
    float qj = (left.r * right.j) + (left.j * right.r) + (left.k * right.i) - (left.i * right.k);
    float qk = (left.r * right.k) + (left.k * right.r) + (left.i * right.j) - (left.j * right.i);
    
    float qr = (left.r * right.r) - (left.i * right.i) - (left.j * right.j) - (left.k * right.k);
    
    return dm_quat_set(qi,qj,qk,qr);
}

DM_INLINE
dm_quat dm_quat_mul_quat(dm_quat left, dm_quat right)
{
    float i = (left.r * right.i) + (left.i * right.r) + (left.j * right.k) - (left.k * right.j);
    float j = (left.r * right.j) - (left.i * right.k) + (left.j * right.r) + (left.k * right.i);
    float k = (left.r * right.k) + (left.i * right.j) - (left.j * right.i) + (left.k * right.r);
    float r = (left.r * right.r) - (left.i * right.i) - (left.j * right.j) - (left.k * right.k);
    
    return dm_quat_set(i,j,k,r);
}

DM_INLINE
void dm_quat_mul_quat_inpl(dm_quat left, dm_quat right, dm_quat* out)
{
	*out = dm_quat_mul_quat(left, right);
}

DM_INLINE
dm_quat dm_vec3_mul_quat(dm_vec3 v, dm_quat q)
{
    return dm_quat_mul_quat((dm_quat){ v.x, v.y, v.z, 0 }, q);
}

DM_INLINE
dm_vec4 dm_vec4_from_quat(dm_quat quat)
{
	return dm_vec4_set(quat.i, quat.j, quat.k, quat.r);
}

DM_INLINE
dm_quat dm_quat_scale(dm_quat quat, float scalar)
{
	return dm_quat_set(quat.i* scalar, quat.j* scalar, quat.k* scalar, quat.r* scalar);
}

DM_INLINE
void dm_quat_scale_inpl(dm_quat quat, float scalar, dm_quat* out)
{
	*out = dm_quat_scale(quat, scalar);
}

DM_INLINE
dm_quat dm_quat_conjugate(dm_quat quat)
{
	return dm_quat_set(-quat.i, -quat.j, -quat.k, quat.r);
}

DM_INLINE
float dm_quat_mag(dm_quat quat)
{
	return dm_sqrtf((quat.i * quat.i) + (quat.j * quat.j) + (quat.k * quat.k) + (quat.r * quat.r));
}

DM_INLINE
dm_quat dm_quat_norm(dm_quat quat)
{
	float mag = dm_quat_mag(quat);
    
	if (mag > 0) return dm_quat_scale(quat, 1.0f / mag);
	return quat;
}

DM_INLINE
void dm_quat_norm_inpl(dm_quat* out)
{
	*out = dm_quat_norm(*out);
}

DM_INLINE
dm_quat dm_quat_inverse(dm_quat quat)
{
	dm_quat conj = dm_quat_conjugate(quat);
    
	if (dm_quat_mag(conj) == 1) return conj;
    
	float mag = dm_quat_mag(quat);
	return dm_quat_scale(conj, mag * mag);
}

DM_INLINE
void dm_quat_inverse_inpl(dm_quat quat, dm_quat* out)
{
	*out = dm_quat_inverse(quat);
}

DM_INLINE
float dm_quat_dot(dm_quat left, dm_quat right)
{
	return (left.i * right.i) + (left.j * right.j) + (left.k * right.k) + (left.r + right.r);
}

DM_INLINE
float dm_quat_angle(dm_quat left, dm_quat right)
{
	float mag = dm_quat_mag(left) * dm_quat_mag(right);
    
	if (mag == 0) return 0;
    
	float angle = dm_quat_dot(left, right) / mag;
	return dm_acos(angle);
}

DM_INLINE
dm_quat dm_quat_from_axis_angle(dm_vec3 axis, float angle)
{
    float s = dm_sin(angle * 0.5f);
    float c = dm_cos(angle * 0.5f);
    return dm_quat_norm(dm_quat_set(axis.x * s, axis.y * s, axis.z * s, c));
}

DM_INLINE
dm_quat dm_quat_from_axis_angle_deg(dm_vec3 axis, float angle)
{
    return dm_quat_from_axis_angle(axis, dm_deg_to_rad(angle));
}

DM_INLINE
dm_quat dm_quat_from_vectors(dm_vec3 vec1, dm_vec3 vec2)
{
    dm_vec3 u = dm_vec3_norm(vec1);
    dm_vec3 w = dm_vec3_norm(vec2);
    
    float angle = dm_acos(dm_vec3_dot(u,w));
    dm_vec3 axis = dm_vec3_norm(dm_vec3_cross(u,w));
    
    return dm_quat_from_axis_angle(axis, angle);
}

DM_INLINE
dm_quat dm_quat_negate(dm_quat q)
{
    return dm_quat_scale(q, -1);
}

DM_INLINE
dm_quat dm_quat_nlerp(dm_quat quat_a, dm_quat quat_b, float t)
{
    dm_quat lerp = dm_quat_add_quat(quat_a, dm_quat_scale(dm_quat_sub_quat(quat_b, quat_a), t));
    return dm_quat_norm(lerp);
}

DM_INLINE
dm_quat dm_quat_from_to_direction(dm_vec3 from, dm_vec3 to)
{
    //from = dm_vec3_norm(from);
    //to   = dm_vec3_norm(to);
    
    float d = dm_vec3_dot(from, to);
    
    // TODO: this is super jank
    if(dm_fabs(d - 1.0f) <= 0.00001f) return dm_quat_set(0,0,0,1);
    
    dm_vec3 axis = dm_vec3_norm(dm_vec3_cross(from, to));
    float angle = dm_acos(d);
    
    return dm_quat_from_axis_angle(axis, angle);
}

/****
MAT2
******/
DM_INLINE
dm_mat2 dm_mat2_identity(void)
{
	dm_mat2 result = { 0 };
    
	int N = 2;
    
	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}
    
	return result;
}

DM_INLINE
void dm_mat2_identity_inpl(dm_mat2* out)
{
	*out = dm_mat2_identity();
}

DM_INLINE
dm_mat2 dm_mat2_transpose(dm_mat2 mat)
{
	dm_mat2 result = { 0 };
    
	int N = 2;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}
	return result;
}

DM_INLINE
void dm_mat2_inpl(dm_mat2 mat, dm_mat2* out)
{
	*out = dm_mat2_transpose(mat);
}

DM_INLINE
dm_mat2 dm_mat2_mul_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };
	right = dm_mat2_transpose(right);
    
	int N = 2;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec2_dot(left.rows[i], right.rows[j]);
		}
	}
    
	return result;
}

DM_INLINE
void dm_mat2_mul_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_mul_mat2(left, right);
}

DM_INLINE
dm_vec2 dm_mat2_mul_vec2(dm_mat2 mat, dm_vec2 vec)
{
	dm_vec2 result = { 0 };
    
	int N = 2;
	for (int i = 0; i < N; i++)
	{
		result.v[i] = dm_vec2_dot(mat.rows[i], vec);
	}
    
	return result;
}

DM_INLINE
void dm_mat2_mul_vec2_inpl(dm_mat2 mat, dm_vec2 vec, dm_vec2* out)
{
	*out = dm_mat2_mul_vec2(mat, vec);
}

DM_INLINE
dm_mat2 dm_mat2_mul_scalar(dm_mat2 mat, float scalar)
{
	dm_mat2 result = { 0 };
    
	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}
	return result;
}

DM_INLINE
void dm_mat2_mul_scalar_inpl(dm_mat2 mat, float scalar, dm_mat2* out)
{
	*out = dm_mat2_mul_scalar(mat, scalar);
}

DM_INLINE
dm_mat2 dm_mat2_add_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };
    
	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}
    
	return result;
}

DM_INLINE
void dm_mat2_add_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_add_mat2(left, right);
}

DM_INLINE
dm_mat2 dm_mat2_sub_mat2(dm_mat2 left, dm_mat2 right)
{
	dm_mat2 result = { 0 };
    
	int N = 2;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] - right.m[i];
	}
    
	return result;
}

DM_INLINE
void dm_mat2_sub_mat2_inpl(dm_mat2 left, dm_mat2 right, dm_mat2* out)
{
	*out = dm_mat2_sub_mat2(left, right);
}

DM_INLINE
dm_mat2 dm_mat2_inverse(dm_mat2 mat)
{
	dm_mat2 result = { 0 };
    
	float det = mat.m[0] * mat.m[3] - mat.m[1] * mat.m[2];
	if (det == 0)
	{
		//DM_LOG_WARN("Trying to invert non-invertible 2x2 matrix (determinant is zero)! Returning input matrix...");
		return mat;
	}
    
	det = 1.0f / det;
	int N = 2;
	for (int i = 0; i < N; i++)
	{
		result.m[i] = mat.m[N - i];
	}
    
	return dm_mat2_mul_scalar(mat, det);
}

DM_INLINE
void dm_mat2_inverse_inpl(dm_mat2 mat, dm_mat2* out)
{
	*out = dm_mat2_inverse(mat);
}

DM_INLINE
float dm_mat2_det(dm_mat2 mat)
{
	return (float)(mat.m[0] * mat.m[3] - mat.m[1] * mat.m[2]);
}

/****
MAT3
******/
DM_INLINE
dm_mat3 dm_mat3_identity(void)
{
	dm_mat3 result = { 0 };
    
	int N = 3;
    
	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}
	return result;
}

DM_INLINE
void dm_mat3_identity_inpl(dm_mat3* out)
{
	*out = dm_mat3_identity();
}

DM_INLINE
dm_mat3 dm_mat3_transpose(dm_mat3 mat)
{
	dm_mat3 result = { 0 };
    
	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}
    
	return result;
}

DM_INLINE
void dm_mat3_transpose_inpl(dm_mat3 mat, dm_mat3* out)
{
	*out = dm_mat3_transpose(mat);
}

DM_INLINE
dm_mat3 dm_mat3_mul_mat3(dm_mat3 left, dm_mat3 right)
{
	dm_mat3 result = { 0 };
	right = dm_mat3_transpose(right);
    
	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec3_dot(left.rows[i], right.rows[j]);
		}
	}
    
	return result;
}

DM_INLINE
void dm_mat3_mul_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_mul_mat3(left, right);
}

DM_INLINE
dm_vec3 dm_mat3_mul_vec3(dm_mat3 mat, dm_vec3 vec)
{
	dm_vec3 result = { 0 };
    
	int N = 3;
	for (int i = 0; i < N; i++)
	{
		result.v[i] = dm_vec3_dot(mat.rows[i], vec);
	}
    
	return result;
}

DM_INLINE
void dm_mat3_mul_vec3_inpl(dm_mat3 mat, dm_vec3 vec, dm_vec3* out)
{
	*out = dm_mat3_mul_vec3(mat, vec);
}

DM_INLINE
dm_mat3 dm_mat3_mul_scalar(dm_mat3 mat, float scalar)
{
	dm_mat3 result = { 0 };
    
	int N = 3;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}
    
	return result;
}

DM_INLINE
void dm_mat3_mul_scalar_inpl(dm_mat3 mat, float scalar, dm_mat3* out)
{
	*out = dm_mat3_mul_scalar(mat, scalar);
}

DM_INLINE
dm_mat3 dm_mat3_add_mat3(dm_mat3 left, dm_mat3 right)
{
	dm_mat3 result = { 0 };
    
	int N = 3;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}
    
	return result;
}

DM_INLINE
void dm_mat3_add_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_add_mat3(left, right);
}

DM_INLINE
dm_mat3 dm_mat3_sub_mat3(dm_mat3 left, dm_mat3 right)
{
	return dm_mat3_add_mat3(left, dm_mat3_mul_scalar(right, -1.0f));
}

DM_INLINE
void dm_mat3_sub_mat3_inpl(dm_mat3 left, dm_mat3 right, dm_mat3* out)
{
	*out = dm_mat3_sub_mat3(left, right);
}

DM_INLINE
dm_mat2 dm_mat3_minor(dm_mat3 mat, dm_vec2 cols, dm_vec2 rows)
{
	int N = 3;
	dm_mat2 minor = { 0 };
    
	for (int i = 0; i < 2; i++)
	{
		int col = (int)cols.v[i];
		for (int j = 0; j < 2; j++)
		{
			int row = (int)rows.v[j];
			minor.m[j * 2 + i] = mat.m[row * N + col];
		}
	}
    
	return minor;
}

DM_INLINE
float dm_mat3_det(dm_mat3 mat)
{
	float det1 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 1, 2 }));
	float det2 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 1, 2 }));
	float det3 = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 1, 2 }));
    
	return mat.m[0] * det1 - mat.m[1] * det2 + mat.m[2] * det3;
}

DM_INLINE
dm_mat3 dm_mat3_cofactors(dm_mat3 mat)
{
	dm_mat3 result = { 0 };
    
	result.m[0] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 1, 2 }));
	result.m[1] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 1, 2 }));
	result.m[2] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 1, 2 }));
    
	result.m[3] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 0, 2 }));
	result.m[4] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 0, 2 }));
	result.m[5] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 0, 2 }));
    
	result.m[6] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 1, 2 }, (dm_vec2) { 0, 1 }));
	result.m[7] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 2 }, (dm_vec2) { 0, 1 }));
	result.m[8] = dm_mat2_det(dm_mat3_minor(mat, (dm_vec2) { 0, 1 }, (dm_vec2) { 0, 1 }));
    
	return result;
}

DM_INLINE
dm_mat3 dm_mat3_inverse(dm_mat3 mat)
{
	dm_mat3 result = { 0 };
    
	float det = dm_mat3_det(mat);
	if (det == 0)
	{
		//DM_LOG_WARN("Trying to invert non-invertible 3x3 matrix! Returning input...");
		return mat;
	}
	det = 1.0f / det;
    
	// cofactors
	result = dm_mat3_cofactors(mat);
	int N = 3;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_powf(-1.0f, (float)(i + j)) * result.m[i * N + j];
		}
	}
    
	// adjugate
	result = dm_mat3_transpose(result);
    
	return dm_mat3_mul_scalar(result, det);
}

DM_INLINE
dm_mat3 dm_mat3_rotation(float radians, dm_vec3 axis)
{
	dm_mat3 result = { 0 };
    
	float C = dm_cos(radians);
	float S = dm_sin(radians);
	float t = 1 - C;
    
	result.m[0] = t * axis.x * axis.x + C;
	result.m[1] = t * axis.x * axis.y - S * axis.z;
	result.m[2] = t * axis.x * axis.z + S * axis.y;
    
	result.m[3] = t * axis.x * axis.y + S * axis.z;
	result.m[4] = t * axis.y * axis.y + C;
	result.m[5] = t * axis.y * axis.z - S * axis.x;
    
	result.m[6] = t * axis.x * axis.z - S * axis.y;
	result.m[7] = t * axis.y * axis.z + S * axis.x;
	result.m[8] = t * axis.z * axis.z + C;
    
	return result;
}

DM_INLINE
void dm_mat3_rotation_inpl(float radians, dm_vec3 axis, dm_mat3* out)
{
	*out = dm_mat3_rotation(radians, axis);
}

DM_INLINE
dm_mat3 dm_mat3_rotation_degrees(float degrees, dm_vec3 axis)
{
	return dm_mat3_rotation(dm_deg_to_rad(degrees), axis);
}

DM_INLINE
void dm_mat3_rotation_degrees_inpl(float degrees, dm_vec3 axis, dm_mat3* out)
{
	*out = dm_mat3_rotation_degrees(degrees, axis);
}

DM_INLINE
dm_mat3 dm_mat3_rotate_from_quat(dm_quat quat)
{
    float xx = quat.i * quat.i;
    float yy = quat.j * quat.j;
    float zz = quat.k * quat.k;
    
    float xy = quat.i * quat.j;
    float xz = quat.i * quat.k;
    float xw = quat.i * quat.r;
    float yz = quat.j * quat.k;
    float yw = quat.j * quat.r;
    float zw = quat.k * quat.r;
    
    dm_mat3 result = { 0 };
    
    result.mm[0][0] = 1 - 2 * (yy + zz);
    result.mm[1][0] = 2 * (xy - zw);
    result.mm[2][0] = 2 * (xz + yw);
    
    result.mm[0][1] = 2 * (xy + zw);
    result.mm[1][1] = 1 - 2 * (xx + zz);
    result.mm[2][1] = 2 * (yz - xw);
    
    result.mm[0][2] = 2 * (xz - yw);
    result.mm[1][2] = 2 * (yz + xw);
    result.mm[2][2] = 1 - 2 * (xx + yy);
    
    return result;
}

/****
MAT4
******/
DM_INLINE
dm_mat4 dm_mat4_identity(void)
{
	dm_mat4 result = { 0 };
    
	int N = 4;
    
	for (int i = 0; i < N; i++)
	{
		result.m[i * (N + 1)] = 1;
	}
	return result;
}

DM_INLINE
void dm_mat4_identity_inpl(dm_mat4* out)
{
	*out = dm_mat4_identity();
}

DM_INLINE
dm_mat4 dm_mat4_transpose(dm_mat4 mat)
{
	dm_mat4 result = { 0 };
    
	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = mat.m[j * N + i];
		}
	}
    
	return result;
}

DM_INLINE
void dm_mat4_transpose_inpl(dm_mat4 mat, dm_mat4* out)
{
	*out = dm_mat4_transpose(mat);
}

DM_INLINE
dm_mat4 dm_mat4_mul_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };
    
	right = dm_mat4_transpose(right);
    
	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_vec4_dot(left.rows[i], right.rows[j]);
		}
	}
    
	return result;
}

DM_INLINE
void dm_mat4_mul_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_mul_mat4(left, right);
}

DM_INLINE
dm_vec4 dm_mat4_mul_vec4(dm_mat4 mat, dm_vec4 vec)
{
	dm_vec4 result = { 0 };
    
#if 1
    result.x = mat.rows[0].x * vec.x + mat.rows[0].y * vec.y + mat.rows[0].z * vec.z + mat.rows[0].w * vec.w;
    result.y = mat.rows[1].x * vec.x + mat.rows[1].y * vec.y + mat.rows[1].z * vec.z + mat.rows[1].w * vec.w;
    result.z = mat.rows[2].x * vec.x + mat.rows[2].y * vec.y + mat.rows[2].z * vec.z + mat.rows[2].w * vec.w;
    result.w = mat.rows[3].x * vec.x + mat.rows[3].y * vec.y + mat.rows[3].z * vec.z + mat.rows[3].w * vec.w;
#else
    // THIS IS A FIRST ATTEMPT AT SIMD
    __m128 vin, vout, m0, m1, m2, m3, r0, r1, r2, r3;
    
    dm_mat4 matT = dm_mat4_transpose(mat);
    
    vin = _mm_load_ps(vec.v);
    m0 = _mm_load_ps(matT.mm[0]);
    m1 = _mm_load_ps(matT.mm[1]);
    m2 = _mm_load_ps(matT.mm[2]);
    m3 = _mm_load_ps(matT.mm[3]);
    
    r0 = _mm_shuffle_ps(vin, vin, _MM_SHUFFLE(0,0,0,0));
    r1 = _mm_shuffle_ps(vin, vin, _MM_SHUFFLE(1,1,1,1));
    r2 = _mm_shuffle_ps(vin, vin, _MM_SHUFFLE(2,2,2,2));
    r3 = _mm_shuffle_ps(vin, vin, _MM_SHUFFLE(3,3,3,3));
    
    vout = _mm_mul_ps(m3, r3);
    vout = _mm_fmadd_ps(m2, r2, vout);
    vout = _mm_fmadd_ps(m1, r1, vout);
    vout = _mm_fmadd_ps(m0, r0, vout);
    
    _mm_store_ps(result.v, vout);
#endif
    return result;
}

DM_INLINE
void dm_mat4_mul_vec4_inpl(dm_mat4 mat, dm_vec4 vec, dm_vec4* out)
{
	*out = dm_mat4_mul_vec4(mat, vec);
}

DM_INLINE
dm_vec4 dm_mat4_mul_vec3(dm_mat4 mat, dm_vec3 vec)
{
	dm_vec4 new_vec = dm_vec4_set(vec.x, vec.y, vec.z, 1.0f);
	return dm_mat4_mul_vec4(mat, new_vec);
}

DM_INLINE
void dm_mat4_mul_vec3_inpl(dm_mat4 mat, dm_vec3 vec, dm_vec4* out)
{
	*out = dm_mat4_mul_vec3(mat, vec);
}

DM_INLINE
dm_mat4 dm_mat4_mul_scalar(dm_mat4 mat, float scalar)
{
	dm_mat4 result = { 0 };
    
	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = mat.m[i] * scalar;
	}
    
	return result;
}

DM_INLINE
void dm_mat4_mul_scalar_inpl(dm_mat4 mat, float scalar, dm_mat4* out)
{
	*out = dm_mat4_mul_scalar(mat, scalar);
}

DM_INLINE
dm_mat4 dm_mat4_add_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };
    
	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] + right.m[i];
	}
    
	return result;
}

DM_INLINE
void dm_mat4_add_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_add_mat4(left, right);
}

DM_INLINE
dm_mat4 dm_mat4_sub_mat4(dm_mat4 left, dm_mat4 right)
{
	dm_mat4 result = { 0 };
    
	int N = 4;
	for (int i = 0; i < N * N; i++)
	{
		result.m[i] = left.m[i] - right.m[i];
	}
    
	return result;
}

DM_INLINE
void dm_mat4_sub_mat4_inpl(dm_mat4 left, dm_mat4 right, dm_mat4* out)
{
	*out = dm_mat4_sub_mat4(left, right);
}

DM_INLINE
dm_mat3 dm_mat4_minor(dm_mat4 mat, dm_vec3 cols, dm_vec3 rows)
{
	int N = 4;
	dm_mat3 minor = { 0 };
    
	for (int i = 0; i < 3; i++)
	{
		int col = (int)cols.v[i];
		for (int j = 0; j < 3; j++)
		{
			int row = (int)rows.v[j];
			minor.m[j * 3 + i] = mat.m[row * N + col];
		}
	}
    
	return minor;
}

DM_INLINE
float dm_mat4_det(dm_mat4 mat)
{
	float det1 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det2 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det3 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 1, 2, 3 }));
	float det4 = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 1, 2, 3 }));
    
	return mat.m[0] * det1 - mat.m[1] * det2 + mat.m[2] * det3 - mat.m[3] * det4;
}

DM_INLINE
dm_mat4 dm_mat4_cofactors(dm_mat4 mat)
{
	dm_mat4 result = { 0 };
    
	result.m[0] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[1] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[2] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 1, 2, 3 }));
	result.m[3] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 1, 2, 3 }));
    
	result.m[4] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[5] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[6] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 2, 3 }));
	result.m[7] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 2, 3 }));
    
	result.m[8] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[9] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[10] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 1, 3 }));
	result.m[11] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 1, 3 }));
    
	result.m[12] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 1, 2, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[13] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 2, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[14] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 3 }, (dm_vec3) { 0, 1, 2 }));
	result.m[15] = dm_mat3_det(dm_mat4_minor(mat, (dm_vec3) { 0, 1, 2 }, (dm_vec3) { 0, 1, 2 }));
    
	return result;
}

DM_INLINE
dm_mat4 dm_mat4_inverse(dm_mat4 mat)
{
	dm_mat4 result = { 0 };
    
	float det = dm_mat4_det(mat);
	if (det == 0)
	{
		//DM_LOG_WARN("Trying to invert non-invertible 4x4 matrix! Returning input...");
		return mat;
	}
	det = 1.0f / det;
    
	// cofactors
	result = dm_mat4_cofactors(mat);
	int N = 4;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			result.m[i * N + j] = dm_powf(-1.0f, (float)(i + j)) * result.m[i * N + j];
		}
	}
    
	// adjugate
	result = dm_mat4_transpose(result);
    
	return dm_mat4_mul_scalar(result, det);
}

#ifndef DM_PLATFORM_APPLE
/*
Apparently this comes from Intel, see: https://github.com/niswegmann/small-matrix-inverse/blob/master/invert4x4_sse.h
Sadly, link is dead to Intel PDF so have to trust it
*/
DM_INLINE
dm_mat4 dm_mat4_inverse_simd(const float* src)
{
    __m128 minor0, minor1, minor2, minor3;
    __m128 row0, row1, row2, row3;
    __m128 det, tmp1;
    
    dm_mat4 dst = { 0 };
    
    //tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
    //row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));
    
    tmp1 = _mm_load_ps(src);
    row1 = _mm_load_ps(src+8);
    
    row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
    row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
    
    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
    row3 = _mm_load_ps(src+10);
    //row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));
    
    row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
    row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
    
    tmp1 = _mm_mul_ps(row2, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    
    minor0 = _mm_mul_ps(row1, tmp1);
    minor1 = _mm_mul_ps(row0, tmp1);
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
    minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
    minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
    
    tmp1 = _mm_mul_ps(row1, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    
    minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
    minor3 = _mm_mul_ps(row0, tmp1);
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
    minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
    
    tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    row2 = _mm_shuffle_ps(row2, row2, 0x4E);
    
    minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
    minor2 = _mm_mul_ps(row0, tmp1);
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
    minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
    
    tmp1 = _mm_mul_ps(row0, row1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    
    minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
    
    tmp1 = _mm_mul_ps(row0, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
    minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
    
    tmp1 = _mm_mul_ps(row0, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    
    minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
    
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
    
    det = _mm_mul_ps(row0, minor0);
    det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
    det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
    
    tmp1 = _mm_rcp_ss(det);
    
    det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
    det = _mm_shuffle_ps(det, det, 0x00);
    
    minor0 = _mm_mul_ps(det, minor0);
    _mm_storel_pi((__m64*)(dst.m), minor0);
    _mm_storeh_pi((__m64*)(dst.m+2), minor0);
    
    minor1 = _mm_mul_ps(det, minor1);
    _mm_storel_pi((__m64*)(dst.m+4), minor1);
    _mm_storeh_pi((__m64*)(dst.m+6), minor1);
    
    minor2 = _mm_mul_ps(det, minor2);
    _mm_storel_pi((__m64*)(dst.m+ 8), minor2);
    _mm_storeh_pi((__m64*)(dst.m+10), minor2);
    
    minor3 = _mm_mul_ps(det, minor3);
    _mm_storel_pi((__m64*)(dst.m+12), minor3);
    _mm_storeh_pi((__m64*)(dst.m+14), minor3);
    
    return dst;
}
#endif

DM_INLINE
bool dm_mat4_is_equal(dm_mat4 left, dm_mat4 right)
{
	int N = 4;
    
	for (int i = 0; i < N * N; i++)
	{
		if (left.m[i] != right.m[i]) return false;
	}
    
	return true;
}

DM_INLINE
dm_mat4 dm_mat4_from_mat3(dm_mat3 mat)
{
	dm_mat4 result = { 0 };
    
	for (int i = 0; i < 3; i++)
	{
		result.rows[i] = dm_vec4_set_from_vec3(mat.rows[i]);
	}
    
	return result;
}

DM_INLINE
dm_mat4 dm_mat4_rotate_from_quat(dm_quat quat)
{
    dm_mat4 result = dm_mat4_from_mat3(dm_mat3_rotate_from_quat(quat));
    result.m[15] = 1;
    
    return result;
}

/*****************
MATRIX TRANSFORMS
*******************/
DM_INLINE
dm_mat4 dm_mat_scale_make(dm_vec3 scale)
{
	dm_mat4 result = dm_mat4_identity();
    
	result.m[0] = scale.x;
	result.m[5] = scale.y;
	result.m[10] = scale.z;
    
	return result;
}

DM_INLINE
dm_mat4 dm_mat_rotation_make(float radians, dm_vec3 axis)
{
	if (dm_vec3_len(axis) != 1)
	{
		axis = dm_vec3_norm(axis);
	}
    
	dm_mat4 result = { 0 };
	dm_mat3 rotation = dm_mat3_rotation(radians, axis);
    
	result = dm_mat4_from_mat3(rotation);
	result.m[15] = 1;
    
	return result;
}

DM_INLINE
dm_mat4 dm_mat_rotation_degrees_make(float degrees, dm_vec3 axis)
{
	return dm_mat_rotation_make(dm_deg_to_rad(degrees), axis);
}

DM_INLINE
dm_mat4 dm_mat_translate_make(dm_vec3 translation)
{
	dm_mat4 result = dm_mat4_identity();
    
	result.rows[3] = dm_vec4_set(translation.x, translation.y, translation.z, 1.0f);
    
	return result;
}

DM_INLINE
dm_mat4 dm_mat_translate(dm_mat4 mat, dm_vec3 translation)
{
	dm_mat4 result = mat;
    
	result.rows[3].x = translation.x;
	result.rows[3].y = translation.y;
	result.rows[3].z = translation.z;
    
	return result;
}

DM_INLINE
void dm_mat_translate_inpl(dm_mat4 mat, dm_vec3 translation, dm_mat4* out)
{
	*out = dm_mat_translate(mat, translation);
}

DM_INLINE
dm_mat4 dm_mat_scale(dm_mat4 mat, dm_vec3 scale)
{
	dm_mat4 result = mat;
    
	result.rows[0] = dm_vec4_scale(result.rows[0], scale.x);
	result.rows[1] = dm_vec4_scale(result.rows[1], scale.y);
	result.rows[2] = dm_vec4_scale(result.rows[2], scale.z);
    
	return result;
}

DM_INLINE
void dm_mat_scale_inpl(dm_mat4 mat, dm_vec3 scale, dm_mat4* out)
{
	*out = dm_mat_scale(mat, scale);
}

DM_INLINE
dm_mat4 dm_mat_rotate(dm_mat4 mat, float radians, dm_vec3 axis)
{
	dm_mat4 rotation = dm_mat_rotation_make(radians, axis);
    
	return dm_mat4_mul_mat4(mat, rotation);
}

DM_INLINE
void dm_mat_rotate_inpl(dm_mat4 mat, float radians, dm_vec3 axis, dm_mat4* out)
{
	*out = dm_mat_rotate(mat, radians, axis);
}

DM_INLINE
dm_mat4 dm_mat_rotate_degrees(dm_mat4 mat, float degrees, dm_vec3 axis)
{
	return dm_mat_rotate(mat, dm_deg_to_rad(degrees), axis);
}

DM_INLINE
void dm_mat_rotate_degrees_inpl(dm_mat4 mat, float degrees, dm_vec3 axis, dm_mat4* out)
{
	*out = dm_mat_rotate_degrees(mat, degrees, axis);
}

/***************
CAMERA MATRICES
*****************/
DM_INLINE
dm_mat4 dm_mat_view_v(dm_vec3 view_origin, dm_vec3 target, dm_vec3 up)
{
	dm_vec3 w = dm_vec3_sub_vec3(target, view_origin);
	w = dm_vec3_norm(w);
    
	dm_vec3 u = dm_vec3_cross(w, up);
	u = dm_vec3_norm(u);
    
	dm_vec3 v = dm_vec3_cross(u, w);
    
	dm_mat4 view = { 0 };
    
	view.rows[0] = (dm_vec4){ u.x, v.x, -w.x, 0 };
	view.rows[1] = (dm_vec4){ u.y, v.y, -w.y, 0 };
	view.rows[2] = (dm_vec4){ u.z, v.z, -w.z, 0 };
	view.rows[3] = (dm_vec4){
		-dm_vec3_dot(view_origin, u),
		-dm_vec3_dot(view_origin, v),
		dm_vec3_dot(view_origin, w),
		1
	};
    
	return view;
}

DM_INLINE
dm_mat4 dm_mat_view(float view_x, float view_y, float view_z, float target_x, float target_y, float target_z, float up_x, float up_y, float up_z)
{
    return dm_mat_view_v((dm_vec3){view_x, view_y, view_z}, (dm_vec3){target_x, target_y, target_z}, (dm_vec3){up_x, up_y, up_z});
}

DM_INLINE
dm_mat4 dm_mat_perspective(float fov, float aspect_ratio, float n, float f)
{
	dm_mat4 projection = { 0 };
    
	float t = 1.0f / dm_tan(fov * 0.5f);
	float fn = 1.0f / (n - f);
	projection.m[0] = t / aspect_ratio;
	projection.m[5] = t;
	projection.m[10] = (f + n) * fn;
	projection.m[11] = -1.0f;
	projection.m[14] = 2.0f * n * f * fn;
    
	return projection;
}

DM_INLINE
dm_mat4 dm_mat_perspective_deg(float fov, float aspect_ratio, float n, float f)
{
    return dm_mat_perspective(dm_deg_to_rad(fov), aspect_ratio, n, f);
}

DM_INLINE
dm_mat4 dm_mat_ortho(float left, float right, float bottom, float top, float n, float f)
{
	dm_mat4 ortho = { 0 };
    
	ortho.m[0] = 2.0f / (right - left);
	ortho.m[5] = 2.0f / (top - bottom);
	ortho.m[10] = 2.0f / (n - f);
	ortho.m[12] = -(right + left) / (right - left);
	ortho.m[13] = -(top + bottom) / (top - bottom);
	ortho.m[14] = -(f + n) / (f - n);
	ortho.m[15] = 1.0f;
    
	return ortho;
}

#endif //DM_MATH_H