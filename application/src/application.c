#include "application.h"
#include <core/dm_logger.h>
#include <rendering/dm_renderer.h>
#include <input/dm_input.h>

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");

	return true;
}

void dm_application_shutdown(dm_application* app)
{

}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_vec3 pos_delta = { 0 };
	
	if (dm_input_key_just_pressed(DM_KEY_A))
	{
		pos_delta.x = -1;
	}
	else if (dm_input_key_just_pressed(DM_KEY_D))
	{
		pos_delta.x = 1;
	}

	if (dm_input_key_just_pressed(DM_KEY_W))
	{
		pos_delta.z = 1;
	}
	else if (dm_input_key_just_pressed(DM_KEY_S))
	{
		pos_delta.z = -1;
	}

	dm_renderer_update_camera_pos(pos_delta);

	return true;
}

bool dm_application_render(dm_application* app, float delta_time)
{
	return true;
}

bool dm_application_resize(dm_application* app, uint32_t width, uint32_t height)
{
	return true;
}