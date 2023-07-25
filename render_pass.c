#include "render_pass.h"
#include "app.h"

typedef struct vertex_t
{
    float pos[N3];
} vertex;

typedef struct inst_vertex_t
{
    float model[M4];
    float color[N4];
} inst_vertex;

typedef struct uniform_t
{
    float view_proj[M4];
} uniform;

#define MAX_ENTITIES_PER_FRAME MAX_ENTITIES
typedef struct render_pass_data_t
{
    dm_render_handle vb, instb, ib, shader, pipe, uni;
    basic_camera     camera;
    
    uint32_t         entity_count, instance_count;
    
    dm_entity        entities[MAX_ENTITIES_PER_FRAME];
    inst_vertex      insts[MAX_ENTITIES_PER_FRAME];
} render_pass_data;

bool render_pass_init(dm_context* context)
{
    application_data* app_data = context->app_data;
    app_data->render_pass_data = dm_alloc(sizeof(render_pass_data));
    render_pass_data* pass_data = app_data->render_pass_data;
    
    {
        vertex vertices[] = {
            { -0.5f,-0.5f,0 },
            { 0.5f,-0.5f,0 },
            { 0.5f,0.5f,0 },
            { -0.5f,0.5f,0 },
        };
        
        uint32_t indices[] = {
            0,1,2,
            2,3,0
        };
        
        dm_vertex_attrib_desc attrib_descs[] = {
            { .name="POSITION", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(vertex), .offset=offsetof(vertex, pos), .count=3, .index=0, .normalized=false },
            { .name="MODEL", .data_t=DM_VERTEX_DATA_T_MATRIX_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_INSTANCE, .stride=sizeof(inst_vertex), .offset=offsetof(inst_vertex, model), .count=4, .index=0, .normalized=false},
            { .name="COLOR", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_INSTANCE, .stride=sizeof(inst_vertex), .offset=offsetof(inst_vertex, color), .count=4, .index=0, .normalized=false }
        };
        
        dm_pipeline_desc pipeline_desc = dm_renderer_default_pipeline();
        
        // resources
        if(!dm_renderer_create_static_vertex_buffer(vertices, sizeof(vertices), sizeof(vertex), &pass_data->vb, context)) return false;
        if(!dm_renderer_create_dynamic_vertex_buffer(NULL, sizeof(inst_vertex) * MAX_ENTITIES_PER_FRAME, sizeof(inst_vertex), &pass_data->instb, context)) return false;
        if(!dm_renderer_create_static_index_buffer(indices, sizeof(indices), &pass_data->ib, context)) return false;
        if(!dm_renderer_create_uniform(sizeof(uniform), DM_UNIFORM_STAGE_VERTEX, &pass_data->uni, context)) return false;
        
        dm_shader_desc shader_desc = { 0 };
#ifdef DM_VULKAN
        strcpy(shader_desc.vertex, "assets/shaders/test_vertex.spv");
        strcpy(shader_desc.pixel, "assets/shaders/test_pixel.spv");
#elif defined(DM_OPENGL)
        strcpy(shader_desc.vertex, "assets/shaders/test_vertex.glsl");
        strcpy(shader_desc.pixel, "assets/shaders/test_pixel.glsl");
#elif defined(DM_DIRECTX)
        strcpy(shader_desc.vertex, "assets/shaders/test_vertex.fxc");
        strcpy(shader_desc.pixel, "assets/shaders/test_pixel.fxc");
#else
        strcpy(shader_desc.vertex, "vertex_main");
        strcpy(shader_desc.pixel, "fragment_main");
        strcpy(shader_desc.master, "assets/shaders/test.metallib");
#endif
        
        if(!dm_renderer_create_shader_and_pipeline(shader_desc, pipeline_desc, attrib_descs, DM_ARRAY_LEN(attrib_descs), &pass_data->shader, &pass_data->pipe, context)) return false;
    }
    
    return true;
}

void render_pass_shutdown(dm_context* context)
{
    application_data* app_data = context->app_data;
    dm_free(app_data->render_pass_data);
}

void render_pass_submit_entity(dm_entity entity, dm_context* context)
{
    application_data* app_data = context->app_data;
    render_pass_data* pass_data = app_data->render_pass_data;
    
    pass_data->entities[pass_data->entity_count++] = entity;
}

bool render_pass_render(dm_context* context)
{
    application_data* app_data = context->app_data;
    render_pass_data* pass_data = app_data->render_pass_data;
    
    float obj_rm[M4];
    dm_component_transform transform = { 0 };
    inst_vertex* inst = NULL;
    
    for(uint32_t i=0; i<pass_data->entity_count; i++)
    {
        transform = dm_ecs_entity_get_transform(pass_data->entities[i], context);
        inst = &pass_data->insts[i];
        
        dm_mat4_rotate_from_quat(transform.rot, obj_rm);
        
        dm_mat_scale_make(transform.scale, inst->model);
        dm_mat4_mul_mat4(inst->model, obj_rm, inst->model);
        dm_mat_translate(inst->model, transform.pos, inst->model);
#ifdef DM_DIRECTX
        dm_mat4_transpose(inst->model, inst->model);
#endif
        
        inst->color[0] = 1; 
        inst->color[1] = 1; 
        inst->color[2] = 1; 
        inst->color[3] = 1;
        
        pass_data->instance_count++;
    }
    
    // uniform
    uniform uni = { 0 };
    
    dm_mat4_identity(uni.view_proj);
    dm_mat_translate(uni.view_proj, app_data->camera.pos,  uni.view_proj);
    dm_mat4_mul_mat4(uni.view_proj, app_data->camera.proj, uni.view_proj);
    
#ifdef DM_DIRECTX
    dm_mat4_transpose(uni.view_proj, uni.view_proj);
#endif
    
    // render
#if 1
    dm_render_command_set_default_viewport(context);
    dm_render_command_clear(0.1f,0.3f,0.5f,1,context);
    
    dm_render_command_bind_shader(pass_data->shader, context);
    dm_render_command_bind_pipeline(pass_data->pipe, context);
    dm_render_command_bind_buffer(pass_data->vb, 0, context);
    dm_render_command_bind_buffer(pass_data->instb, 1, context);
    dm_render_command_update_buffer(pass_data->instb, pass_data->insts, sizeof(pass_data->insts), 0, context);
    dm_render_command_bind_uniform(pass_data->uni, 0, DM_UNIFORM_STAGE_VERTEX, 0, context);
    dm_render_command_update_uniform(pass_data->uni, &uni, sizeof(uni), context);
    
    dm_render_command_bind_buffer(pass_data->ib, 0, context);
    dm_render_command_draw_instanced(6,pass_data->instance_count,0,0,0, context);
#endif
    
    // reset counts back to 0
    pass_data->entity_count = 0;
    pass_data->instance_count = 0;
    
    return true;
}