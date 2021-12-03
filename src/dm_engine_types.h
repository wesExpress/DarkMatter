#ifndef __ENGINE_TYPES_H__
#define __ENGINE_TYPES_H__

typedef struct dm_platform_data
{
	void* internal_data;
	int window_width, window_height;
	const char* window_title;
} dm_platform_data;

typedef struct dm_engine_data
{
    dm_platform_data* platform_data;
} dm_engine_data;

#endif