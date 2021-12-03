#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdbool.h>
#include <stdlib.h>

#ifdef __APPLE__
#define PLATFORM_GLFW
#define INLINE
#elif __WIN32__ || _WIN32 || WIN32
#define PLATFORM_GLFW
#define INLINE __forceinline
#elif __linux__ || __gnu_linux__
#define PLATFORM_GLFW
#define INLINE __always_inline
#endif

#endif