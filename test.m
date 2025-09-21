#define DM_IMPLEMENTATION
#include "dm.h"

#include <stdio.h>

typedef enum exit_code_t
{
    EXIT_CODE_SUCCESS = 0,
    EXIT_CODE_WINDOW_CLOSE = 0,
    EXIT_CODE_INIT_FAIL = -1,
    EXIT_CODE_RESOURCE_CREATE_FAIL = -2,
    EXIT_CODE_RESIZE_FAIL = -3,
    EXIT_CODE_RENDER_FAIL = -4
} exit_code;

typedef struct vertex_t
{
    float position[4];
    float color[4];
} vertex;

typedef struct simple_camera_t
{
    mat4 ortho;
    vec3 position;
} simple_camera;

#ifdef DM_METAL
typedef uint64_t descriptor_index;
#else
typedef uint32_t descriptor_index;
#endif

typedef struct render_resources_t
{
    descriptor_index constant_buffer;
} render_resources;

typedef struct application_t
{
    dm_renderer renderer;
    dm_window window;

    dm_renderpass_handle pass;
    dm_pipeline_handle pipeline;
    dm_resource_handle vb, ib, cb;

    render_resources resources;

    simple_camera camera;
} application;

bool app_init(application* app)
{
    if(!dm_window_create(0,0,1280,720, "test", DM_WINDOW_CREATE_FLAG_CENTER, &app->window)) return false;
    if(!dm_renderer_init(app->window, &app->renderer))
    {
        dm_window_shutdown(&app->window);
        return false;
    }
    
    return true;
}

void app_shutdown(application* app)
{
    dm_renderer_shutdown(&app->renderer);
    dm_window_shutdown(&app->window);
}

bool create_resources(application* app)
{
    dm_renderpass_desc pass_desc = { .type=DM_RENDERPASS_TYPE_DEFAULT };
    if(!dm_renderer_create_renderpass(pass_desc, &app->pass, &app->renderer)) return false;

    dm_rasterizer_desc rasterizer = {
        .vertex_shader_desc.path="assets/shaders/vertex_shader.metallib",
        .pixel_shader_desc.path="assets/shaders/pixel_shader.metallib",
        .cull_mode=DM_RASTERIZER_CULL_MODE_BACK, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE,
        .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL
    };

    dm_raster_input_assembler_desc input_assembler = {
        .topology=DM_INPUT_TOPOLOGY_TRIANGLE_LIST
    };

    dm_raster_pipeline_desc pipe_desc = {
        .rasterizer=rasterizer, .input_assembler=input_assembler,
        .viewport={ .type=DM_VIEWPORT_TYPE_DEFAULT }, .scissor={ .type=DM_SCISSOR_TYPE_DEFAULT }
    };

    if(!dm_renderer_create_raster_pipeline(pipe_desc, &app->pipeline, &app->renderer)) return false;

    vertex vertices[] = {
        { { -0.5f,-0.5f,0.f }, {1,0,0,1} },
        { {  0.5f,-0.5f,0.f }, {0,1,0,1} },
        { {  0.f,  0.5f,0.f }, {0,0,1,1} },
    };

    uint32_t indices[] = {
        0,1,2
    };

    dm_resource_handle vb, ib;

    dm_vertex_buffer_desc vb_desc = {
        .size=sizeof(vertices), .stride=sizeof(vertex),
        .data=vertices
    };

    dm_index_buffer_desc ib_desc = {
        .size=sizeof(indices), .index_type=DM_INDEX_BUFFER_INDEX_TYPE_UINT32,
        .data=indices
    };

    dm_constant_buffer_desc cb_desc = {
        .size=sizeof(mat4),
        .data=app->camera.ortho
    };

    if(!dm_renderer_create_vertex_buffer(vb_desc, &app->vb, &app->renderer)) return false;
    if(!dm_renderer_create_index_buffer(ib_desc, &app->ib, &app->renderer)) return false;
    if(!dm_renderer_create_constant_buffer(cb_desc, &app->cb, &app->renderer)) return false;

    //
    if(!dm_renderer_finish_init(&app->renderer)) return false;

    return true;
}

exit_code app_run(application* app)
{
    dm_timer timer = { 0 };
    uint16_t frame_count = 0;
    double frame_time = 0;

    dm_timer_start(&timer);

    exit_code code = EXIT_CODE_SUCCESS;

    while(true)
    {
        dm_timer frame_timer = { 0 };
        dm_timer_start(&frame_timer);

        // input polling
        if(!dm_window_poll_events(&app->window)) break;
        if(dm_window_should_close(app->window)) break;
        if(dm_input_is_key_pressed(DM_KEY_ESCAPE, app->window)) { dm_log(DM_LOG_WARN, "window closed"); code=EXIT_CODE_WINDOW_CLOSE; break; }

        // window resizing
        if(dm_window_resized(app->window))
        {
            if(!dm_renderer_resize(app->window, &app->renderer)) { dm_log(DM_LOG_FATAL, "Window resize failed"); code=EXIT_CODE_RESIZE_FAIL; break; }
        }

        // rendering
        if(!dm_renderer_begin_frame(&app->renderer)) { dm_log(DM_LOG_FATAL, "begin frame failed"); code=EXIT_CODE_RENDER_FAIL; break; }

        app->resources.constant_buffer = app->cb.gpu_address;

        glm_mat4_identity(app->camera.ortho);

        dm_render_command_begin_update(&app->renderer);
            dm_render_command_update_constant_buffer(app->cb, &app->camera.ortho, sizeof(mat4), 0, &app->renderer);
        dm_render_command_end_update(&app->renderer);

        dm_render_command_begin_render_pass(app->pass, 0.5f,0.7f,0.9f,1,1, &app->renderer);
            dm_render_command_bind_raster_pipeline(app->pipeline, &app->renderer);
            dm_render_command_submit_resources(&app->cb, 1, &app->renderer);
            dm_render_command_bind_vertex_buffer(app->vb, 0, 0, &app->renderer);
            dm_render_command_bind_index_buffer(app->ib, 0, &app->renderer);
            dm_render_command_draw_instanced_indexed(1,0,3,0,0, &app->renderer);
        dm_render_command_end_render_pass(app->pass, &app->renderer);

        if(!dm_renderer_submit_render_commands(&app->renderer)) { dm_log(DM_LOG_FATAL, "submit commands failed"); code=EXIT_CODE_RENDER_FAIL; break; }
        if(!dm_renderer_end_frame(false, &app->renderer))   { dm_log(DM_LOG_FATAL, "end frame failed"); code=EXIT_CODE_RENDER_FAIL; break; }

        // frame timing and fps
        if(dm_timer_elapsed(&timer) >= 1)
        {
            dm_log(DM_LOG_TRACE, "FPS: %d", frame_count);
            dm_timer_start(&timer);
            frame_count = 0;
        }
        else frame_count++;

        frame_time = dm_timer_elapsed(&frame_timer);
    }

    app_shutdown(app);

    return code;
}

/********
 * MAIN *
 ********/
int main()
{
    application app = { 0 };

    if(!app_init(&app)) return EXIT_CODE_INIT_FAIL;

    // camera
    app.camera.position[2] = 1;

    glm_ortho(0, dm_window_get_width(app.window), dm_window_get_height(app.window), 0, 0.1f, 100.f, app.camera.ortho);

    if(!create_resources(&app))
    {
        app_shutdown(&app);
        return EXIT_CODE_RESOURCE_CREATE_FAIL;
    }

    return app_run(&app);
}
