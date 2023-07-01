#define DM_IMPL
#include "dm.h"

typedef enum error_code_t
{
    ERROR_CODE_SUCCESS,
    ERROR_CODE_INIT_FAIL,
    ERROR_CODE_RESOURCE_CREATION_FAIL,
    ERROR_CODE_UPDATE_BEGIN_FAIL,
    ERROR_CODE_UPDATE_END_FAIL,
    ERROR_CODE_RENDER_BEGIN_FAIL,
    ERROR_CODE_RENDER_END_FAIL,
    ERROR_CODE_APP_UPDATE_FAIL,
    ERROR_CODE_APP_RENDER_FAIL,
    ERROR_CODE_UNKNOWN
} error_code;

int main(int argc, char** argv)
{
    error_code e = ERROR_CODE_SUCCESS;
    
    dm_context* context = dm_init(100,100,800,800,"test");
    
    if(!context) return ERROR_CODE_INIT_FAIL;
    
    // testing data
    float vertices[] = {
        -0.1f,-0.99f,0,
        0.45f,-0.67f,0,
        0,0.5f,0
    };
    
    uint32_t indices[] = {
        0,1,2
    };
    
    typedef struct uniform_t
    {
        dm_mat4 view;
    } uniform;
    
    dm_vertex_attrib_desc attrib_descs[] = {
        { .name="POSITION", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(float) * 3, .offset=0, .count=3, .index=0, .normalized=false },
        { .name="MODEL", .data_t=DM_VERTEX_DATA_T_MATRIX_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_INSTANCE, .stride=sizeof(dm_mat4), .offset=0, .count=4, .index=0, .normalized=false},
    };
    
    dm_pipeline_desc pipeline_desc = { 0 };
    pipeline_desc.cull_mode = DM_CULL_BACK;
    pipeline_desc.winding_order = DM_WINDING_COUNTER_CLOCK;
    pipeline_desc.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;
    
    pipeline_desc.depth = true;
    pipeline_desc.depth_comp = DM_COMPARISON_LESS;
    
    pipeline_desc.blend = true;
    pipeline_desc.blend_eq = DM_BLEND_EQUATION_ADD;
    pipeline_desc.blend_src_f = DM_BLEND_FUNC_SRC_ALPHA;
    pipeline_desc.blend_dest_f = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
    
    // resources
    dm_render_handle vb, instb, ib, shader, pipe, view;
    
    //if(!dm_renderer_create_static_vertex_buffer(vertices, sizeof(vertices), sizeof(float) * 3, &vb, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
    if(!dm_renderer_create_dynamic_vertex_buffer(vertices, sizeof(vertices), sizeof(float) * 3, &vb, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
    if(!dm_renderer_create_dynamic_vertex_buffer(NULL, sizeof(dm_mat4), sizeof(dm_mat4), &instb, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
    if(!dm_renderer_create_static_index_buffer(indices, sizeof(uint32_t) * 3, &ib, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
    if(!dm_renderer_create_uniform(sizeof(uniform), DM_UNIFORM_STAGE_VERTEX, &view, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
    
#ifdef DM_OPENGL
    dm_render_handle vbs[] = { vb };
    if(!dm_renderer_create_shader("assets/shaders/test_vertex.glsl", "assets/shaders/test_pixel.glsl", vbs, DM_ARRAY_LEN(vbs), attrib_descs, DM_ARRAY_LEN(attrib_descs), &shader, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
#elif defined(DM_DIRECTX)
    if(!dm_renderer_create_shader("assets/shaders/test_vertex.fxc", "assets/shaders/test_pixel.fxc", attrib_descs, DM_ARRAY_LEN(attrib_descs), &shader, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
#elif defined(DM_METAL)
    if(!dm_renderer_create_shader("assets/shaders/test.metallib", attrib_descs, DM_ARRAY_LEN(attrib_descs), &shader, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
#endif
    
#ifdef DM_METAL
    if(!dm_renderer_create_pipeline(pipeline_desc, shader, &pipe, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
#else
    if(!dm_renderer_create_pipeline(pipeline_desc, &pipe, context)) e = ERROR_CODE_RESOURCE_CREATION_FAIL;
#endif
    
    if(e != ERROR_CODE_SUCCESS)
    {
        dm_shutdown(context);
        getchar();
        return e;
    }
    
    dm_vec3 obj_p = {0,0,0};
    dm_vec3 obj_s = {1,1,1};
    dm_quat obj_r = {0,0,0,1};
    
    dm_vec3 camera_p = {0,0,0};
    
    dm_mat4 ortho = dm_mat_ortho(-1,1,-1,1,-1.0f,1.0f);
    
    float t = 0;
    float move_speed = 0.1f;
    while(context->is_running)
    {
        // updating
        if(!dm_update_begin(context)) e = ERROR_CODE_UPDATE_BEGIN_FAIL;
        if(!dm_update_end(context)) e = ERROR_CODE_UPDATE_END_FAIL;
        
        if(dm_input_is_key_pressed(DM_KEY_A, context)) camera_p.x += move_speed;
        else if(dm_input_is_key_pressed(DM_KEY_D, context)) camera_p.x -= move_speed;
        
        if(dm_input_is_key_pressed(DM_KEY_W, context)) camera_p.y -= move_speed;
        if(dm_input_is_key_pressed(DM_KEY_S, context)) camera_p.y += move_speed;
        
        // obj model matrix
        dm_mat4 obj_m = dm_mat4_identity();
        obj_m = dm_mat_scale(obj_m, obj_s);
        obj_m = dm_mat4_mul_mat4(obj_m, dm_mat4_rotate_from_quat(obj_r));
        obj_m = dm_mat_translate(obj_m, obj_p);
        
        // camera view matrix
        dm_mat4 uni = dm_mat4_mul_mat4(dm_mat_translate(dm_mat4_identity(), camera_p), ortho);
        
#ifdef DM_DIRECTX
        uni = dm_mat4_transpose(uni);
        obj_m = dm_mat4_transpose(obj_m);
#endif
        
        // rendering
        if(!dm_renderer_begin_frame(context)) e = ERROR_CODE_RENDER_BEGIN_FAIL;
        
        // testing
        dm_render_command_clear(1,0.1f,0.4f,1,context);
        
        dm_render_command_bind_shader(shader, context);
        dm_render_command_bind_pipeline(pipe, context);
        dm_render_command_bind_buffer(vb, 0, context);
        dm_render_command_update_buffer(vb, &vertices, sizeof(vertices), 0, context);
        dm_render_command_bind_buffer(instb, 1, context);
        dm_render_command_update_buffer(instb, &obj_m, sizeof(dm_mat4), 0, context);
        dm_render_command_bind_uniform(view, 0, DM_UNIFORM_STAGE_VERTEX, 0, context);
        dm_render_command_update_uniform(view, &uni, sizeof(uni), context);
        
        dm_render_command_bind_buffer(ib, 0, context);
        dm_render_command_draw_indexed(3,0,0, context);
        //dm_render_command_draw_arrays(0,3,context);
        
        if(!dm_renderer_end_frame(true, context)) e = ERROR_CODE_RENDER_END_FAIL;
    }
    
    // cleanup
    dm_shutdown(context);
    
    // TODO: remove
    getchar();
    
    return e;
}