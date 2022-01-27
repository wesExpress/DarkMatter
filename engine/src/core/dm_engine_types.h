#ifndef __ENGINE_TYPES_H__
#define __ENGINE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include "rendering/dm_render_types.h"

typedef struct dm_engine_config
{
	uint32_t start_x, start_y, start_width, start_height;
	dm_color clear_color;
	char* name;
} dm_engine_config;

typedef struct dm_application
{
	dm_engine_config engine_config;

	bool (*dm_application_init)(struct dm_application* app);
	void (*dm_application_shutdown)(struct dm_application* app);
	bool (*dm_application_update)(struct dm_application* app, float delta_time);
	bool (*dm_application_render)(struct dm_application* app, float delta_time);
	bool (*dm_application_resize)(struct dm_application* app, uint32_t width, uint32_t height);

	void* state;
} dm_application;

typedef struct dm_platform_data
{
	int window_width, window_height;
	const char* window_title;
	void* internal_data;
} dm_platform_data;

typedef struct dm_engine_data
{
    dm_platform_data* platform_data;
	dm_application* application;
	bool is_running, is_suspended;
} dm_engine_data;

#endif