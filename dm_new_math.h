#ifndef DM_NEW_MATH_H
#define DM_NEW_MATH_H

#define N2 2
#define N3 3
#define N4 4
#define M2 N2 * N2
#define M3 N3 * N3
#define M4 N4 * N4

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
void dm_vec2_add_vec2(float left[N2], float right[N2], float out[N2])
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
}

DM_INLINE
void dm_vec2_sub_vec2(float left[N2], float right[N2], float out[N2])
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
}

DM_INLINE
float dm_vec2_dot(float left[N2], float right[N2])
{
    return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]));
}

DM_INLINE
void dm_vec2_scale(float vec[N2], float s, float out[N2])
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
}

DM_INLINE
float dm_vec2_mag(float vec[N2])
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[2]));
}

DM_INLINE
void dm_vec2_norm(float vec[N2], float out[N2])
{
    dm_memcpy(out, vec, sizeof(float) * N2);
	
    float mag = dm_vec2_mag(out);
	
    // early out
    if (mag == 0) return; 
    
    float s = 1.0f / mag;
    dm_vec2_scale(out, s, out);
}

/****
VEC3
******/
DM_INLINE
void dm_vec3_add_scalar(float vec[N3], float scalar, float out[N3])
{
    out[0] = vec[0] + scalar;
    out[1] = vec[1] + scalar;
    out[2] = vec[2] + scalar;
}

DM_INLINE
void dm_vec3_sub_scalar(float vec[N3], float scalar, float out[N3])
{
    out[0] = vec[0] - scalar;
    out[1] = vec[1] - scalar;
    out[2] = vec[2] - scalar;
}

DM_INLINE
void dm_vec3_add_vec3(float left[N3], float right[N3], float out[N3])
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[2];
}

DM_INLINE
void dm_vec3_sub_vec3(float left[N3], float right[N3], float out[N3])
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
}

DM_INLINE
float dm_vec3_dot(float left[N3], float right[N3])
{
    return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]));
}

DM_INLINE
void dm_vec3_cross(float left[N3], float right[N3], float out[N3])
{
    out[0] = left[1] * right[2] - left[2] * right[1];
    out[1] = left[2] * right[0] - left[0] * right[2];
    out[2] = left[0] * right[1] - left[1] * right[0];
}

DM_INLINE
void dm_vec3_scale(float vec[N3], float s, float out[N3])
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
    out[2] = vec[2] * s;
}

DM_INLINE
float dm_vec3_mag(float vec[N3])
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[2]) + (vec[2] * vec[2]));
}

DM_INLINE
void dm_vec3_norm(float vec[N3], float out[N3])
{
    float mag = dm_vec3_mag(vec);
	
    // early out
    if (mag == 0) return; 
    
    float s = 1.0f / mag;
    dm_vec3_scale(out, s, out);
}

DM_INLINE
void dm_vec3_rotate(float vec[N3], float quat[N4], float out[N3])
{
    float qv[N3] = { quat[0],quat[1],quat[2] };
    float t[N3];
    float a[N3];
    
    dm_vec3_cross(qv, vec, t);
    dm_vec3_scale(t, 2, t);
    dm_vec3_cross(qv, t, a);
    dm_vec3_scale(t, quat[3], t);
    dm_vec3_add_vec3(vec,a,out);
}

DM_INLINE
void dm_vec3_negate(float vec[N3], float out[N3])
{
    out[0] = -vec[0];
    out[1] = -vec[1];
    out[2] = -vec[2];
}

DM_INLINE
bool dm_vec3_same_direction(float left[N3], float right[N3])
{
    return dm_vec3_dot(left,right) > 0;
}

/****
VEC4
******/
DM_INLINE
void dm_vec4_add_vec4(float left[N4], float right[N4], float out[N4])
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[2];
    out[3] = left[3] + right[3];
}

DM_INLINE
void dm_vec4_sub_vec4(float left[N4], float right[N4], float out[N4])
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
    out[3] = left[3] - right[3];
}

DM_INLINE
float dm_vec4_dot(float left[N4], float right[N4])
{
    return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]) + (left[3] * right[3]));
}

DM_INLINE
void dm_vec4_scale(float vec[N4], float s, float out[N4])
{
    out[0] = vec[0] * s;
    out[1] = vec[1] * s;
    out[2] = vec[2] * s;
    out[3] = vec[3] * s;
}

DM_INLINE
float dm_vec4_mag(float vec[N4])
{
    return dm_sqrtf((vec[0] * vec[0]) + (vec[1] * vec[2]) + (vec[2] * vec[2]) + (vec[3] * vec[3]));
}

