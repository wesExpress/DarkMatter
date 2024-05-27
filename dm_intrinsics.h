#ifndef DM_INTRINSICS_H
#define DM_INTRINSICS_H

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#define DM_SIMD_X86
#ifdef __AVX__
#define DM_SIMD_AVX
#endif
#elif defined(__aarch64__)
#define DM_SIMD_ARM
#endif

#ifdef DM_SIMD_X86
#include <immintrin.h>
//#include <emmintrin.h>
//#include <xmmintrin.h>
#else
#include <arm_neon.h>
#endif

#ifdef DM_SIMD_X86
typedef __m128  dm_simd_float;
typedef __m128i dm_simd_int;

#ifdef DM_SIMD_AVX
typedef __m256  dm_simd256_float;
typedef __m256i dm_simd256_int;
#define DM_SIMD256_FLOAT_N 8
#endif

#elif defined(DM_SIMD_ARM)
// neon does not support 256bit registers
typedef float32x4_t dm_simd_float;
typedef int32x4_t   dm_simd_int;
#endif

// this is mostly to make a compiler happy, but possibly lets it be orphaned? not sure
#ifndef DM_H

#ifdef __APPLE__
#define DM_INLINE __attribute__((always_inline)) inline
#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define DM_INLINE __forceinline
#elif __linux__ || __gnu_linux__
#define DM_INLINE __always_inline
#endif

/*********
ALIGNMENT
***********/
#ifdef __MSC_VER_
#define DM_ALIGN(X) __declspec(align(X))
#else
#define DM_ALIGN(X) __attribute((aligned(X)))
#endif

typedef float dm_vec2[2];
typedef float dm_vec3[3];
typedef DM_ALIGN(16) float dm_vec4[4];
typedef DM_ALIGN(16) float dm_quat[4];

typedef float dm_mat2[2][2];
typedef float dm_mat3[3][3];
typedef DM_ALIGN(16) float dm_mat4[4][4];

#define DM_SIMD_FLOAT_N    4

#endif // dm block

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
#elif defined(DM_SIMD_ARM)
    const float t[4] = { w,z,y,x };
    return vld1q_f32(t);
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
dm_simd_float dm_simd_inv_float(dm_simd_float v)
{
    return dm_simd_div_float(dm_simd_set1_float(1.f), v);
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
#elif defined(DM_SIMD_ARM)
    return vrsqrteq_f32(mm);
#endif
}

DM_INLINE
dm_simd_float dm_simd_hadd_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_hadd_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vpaddq_f32(left, right);
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
    dm_simd_float sums = dm_simd_hadd_fast_float(mm);
    
    return _mm_cvtss_f32(sums);
#endif
}

DM_INLINE
float dm_simd_dot_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    dm_simd_float dp = _mm_dp_ps(left,right, 0xFF);
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
    mag = dm_simd_sqrt_float(mag);
    
    return dm_simd_div_float(mm, mag);
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

DM_INLINE
dm_simd_float dm_simd_eq_float(dm_simd_float left, dm_simd_float right)
{
#ifdef DM_SIMD_X86
    return _mm_cmpeq_ps(left, right);
#elif defined(DM_SIMD_ARM)
    return vceqq_f32(left, right);
#endif
}

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
dm_simd_int dm_simd_load_i(const int* d)
{
#ifdef DM_SIMD_X86
    return _mm_load_si128((dm_simd_int*)d);
#elif defined(DM_SIMD_ARM)
    return vld1q_s32(d);
#endif
}

DM_INLINE
dm_simd_int dm_simd_set1_i(const int d)
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
#elif defined(DM_SIMD_ARM)
    return vminq_s32(left, right);
#endif
}

DM_INLINE
dm_simd_int dm_simd_max_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_max_epu32(left, right);
#elif defined(DM_SIMD_ARM)
    return vmaxq_s32(left, right);
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

DM_INLINE
dm_simd_int dm_simd_eq_i(dm_simd_int left, dm_simd_int right)
{
#ifdef DM_SIMD_X86
    return _mm_cmpeq_epi32(left, right);
#elif defined(DM_SIMD_ARM)
    return vceqq_s32(left, right);
#endif
}

#ifdef DM_SIMD_X86
DM_INLINE
dm_simd_int dm_simd_neq_i(dm_simd_int left, dm_simd_int right)
{
    return _mm_andnot_si128(_mm_cmpeq_epi32(left, right), _mm_set1_epi8(-1));
}
#endif

/*
MATRIX
*/
#ifdef DM_SIMD_X86
DM_INLINE
void dm_simd_transpose_mat4(dm_simd_float* row1, dm_simd_float* row2, dm_simd_float* row3, dm_simd_float* row4)
{
    _MM_TRANSPOSE4_PS(*row1, *row2, *row3, *row4);
}
#endif

#ifdef DM_SIMD_AVX 

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

#endif //DM_INTRINSICS_H
