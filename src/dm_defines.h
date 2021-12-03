#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdbool.h>
#include <stdlib.h>

#ifdef __APPLE__
#define DM_PLATFORM_GLFW
#define DM_INLINE
#elif __WIN32__ || _WIN32 || WIN32
#define DM_PLATFORM_GLFW
#define DM_INLINE __forceinline
#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_GLFW
#define DM_INLINE __always_inline
#endif

#endif