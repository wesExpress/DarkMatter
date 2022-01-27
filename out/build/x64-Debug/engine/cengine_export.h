
#ifndef CENGINE_EXPORT_H
#define CENGINE_EXPORT_H

#ifdef CENGINE_STATIC_DEFINE
#  define CENGINE_EXPORT
#  define CENGINE_NO_EXPORT
#else
#  ifndef CENGINE_EXPORT
#    ifdef CEngine_EXPORTS
        /* We are building this library */
#      define CENGINE_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define CENGINE_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef CENGINE_NO_EXPORT
#    define CENGINE_NO_EXPORT 
#  endif
#endif

#ifndef CENGINE_DEPRECATED
#  define CENGINE_DEPRECATED __declspec(deprecated)
#endif

#ifndef CENGINE_DEPRECATED_EXPORT
#  define CENGINE_DEPRECATED_EXPORT CENGINE_EXPORT CENGINE_DEPRECATED
#endif

#ifndef CENGINE_DEPRECATED_NO_EXPORT
#  define CENGINE_DEPRECATED_NO_EXPORT CENGINE_NO_EXPORT CENGINE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef CENGINE_NO_DEPRECATED
#    define CENGINE_NO_DEPRECATED
#  endif
#endif

#endif /* CENGINE_EXPORT_H */
