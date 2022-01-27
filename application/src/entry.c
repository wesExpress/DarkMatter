#include "application.h"
#include "core/dm_engine_entry.h"

static uint32_t window_width = 1280;
static uint32_t window_height = 720;
static uint32_t window_start_x = 100;
static uint32_t window_start_y = 100;
static dm_color clear_color = { 0.2f, 0.5f, 0.8f, 1.0f };

bool dm_create_app(dm_application* application)
{
	application->engine_config.start_x = window_start_x;
	application->engine_config.start_y = window_start_y;
	application->engine_config.start_width = window_width;
	application->engine_config.start_height = window_height;
	application->engine_config.clear_color = clear_color;

	application->dm_application_init = dm_application_init;
	application->dm_application_shutdown = dm_application_shutdown;
	application->dm_application_update = dm_application_update;
	application->dm_application_render = dm_application_render;
	application->dm_application_resize = dm_application_resize;

	application->engine_config.name = "Dark Matter Engine";

	return true;
}