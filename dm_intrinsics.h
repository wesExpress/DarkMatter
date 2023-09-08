#ifndef DM_INTRINSICS_H
#define DM_INTRINSICS_H

/*******
casting
*********/
#ifndef DM_PLATFORM_APPLE
DM_INLINE
dm_mm_int dm_mm_cast_float_to_int(dm_mm_float mm)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_castps_si128(mm);
#else

#endif
}

DM_INLINE
dm_mm_float dm_mm_cast_int_to_float(dm_mm_int mm)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_castsi128_ps(mm);
#else
#endif
}
#endif
/*****
float
*******/
DM_INLINE
dm_mm_float dm_mm_load_ps(float* d)
{
#ifdef DM_PLATFORM_LINUX
    return _mm_loadu_ps(d);
#elif defined(DM_PLATFORM_WINDOWS)
    return _mm_load_ps(d);
#else
    return vld1q_f32(d);
#endif
}

DM_INLINE
dm_mm_float dm_mm_set1_ps(float d)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_set1_ps(d);
#else
    return vdupq_n_f32(d);
#endif
}

DM_INLINE
void dm_mm_store_ps(float* d, dm_mm_float mm)
{
#ifdef DM_PLATFORM_LINUX
    _mm_storeu_ps(d, mm);
#elif defined(DM_PLATFORM_WINDOWS)
    _mm_store_ps(d, mm);
#else
    vst1q_f32(d, mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_add_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_add_ps(left, right);
#else
    return vaddq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sub_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_sub_ps(left, right);
#else
    return vsubq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_mul_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_mul_ps(left, right);
#else
    return vmulq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_div_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_div_ps(left, right);
#else
    return vdivq_f32(left, right);
#endif
}

DM_INLINE
dm_mm_float dm_mm_sqrt_ps(dm_mm_float mm)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_sqrt_ps(mm);
#else
    return vsqrtq_f32(mm);
#endif
}

DM_INLINE
dm_mm_float dm_mm_fmadd_ps(dm_mm_float a, dm_mm_float b, dm_mm_float c)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_fmadd_ps(a, b, c);
#else
    return vfmaq_f32(a,b,c);
#endif
}

DM_INLINE
dm_mm_float dm_mm_max_ps(dm_mm_float a, dm_mm_float b)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_max_ps(a, b);
#else
    return vmaxq_f32(a, b);
#endif
}

DM_INLINE
dm_mm_float dm_mm_min_ps(dm_mm_float a, dm_mm_float b)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_min_ps(a, b);
#else
    return vminq_f32(a, b);
#endif
}

#ifndef DM_PLATFORM_APPLE
DM_INLINE
float dm_mm_extract_float(dm_mm_float mm)
{
    return _mm_cvtss_f32(mm);
}
#endif

#ifdef DM_PLATFORM_APPLE
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
#ifndef DM_PLATFORM_APPLE
    return _mm_cmp_ps(left, right, _CMP_GT_OQ);
#else
    return vcgtq_f32(left, right);
#endif
}

// geq
DM_INLINE
dm_mm_float dm_mm_geq_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_cmp_ps(left, right, _CMP_GE_OQ);
#else
    return vcgeq_f32(left, right);
#endif
}

// lt
DM_INLINE
dm_mm_float dm_mm_lt_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_cmp_ps(left, right, _CMP_LT_OQ);
#else
    return vcltq_f32(left, right);
#endif
}

// leq
DM_INLINE
dm_mm_float dm_mm_leq_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_cmp_ps(left, right, _CMP_LE_OQ);
#else
    return vcleq_f32(left, right);
#endif
}

/*******
BITWISE
*********/
DM_INLINE
dm_mm_float dm_mm_and_ps(dm_mm_float left, dm_mm_float right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_and_ps(left, right);
#else
    return vandq_s32(left, right);
#endif
}

/*
 int
*/
DM_INLINE
dm_mm_int dm_mm_load_i(int* d)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_load_si128((dm_mm_int*)d);
#else
    return vld1q_s32(d);
#endif
}

DM_INLINE
dm_mm_int dm_mm_set1_i(int d)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_set1_epi32(d);
#else
    return vdupq_n_s32(d);
#endif
}

DM_INLINE
void dm_mm_store_i(int* i, dm_mm_int mm)
{
#ifndef DM_PLATFORM_APPLE
    _mm_store_si128((dm_mm_int*)i, mm);
#else
     vst1q_s32(i, mm);
#endif
}

DM_INLINE
dm_mm_int dm_mm_add_i(dm_mm_int left, dm_mm_int right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_add_epi32(left, right);
#else
    return vaddq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_sub_i(dm_mm_int left, dm_mm_int right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_sub_epi32(left, right);
#else
    return vsubq_s32(left, right);
#endif
}

DM_INLINE
dm_mm_int dm_mm_mul_i(dm_mm_int left, dm_mm_int right)
{
#ifndef DM_PLATFORM_APPLE
    return _mm_mul_epi32(left, right);
#else
    return vmulq_s32(left, right);
#endif
}

#endif //DM_INTRINSICS_H
