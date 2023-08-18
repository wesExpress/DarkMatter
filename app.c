#include "app.h"
#include "components.h"
#include "render_pass.h"
#include "debug_render_pass.h"
#include "physics_system.h"

#define WORLD_SIZE_X 30
#define WORLD_SIZE_Y 30

dm_entity create_entity(application_data* app_data, dm_context* context)
{
    dm_entity entity = dm_ecs_create_entity(context);
    
    static const float w = (float)WORLD_SIZE_X * 0.5f;
    static const float h = (float)WORLD_SIZE_Y * 0.5f;
    
    float pos_x = dm_random_float(context) * w * 2 - w;
    float pos_y = dm_random_float(context) * h * 2 - h;
    float pos_z = dm_random_float(context) * h * 2 - h;
    
    float scale_x = dm_random_float_range(0.5,3,context);
    float scale_y = dm_random_float_range(0.5,3,context);
    float scale_z = dm_random_float_range(0.5,3,context);
    
    float rot_i = dm_random_float(context);
    float rot_j = dm_random_float(context);
    float rot_k = dm_random_float(context);
    float rot_r = dm_random_float(context);
    float mag   = dm_sqrtf(rot_i*rot_i + rot_j*rot_j + rot_k*rot_k + rot_r*rot_r);
    rot_i /= mag;
    rot_j /= mag;
    rot_k /= mag;
    rot_r /= mag;
    
    entity_add_transform(entity, app_data->components.transform, pos_x,pos_y,pos_z, scale_x,scale_y,scale_z, rot_i,rot_j,rot_k,rot_r, context);
    entity_add_box_collider(entity, app_data->components.collision, 0,0,0, scale_x,scale_y,scale_z, context);
    
    float dim[6] = {
        -scale_x * 0.5f,
        -scale_y * 0.5f,
        -scale_z * 0.5f,
        scale_x * 0.5f,
        scale_y * 0.5f,
        scale_z * 0.5f
    };
    
    entity_add_kinematics(entity, app_data->components.physics, dm_random_float(context) * 10, 0,0,0, 0,0, dim, context);
    entity_add_angular_velocity(entity, app_data->components.physics, dm_random_float(context),dm_random_float(context),dm_random_float(context), context);
    
    return entity;
}

void init_camera(const float pos[N3], const float forward[N3], float near_plane, float far_plane, float fov, uint32_t width, uint32_t height, float move_speed, float look_sens, basic_camera* camera)
{
    static const size_t vec3_size = sizeof(float) * 3;
    
    camera->fov        = fov;
    camera->near_plane = near_plane;
    camera->far_plane  = far_plane;
    camera->move_speed = move_speed;
    camera->look_sens  = look_sens;
    camera->width      = width;
    camera->height     = height;
    
    dm_memcpy(camera->pos,     pos,     vec3_size);
    dm_memcpy(camera->forward, forward, vec3_size);
    camera->up[0] = 0; camera->up[1] = 1; camera->up[2] = 0;
    
    float look[N3];
    dm_vec3_add_vec3(camera->pos, camera->forward, look);
    
    dm_mat_view(camera->pos, look, camera->up, camera->view);
    dm_mat4_inverse(camera->view, camera->inv_view);
    dm_mat_perspective(dm_deg_to_rad(camera->fov), (float)width / (float)height, camera->near_plane, camera->far_plane, camera->proj);
    dm_mat4_mul_mat4(camera->view, camera->proj, camera->view_proj);
    
    dm_vec3_cross(camera->forward, camera->up, camera->right);
}

void update_camera_view(basic_camera* camera)
{
    float look[3];
    dm_vec3_add_vec3(camera->pos, camera->forward, look);
    
    dm_vec3_cross(camera->right, camera->forward, camera->up);
    dm_vec3_norm(camera->up, camera->up);
    
    dm_mat_view(camera->pos, look, camera->up, camera->view);
    dm_mat4_inverse(camera->view, camera->inv_view);
    
    dm_mat4_mul_mat4(camera->view, camera->proj, camera->view_proj);
}

void update_camera_proj(basic_camera* camera)
{
    dm_mat_perspective(dm_deg_to_rad(camera->fov), (float)camera->width / (float)camera->height, camera->near_plane, camera->far_plane, camera->proj);
    
    dm_mat4_mul_mat4(camera->view, camera->proj, camera->view_proj);
}

void resize_camera(basic_camera* camera, dm_context* context)
{
    uint32_t width  = DM_SCREEN_WIDTH(context);
    uint32_t height = DM_SCREEN_HEIGHT(context);
    if((width == camera->width) && (height == camera->height)) return;
    
    camera->width = width;
    camera->height = height;
    
    update_camera_proj(camera);
}

