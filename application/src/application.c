#include "application.h"

dm_entity cube;
dm_entity light;
dm_entity editor_camera;
dm_entity cube2;
dm_entity textured_cube;

dm_entity model_load;

float radius = 2.5f;
float angle = 0.0f;

bool dm_application_init(dm_application* app)
{
	DM_LOG_TRACE("Hellow from the application!\n");
	
    dm_renderer_api_set_clear_color((dm_vec3) { 0, 0, 0 });
    
    // images
    if(!dm_renderer_api_register_image("assets/container.jpg", "uTexture1", false, DM_TEXTURE_FORMAT_RGB, DM_TEXTURE_FORMAT_RGB)) return false;
    if(!dm_renderer_api_register_image("assets/awesomeface.png", "uTexture2", true, DM_TEXTURE_FORMAT_RGBA, DM_TEXTURE_FORMAT_RGB)) return false;
    if(!dm_renderer_api_register_image("assets/container_diffuse.png", "container_diffuse", true, DM_TEXTURE_FORMAT_RGBA, DM_TEXTURE_FORMAT_RGB)) return false;
    if(!dm_renderer_api_register_image("assets/container_specular.png", "container_specular", true, DM_TEXTURE_FORMAT_RGBA, DM_TEXTURE_FORMAT_RGB)) return false;
    
    // models
    //if(!dm_load_model("assets/fox.gltf", true)) return false;
    //if(!dm_load_model("assets/duck.gltf", false)) return false;
    
    // camera
    editor_camera = dm_ecs_create_entity();
    if(!dm_ecs_add_editor_camera(editor_camera, &(dm_editor_camera){.pos={0,0,4}, .yaw=-90.0f, .move_velocity=2.5f, .look_sens=0.1f})) return false;
    dm_editor_camera* c = dm_ecs_get_component(editor_camera, DM_COMPONENT_EDITOR_CAMERA);
    
    dm_input_get_mouse_pos(&c->last_x, &c->last_y);
    
    // textured cube
    textured_cube = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(textured_cube, &(dm_transform_component){.position={-2.0f,-2.0f,-2.0f}, .scale={1.0f,1.0f,1.0f}})) return false;
    if(!dm_ecs_add_mesh(textured_cube, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_material(textured_cube, &(dm_material_component){.diffuse_map="container_diffuse", .specular_map="container_specular", .shininess=64})) return false;
    
    // orange cube
    cube = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(cube, &(dm_transform_component){.position={0.0f,0.0f,0.0f}, .scale={1.0f,1.0f,1.0f}})) return false;
    if(!dm_ecs_add_mesh(cube, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_color(cube, &(dm_color_component){.diffuse={1.0f, 0.5f, 0.31f}, .specular={1.0f, 0.5f, 0.31f}, .shininess=32})) return false;
    
    // light src
    light = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(light, &(dm_transform_component){.position={1.2f,1.0f,2.0f}, .scale={0.2f,0.2f,0.2f}})) return false;
    if(!dm_ecs_add_mesh(light, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_light_src(light,&(dm_light_src_component){.ambient={0.2f,0.2f,0.2f,1.0f}, .diffuse={0.5f,0.5f,0.5f,1.0f}, .specular={1.0f,1.0f,1.0f,1.0f}, .strength=1.0f})) return false;
    
    // blue cube
    cube2 = dm_ecs_create_entity();
    
    if(!dm_ecs_add_transform(cube2, &(dm_transform_component){.position={2.0f,2.0f,2.0f}, .scale={1.0f,1.0f,1.0f}})) return false;
    if(!dm_ecs_add_mesh(cube2, &(dm_mesh_component){.name="cube"})) return false;
    if(!dm_ecs_add_color(cube2, &(dm_color_component){.diffuse={0.31f, 0.5f, 1.f}, .specular={0.31f, 0.5f, 1.f}, .shininess=32})) return false;
    
    
    
    // model 
    model_load= dm_ecs_create_entity();
    if(!dm_ecs_add_transform(model_load, &(dm_transform_component){.position={0.0f, 2.5f, -1.5f}, .scale={1,1,1}})) return false;
    //if(!dm_ecs_add_mesh(model_load, &(dm_mesh_component){.name="fox1"})) return false;
    if(!dm_ecs_add_mesh(model_load, &(dm_mesh_component){.name="LOD3spShape"})) return false;
    if(!dm_ecs_add_color(model_load, &(dm_color_component){.diffuse={1,0,0}, .specular={1,0,0}, .shininess=16})) return false;
    
    return true;
}

void dm_application_shutdown(dm_application* app)
{
    
}

bool dm_application_update(dm_application* app, float delta_time)
{
	dm_ecs_update_editor_camera(delta_time);
    
    // move cube
    dm_transform_component* transform = dm_ecs_get_component(cube, DM_COMPONENT_TRANSFORM);
    
    transform->position.x = dm_cos(angle) * radius;
    transform->position.y = dm_sin(angle) * radius;
    
    // move light
    transform = dm_ecs_get_component(light, DM_COMPONENT_TRANSFORM);
    
    transform->position.z = dm_sin(angle) * radius;
    transform->position.x = dm_cos(angle) * radius;
    
    /*
    // change light color
    dm_light_src_component* light_src = dm_ecs_get_component(light, DM_COMPONENT_LIGHT_SRC);
    
    float scaling = (dm_sin(dm_get_time()) + 1) / 0.5f;
    //float scaling = dm_sin(dm_get_time());
    light_src->ambient = dm_vec4_set(scaling * 2, scaling * 0.7f, scaling * 1.3f, 1.0f);
    light_src->ambient = dm_vec4_scale(light_src->ambient, 0.5f * 0.2f);
    light_src->diffuse = dm_vec4_set(scaling * 2, scaling * 0.7f, scaling * 1.3f, 1.0f);
    light_src->diffuse = dm_vec4_scale(light_src->diffuse, 0.5f);
    */
    // increase angle
    angle += delta_time;
    
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