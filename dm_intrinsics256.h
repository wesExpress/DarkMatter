#ifndef DM_INTRINSICS256_H
#define DM_INTRINSICS256_H

#ifndef DM_SIMD_ARM

DM_INLINE
dm_simd256_int dm_simd256_cast_float_to_int(dm_simd256_float mm)
{
    return _mm256_castps_si256(mm);
}

DM_INLINE
dm_simd256_float dm_simd256_cast_int_to_float(dm_simd256_int mm)
{
    return _mm256_castsi256_ps(mm);
}

DM_INLINE
dm_simd256_float dm_simd256_load_float(const float* d)
{
#ifdef DM_PLATFORM_LINUX
    return _mm256_loadu_ps(d);
#else
    return _mm256_load_ps(d);
#endif
}

DM_INLINE
dm_simd256_float dm_simd256_set1_float(const float d)
{
    return _mm256_set1_ps(d);
}

DM_INLINE
dm_simd256_float dm_simd256_set_float(float h, float g, float f, float e, float d, float c, float b, float a)
{
    return _mm256_set_ps(h,g,f,e,d,c,b,a);
}

DM_INLINE
void dm_simd256_store_float(float* d, dm_simd256_float mm)
{
#ifdef DM_PLATFORM_LINUX
    _mm256_storeu_ps(d, mm);
#else
    _mm256_store_ps(d, mm);
#endif
}

DM_INLINE
dm_simd256_float dm_simd256_add_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_add_ps(left, right);
}

DM_INLINE
dm_simd256_float dm_simd256_sub_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_sub_ps(left, right);
}

DM_INLINE
dm_simd256_float dm_simd256_mul_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_mul_ps(left, right);
}

DM_INLINE
dm_simd256_float dm_simd256_div_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_div_ps(left, right);
}

DM_INLINE
dm_simd256_float dm_simd256_sqrt_float(dm_simd256_float mm)
{
    return _mm256_sqrt_ps(mm);
}

DM_INLINE
dm_simd256_float dm_simd256_hadd_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_hadd_ps(left, right);
}

DM_INLINE
dm_simd256_float dm_simd256_fmadd_float(dm_simd256_float a, dm_simd256_float b, dm_simd256_float c)
{
    return _mm256_fmadd_ps(a, b, c);
}

DM_INLINE
dm_simd256_float dm_simd256_max_float(dm_simd256_float a, dm_simd256_float b)
{
    return _mm256_max_ps(a, b);
}

DM_INLINE
dm_simd256_float dm_simd256_min_float(dm_simd256_float a, dm_simd256_float b)
{
    return _mm256_min_ps(a, b);
}

DM_INLINE
float dm_simd256_extract_float(dm_simd256_float mm)
{
    return _mm256_cvtss_f32(mm);
}

// https://stackoverflow.com/questions/13219146/how-to-sum-m256-horizontally
DM_INLINE
float dm_simd256_sum_elements(dm_simd256_float mm)
{
    // hiQuad = ( x7, x6, x5, x4 )
    const __m128 hiQuad = _mm256_extractf128_ps(mm, 1);
    // loQuad = ( x3, x2, x1, x0 )
    const __m128 loQuad = _mm256_castps256_ps128(mm);
    // sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
    const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);
    // loDual = ( -, -, x1 + x5, x0 + x4 )
    const __m128 loDual = sumQuad;
    // hiDual = ( -, -, x3 + x7, x2 + x6 )
    const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
    // sumDual = ( -, -, x1 + x3 + x5 + x7, x0 + x2 + x4 + x6 )
    const __m128 sumDual = _mm_add_ps(loDual, hiDual);
    // lo = ( -, -, -, x0 + x2 + x4 + x6 )
    const __m128 lo = sumDual;
    // hi = ( -, -, -, x1 + x3 + x5 + x7 )
    const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
    // sum = ( -, -, -, x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 )
    const __m128 sum = _mm_add_ss(lo, hi);
    return _mm_cvtss_f32(sum);
}

