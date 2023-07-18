#include "app.h"
#include "render_pass.h"

#define CAMERA_SIZE 100

dm_entity create_entity(dm_context* context)
{
    dm_entity entity = dm_ecs_create_entity(context);
    
    dm_component_transform t = { 0 };
    
    t.pos[0] = dm_random_float(context) * CAMERA_SIZE * 2 - CAMERA_SIZE;
    t.pos[1] = dm_random_float(context) * CAMERA_SIZE * 2 - CAMERA_SIZE;
    //t.pos[2] = dm_random_float(context) * 20 - 10;
    
    t.scale[0] = dm_random_float_range(0.5,3,context);
    t.scale[1] = dm_random_float_range(0.5,3,context);
    t.scale[2] = dm_random_float_range(0.5,3,context);
    
    t.rot[0] = 0;
    t.rot[1] = 0;
    t.rot[2] = 0;
    t.rot[3] = 1;
    
    dm_ecs_entity_add_transform(entity, t, context);
    dm_ecs_entity_add_box_collider(entity, (float[]){0,0,0}, t.scale, context);
    
    return entity;
}

bool app_init(dm_context* context)
{
    context->app_data = dm_alloc(sizeof(application_data));
    application_data* app_data = context->app_data;
    
    // entities
    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        app_data->entities[i] = create_entity(context);
    }
    
    // camera
    {
        app_data->camera.pos[0] = 0; 
        app_data->camera.pos[1] = 0; 
        app_data->camera.pos[2] = 0;
        dm_mat_ortho(-CAMERA_SIZE,CAMERA_SIZE,-CAMERA_SIZE,CAMERA_SIZE, -1,1, app_data->camera.proj);
    }
    
    if(!render_pass_init(context)) return false;
    
    return true;
}

void app_shutdown(dm_context* context)
{
    render_pass_shutdown(context);
    dm_free(context->app_data);
}

bool app_update(dm_context* context)
{
    application_data* app_data = context->app_data;
    
    {
        static float move_speed = 0.5f;
        float move[3] = { 0 };
        
        if(dm_input_is_key_pressed(DM_KEY_A, context))      move[0] = 1;
        else if(dm_input_is_key_pressed(DM_KEY_D, context)) move[0] = -1;
        
        if(dm_input_is_key_pressed(DM_KEY_W, context))      move[1] = -1;
        else if(dm_input_is_key_pressed(DM_KEY_S, context)) move[1] = 1;
        
        dm_vec3_norm(move, move);
        dm_vec3_scale(move, move_speed, move);
        
        dm_vec3_add_vec3(app_data->camera.pos, move, app_data->camera.pos);
    }
    
    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        render_pass_submit_entity(app_data->entities[i], context);
    }
    
    return true;
}

bool app_render(dm_context* context)
{
    if(!render_pass_render(context)) return false;
    
    return true;
}