#ifndef __ENGINE_TYPES_H__
#define __ENGINE_TYPES_H__

typedef struct platform_data
{
	void* internal_data;
	int window_width, window_height;
	const char* window_title;
} platform_data;

typedef struct engine_data
{
    platform_data* platform_data;
} engine_data;

#endif