DM_INLINE
void dm_vec4_norm(float vec[N4], float out[N4])
{
	dm_memcpy(out, vec, sizeof(float) * N4);
	
    float mag = dm_vec4_mag(out);
	
    // early out
    if (mag == 0) return; 
    
    float s = 1.0f / mag;
    dm_vec4_scale(out, s, out);
}

/**********
QUATERNION
************/
DM_INLINE
void dm_quat_add_quat(float left[N4], float right[N4], float out[N4])
{
    out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[3];
    out[3] = left[3] + right[3];
}

DM_INLINE
void dm_quat_sub_quat(float left[N4], float right[N4], float out[N4])
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[3];
    out[3] = left[3] - right[3];
}

DM_INLINE
void dm_quat_cross(float left[N4], float right[N4], float out[N4])
{
    out[0] = (left[3] * right[0]) + (left[0] * right[3]) + (left[1] * right[2]) - (left[2] * right[1]);
    out[1] = (left[3] * right[1]) + (left[1] * right[3]) + (left[2] * right[0]) - (left[0] * right[2]);
    out[2] = (left[3] * right[2]) + (left[2] * right[3]) + (left[0] * right[1]) - (left[1] * right[0]);
    
    out[3] = (left[3] * right[3]) - (left[0] * right[0]) - (left[1] * right[1]) - (left[2] * right[2]);
}

DM_INLINE
void dm_quat_mul_quat(float left[N4], float right[N4], float out[N4])
{
    out[0] = (left[3] * right[0]) + (left[0] * right[3]) + (left[1] * right[2]) - (left[2] * right[1]);
    out[1] = (left[3] * right[1]) - (left[0] * right[2]) + (left[1] * right[3]) + (left[2] * right[0]);
    out[2] = (left[3] * right[2]) + (left[0] * right[1]) - (left[1] * right[0]) + (left[2] * right[3]);
    out[3] = (left[3] * right[3]) - (left[0] * right[0]) - (left[1] * right[1]) - (left[2] * right[2]);
}

DM_INLINE
void dm_vec3_mul_quat(float v[N3], float q[N4], float out[N4])
{
    float vq[N4] = { v[0],v[1],v[2],0 };
    dm_quat_mul_quat(vq, q, out);
}

DM_INLINE
float dm_quat_mag(float quat[N4])
{
	return dm_sqrtf((quat[0] * quat[0]) + (quat[1] * quat[1]) + (quat[2] * quat[2]) + (quat[3] * quat[3]));
}

DM_INLINE
void dm_quat_scale(float quat[N4], float s, float out[N4])
{
    out[0] = quat[0] * s;
    out[1] = quat[1] * s;
    out[2] = quat[2] * s;
    out[3] = quat[3] * s;
}

DM_INLINE
void dm_quat_norm(float quat[N4], float out[N4])
{
	float mag = dm_quat_mag(quat);
    
	if (mag > 0) 
    {
        float s = 1.0f/mag;
        
        dm_quat_scale(quat, s, out);
    }
    else
    {
        dm_memcpy(out, quat, sizeof(float) * N4);
    }
}


DM_INLINE
void dm_quat_conjugate(float quat[N4], float out[N4])
{
    out[0] = -quat[0];
    out[1] = -quat[1];
    out[2] = -quat[2];
    out[3] =  quat[3];
}

DM_INLINE
void dm_quat_inverse(float quat[N4], float out[N4])
{
    dm_quat_conjugate(quat,out);
    
	if (dm_quat_mag(out) == 1) return;
    
	float mag = dm_quat_mag(quat);
    mag *= mag;
    
    dm_quat_scale(out, mag, out);
}

DM_INLINE
float dm_quat_dot(float left[N4], float right[N4])
{
	return (left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]) + (left[3] * right[3]);
}

DM_INLINE
float dm_quat_angle(float left[N4], float right[N4])
{
	float mag = dm_quat_mag(left) * dm_quat_mag(right);
    
	if (mag == 0) return 0;
    
	float angle = dm_quat_dot(left, right) / mag;
	return dm_acos(angle);
}

DM_INLINE
void dm_quat_from_axis_angle(float axis[N3], float angle, float out[N4])
{
    float s = dm_sin(angle * 0.5f);
    
    out[0] = axis[0] * s;
    out[1] = axis[1] * s; 
    out[2] = axis[2] * s;
    out[3] = dm_cos(angle * 0.5f);
    
    dm_quat_norm(out,out);
}

DM_INLINE
void dm_quat_from_axis_angle_deg(float axis[N3], float angle, float out[N4])
{
    dm_quat_from_axis_angle(axis, dm_deg_to_rad(angle), out);
}

