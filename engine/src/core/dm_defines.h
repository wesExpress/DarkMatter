#ifndef __DM_DEFINES_H__
#define __DM_DEFINES_H__

#ifdef DM_OPENGL
#define DM_OPENGL_MAJOR 4
#define DM_OPENGL_MINOR 6
#endif

#ifdef __APPLE__
#ifdef DM_OPENGL
#define DM_PLATFORM_GLFW

// OpenGL is deprecated, so we just use Metal on Mac
#else
#define DM_PLATFORM_APPLE
#define DM_METAL
#endif
#define DM_INLINE

#elif __WIN32__ || _WIN32 || WIN32
#define DM_PLATFORM_WIN32
#ifndef DM_OPENGL
#define DM_DIRECTX
#endif
#define DM_INLINE __forceinline

#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_LINUX
#define DM_INLINE __always_inline

#else
#define DM_PLATFORM_GLFW
#define DM_INLINE
#endif

#ifdef DM_EXPORT
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