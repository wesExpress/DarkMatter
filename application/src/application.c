#include "application.h"

static float velocity = 2.0f;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");

	// vertex data
	DM_LOG_DEBUG("Submitting test vertex data from app...");

	dm_vertex_t vertices[] = {
	{ {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
	{ { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
	};

#ifdef DM_OPENGL
	dm_index_t indices[] = {
		0, 1, 2,
		2, 3, 0
	};
#elif defined DM_DIRECTX
	dm_index_t indices[] = {
		0, 2, 1,
		2, 0, 3
	};
#endif

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

	if (!dm_renderer_api_submit_textures(image_descs, sizeof(image_descs) / sizeof(dm_image_desc))) return false;

	// camera
	dm_renderer_api_set_camera_pos((dm_vec3) { 0, 0, 2 });

	return true;
}

void dm_application_shutdown(dm_application* app)
{

}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_vec3 pos_delta = { 0 };
	dm_vec3 forward_delta = { 0 };
	
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
		pos_delta.z = -1;
	}
	else if (dm_input_key_just_pressed(DM_KEY_S))
	{
		pos_delta.z = 1;
	}

	if (dm_input_key_just_pressed(DM_KEY_Q))
	{
		forward_delta.x = -1;
	}
	else if (dm_input_key_just_pressed(DM_KEY_E))
	{
		forward_delta.x = 1;
	}

	pos_delta = dm_vec3_scale(pos_delta, velocity * delta_time);
	forward_delta = dm_vec3_scale(forward_delta, velocity * delta_time);

	dm_renderer_api_update_camera_pos(pos_delta);
	dm_renderer_api_update_camera_forward(forward_delta);

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