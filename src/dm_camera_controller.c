#include "dm_camera_controller.h"
#include "input/dm_input.h"

void dm_camera_update(dm_camera* camera, float delta_time)
{
	if (dm_input_is_key_pressed(DM_KEY_A))
	{
		camera->pos.x -= 0.1;
	}
	if (dm_input_is_key_pressed(DM_KEY_D))
	{
		camera->pos.x += 0.1;
	}
	if (dm_input_is_key_pressed(DM_KEY_W))
	{
		camera->pos.z -= 0.1;
	}
	if (dm_input_is_key_pressed(DM_KEY_S))
	{
		camera->pos.z += 0.1;
	}

	dm_camera_set_view(camera, camera->pos, camera->forward, camera->up);
}