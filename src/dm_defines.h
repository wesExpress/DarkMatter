#ifndef __DEFINES_H__
#define __DEFINES_H__

#ifdef __APPLE__
#ifdef DM_APPLE_OPENGL
#define DM_PLATFORM_GLFW
#define DM_OPENGL
#else
#define DM_PLATFORM_APPLE
#define DM_METAL
#endif
#define DM_INLINE

#elif __WIN32__ || _WIN32 || WIN32
#ifdef DM_WIN_OPENGL
#define DM_PLATFORM_GLFW
#define DM_OPENGL
#else
#define DM_PLATFORM_WIN32
#define DM_DIRECTX
#endif
#define DM_INLINE __forceinline

#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_GLFW
#define DM_OPENGL
#define DM_INLINE __always_inline

#else
#define DM_PLATFORM_UNSUPPORTED
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

#define STB_IMAGE_IMPLEMENTATION

#endif