#ifndef __DEFINES_H__
#define __DEFINES_H__

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

#ifdef DMEXPORT
#ifdef _MSC_VER
#define DM_API __declspec(dllexport)
#else
#define DM_API __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define DM_API __declspec(dllimport)
#else
#define DM_API
#endif
#endif

#endif