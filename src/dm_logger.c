#include "dm_logger.h"
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "dm_assert.h"
#include "platform/dm_platform.h"

/*
* Indebtted to Travis Vroman for his logging approach
Game Engine in C from Scratch:
https://www.youtube.com/watch?v=l9e8PJskYnI&list=PLv8Ddw9K0JPg1BEO-RS-0MYs423cvLVtj&index=5
*/

typedef struct logtime
{
	int hours, mins, secs;
} logtime;

void assertion_failure(const char* expression, const char* message, const char* file, int line)
{
	DM_LOG_FATAL("Assertion failure: %s;\n message: %s\n File: %s\n Line: %d", expression, message, file, line);
}

static logtime LOGTIME = { 0 };

void dm_log_set_time()
{
	time_t now;
	time(&now);
	struct tm* local = localtime(&now);
	LOGTIME.hours = local->tm_hour;
	LOGTIME.mins = local->tm_min;
	LOGTIME.secs = local->tm_sec;
}

void dm_log_output(log_level level, const char* message, ...)
{
	const char* log_tag[6] = { "DM_TRACE", "DM_DEBUG", "DM_INFO ", "DM_WARN ", "DM_ERROR", "DM_FATAL" };

	char msg_fmt[5000];
	memset(msg_fmt, 0, sizeof(msg_fmt));

	// ar_ptr lets us move through any variable number of arguments. 
	// start it one argument to the left of the va_args, here that is message.
	// then simply shove each of those arguments into message, which should be 
	// formatted appropriately beforehand
	va_list ar_ptr;
	va_start(ar_ptr, message);
	vsnprintf(msg_fmt, 5000, message, ar_ptr);
	va_end(ar_ptr);

	// add log tag and time code of message
	char out[5000];

	dm_log_set_time();
	sprintf(
		out, 
		//"[%02d:%02d:%02d %s]: %s\n", 
		"[%s] (%02d:%02d:%02d): %s\n",
		log_tag[level],
		LOGTIME.hours, LOGTIME.mins, LOGTIME.secs,
		 
		msg_fmt
	);

	dm_platform_write(out, level);
}