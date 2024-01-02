#ifndef DM_INTRINSICS_H
#define DM_INTRINSICS_H

/*******
casting
*********/
#ifndef DM_SIMD_ARM
DM_INLINE
dm_simd_int dm_simd_cast_float_to_int(dm_simd_float mm)
{
    return _mm_castps_si128(mm);
}

DM_INLINE
dm_simd_float dm_simd_cast_int_to_float(dm_simd_int mm)
{
    return _mm_castsi128_ps(mm);
}
#endif

/*****
float
*******/
DM_INLINE
dm_simd_float dm_simd_load_float(const float* d)
{
#ifdef DM_SIMD_X86
    return _mm_load_ps(d);
#elif defined(DM_SIMD_ARM)
    return vld1q_f32(d);
#endif
}

DM_INLINE
dm_simd_float dm_simd_set_float(const float x, const float y, const float z, const float w)
{
#ifdef DM_SIMD_X86
    return _mm_set_ps(x,y,z,w);
#endif
}

DM_INLINE
dm_simd_float dm_simd_set1_float(const float d)
{
#ifdef DM_SIMD_X86
    return _mm_set1_ps(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_f32(d);
#endif
}

DM_INLINE
void dm_simd_store_float(float* d, dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    _mm_store_ps(d, mm);
#elif defined(DM_SIMD_ARM)
    vst1q_f32(d, mm);
#endif
}

DM_INLINE
dm_simd_float dm_simd_add_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_add_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_f32(left, right);
#endif
}

DM_INLINE
dm_simd_float dm_simd_sub_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_sub_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_f32(left, right);
#endif
}

DM_INLINE
dm_simd_float dm_simd_mul_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_mul_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_f32(left, right);
#endif
}

DM_INLINE
dm_simd_float dm_simd_div_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_div_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vdivq_f32(left, right);
#endif
}

DM_INLINE
dm_simd_float dm_simd_sqrt_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_sqrt_ps(mm);
#elif defined(DM_SIMD_ARM)
    return vsqrtq_f32(mm);
#endif
}

DM_INLINE
dm_simd_float dm_simd_rsqrt_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_rsqrt_ps(mm);
#endif
}

DM_INLINE
dm_simd_float dm_simd_hadd_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_hadd_ps(left, right);
#endif
}

DM_INLINE
dm_simd_float dm_simd_fmadd_float(dm_simd_float a, dm_simd_float b, dm_simd_float c)
{
#ifdef DM_SIMD_X86
    return _mm_fmadd_ps(a, b, c);
#elif defined(DM_SIMD_ARM)
    // the order is backwards for some reason!
    return vfmaq_f32(c,a,b);
#endif
}

#ifdef DM_SIMD_ARM
DM_INLINE
dm_simd_float dm_simd_fsubps(dm_simd_float a, dm_simd_float b, dm_simd_float c)
{
    // the order is backwards for some reason!
    return vfmsq_f32(c,a,b);
}
#endif

DM_INLINE
dm_simd_float dm_simd_max_float(dm_simd_float a, dm_simd_float b)
{
#ifdef DM_SIMD_X86
    return _mm_max_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vmaxq_f32(a, b);
#endif
}

DM_INLINE
dm_simd_float dm_simd_min_float(dm_simd_float a, dm_simd_float b)
{
#ifdef DM_SIMD_X86
    return _mm_min_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vminq_f32(a, b);
#endif
}

DM_INLINE
float dm_simd_extract_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_cvtss_f32(mm);
#elif defined(DM_SIMD_ARM)
    return vgetq_lane_f32(mm, 0);
#endif
}

DM_INLINE
dm_simd_float dm_simd_hadd_fast_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    dm_simd_float shuf_reg, sums_reg;
    
    shuf_reg = _mm_movehdup_ps(mm);
    sums_reg = _mm_add_ps(mm, shuf_reg);
    shuf_reg = _mm_movehl_ps(shuf_reg, sums_reg);
    sums_reg = _mm_add_ss(sums_reg, shuf_reg);
    
    return dm_simd_set1_float(dm_simd_extract_float(sums_reg));
#elif defined(DM_SIMD_ARM)
    return dm_simd_set1_float(vaddvq_f32(mm));
#endif
}

DM_INLINE
dm_simd_float dm_simd_blendv_float(dm_simd_float left, dm_simd_float right, dm_simd_float mask)
{
#ifdef DM_SIMD_X86
    return _mm_blendv_ps(left, right, mask);
#elif defined(DM_SIMD_ARM)
    return vbslq_f32(left, right, mask);
#endif
}

// https://stackoverflow.com/a/35270026/195787
DM_INLINE
float dm_simd_sum_elements(dm_simd_float mm)
{
#ifdef DM_SIMD_ARM
    return vaddvq_f32(mm);
#elif defined(DM_SIMD_X86)
    dm_simd_float sums = dm_simd_hadd_fast_ps(mm);
    
    return _mm_cvtss_f32(sums);
#endif
}

