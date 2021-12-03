#ifndef __DM_LOG_H__
#define __DM_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include "dm_defines.h"

typedef enum log_level
{
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL
} log_level;

DM_API void dm_log_output(log_level level, const char* message, ...);

#define DM_TRACE(message, ...) dm_log_output(LOG_TRACE, message, ##__VA_ARGS__)
#define DM_DEBUG(message, ...) dm_log_output(LOG_DEBUG, message, ##__VA_ARGS__)
#define DM_INFO(message, ...)  dm_log_output(LOG_INFO,  message, ##__VA_ARGS__)
#define DM_WARN(message, ...)  dm_log_output(LOG_WARN,  message, ##__VA_ARGS__)
#define DM_ERROR(message, ...) dm_log_output(LOG_ERROR, message, ##__VA_ARGS__)
#define DM_FATAL(message, ...) dm_log_output(LOG_FATAL, message, ##__VA_ARGS__)

#undef EXTERNC

#endif