DM_INLINE
float dm_simd256_mul_elements(dm_simd256_float mm)
{
    // hiQuad = ( x7, x6, x5, x4 )
    const __m128 hiQuad = _mm256_extractf128_ps(mm, 1);
    // loQuad = ( x3, x2, x1, x0 )
    const __m128 loQuad = _mm256_castps256_ps128(mm);
    // sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
    const __m128 sumQuad = _mm_mul_ps(loQuad, hiQuad);
    // loDual = ( -, -, x1 + x5, x0 + x4 )
    const __m128 loDual = sumQuad;
    // hiDual = ( -, -, x3 + x7, x2 + x6 )
    const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
    // sumDual = ( -, -, x1 + x3 + x5 + x7, x0 + x2 + x4 + x6 )
    const __m128 sumDual = _mm_mul_ps(loDual, hiDual);
    // lo = ( -, -, -, x0 + x2 + x4 + x6 )
    const __m128 lo = sumDual;
    // hi = ( -, -, -, x1 + x3 + x5 + x7 )
    const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
    // sum = ( -, -, -, x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 )
    const __m128 sum = _mm_mul_ss(lo, hi);
    return _mm_cvtss_f32(sum);
}

DM_INLINE
dm_simd256_float dm_simd256_gt_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_cmp_ps(left, right, _CMP_GT_OQ);
}

DM_INLINE
dm_simd256_float dm_simd256_geq_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_cmp_ps(left, right, _CMP_GE_OQ);
}

DM_INLINE
dm_simd256_float dm_simd256_lt_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_cmp_ps(left, right, _CMP_LT_OQ);
}

DM_INLINE
dm_simd256_float dm_simd256_leq_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_cmp_ps(left, right, _CMP_LE_OQ);
}

// any non-zero
// https://stackoverflow.com/questions/28923766/intel-simd-how-can-i-check-if-an-m256-contains-any-non-zero-values
DM_INLINE
int dm_simd256_any_non_zero(dm_simd256_float mm)
{
    dm_simd256_float vcmp = _mm256_cmp_ps(mm, _mm256_set1_ps(0), _CMP_EQ_OQ);
    int mask = _mm256_movemask_ps(vcmp);
    return (mask != 0xff);
}

DM_INLINE
int dm_simd256_any_zero(dm_simd256_float mm)
{
    dm_simd256_float vcmp = _mm256_cmp_ps(mm, _mm256_set1_ps(0), _CMP_EQ_OQ);
    int mask = _mm256_movemask_ps(vcmp);
    return (mask != 0);
}

DM_INLINE
dm_simd256_float dm_simd256_and_float(dm_simd256_float left, dm_simd256_float right)
{
    return _mm256_and_ps(left, right);
}

DM_INLINE
dm_simd256_int dm_simd256_load_i(int* d)
{
    return _mm256_load_si256((dm_simd256_int*)d);
}

DM_INLINE
dm_simd256_int dm_simd256_set1_i(int d)
{
    return _mm256_set1_epi32(d);
}

DM_INLINE
void dm_simd256_store_i(int* i, dm_simd256_int mm)
{
    _mm256_store_si256((dm_simd256_int*)i, mm);
}

DM_INLINE
dm_simd256_int dm_simd256_add_i(dm_simd256_int left, dm_simd256_int right)
{
    return _mm256_add_epi32(left, right);
}

DM_INLINE
dm_simd256_int dm_simd256_sub_i(dm_simd256_int left, dm_simd256_int right)
{
    return _mm256_sub_epi32(left, right);
}

DM_INLINE
dm_simd256_int dm_simd256_mul_i(dm_simd256_int left, dm_simd256_int right)
{
    return _mm256_mul_epi32(left, right);
}

DM_INLINE
dm_simd256_int dm_simd256_hadd_i(dm_simd256_int left, dm_simd256_int right)
{
    return _mm256_hadd_epi32(left, right);
}

DM_INLINE
dm_simd256_int dm_simd256_shiftl_1(dm_simd256_int mm)
{
    return _mm256_slli_si256(mm, sizeof(int));
}

DM_INLINE
dm_simd256_int dm_simd256_shiftr_1(dm_simd256_int mm)
{
    return _mm256_bsrli_epi128(mm, sizeof(int));
}

#endif

#endif