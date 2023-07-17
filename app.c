#include "app.h"
#include "render_pass.h"

bool app_init(dm_context* context)
{
    context->app_data = dm_alloc(sizeof(application_data));
    application_data* app_data = context->app_data;
    
    // entities
    {
        app_data->entities[0] = dm_ecs_create_entity(context);
        app_data->entities[1] = dm_ecs_create_entity(context);
        
        dm_component_transform box_transform = {
            { 0,5,0 },
            { 1,1,1 },
            { 0,0,0,1 }
        };
        
        dm_component_transform box2_transform = {
            { 2,-5,0 },
            { 2,2,2 },
            { 0,0,0,1 }
        };
        
        dm_ecs_entity_add_transform(app_data->entities[0], box_transform, context);
        dm_ecs_entity_add_transform(app_data->entities[1], box2_transform, context);
        
        dm_ecs_entity_add_box_collider(app_data->entities[0], (float[]){0,0,0}, (float[]){1,1,1}, context);
        dm_ecs_entity_add_box_collider(app_data->entities[1], (float[]){0,0,0}, (float[]){2,2,2}, context);
    }
    
    // camera
    {
        app_data->camera.pos[0] = 0; 
        app_data->camera.pos[1] = 0; 
        app_data->camera.pos[2] = 0;
        dm_mat_ortho(-10,10,-10,10, -1,1, app_data->camera.proj);
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
    
    // submit entities
    {
        render_pass_submit_entity(app_data->entities[0], context);
        render_pass_submit_entity(app_data->entities[1], context);
    }
    
    return true;
}

bool app_render(dm_context* context)
{
    if(!render_pass_render(context)) return false;
    
    return true;
}