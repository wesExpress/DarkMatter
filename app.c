#include "app.h"
#include "render_pass.h"

static float zoom_levels[] = {
    0.1f,0.25f,0.5f,0.75f
};
static float move_speeds[] = {
    1,2,4,10
};

static int zoom_index = 0;

dm_entity create_entity(dm_context* context)
{
    dm_entity entity = dm_ecs_create_entity(context);
    
    dm_component_transform t = { 0 };
    
    float w = (float)DM_SCREEN_WIDTH(context) * 0.5f;
    float h = (float)DM_SCREEN_HEIGHT(context) * 0.5f;
    
    t.pos[0] = dm_random_float(context) * w * 2 - w;
    t.pos[1] = dm_random_float(context) * h * 2 - h;
    
    t.scale[0] = dm_random_float_range(0.5,3,context);
    t.scale[1] = dm_random_float_range(0.5,3,context);
    t.scale[2] = dm_random_float_range(0.5,3,context);
    
    t.rot[0] = 0;
    t.rot[1] = 0;
    t.rot[2] = 0;
    t.rot[3] = 1;
    
    dm_ecs_entity_add_transform(entity, t, context);
    //dm_ecs_entity_add_box_collider(entity, (float[]){0,0,0}, t.scale, context);
    
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
        
        float w = DM_SCREEN_WIDTH(context)  * 0.5f * app_data->camera.zoom;
        float h = DM_SCREEN_HEIGHT(context) * 0.5f * app_data->camera.zoom;
        
        dm_mat_ortho(-w,w,-h,h, -1,1, app_data->camera.proj);
        
        app_data->camera.zoom = zoom_levels[zoom_index];
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
    
    // scroll
    {
        if(dm_input_mouse_has_scrolled(context)) 
        { 
            zoom_index += dm_input_get_mouse_scroll(context);
            zoom_index = DM_CLAMP(zoom_index, 0, DM_ARRAY_LEN(zoom_levels)-1);
            app_data->camera.zoom = zoom_levels[zoom_index];
        }
    }
    
    // move
    {
        float move[3] = { 0 };
        
        if(dm_input_is_key_pressed(DM_KEY_A, context))      move[0] = 1;
        else if(dm_input_is_key_pressed(DM_KEY_D, context)) move[0] = -1;
        
        if(dm_input_is_key_pressed(DM_KEY_W, context))      move[1] = -1;
        else if(dm_input_is_key_pressed(DM_KEY_S, context)) move[1] = 1;
        
        dm_vec3_norm(move, move);
        dm_vec3_scale(move, move_speeds[zoom_index], move);
        
        dm_vec3_add_vec3(app_data->camera.pos, move, app_data->camera.pos);
    }
    
    // camera ortho
    float w = DM_SCREEN_WIDTH(context)  * 0.5f * app_data->camera.zoom;
    float h = DM_SCREEN_HEIGHT(context) * 0.5f * app_data->camera.zoom;
    
    dm_mat_ortho(-w,w,-h,h, -1,1, app_data->camera.proj);
    
    // submit entities
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