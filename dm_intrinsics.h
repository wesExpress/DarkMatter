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
dm_mm_float dm_mm_load_ps(float* d)
{
#ifdef DM_SIMD_x86
    
#ifdef DM_PLATFORM_LINUX
    return _mm_loadu_ps(d);
#elif defined(DM_PLATFORM_WIN32)
    return _mm_load_ps(d);
#endif
    
#elif defined(DM_SIMD_ARM)
    return vld1q_f32(d);
#endif
}

DM_INLINE
dm_mm_float dm_mm_set1_ps(float d)
{
#ifdef DM_SIMD_x86
    return _mm_set1_ps(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_f32(d);
#endif
}

DM_INLINE
void dm_mm_store_ps(float* d, dm_mm_float mm)
{
#ifdef DM_SIMD_x86
    
#ifdef DM_PLATFORM_LINUX
    _mm_storeu_ps(d, mm);
#elif defined(DM_PLATFORM_WIN32)
    _mm_store_ps(d, mm);
#endif
    
#elif defined(DM_SIMD_ARM)
    vst1q_f32(d, mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_add_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_add_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sub_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_sub_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_mul_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_mul_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_div_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_div_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vdivq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sqrt_ps(dm_mm_float mm)
{
#ifdef DM_SIMD_x86
    return _mm_sqrt_ps(mm);
#elif defined(DM_SIMD_ARM)
    return vsqrtq_f32(mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_fmadd_ps(dm_mm_float a, dm_mm_float b, dm_mm_float c)
{
#ifdef DM_SIMD_x86
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
#ifdef DM_SIMD_x86
    return _mm_max_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vmaxq_f32(a, b);
#endif
}

DM_INLINE
dm_mm_float dm_mm_min_ps(dm_mm_float a, dm_mm_float b)
{
#ifdef DM_SIMD_x86
    return _mm_min_ps(a, b);
#elif defined(DM_SIMD_ARM)
    return vminq_f32(a, b);
#endif
}

#ifdef DM_SIMD_x86
DM_INLINE
float dm_mm_extract_float(dm_mm_float mm)
{
    return _mm_cvtss_f32(mm);
}
#endif

#ifdef DM_SIMD_ARM
DM_INLINE
float dm_mm_sum_elements(dm_mm_float mm)
{
    return vaddvq_f32(mm);
}
#endif

/************
COMPARISSONS
**************/
// gt
DM_INLINE
dm_mm_float dm_mm_gt_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_cmp_ps(left, right, _CMP_GT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgtq_f32(left, right);
#endif
}

// geq
DM_INLINE
dm_mm_float dm_mm_geq_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_cmp_ps(left, right, _CMP_GE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcgeq_f32(left, right);
#endif
}

// lt
DM_INLINE
dm_mm_float dm_mm_lt_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_cmp_ps(left, right, _CMP_LT_OQ);
#elif defined(DM_SIMD_ARM)
    return vcltq_f32(left, right);
#endif
}

// leq
DM_INLINE
dm_mm_float dm_mm_leq_ps(dm_mm_float left, dm_mm_float right)
{
#ifdef DM_SIMD_x86
    return _mm_cmp_ps(left, right, _CMP_LE_OQ);
#elif defined(DM_SIMD_ARM)
    return vcleq_f32(left, right);
#endif
}

DM_INLINE
int dm_mm_any_non_zero(dm_mm_float mm)
{
#ifdef DM_SIMD_x86
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
#ifdef DM_SIMD_x86
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
#ifdef DM_SIMD_x86
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
#ifdef DM_SIMD_x86
    return _mm_load_si128((dm_mm_int*)d);
#elif defined(DM_SIMD_ARM)
    return vld1q_s32(d);
#endif
}

DM_INLINE
dm_mm_int dm_mm_set1_i(int d)
{
#ifdef DM_SIMD_x86
    return _mm_set1_epi32(d);
#elif defined(DM_SIMD_ARM)
    return vdupq_n_s32(d);
#endif
}

DM_INLINE
void dm_mm_store_i(int* i, dm_mm_int mm)
{
#ifdef DM_SIMD_x86
    _mm_store_si128((dm_mm_int*)i, mm);
#elif defined(DM_SIMD_ARM)
    vst1q_s32(i, mm);
#endif
}

DM_INLINE
dm_mm_int dm_mm_add_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_x86
    return _mm_add_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vaddq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_sub_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_x86
    return _mm_sub_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vsubq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_mul_i(dm_mm_int left, dm_mm_int right)
{
#ifdef DM_SIMD_x86
    return _mm_mul_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vmulq_s32(left, right);
#endif
}

#endif //DM_INTRINSICS_H
