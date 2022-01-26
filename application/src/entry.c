#include "core/dm_engine_entry.h"
#include <core/dm_app_config.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t window_width = 1280;
static uint32_t window_height = 720;
static uint32_t window_start_x = 100;
static uint32_t window_start_y = 100;

bool dm_create_app(dm_application* application)
{
	application->engine_config.start_x = window_start_x;
	application->engine_config.start_y = window_start_y;
	application->engine_config.start_width = window_width;
	application->engine_config.start_height = window_height;

	application->engine_config.name = "Dark Matter Engine";

	return true;
}