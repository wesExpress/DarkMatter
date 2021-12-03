#ifndef __DM_ASSERT_H__
#define __DM_ASSERT_H__

#include "dm_defines.h"
#include "dm_logger.h"

#define DM_ASSERTIONS_ENABLED

#ifdef DM_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debug_break() __debugbreak()
#else
#define debug_break() __builtin_trap()
#endif

DM_API void assertion_failure(const char* expression, const char* message, const char* file, int line);

#define DM_ASSERT(expr)\
{\
	if (expr){\
	} else {\
		assertion_failure(#expr, "", __FILE__, __LINE__);\
		debug_break();\
	}\
}

#define DM_ASSERT_MSG(expr, message, ...)\
{\
	if (expr){\
	} else {\
		DM_FATAL(message, ##__VA_ARGS__);\
		assertion_failure(#expr, "", __FILE__, __LINE__);\
		debug_break();\
	}\
}

#else
#define DM_ASSERT(expr)
#define DM_ASSERT_MSG(expr, message)
#endif

#endif