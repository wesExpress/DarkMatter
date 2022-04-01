#include "application.h"

static dm_editor_camera camera = {
    .pos = {0,0,4}, .forward = {0,0,0}, .up = {0,1,0},
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

dm_entity cube;
dm_entity light;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");
    
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
    
	// clear color
	dm_renderer_api_set_clear_color((dm_vec3) { 0, 0, 0 });
    
	// camera
	dm_renderer_api_set_camera_pos(camera.pos);
	dm_renderer_api_set_camera_forward(camera.forward);
    
    dm_input_get_mouse_pos(&camera.last_x, &camera.last_y);
    
	objects = dm_list_create(sizeof(dm_game_object), 0);
	dm_list_append(objects, &(dm_game_object){
                       .transform = { {0, 0, 0}, { 1,1,1 } },
                       .color = { 1, 0.5, 0.31 },
                       .texture = 0,
                       .mesh = "cube",
                       .render_pass = "object"
                   });
	dm_list_append(objects, &(dm_game_object){
                       .transform = { {1.2, 1, 2}, { 0.2,0.2,0.2 } },
                       .color = { 1,1,1 },
                       .texture = 0,
                       .mesh = "cube",
                       .render_pass = "light_src"
                   });
    
    cube = dm_ecs_create_entity();
    light = dm_ecs_create_entity();
    
    if(!dm_ecs_add_component(cube, DM_COMPONENT_TRANSFORM, &(dm_transform){{0,0,0}, {1,1,1}})) return false;
    if(!dm_ecs_add_component(light, DM_COMPONENT_TRANSFORM, &(dm_transform){{1.2, 1, 2}, {0.2,0.2,0.2}})) return false;
    
    return dm_renderer_api_submit_objects(objects);
}

void dm_application_shutdown(dm_application* app)
{
    
    
	dm_list_destroy(objects);
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