#include "dm_renderer_api.h"
#include "rendering/dm_renderer.h"

void dm_renderer_api_submit_vertex_data(dm_vertex_t* vertex_data, dm_index_t* index_data, uint32_t num_vertices, uint32_t num_indices)
{
	dm_renderer_submit_vertex_data(vertex_data, index_data, num_vertices, num_indices);
}

bool dm_renderer_api_submit_textures(dm_image_desc* image_descs, uint32_t num_desc)
{
	return dm_renderer_submit_textures(image_descs, num_desc);
}

void dm_renderer_api_submit_object_transforms(dm_transform* transforms, uint32_t num_transforms)
{
	dm_renderer_submit_object_transforms(transforms, num_transforms);
}

void dm_renderer_api_update_object_transforms(dm_transform* transforms, uint32_t num_transforms)
{
	dm_renderer_update_object_transforms(transforms, num_transforms);
}

void dm_renderer_api_set_camera_pos(dm_vec3 pos)
{
	dm_renderer_set_camera_pos(pos);
}

void dm_renderer_api_update_camera_pos(dm_vec3 delta_pos)
{
	dm_renderer_update_camera_pos(delta_pos);
}

void dm_renderer_api_set_camera_forward(dm_vec3 forward)
{
	dm_renderer_set_camera_forward(forward);
}

void dm_renderer_api_update_camera_forward(dm_vec3 delta_forward)
{
	dm_renderer_update_camera_forward(delta_forward);
}

void dm_renderer_api_set_camera_euler(dm_vec3 delta_euler)
{

}

void dm_renderer_api_update_camera_euler(dm_vec3 delta_euler)
{

}