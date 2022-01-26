#ifndef __DM_LOG_H__
#define __DM_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include "dm_defines.h"

typedef enum log_level
{
	DM_LOG_TRACE,
	DM_LOG_DEBUG,
	DM_LOG_INFO,
	DM_LOG_WARN,
	DM_LOG_ERROR,
	DM_LOG_FATAL
} log_level;

DM_API void dm_log_output(log_level level, const char* message, ...);

#define DM_LOG_TRACE(message, ...) dm_log_output(DM_LOG_TRACE, message, ##__VA_ARGS__)
#define DM_LOG_DEBUG(message, ...) dm_log_output(DM_LOG_DEBUG, message, ##__VA_ARGS__)
#define DM_LOG_INFO(message, ...)  dm_log_output(DM_LOG_INFO,  message, ##__VA_ARGS__)
#define DM_LOG_WARN(message, ...)  dm_log_output(DM_LOG_WARN,  message, ##__VA_ARGS__)
#define DM_LOG_ERROR(message, ...) dm_log_output(DM_LOG_ERROR, message, ##__VA_ARGS__)
#define DM_LOG_FATAL(message, ...) dm_log_output(DM_LOG_FATAL, message, ##__VA_ARGS__)

#undef EXTERNC

#endif