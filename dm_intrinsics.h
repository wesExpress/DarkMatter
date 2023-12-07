#ifndef DM_INTRINSICS_H
#define DM_INTRINSICS_H

/*******
casting
*********/
#ifndef DM_SIMD_ARM
DM_INLINE
dm_mm_int dm_mm_cast_float_to_int(dm_mm_float mm)
{
    return _mm_castps_si128(mm);
}

DM_INLINE
dm_mm_float dm_mm_cast_int_to_float(dm_mm_int mm)
{
    return _mm_castsi128_ps(mm);
}
#endif

/*****
float
*******/
DM_INLINE
dm_mm_float dm_mm_load_ps(const float* d)
{
#ifdef DM_SIMD_X86
    return _mm_load_ps(d);
#elif defined(DM_SIMD_ARM)
    return vld1q_f32(d);
#endif
}

DM_INLINE
dm_mm_float dm_mm_set_ps(const float x, const float y, const float z, const float w)
{
#ifdef DM_SIMD_X86
    return _mm_set_ps(x,y,z,w);
#endif
}

DM_INLINE
dm_mm_float dm_mm_set1_ps(const float d)
{
#ifdef DM_SIMD_X86
    return _mm_set1_ps(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_f32(d);
#endif
}

DM_INLINE
void dm_mm_store_ps(float* d, dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    _mm_store_ps(d, mm);
#elif defined(DM_SIMD_ARM)
    vst1q_f32(d, mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_add_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_add_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sub_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_sub_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_mul_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_mul_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_div_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_div_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vdivq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sqrt_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_sqrt_ps(mm);
#elif defined(DM_SIMD_ARM)
    return vsqrtq_f32(mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_rsqrt_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_rsqrt_ps(mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_hadd_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_hadd_ps(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_fmadd_ps(dm_mm_float a, dm_mm_float b, dm_mm_float c)
{
#ifdef DM_SIMD_X86
    return _mm_fmadd_ps(a, b, c);
#elif defined(DM_SIMD_ARM)
    return vfmaq_f32(c,a,b);
#endif
}

#ifdef DM_SIMD_ARM
DM_INLINE
dm_mm_float dm_mm_fsubps(dm_mm_float a, dm_mm_float b, dm_mm_float c)
{
    return vfmsq_f32(c,a,b);
}
#endif

DM_INLINE
dm_mm_float dm_mm_max_ps(dm_mm_float a, dm_mm_float b)
{
#ifdef DM_SIMD_X86
    return _mm_max_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vmaxq_f32(a, b);
#endif
}

DM_INLINE
dm_mm_float dm_mm_min_ps(dm_mm_float a, dm_mm_float b)
{
#ifdef DM_SIMD_X86
    return _mm_min_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vminq_f32(a, b);
#endif
}

#ifdef DM_SIMD_X86
DM_INLINE
float dm_mm_extract_float(dm_mm_float mm)
{
    return _mm_cvtss_f32(mm);
}
#endif

DM_INLINE
dm_mm_float dm_mm_hadd_fast_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    dm_mm_float shuf_reg, sums_reg;
    
    shuf_reg = _mm_movehdup_ps(mm);
    sums_reg = _mm_add_ps(mm, shuf_reg);
    shuf_reg = _mm_movehl_ps(shuf_reg, sums_reg);
    sums_reg = _mm_add_ss(sums_reg, shuf_reg);
    
    return dm_mm_set1_ps(dm_mm_extract_float(sums_reg));
#endif
}

// https://stackoverflow.com/a/35270026/195787
DM_INLINE
float dm_mm_sum_elements(dm_mm_float mm)
{
#ifdef DM_SIMD_ARM
    return vaddvq_f32(mm);
#elif defined(DM_SIMD_X86)
    dm_mm_float sums = dm_mm_hadd_fast_ps(mm);
    
    return _mm_cvtss_f32(sums);
}
#endif

DM_INLINE
float dm_mm_dot_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    dm_mm_float dp = _mm_dp_ps(left,right, 0x7F);
    return dm_mm_extract_float(dp);
#endif
}

DM_INLINE
dm_mm_float dm_mm_normalize_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    dm_mm_float dp = _mm_dp_ps(mm,mm, 0x7F);
    dm_mm_float mag = _mm_shuffle_ps(dp,dp, _MM_SHUFFLE(0,0,0,0));
    mag = dm_mm_sqrt_ps(mag);
    
    return dm_mm_div_ps(mm, mag);
#endif
}

DM_INLINE
dm_mm_float dm_mm_broadcast_x_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(0,0,0,0));
#endif
}

DM_INLINE
dm_mm_float dm_mm_broadcast_y_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(1,1,1,1));
#endif
}

