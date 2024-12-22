#ifndef DM_DEFINES_H
#define DM_DEFINES_H

/********************************************
DETERMINE PLATFORM AND RENDERING BACKEND
**********************************************/
#ifdef __APPLE__
#define DM_PLATFORM_APPLE
#define DM_METAL
#define DM_INLINE __attribute__((always_inline)) inline

#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define DM_PLATFORM_WIN32
#define DM_INLINE __forceinline

#ifndef DM_VULKAN
#define DM_DIRECTX12
#endif

#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_LINUX
#define DM_INLINE __always_inline
#define DM_VULKAN
#define _GNU_SOURCE

#endif

/*********
ALIGNMENT
***********/
#ifdef __MSC_VER_
#define DM_ALIGN(X) __declspec(align(X))
#else
#define DM_ALIGN(X) __attribute((aligned(X)))
#endif

/*****
TYPES
*******/
#ifndef __cplusplus
#define false 0
#define true  1
#ifndef __bool_true_false_are_defined
typedef _Bool bool;
#endif
#endif

/****
MISC
******/
#define DM_ARRAY_LEN(ARRAY) sizeof(ARRAY) / sizeof(ARRAY[0])
#define DM_ALIGN_BYTES(SIZE, ALIGNMENT) ((SIZE + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

#endif
