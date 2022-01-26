#include "dm_camera.h"
#include "dm_renderer.h"
#include "input/dm_input.h"
#include "core/dm_mem.h"
#include "core/dm_math.h"

static dm_camera* camera;

void dm_camera_init(dm_camera* camera, dm_vec3 pos, float fov, int width, int height, float n, float f, dm_camera_type type)
{
	camera->pos = pos;
	camera->forward = (dm_vec3){ 0,0,-1 };
	camera->up = (dm_vec3){ 0,1,0 };
	camera->fov = fov;
	camera->aspect_ratio = (float)width / (float)height;
	camera->nZ = n;
	camera->fZ = f;
	camera->type = type;

	dm_camera_set_view(
		camera,
		camera->pos,
		camera->forward,
		camera->up
	);

	dm_camera_set_projection(
		camera,
		camera->fov,
		width, height,
		camera->nZ,
		camera->fZ
	);

	camera->view_proj = dm_mat4_mul_mat4(camera->view, camera->projection);
}

void dm_camera_set_pos(dm_camera* camera, dm_vec3 pos)
{
	camera->pos = pos;

	dm_camera_set_view(
		camera,
		camera->pos,
		camera->forward,
		camera->up
	);
}

void dm_camera_set_view(dm_camera* camera, dm_vec3 view_origin, dm_vec3 target, dm_vec3 up)
{
	camera->view = dm_mat_view(view_origin, dm_vec3_add_vec3(view_origin, target), up);

	dm_camera_update_view_proj(camera);
}

void dm_camera_set_projection(dm_camera* camera, float fov, int width, int height, float n, float f)
{
	switch (camera->type)
	{
	case DM_CAMERA_PERSPECTIVE:
	{
		float aspect_ratio = (float)width / (float)height;
		camera->projection = dm_mat_perspective(fov, aspect_ratio, n, f);
	} break;
	case DM_CAMERA_ORTHOGRAPHIC:
		camera->projection = dm_mat_ortho(0, width, 0, height, n, f);
		break;
	default: break;
	}

	dm_camera_update_view_proj(camera);
}

void dm_camera_update_view_proj(dm_camera* camera)
{
	camera->view_proj = dm_mat4_mul_mat4(camera->view, camera->projection);
}