DM_INLINE
dm_mm_float dm_mm_broadcast_z_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(2,2,2,2));
#endif
}

DM_INLINE
dm_mm_float dm_mm_broadcast_w_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(3,3,3,3));
#endif
}

/************
COMPARISSONS
**************/
// gt
DM_INLINE
dm_mm_float dm_mm_gt_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_GT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgtq_f32(left, right);
#endif
}

// geq
DM_INLINE
dm_mm_float dm_mm_geq_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_GE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgeq_f32(left, right);
#endif
}

// lt
DM_INLINE
dm_mm_float dm_mm_lt_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_LT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcltq_f32(left, right);
#endif
}

// leq
DM_INLINE
dm_mm_float dm_mm_leq_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_LE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcleq_f32(left, right);
#endif
}

DM_INLINE
int dm_mm_any_non_zero(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    dm_mm_float vcmp = _mm_cmp_ps(mm, _mm_set1_ps(0), _CMP_EQ_OQ);
    int mask = _mm_movemask_ps(vcmp);
    return (mask != 0xff);
#elif defined(DM_SIMD_ARM)
    uint32x2_t tmp = vorr_u32(vget_low_u32(mm), vget_high_u32(mm));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0);
#endif
}

DM_INLINE
int dm_mm_any_zero(dm_mm_float mm)
{
#ifdef DM_SIMD_X86
    dm_mm_float vcmp = _mm_cmp_ps(mm, _mm_set1_ps(0), _CMP_EQ_OQ);
    int mask = _mm_movemask_ps(vcmp);
    return (mask == 0xff);
#elif defined(DM_SIMD_ARM)
    return vminvq_f32(mm)==0;
#endif
}

/*******
BITWISE
*********/
DM_INLINE
dm_mm_float dm_mm_and_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_X86
    return _mm_and_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vandq_s32(left, right);
#endif
}

/*
 int
*/
DM_INLINE
dm_mm_int dm_mm_load_i(int* d)
{
#ifdef DM_SIMD_X86
    return _mm_load_si128((dm_mm_int*)d);
#elif defined(DM_SIMD_ARM)
    return vld1q_s32(d);
#endif
}

DM_INLINE
dm_mm_int dm_mm_set1_i(int d)
{
#ifdef DM_SIMD_X86
    return _mm_set1_epi32(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_s32(d);
#endif
}

DM_INLINE
void dm_mm_store_i(int* i, dm_mm_int mm)
{
#ifdef DM_SIMD_X86
    _mm_store_si128((dm_mm_int*)i, mm);
#elif defined(DM_SIMD_ARM)
    vst1q_s32(i, mm);
#endif
}

DM_INLINE
dm_mm_int dm_mm_add_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_X86
    return _mm_add_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_sub_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_X86
    return _mm_sub_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_mul_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_X86
    return _mm_mul_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_s32(left, right);
#endif
}

/*
MATRIX
*/
DM_INLINE
void dm_mm_transpose_mat4(dm_mm_float* row1, dm_mm_float* row2, dm_mm_float* row3, dm_mm_float* row4)
{
#ifdef DM_SIMD_X86
    _MM_TRANSPOSE4_PS(*row1, *row2, *row3, *row4);
#endif
}

#endif //DM_INTRINSICS_H
