#ifndef __ENGINE_TYPES_H__
#define __ENGINE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct dm_platform_data
{
	int window_width, window_height;
	const char* window_title;
	void* internal_data;
} dm_platform_data;

typedef struct dm_engine_data
{
    dm_platform_data* platform_data;
	bool is_running, is_suspended;
} dm_engine_data;

typedef struct dm_engine_config
{
	uint32_t start_x, start_y, start_width, start_height;
	char* name;
} dm_engine_config;

#endif