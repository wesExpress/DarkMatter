#include "dm_systems.h"
#include "input/dm_input.h"
#include "core/dm_math.h"
#include "rendering/dm_renderer_api.h"

void dm_ecs_update_editor_camera(dm_editor_camera* camera, float delta_time)
{
    dm_vec3 forward = { 0 };
    dm_vec3 delta_pos = { 0 };
    
	// camera direction
	if (dm_input_is_key_pressed(DM_KEY_LSHIFT))
	{
		camera->yaw += dm_input_get_mouse_delta_x() * camera->look_sens;
		camera->pitch -= dm_input_get_mouse_delta_y() * camera->look_sens;
        
		DM_CLAMP(camera->pitch, -89, 89);
	}
    
	forward.x = dm_cos(dm_deg_to_rad(camera->yaw)) * dm_cos(dm_deg_to_rad(camera->pitch));
	forward.y = dm_sin(dm_deg_to_rad(camera->pitch));
	forward.z = dm_sin(dm_deg_to_rad(camera->yaw)) * dm_cos(dm_deg_to_rad(camera->pitch));
	forward = dm_vec3_norm(forward);
    
	// camera position
	if (dm_input_is_key_pressed(DM_KEY_A))
	{
        delta_pos = dm_vec3_cross(camera->forward, camera->up);
		delta_pos = dm_vec3_norm(delta_pos);
		delta_pos = dm_vec3_scale(delta_pos, -camera->move_velocity * delta_time);
	}
	else if (dm_input_is_key_pressed(DM_KEY_D))
	{
        delta_pos = dm_vec3_cross(camera->forward, camera->up);
		delta_pos = dm_vec3_norm(delta_pos);
		delta_pos = dm_vec3_scale(delta_pos, camera->move_velocity * delta_time);
	}
    
	if (dm_input_is_key_pressed(DM_KEY_W))
	{
        delta_pos = dm_vec3_add_vec3(delta_pos, dm_vec3_scale(camera->forward, camera->move_velocity * delta_time));
	}
	else if (dm_input_is_key_pressed(DM_KEY_S))
	{
		delta_pos = dm_vec3_add_vec3(delta_pos, dm_vec3_scale(camera->forward, -camera->move_velocity * delta_time));
	}
    
    //dm_renderer_api_update_camera_pos(delta_pos);
    
    // vertical movement
	if (dm_input_is_key_pressed(DM_KEY_Z))
	{
		delta_pos = dm_vec3_add_vec3(delta_pos, (dm_vec3){0, -camera->move_velocity * delta_time, 0});
	}
	else if (dm_input_is_key_pressed(DM_KEY_X))
	{
        delta_pos = dm_vec3_add_vec3(delta_pos, (dm_vec3){0, camera->move_velocity * delta_time, 0});
	}
    
	// update the camera
	camera->pos = dm_vec3_add_vec3(camera->pos, dm_vec3_norm(delta_pos));
    camera->forward = forward;
    
    dm_renderer_api_set_camera_forward(camera->forward);
    dm_renderer_api_set_camera_pos(camera->pos);
}