DM_INLINE
float dm_simd_dot_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    dm_simd_float dp = _mm_dp_ps(left,right, 0x7F);
    return dm_simd_extract_float(dp);
#elif defined(DM_SIMD_ARM)
    dm_simd_float v = dm_simd_mul_float(left, right);
    return dm_simd_sum_elements(v);
#endif
}

DM_INLINE
dm_simd_float dm_simd_normalize_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    dm_simd_float dp = _mm_dp_ps(mm,mm, 0x7F);
    dm_simd_float mag = _mm_shuffle_ps(dp,dp, _MM_SHUFFLE(0,0,0,0));
    mag = dm_simd_sqrt_ps(mag);
    
    return dm_simd_div_ps(mm, mag);
#elif defined(DM_SIMD_ARM)
    dm_simd_float mag = dm_simd_set1_float(dm_simd_dot_float(mm, mm));
    mag = dm_simd_sqrt_float(mag);
    return dm_simd_div_float(mm, mag);
#endif
}

DM_INLINE
dm_simd_float dm_simd_broadcast_x_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(0,0,0,0));
#elif defined(DM_SIMD_ARM)
    return vdupq_lane_f32(vget_low_f32(mm), 0);
#endif
}

DM_INLINE
dm_simd_float dm_simd_broadcast_y_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(1,1,1,1));
#elif defined(DM_SIMD_ARM)
    return vdupq_lane_f32(vget_low_f32(mm), 1);
#endif
}

DM_INLINE
dm_simd_float dm_simd_broadcast_z_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(2,2,2,2));
#elif defined(DM_SIMD_ARM)
    return vdupq_lane_f32(vget_high_f32(mm), 0);
#endif
}

DM_INLINE
dm_simd_float dm_simd_broadcast_w_float(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    return _mm_shuffle_ps(mm,mm, _MM_SHUFFLE(3,3,3,3));
#elif defined(DM_SIMD_ARM)
    return vdupq_lane_f32(vget_high_f32(mm), 1);
#endif
}

/************
COMPARISSONS
**************/
// gt
DM_INLINE
dm_simd_float dm_simd_gt_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_GT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgtq_f32(left, right);
#endif
}

// geq
DM_INLINE
dm_simd_float dm_simd_geq_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_GE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgeq_f32(left, right);
#endif
}

// lt
DM_INLINE
dm_simd_float dm_simd_lt_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_LT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcltq_f32(left, right);
#endif
}

// leq
DM_INLINE
dm_simd_float dm_simd_leq_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmp_ps(left, right, _CMP_LE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcleq_f32(left, right);
#endif
}

#if 0
DM_INLINE
int dm_simd_any_non_zero(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    dm_simd_float vcmp = _mm_cmp_ps(mm, _mm_set1_ps(0), _CMP_EQ_OQ);
    int mask = _mm_movemask_ps(vcmp);
    return (mask != 0xff);
#elif defined(DM_SIMD_ARM)
    uint32x2_t tmp = vorr_u32(vget_low_u32(mm), vget_high_u32(mm));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0);
#endif
}
#endif

DM_INLINE
int dm_simd_any_zero(dm_simd_float mm)
{
#ifdef DM_SIMD_X86
    dm_simd_float vcmp = _mm_cmp_ps(mm, _mm_set1_ps(0), _CMP_EQ_OQ);
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
dm_simd_float dm_simd_and_float(dm_simd_float left, dm_simd_float right)
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
dm_simd_int dm_simd_load_i(int* d)
{
#ifdef DM_SIMD_X86
    return _mm_load_si128((dm_simd_int*)d);
#elif defined(DM_SIMD_ARM)
    return vld1q_s32(d);
#endif
}

DM_INLINE
dm_simd_int dm_simd_set1_i(int d)
{
#ifdef DM_SIMD_X86
    return _mm_set1_epi32(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_s32(d);
#endif
}

DM_INLINE
void dm_simd_store_i(int* i, dm_simd_int mm)
{
#ifdef DM_SIMD_X86
    _mm_store_si128((dm_simd_int*)i, mm);
#elif defined(DM_SIMD_ARM)
    vst1q_s32(i, mm);
#endif
}

DM_INLINE
dm_simd_int dm_simd_add_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_add_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_s32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_sub_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_sub_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_s32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_mul_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_mul_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_s32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_min_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_min_epu32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_max_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_max_epu32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_blendv_i(dm_simd_int left, dm_simd_int right, dm_simd_int mask)
{
#ifdef DM_SIMD_X86
    return _mm_blendv_epi8(left, right, mask);
#elif defined(DM_SIMD_ARM)
    return vbslq_u32(left, right, mask);
#endif
}

/*
MATRIX
*/
#ifdef DM_SIMD_X86
DM_INLINE
void dm_simd_transpose_mat4(dm_simd_float* row1, dm_simd_float* row2, dm_simd_float* row3, dm_simd_float* row4)
{
#ifdef DM_SIMD_X86
    _MM_TRANSPOSE4_PS(*row1, *row2, *row3, *row4);
#endif
}
#endif

#endif //DM_INTRINSICS_H
