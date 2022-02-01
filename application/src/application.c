#include "application.h"

static float move_vel = 2.5f;
static float look_vel = 1.5f;

dm_transform transforms[] = {
	{0,0,0},
	//{2,5,-15},
	//{-1.5,-2.2,-2.5},
	//{-3.8,-2,-12.3},
	//{2.4,-0.4,-3.5},
	//{-1.7,3,-7.5},
	//{1.3,-2,-2.5},
	//{1.5,2,-2.5},
	//{1.5,0.2,-1.5},
	//{-1.3,1,-1.5}
};

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");

	// vertex data
	DM_LOG_DEBUG("Submitting test vertex data from app...");

	dm_vertex_t vertices[] = {
	// front face
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 0
	{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
	// back face
	{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f} }, // 4
	{ { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f} },
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

	dm_renderer_api_submit_vertex_data(vertices, indices, num_vertices, num_indices);

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

	DM_LOG_DEBUG("Submitting object transforms...");
	dm_renderer_api_submit_object_transforms(transforms, sizeof(transforms) / sizeof(dm_transform));

	return true;
}

void dm_application_shutdown(dm_application* app)
{

}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_vec3 pos_delta = { 0 };
	dm_vec3 forward_delta = { 0 };
	
	// camera position
	if (dm_input_is_key_pressed(DM_KEY_A))
	{
		pos_delta.x = -1;
	}
	else if (dm_input_is_key_pressed(DM_KEY_D))
	{
		pos_delta.x = 1;
	}

	if (dm_input_is_key_pressed(DM_KEY_W))
	{
		pos_delta.z = -1;
	}
	else if (dm_input_is_key_pressed(DM_KEY_S))
	{
		pos_delta.z = 1;
	}

	if (dm_input_is_key_pressed(DM_KEY_Z))
	{
		pos_delta.y = 1;
	}
	else if (dm_input_is_key_pressed(DM_KEY_X))
	{
		pos_delta.y = -1;
	}

	// camera direction
	if (dm_input_is_key_pressed(DM_KEY_Q))
	{
		forward_delta.x = -1;
	}
	else if (dm_input_is_key_pressed(DM_KEY_E))
	{
		forward_delta.x = 1;
	}

	if (dm_input_is_key_pressed(DM_KEY_R))
	{
		forward_delta.y = 1;
	}
	else if (dm_input_is_key_pressed(DM_KEY_F))
	{
		forward_delta.y = -1;
	}

	// norm and scale the deltas
	pos_delta = dm_vec3_norm(pos_delta);
	forward_delta = dm_vec3_norm(forward_delta);

	pos_delta = dm_vec3_scale(pos_delta, move_vel * delta_time);
	forward_delta = dm_vec3_scale(forward_delta, look_vel * delta_time);

	// update the camera
	dm_renderer_api_update_camera_pos(pos_delta);
	dm_renderer_api_update_camera_forward(forward_delta);

	// object transforms
	

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