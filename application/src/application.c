#include "application.h"

static dm_editor_camera camera = {
	.pitch = 0, .yaw=-90, .roll=0,
	.move_velocity = 2.5f,
	.look_sens = 0.1f
};

dm_transform transforms[] = {
	{0, 0, 0},
	{2, 5, -15},
	{-1.5, -2.2, -2.5},
	{-3.8, -2, -12.3},
	{2.4, -0.4, -3.5},
	{-1.7, 3, -7.5},
	{1.3, -2, -2.5},
	{1.5, 2, -2.5},
	{1.5, 0.2, -1.5},
	{-1.3, 1, -1.5}
};

dm_map_t* object_map = NULL;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");

	// vertex data
	DM_LOG_DEBUG("Submitting test vertex data from app...");

	dm_vertex_t vertices[] = {
	// front face
	{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f} }, // 0
	{ { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f} },
	// bacxk face
	{ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 4
	{ {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
	{ {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
	// top face
	{ {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f} }, // 8
	{ { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
	// bottom face
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 12
	{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
	{ { 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f} },
	{ {-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f} },
	// left face
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 16
	{ {-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f} },
	{ {-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
	// right face
	{ { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f} }, // 20
	{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
	{ { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f} }
	};

	dm_index_t indices[] = {
		 0,  1,  2,    2,  3,  0,
		 4,  5,  6,    6,  7,  4,
		 8,  9, 10,   10, 11,  8,
	    12, 13, 14,   14, 15, 12,
		16, 17, 18,   18, 19, 16,
		20, 21, 22,   22, 23, 20
	};

	uint32_t num_vertices = sizeof(vertices) / sizeof(dm_vertex_t);
	uint32_t num_indices = sizeof(indices) / sizeof(dm_index_t);

	dm_renderer_api_submit_vertex_data("cube", vertices, indices, num_vertices, num_indices);

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

	if (!dm_renderer_api_submit_images(image_descs, sizeof(image_descs) / sizeof(dm_image_desc))) return false;

	// camera
	dm_renderer_api_set_camera_pos((dm_vec3) { 0, 0, 4 });
	dm_input_get_mouse_pos(&camera.last_x, &camera.last_y);

	// transforms
	dm_renderer_api_submit_object_transforms("cube", transforms, sizeof(transforms) / sizeof(transforms[0]));

	return true;
}

void dm_application_shutdown(dm_application* app)
{

}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_vec3 forward = { 0 };

	// camera direction
	if (dm_input_is_key_pressed(DM_KEY_SHIFT))
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