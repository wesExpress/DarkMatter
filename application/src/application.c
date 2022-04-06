#include "application.h"

static dm_editor_camera camera = {
    .pos = {0,0,4}, .forward = {0,0,0}, .up = {0,1,0},
	.pitch = 0, .yaw=-90, .roll=0,
	.move_velocity = 2.5f,
	.look_sens = 0.1f
};

/*
dm_transform_component transforms_1[] = {
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
*/

dm_entity cube;
dm_entity light;
dm_entity editor_camera;
dm_entity cube2;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");
    
    /*
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
    */
	//dm_image_desc image_descs[] = { image_desc1, image_desc2 };
    
	//if (!dm_renderer_api_submit_images(image_descs, sizeof(image_descs) / sizeof(dm_image_desc))) return false;
    
	dm_renderer_api_set_clear_color((dm_vec3) { 0, 0, 0 });
    
    dm_input_get_mouse_pos(&camera.last_x, &camera.last_y);
    
    // camera
    editor_camera = dm_ecs_create_entity();
    if(!dm_ecs_add_editor_camera(&editor_camera, &camera)) return false;
    
    // orange cube
    cube = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(&cube, &(dm_transform_component){.position={0.0f,0.0f,0.0f}, .scale={1.0f,1.0f,1.0f}})) return false;
    if(!dm_ecs_add_mesh(&cube, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_color(&cube, &(dm_color_component){.color={1.0f, 0.5f, 0.31f}})) return false;
    
    // light src
    light = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(&light, &(dm_transform_component){.position={1.2f,1.0f,2.0f}, .scale={0.2f,0.2f,0.2f}})) return false;
    if(!dm_ecs_add_mesh(&light, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_color(&light, &(dm_color_component){.color={1.0f, 1.0f, 1.0f}})) return false;
    if(!dm_ecs_add_light_src(&light,&(dm_light_src_component){.color={1.0f, 1.0f, 1.0f}, .sync_to_obj_color=true})) return false;
    
    // blue cube
    cube2 = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(&cube2, &(dm_transform_component){.position={2.0f,2.0f,2.0f}, .scale={1.0f,1.0f,1.0f}})) return false;
    if(!dm_ecs_add_mesh(&cube2, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_color(&cube2, &(dm_color_component){.color={0.31f, 0.5f, 1.f}})) return false;
    
    return true;
}

void dm_application_shutdown(dm_application* app)
{
    
}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_ecs_update_editor_camera(&camera, delta_time);
    
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