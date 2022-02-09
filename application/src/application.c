#include "application.h"

static dm_editor_camera camera = {
	.pitch = 0, .yaw=-90, .roll=0,
	.move_velocity = 2.5f,
	.look_sens = 0.1f
};

dm_transform transforms_1[] = {
	{{0, 0, 0}, {1,1,1}},
	{{2, 5, -15}, {1,1,1}},
	{{-1.5, -2.2, -2.5}, {1,1,1}},
	{{-3.8, -2, -12.3}, {1,1,1}},
	{{2.4, -0.4, -3.5}, {1,1,1}},
	{{-1.7, 3, -7.5}, {1,1,1}},
	{{1.3, -2, -2.5}, {1,1,1}},
	{{1.5, 2, -2.5}, {1,1,1}},
	{{1.5, 0.2, -1.5}, {1,1,1}},
	{{-1.3, 1, -1.5}, {1,1,1}}
};

dm_transform transforms_2[] = {
	{{0,0,0}, {1,1,1}},
	{{1.2,1,2}, {0.2,0.2,0.2}}
};

dm_list* objects = NULL;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");

	// textures
	DM_LOG_DEBUG("Submitting texture data from app...");

	dm_image_desc image_desc1 = { 0 };
	image_desc1.path = "assets/container.jpg";
	image_desc1.name = "uTexture1";
	image_desc1.format = DM_TEXTURE_FORMAT_RGB;
	image_desc1.internal_format = DM_TEXTURE_FORMAT_RGB;

	dm_image_desc image_desc2 = { 0 };
	image_desc2.path = "assets/awesomeface.png";
	image_desc2.name = "uTexture2";
	image_desc2.format = DM_TEXTURE_FORMAT_RGBA;
	image_desc2.internal_format = DM_TEXTURE_FORMAT_RGB;
	image_desc2.flip = true;

	dm_image_desc image_descs[] = { image_desc1, image_desc2 };

	//if (!dm_renderer_api_submit_images(image_descs, sizeof(image_descs) / sizeof(dm_image_desc))) return false;

	// camera
	dm_renderer_api_set_camera_pos((dm_vec3) { 0, 0, 4 });
	dm_input_get_mouse_pos(&camera.last_x, &camera.last_y);

	objects = dm_list_create(sizeof(dm_game_object), 0);
	dm_list_append(objects, &(dm_game_object){ 
		.transform={{0, 0, 0}, { 1,1,1 }}, 
		.color={1, 0.5, 0.31}, 
		.texture=0, 
		.mesh="cube",
		.render_pass="object"
	});
	dm_list_append(objects, &(dm_game_object){ 
		.transform={{1.2, 1, 2}, { 0.2,0.2,0.2 }},
		.color={1,1,1},
		.texture= 0, 
		.mesh = "cube",
		.render_pass="light_src"
	});

	dm_renderer_api_submit_objects(objects);

	// clear color
	dm_renderer_api_set_clear_color((dm_vec3) { 0, 0, 0 });

	return true;
}

void dm_application_shutdown(dm_application* app)
{
	dm_list_destroy(objects);
}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_vec3 forward = { 0 };

	// camera direction
	if (dm_input_is_key_pressed(DM_KEY_LSHIFT))
	{
		camera.yaw += dm_input_get_mouse_delta_x() * camera.look_sens;
		camera.pitch -= dm_input_get_mouse_delta_y() * camera.look_sens;

		DM_CLAMP(camera.pitch, -89, 89);
	}

	forward.x = dm_cos(dm_deg_to_rad(camera.yaw)) * dm_cos(dm_deg_to_rad(camera.pitch));
	forward.y = dm_sin(dm_deg_to_rad(camera.pitch));
	forward.z = dm_sin(dm_deg_to_rad(camera.yaw)) * dm_cos(dm_deg_to_rad(camera.pitch));
	forward = dm_vec3_norm(forward);

	// camera position
	if (dm_input_is_key_pressed(DM_KEY_A))
	{
		dm_vec3 delta_pos = dm_vec3_cross(dm_renderer_api_get_camera_forward(), dm_renderer_api_get_camera_up());
		delta_pos = dm_vec3_norm(delta_pos);
		delta_pos = dm_vec3_scale(delta_pos, -camera.move_velocity * delta_time);
		dm_renderer_api_update_camera_pos(delta_pos);
	}
	else if (dm_input_is_key_pressed(DM_KEY_D))
	{
		dm_vec3 delta_pos = dm_vec3_cross(dm_renderer_api_get_camera_forward(), dm_renderer_api_get_camera_up());
		delta_pos = dm_vec3_norm(delta_pos);
		delta_pos = dm_vec3_scale(delta_pos, camera.move_velocity * delta_time);
		dm_renderer_api_update_camera_pos(delta_pos);
	}

	if (dm_input_is_key_pressed(DM_KEY_W))
	{
		dm_vec3 delta_pos = dm_vec3_scale(dm_renderer_api_get_camera_forward(), camera.move_velocity * delta_time);
		dm_renderer_api_update_camera_pos(delta_pos);
	}
	else if (dm_input_is_key_pressed(DM_KEY_S))
	{
		dm_vec3 delta_pos = dm_vec3_scale(dm_renderer_api_get_camera_forward(), -camera.move_velocity * delta_time);
		dm_renderer_api_update_camera_pos(delta_pos);
	}

	if (dm_input_is_key_pressed(DM_KEY_Z))
	{
		dm_renderer_api_update_camera_pos((dm_vec3) { 0, -camera.move_velocity * delta_time, 0 });
	}
	else if (dm_input_is_key_pressed(DM_KEY_X))
	{
		dm_renderer_api_update_camera_pos((dm_vec3) { 0, camera.move_velocity * delta_time, 0 });
	}

	// update the camera
	dm_renderer_api_set_camera_forward(forward);

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