void update_camera(basic_camera* camera, dm_context* context)
{
    static float up[] = { 0,1,0 };
    
    resize_camera(camera, context);
    
    bool moved = (dm_input_is_key_pressed(DM_KEY_A, context) || dm_input_is_key_pressed(DM_KEY_D, context) || dm_input_is_key_pressed(DM_KEY_W, context) || dm_input_is_key_pressed(DM_KEY_S, context) || dm_input_is_key_pressed(DM_KEY_Q, context) || dm_input_is_key_pressed(DM_KEY_E, context));
    
    int delta_x, delta_y;
    dm_input_get_mouse_delta(&delta_x, &delta_y, context);
    bool rotated = (delta_x != 0) || (delta_y != 0);
    
    // movement
    if(moved)
    {
        float speed         = camera->move_speed * context->delta;
        float delta_pos[N3] = { 0 };
        
        if(dm_input_is_key_pressed(DM_KEY_A, context))      delta_pos[0] = -1;
        else if(dm_input_is_key_pressed(DM_KEY_D, context)) delta_pos[0] =  1;
        
        if(dm_input_is_key_pressed(DM_KEY_W, context))      delta_pos[2] =  1;
        else if(dm_input_is_key_pressed(DM_KEY_S, context)) delta_pos[2] = -1;
        
        if(dm_input_is_key_pressed(DM_KEY_Q, context))      delta_pos[1] = -1;
        else if(dm_input_is_key_pressed(DM_KEY_E, context)) delta_pos[1] =  1;
        
        float move_vec[3], dum[3];
        
        dm_vec3_scale(camera->right, delta_pos[0], move_vec);
        dm_vec3_scale(camera->forward, delta_pos[2], dum);
        dm_vec3_add_vec3(move_vec, dum, move_vec);
        dm_vec3_scale(up, delta_pos[1], dum);
        dm_vec3_add_vec3(move_vec, dum, move_vec);
        
        dm_vec3_norm(move_vec, move_vec);
        dm_vec3_scale(move_vec, speed, move_vec);
        
        dm_vec3_add_vec3(camera->pos, move_vec, camera->pos);
    }
    
    // rotation
    if(rotated)
    {
        float delta_yaw   = (float)delta_x * camera->look_sens;
        float delta_pitch = (float)delta_y * camera->look_sens;
        
        float q1[4], q2[4], rot[4];
        
        dm_quat_from_axis_angle_deg(camera->right, -delta_pitch, q1);
        dm_quat_from_axis_angle_deg(up,            -delta_yaw,   q2);
        dm_quat_cross(q1, q2, rot);
        dm_quat_norm(rot, rot);
        
        dm_vec3_rotate(camera->forward, rot, camera->forward);
        dm_vec3_norm(camera->forward, camera->forward);
        
        dm_vec3_cross(camera->forward, up, camera->right);
    }
    
    if(moved || rotated) update_camera_view(camera);
}

bool app_init(dm_context* context)
{
    context->app_data = dm_alloc(sizeof(application_data));
    application_data* app_data = context->app_data;
    
    app_data->entity_count = 0;
    
    // rendering
    if(!render_pass_init(context))       return false;
    if(!debug_render_pass_init(context)) return false;
    
    // components
    app_data->components.transform = dm_ecs_register_component(sizeof(component_transform_block), context);
    if(app_data->components.transform==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register transform component"); return false; }
    
    app_data->components.collision = dm_ecs_register_component(sizeof(component_collision_block), context);
    if(app_data->components.collision==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register collision component"); return false; }
    
    app_data->components.physics = dm_ecs_register_component(sizeof(component_physics_block), context);
    if(app_data->components.physics==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register physics component"); return false; }
    
    // systems
    if(!physics_system_init(app_data->components.transform, app_data->components.collision, app_data->components.physics, context))    return false;
    
    // camera
    const float cam_pos[] = { -5,0,-5 };
    float cam_forward[] = { 1,0,1 };
    dm_vec3_norm(cam_forward, cam_forward);
    
    init_camera(cam_pos, cam_forward, 0.01f, 1000.0f, 75.0f, DM_SCREEN_WIDTH(context), DM_SCREEN_HEIGHT(context), 10.0f, 1.0f, &app_data->camera); 
    
    // entities
    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        app_data->entities[app_data->entity_count++] = create_entity(app_data, context);
    }
    
    return true;
}

void app_shutdown(dm_context* context)
{
    render_pass_shutdown(context);
    debug_render_pass_shutdown(context);
    
    dm_free(context->app_data);
}

bool app_update(dm_context* context)
{
    application_data* app_data = context->app_data;
    
    update_camera(&app_data->camera, context);
    
    // submit entities
    for(uint32_t i=0; i<app_data->entity_count; i++)
    {
        render_pass_submit_entity(app_data->entities[i], context);
    }
    
    return true;
}

bool app_render(dm_context* context)
{
    if(!render_pass_render(context))       return false;
    if(!debug_render_pass_render(context)) return false;
    
    return true;
}