DM_INLINE
void dm_quat_from_vectors(float vec1[N3], float vec2[N3], float out[N4])
{
    float u[N3];
    float w[N3]; 
    float axis[N3];
    
    dm_vec3_norm(vec1, u);
    dm_vec3_norm(vec2, w);
    
    float angle = dm_acos(dm_vec3_dot(u,w));
    dm_vec3_cross(u,w,axis);
    dm_vec3_norm(axis, axis);
    
    dm_quat_from_axis_angle(axis, angle, out);
}

DM_INLINE
void dm_quat_negate(float quat[N4], float out[N4])
{
    dm_quat_scale(quat, -1, out);
}

DM_INLINE
void dm_quat_nlerp(float quat_a[N4], float quat_b[N4], float t, float out[N4])
{
    float lerp[N4]; 
    float s[N4];
    
    dm_quat_sub_quat(quat_b, quat_a,s);
    dm_quat_scale(s, t, s);
    dm_quat_add_quat(quat_a, s, lerp);
    dm_quat_norm(lerp, lerp);
}


/****
MAT2
******/
DM_INLINE
void dm_mat2_identity(float mat[M2])
{
    mat[0] = 1;
    mat[3] = 1;
}

DM_INLINE
void dm_mat2_transpose(float mat[M2], float out[M2])
{
	for (int i = 0; i < N2; i++)
	{
		for (int j = 0; j < N2; j++)
		{
			out[i * N2 + j] = mat[j * N2 + i];
		}
	}
}

DM_INLINE
void dm_mat2_mul_mat2(float left[M2], float right[M2], float out[M2])
{
	float d[M2];
    
    d[0] = left[0] * right[0] + left[1] * right[2];
    d[1] = left[0] * right[1] + left[1] * right[3];
    
    d[2] = left[2] * right[0] + left[3] * right[2];
    d[3] = left[2] * right[1] + left[3] * right[3];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat2_mul_vec2(float mat[M2], float vec[N2], float out[N2])
{
	out[0] = mat[0] * vec[0] + mat[1] * vec[1];
    out[1] = mat[2] * vec[0] + mat[3] * vec[1];
}

DM_INLINE
void dm_mat2_mul_scalar(float mat[M2], float scalar, float out[M2])
{
	out[0] = mat[0] * scalar;
    out[1] = mat[1] * scalar;
    
    out[2] = mat[2] * scalar;
    out[3] = mat[3] * scalar;
}

DM_INLINE
void dm_mat2_add_mat2(float left[M2], float right[M2], float out[M2])
{
	out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    
    out[2] = left[2] + right[2];
    out[3] = left[3] + right[3];
}

DM_INLINE
void dm_mat2_sub_mat2(float left[M2], float right[M2], float out[M2])
{
	out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    
    out[2] = left[2] - right[2];
    out[3] = left[3] - right[3];
}

DM_INLINE
float dm_mat2_det(float mat[M2])
{
	return (mat[0] * mat[3] - mat[1] * mat[2]);
}

DM_INLINE
void dm_mat2_inverse(float mat[M2], float out[M2])
{
	float det = dm_mat2_det(mat);
	if (det == 0) dm_memcpy(out, mat, sizeof(float) * M2);
	
    
	det = 1.0f / det;
	for (int i = 0; i < N2; i++)
	{
		out[i] = mat[N2 - i];
	}
    
	dm_mat2_mul_scalar(out, det, out);
}

/****
MAT3
******/
DM_INLINE
void dm_mat3_identity(float mat[M3])
{
    dm_memzero(mat, sizeof(float) * M3);
    
    mat[N3 * 0] = 1;
    mat[N3 * 1] = 1;
    mat[N3 * 2] = 1;
}

DM_INLINE
void dm_mat3_transpose(float mat[M3], float out[M3])
{
	float d[M3];
    
    // diagonals the same
    d[0] = mat[0];
    d[4] = mat[4];
    d[8] = mat[8];
    
    // swap rest
    d[1] = mat[3];
    d[2] = mat[6];
    
    d[3] = mat[1];
    d[6] = mat[2];
    
    d[5] = mat[7];
    
    d[7] = mat[5];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat3_mul_mat3(float left[M3], float right[M3], float out[M3])
{
	float d[M3];
    
    d[0] = left[0] * right[0] + left[1] * right[3] + left[2] * right[6];
    d[1] = left[0] * right[1] + left[1] * right[4] + left[2] * right[7];
    d[2] = left[0] * right[2] + left[1] * right[5] + left[2] * right[8];
    
    d[3] = left[3] * right[0] + left[4] * right[3] + left[5] * right[6];
    d[4] = left[3] * right[1] + left[4] * right[4] + left[5] * right[7];
    d[5] = left[3] * right[2] + left[4] * right[5] + left[5] * right[8];
    
    d[6] = left[6] * right[0] + left[7] * right[3] + left[7] * right[6];
    d[7] = left[6] * right[1] + left[7] * right[4] + left[7] * right[7];
    d[8] = left[6] * right[2] + left[7] * right[5] + left[7] * right[8];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat3_mul_vec3(float mat[M3], float vec[N3], float out[N3])
{
    out[0] = mat[0] * vec[0] + mat[2] * vec[1] + mat[3] * vec[2];
    out[1] = mat[3] * vec[0] + mat[4] * vec[1] + mat[5] * vec[2];
    out[2] = mat[6] * vec[0] + mat[7] * vec[1] + mat[8] * vec[2];
}

DM_INLINE
void dm_mat3_mul_scalar(float mat[M3], float scalar, float out[M3])
{
	out[0]  = mat[0]  * scalar;
    out[1]  = mat[1]  * scalar;
    out[2]  = mat[2]  * scalar;
    
    out[3]  = mat[3]  * scalar;
    out[4]  = mat[4]  * scalar;
    out[5]  = mat[5]  * scalar;
    
    out[6]  = mat[6]  * scalar;
    out[7]  = mat[7]  * scalar;
    out[8]  = mat[8]  * scalar;
}

DM_INLINE
void dm_mat3_add_mat3(float left[M3], float right[M3], float out[M3])
{
	out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[2];
    
    out[3] = left[3] + right[3];
    out[4] = left[4] + right[4];
    out[5] = left[5] + right[5];
    
    out[6] = left[6] + right[6];
    out[7] = left[7] + right[7];
    out[8] = left[8] + right[8];
}

DM_INLINE
void dm_mat3_sub_mat3(float left[M3], float right[M3], float out[M3])
{
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
    
    out[3] = left[3] - right[3];
    out[4] = left[4] - right[4];
    out[5] = left[5] - right[5];
    
    out[6] = left[6] - right[6];
    out[7] = left[7] - right[7];
    out[8] = left[8] - right[8];
}

DM_INLINE
void dm_mat3_minor(float mat[M3], int cols[N2], int rows[N2], float out[M2])
{
	for (int i = 0; i < N2; i++)
	{
		for (int j = 0; j < N2; j++)
		{
			out[j * N2 + i] = mat[rows[j] * N3 + cols[i]];
		}
	}
}

DM_INLINE
float dm_mat3_det(float mat[M3])
{
	float m1[M2]; 
    float m2[M2]; 
    float m3[M3]; 
    
    dm_mat3_minor(mat, (int[]) { 1, 2 }, (int[]) { 1, 2 }, m1);
    dm_mat3_minor(mat, (int[]) { 0, 2 }, (int[]) { 1, 2 }, m2);
    dm_mat3_minor(mat, (int[]) { 0, 1 }, (int[]) { 1, 2 }, m3);
    
    float det1 = dm_mat2_det(m1);
	float det2 = dm_mat2_det(m2);
	float det3 = dm_mat2_det(m3);
    
	return mat[0] * det1 - mat[1] * det2 + mat[2] * det3;
}

DM_INLINE
void dm_mat3_cofactors(float mat[M3], float out[M3])
{
	float m1[M2];
    float m2[M2];
    float m3[M2];
    
    dm_mat3_minor(mat, (int[]) { 1, 2 }, (int[]) { 1, 2 }, m1);
    dm_mat3_minor(mat, (int[]) { 0, 2 }, (int[]) { 1, 2 }, m2);
    dm_mat3_minor(mat, (int[]) { 0, 1 }, (int[]) { 1, 2 }, m3);
    
    out[0] = dm_mat2_det(m1);
	out[1] = dm_mat2_det(m2);
	out[2] = dm_mat2_det(m3);
    
    dm_mat3_minor(mat, (int[]) { 1, 2 }, (int[]) { 0, 2 }, m1);
    dm_mat3_minor(mat, (int[]) { 0, 2 }, (int[]) { 0, 2 }, m2);
    dm_mat3_minor(mat, (int[]) { 0, 1 }, (int[]) { 0, 2 }, m3);
    
    out[3] = dm_mat2_det(m1);
	out[4] = dm_mat2_det(m2);
	out[5] = dm_mat2_det(m3);
    
    dm_mat3_minor(mat, (int[]) { 1, 2 }, (int[]) { 0, 1 }, m1);
    dm_mat3_minor(mat, (int[]) { 0, 2 }, (int[]) { 0, 1 }, m2);
    dm_mat3_minor(mat, (int[]) { 0, 1 }, (int[]) { 0, 1 }, m3);
    
	out[6] = dm_mat2_det(m1);
	out[7] = dm_mat2_det(m2);
	out[8] = dm_mat2_det(m3);
}

DM_INLINE
void dm_mat3_inverse(float mat[M3], float out[M3])
{
	float det = dm_mat3_det(mat);
	if (det == 0) dm_memcpy(out, mat, sizeof(float) * M3);
	
	det = 1.0f / det;
    
	// cofactors
	dm_mat3_cofactors(mat, out);
	for (int i = 0; i < N3; i++)
	{
		for (int j = 0; j < N3; j++)
		{
			out[i * N3 + j] = dm_powf(-1.0f, (float)(i + j)) * mat[i * N3 + j];
		}
	}
    
	// adjugate
	dm_mat3_transpose(out, out);
	dm_mat3_mul_scalar(out, det, out);
}

DM_INLINE
void dm_mat3_rotation(float radians, float axis[N3], float out[M3])
{
	float C = dm_cos(radians);
	float S = dm_sin(radians);
	float t = 1 - C;
    
	out[0] = t * axis[0] * axis[0] + C;
	out[1] = t * axis[0] * axis[1] - S * axis[2];
	out[2] = t * axis[0] * axis[2] + S * axis[1];
    
	out[3] = t * axis[0] * axis[1] + S * axis[2];
	out[4] = t * axis[0] * axis[1] + C;
	out[5] = t * axis[0] * axis[2] - S * axis[1];
    
	out[6] = t * axis[0] * axis[2] - S * axis[1];
	out[7] = t * axis[0] * axis[2] + S * axis[0];
	out[8] = t * axis[0] * axis[2] + C;
}

DM_INLINE
void dm_mat3_rotation_degrees(float degrees, float axis[3], float out[M3])
{
	dm_mat3_rotation(dm_deg_to_rad(degrees), axis, out);
}

DM_INLINE
void dm_mat3_rotate_from_quat(float quat[N4], float out[M3])
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
    
    out[0] = 1 - 2 * (yy + zz);
    out[3] = 2 * (xy - zw);
    out[6] = 2 * (xz + yw);
    
    out[1] = 2 * (xy + zw);
    out[4] = 1 - 2 * (xx + zz);
    out[7] = 2 * (yz - xw);
    
    out[2] = 2 * (xz - yw);
    out[5] = 2 * (yz + xw);
    out[8] = 1 - 2 * (xx + yy);
}

/****
MAT4
*****/
DM_INLINE
void dm_mat4_identity(float mat[M4])
{
    dm_memzero(mat, sizeof(float) * M4);
    
	mat[0]  = 1;
    mat[5]  = 1;
    mat[10] = 1;
    mat[15] = 1;
}

DM_INLINE
void dm_mat4_transpose(float mat[M4], float out[M4])
{
	float d[M4];
    
    // diagonals the same
    d[0]  = mat[0];
    d[5]  = mat[5];
    d[10] = mat[10];
    d[15] = mat[15];
    
    // swap rest
    d[1] = mat[4];
    d[2] = mat[8];
    d[3] = mat[12];
    
    d[4] = mat[1];
    d[8] = mat[2];
    d[12] = out[3];
    
    d[6] = mat[9];
    d[7] = mat[13];
    
    d[9] = mat[6];
    d[13] = mat[7];
    
    d[11] = mat[14];
    d[14] = mat[11];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat4_mul_mat4(float left[M4], float right[M4], float out[M4])
{
    float d[M4];
    
    d[0] = left[0] * right[0] + left[1] * right[4] + left[2] * right[8]  + left[3] * right[12];
    d[1] = left[0] * right[1] + left[1] * right[5] + left[2] * right[9]  + left[3] * right[13];
    d[2] = left[0] * right[2] + left[1] * right[6] + left[2] * right[10] + left[3] * right[14];
    d[3] = left[0] * right[3] + left[1] * right[7] + left[2] * right[11] + left[3] * right[15];
    
    d[4] = left[4] * right[0] + left[5] * right[4] + left[6] * right[8]  + left[7] * right[12];
    d[5] = left[4] * right[1] + left[5] * right[5] + left[6] * right[9]  + left[7] * right[13];
    d[6] = left[4] * right[2] + left[5] * right[6] + left[6] * right[10] + left[7] * right[14];
    d[7] = left[4] * right[3] + left[5] * right[7] + left[6] * right[11] + left[7] * right[15];
    
    d[8]  = left[8] * right[0] + left[9] * right[4] + left[10] * right[8]  + left[11] * right[12];
    d[9]  = left[8] * right[1] + left[9] * right[5] + left[10] * right[9]  + left[11] * right[13];
    d[10] = left[8] * right[2] + left[9] * right[6] + left[10] * right[10] + left[11] * right[14];
    d[11] = left[8] * right[3] + left[9] * right[7] + left[10] * right[11] + left[11] * right[15];
    
    d[12] = left[12] * right[0] + left[13] * right[4] + left[14] * right[8]  + left[15] * right[12];
    d[13] = left[12] * right[1] + left[13] * right[5] + left[14] * right[9]  + left[15] * right[13];
    d[14] = left[12] * right[1] + left[13] * right[6] + left[14] * right[10] + left[15] * right[14];
    d[15] = left[12] * right[3] + left[13] * right[7] + left[14] * right[11] + left[15] * right[15];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat4_mul_vec4(float mat[M4], float vec[N4], float out[M4])
{
    float d[M4];
    
    d[0] = mat[0]  * vec[0] + mat[1]  * vec[1] + mat[2]  * vec[2] + mat[3]  * vec[3];
    d[1] = mat[4]  * vec[0] + mat[5]  * vec[1] + mat[6]  * vec[2] + mat[7]  * vec[3];
    d[2] = mat[8]  * vec[0] + mat[9]  * vec[1] + mat[10] * vec[2] + mat[11] * vec[3];
    d[3] = mat[12] * vec[0] + mat[13] * vec[1] + mat[14] * vec[2] + mat[15] * vec[3];
    
    dm_memcpy(out, d, sizeof(d));
}

DM_INLINE
void dm_mat4_mul_vec3(float mat[M4], float vec[N3], float out[N4])
{
    float new_vec[N4] = { vec[0], vec[1], vec[2], 1 };
	dm_mat4_mul_vec4(mat, new_vec, out);
}

DM_INLINE
void dm_mat4_mul_scalar(float mat[M4], float scalar, float out[M4])
{
	out[0]  = mat[0]  * scalar;
    out[1]  = mat[1]  * scalar;
    out[2]  = mat[2]  * scalar;
    out[3]  = mat[3]  * scalar;
    
    out[4]  = mat[4]  * scalar;
    out[5]  = mat[5]  * scalar;
    out[6]  = mat[6]  * scalar;
    out[7]  = mat[7]  * scalar;
    
    out[8]  = mat[8]  * scalar;
    out[9]  = mat[9]  * scalar;
    out[10] = mat[10] * scalar;
    out[11] = mat[11] * scalar;
    
    out[12] = mat[12] * scalar;
    out[13] = mat[13] * scalar;
    out[14] = mat[14] * scalar;
    out[15] = mat[15] * scalar;
}

DM_INLINE
void dm_mat4_add_mat4(float left[M4], float right[M4], float out[M4])
{
	out[0] = left[0] + right[0];
    out[1] = left[1] + right[1];
    out[2] = left[2] + right[2];
    out[3] = left[3] + right[3];
    
    out[4] = left[4] + right[4];
    out[5] = left[5] + right[5];
    out[6] = left[6] + right[6];
    out[7] = left[7] + right[7];
    
    out[8]  = left[8]  + right[8];
    out[9]  = left[9]  + right[9];
    out[10] = left[10] + right[10];
    out[11] = left[11] + right[10];
    
    out[12] = left[12] + right[12];
    out[13] = left[13] + right[13];
    out[14] = left[14] + right[14];
    out[15] = left[15] + right[15];
}

DM_INLINE
void dm_mat4_sub_mat4(float  left[M4], float right[M4], float out[M4])
{
	out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
    out[3] = left[3] - right[3];
    
    out[4] = left[4] - right[4];
    out[5] = left[5] - right[5];
    out[6] = left[6] - right[6];
    out[7] = left[7] - right[7];
    
    out[8]  = left[8]  - right[8];
    out[9]  = left[9]  - right[9];
    out[10] = left[10] - right[10];
    out[11] = left[11] - right[10];
    
    out[12] = left[12] - right[12];
    out[13] = left[13] - right[13];
    out[14] = left[14] - right[14];
    out[15] = left[15] - right[15];
}

DM_INLINE
void dm_mat4_minor(float mat[M4], int cols[3], int rows[3], float out[M4])
{
	for (int i = 0; i < N3; i++)
	{
		for (int j = 0; j < N3; j++)
		{
			out[j * N3 + i] = mat[rows[j] * N3 + cols[i]];
		}
	}
}

DM_INLINE
float dm_mat4_det(float mat[M4])
{
    float m1[M3];
    float m2[M3];
    float m3[M3];
    float m4[M4];
    
    dm_mat4_minor(mat, (int[]) { 1, 2, 3 }, (int[]) { 1, 2, 3 }, m1);
    dm_mat4_minor(mat, (int[]) { 0, 2, 3 }, (int[]) { 1, 2, 3 }, m2);
    dm_mat4_minor(mat, (int[]) { 0, 1, 3 }, (int[]) { 1, 2, 3 }, m3);
    dm_mat4_minor(mat, (int[]) { 0, 1, 2 }, (int[]) { 1, 2, 3 }, m4);
    
	float det1 = dm_mat3_det(m1);
	float det2 = dm_mat3_det(m2);
	float det3 = dm_mat3_det(m3);
	float det4 = dm_mat3_det(m4);
    
	return mat[0] * det1 - mat[1] * det2 + mat[2] * det3 - mat[3] * det4;
}

DM_INLINE
void dm_mat4_cofactors(float mat[M4], float out[M4])
{
	float m1[M3];
    float m2[M3];
    float m3[M3];
    float m4[M4];
    
    dm_mat4_minor(mat, (int[]) { 1, 2, 3 }, (int[]) { 1, 2, 3 }, m1);
    dm_mat4_minor(mat, (int[]) { 0, 2, 3 }, (int[]) { 1, 2, 3 }, m2);
    dm_mat4_minor(mat, (int[]) { 0, 1, 3 }, (int[]) { 1, 2, 3 }, m3);
    dm_mat4_minor(mat, (int[]) { 0, 1, 2 }, (int[]) { 1, 2, 3 }, m4);
    
    out[0] = dm_mat3_det(m1);
	out[1] = dm_mat3_det(m2);
	out[2] = dm_mat3_det(m3);
	out[3] = dm_mat3_det(m4);
    
    dm_mat4_minor(mat, (int[]) { 1, 2, 3 }, (int[]) { 0, 2, 3 }, m1);
    dm_mat4_minor(mat, (int[]) { 0, 2, 3 }, (int[]) { 0, 2, 3 }, m2);
    dm_mat4_minor(mat, (int[]) { 0, 1, 3 }, (int[]) { 0, 2, 3 }, m3);
    dm_mat4_minor(mat, (int[]) { 0, 1, 2 }, (int[]) { 0, 2, 3 }, m4);
    
	out[4] = dm_mat3_det(m1);
	out[5] = dm_mat3_det(m2);
	out[6] = dm_mat3_det(m3);
	out[7] = dm_mat3_det(m4);
    
    dm_mat4_minor(mat, (int[]) { 1, 2, 3 }, (int[]) { 0, 1, 3 }, m1);
    dm_mat4_minor(mat, (int[]) { 0, 2, 3 }, (int[]) { 0, 1, 3 }, m2);
    dm_mat4_minor(mat, (int[]) { 0, 1, 3 }, (int[]) { 0, 1, 3 }, m3);
    dm_mat4_minor(mat, (int[]) { 0, 1, 2 }, (int[]) { 0, 1, 3 }, m3);
    
	out[8]  = dm_mat3_det(m1);
	out[9]  = dm_mat3_det(m2);
	out[10] = dm_mat3_det(m3);
	out[11] = dm_mat3_det(m4);
    
    dm_mat4_minor(mat, (int[]) { 1, 2, 3 }, (int[]) { 0, 1, 2 }, m1);
    dm_mat4_minor(mat, (int[]) { 0, 2, 3 }, (int[]) { 0, 1, 2 }, m2);
    dm_mat4_minor(mat, (int[]) { 0, 1, 3 }, (int[]) { 0, 1, 2 }, m3);
    dm_mat4_minor(mat, (int[]) { 0, 1, 2 }, (int[]) { 0, 1, 2 }, m4);
    
	out[12] = dm_mat3_det(m1);
	out[13] = dm_mat3_det(m2);
	out[14] = dm_mat3_det(m3);
	out[15] = dm_mat3_det(m4);
}

DM_INLINE
void dm_mat4_inverse(float mat[M4], float out[M4])
{
	float det = dm_mat4_det(mat);
	if (det == 0) dm_memcpy(out, mat, sizeof(float) * M4);
	det = 1.0f / det;
    
	// cofactors
	dm_mat4_cofactors(mat, out);
	for (int i = 0; i < N4; i++)
	{
		for (int j = 0; j < N4; j++)
		{
			out[i * N4 + j] = dm_powf(-1.0f, (float)(i + j)) * mat[i * N4 + j];
		}
	}
    
	// adjugate
	dm_mat4_transpose(out, out);
    dm_mat4_mul_scalar(out, det, out);
}

DM_INLINE
bool dm_mat4_is_equal(float left[M4], float right[M4])
{
	for (int i = 0; i < M4; i++)
	{
		if (left[i] != right[i]) return false;
	}
    
	return true;
}

DM_INLINE
void dm_mat4_from_mat3(float mat[M3], float out[M4])
{
    dm_memzero(out, sizeof(float) * M4);
    dm_memcpy(out, mat, sizeof(float) * M3);
    
    out[0] = mat[0];
    out[1] = mat[1];
    out[2] = mat[2];
    
    out[4] = mat[3];
    out[5] = mat[4];
    out[6] = mat[5];
    
    out[8] = mat[6];
    out[9] = mat[7];
    out[10] = mat[8];
}

DM_INLINE
void dm_mat4_rotate_from_quat(float quat[N4], float out[M4])
{
    float rot[M3];
    dm_mat3_rotate_from_quat(quat, rot);
    
    dm_mat4_from_mat3(rot, out);
    out[15] = 1;
}

/*****************
MATRIX TRANSFORMS
*******************/
DM_INLINE
void dm_mat_scale_make(float scale[N3], float out[M4])
{
	dm_mat4_identity(out);
    
    out[0]  = scale[0];
	out[5]  = scale[1];
	out[10] = scale[2];
}

DM_INLINE
void dm_mat_scale(float mat[M4], float scale[N3], float out[M4])
{
    out[0] = mat[0] * scale[0];
    out[1] = mat[1] * scale[0];
    out[2] = mat[2] * scale[0];
    out[3] = mat[3] * scale[0];
    
    out[4] = mat[4] * scale[1];
    out[5] = mat[5] * scale[1];
    out[6] = mat[6] * scale[1];
    out[7] = mat[7] * scale[1];
    
    out[8]  = mat[8]  * scale[2];
    out[9]  = mat[9]  * scale[2];
    out[10] = mat[10] * scale[2];
    out[11] = mat[11] * scale[2];
}

DM_INLINE
void dm_mat_rotation_make(float radians, float axis[N3], float out[M4])
{
	if (dm_vec3_mag(axis) != 1) dm_vec3_norm(axis, axis);
    
    float rotation[M3];
    dm_mat3_rotation(radians, axis, rotation);
    
	dm_mat4_from_mat3(rotation, out);
	out[15] = 1;
}

DM_INLINE
void dm_mat_rotation_degrees_make(float degrees, float axis[N3], float out[M4])
{
	dm_mat_rotation_make(dm_deg_to_rad(degrees), axis, out);
}

DM_INLINE
void dm_mat_translate_make(float translation[N3], float out[M4])
{
    dm_mat4_identity(out);
    
    out[12] = translation[0];
    out[13] = translation[1];
    out[14] = translation[2];
}

DM_INLINE
void dm_mat_translate(float mat[M4], float translation[N3], float out[M4])
{
    out[12] = translation[0];
    out[13] = translation[1];
    out[14] = translation[2];
}

DM_INLINE
void dm_mat_rotate(float mat[M4], float radians, float axis[N3], float out[M4])
{
    float rotation[M4];
    
    dm_mat_rotation_make(radians, axis, rotation);
	dm_mat4_mul_mat4(mat, rotation, out);
}

DM_INLINE
void dm_mat_rotate_degrees(float mat[M4], float degrees, float axis[N3], float out[M4])
{
	dm_mat_rotate(mat, dm_deg_to_rad(degrees), axis, out);
}

/***************
CAMERA MATRICES
*****************/
DM_INLINE
void dm_mat_view(float view_origin[N3], float target[N3], float up[N3], float out[M4])
{
	float w[N3];
    float u[N3];
    float v[N3];
    
    dm_vec3_sub_vec3(target, view_origin, w);
	dm_vec3_norm(w, w);
    
	dm_vec3_cross(w, up, u);
	dm_vec3_norm(u, u);
    
	dm_vec3_cross(u, w, v);
    
    out[0] =  u[0];
    out[1] =  v[0];
    out[2] = -w[0];
    out[3] =  0;
    
    out[4] =  u[1];
    out[5] =  v[1];
    out[6] = -w[1];
    out[7] =  0;
    
    out[8]  =  u[2];
    out[9]  =  v[2];
    out[10] = -w[2];
    out[11] =  0;
    
    out[12] = -dm_vec3_dot(view_origin, u);
    out[13] = -dm_vec3_dot(view_origin, v);
    out[14] =  dm_vec3_dot(view_origin, w);
    out[15] = 1;
}

DM_INLINE
void dm_mat_perspective(float fov, float aspect_ratio, float n, float f, float out[M4])
{
	float t = 1.0f / dm_tan(fov * 0.5f);
	float fn = 1.0f / (n - f);
	
    dm_memzero(out, sizeof(float) * M4);
    
    out[0] = t / aspect_ratio;
	out[5] = t;
	out[10] = (f + n) * fn;
	out[11] = -1.0f;
	out[14] = 2.0f * n * f * fn;
}

DM_INLINE
void dm_mat_ortho(float left, float right, float bottom, float top, float n, float f, float out[M4])
{
    dm_memzero(out, sizeof(float) * M4);
    
	out[0] = 2.0f / (right - left);
	out[5] = 2.0f / (top - bottom);
	out[10] = 2.0f / (n - f);
	out[12] = -(right + left) / (right - left);
	out[13] = -(top + bottom) / (top - bottom);
	out[14] = -(f + n) / (f - n);
	out[15] = 1.0f;
}

#endif //DM_NEW_MATH_H