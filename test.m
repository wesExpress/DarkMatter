#define DM_IMPLEMENTATION
#include "dm.h"

#include <stdio.h>

typedef struct vertex_t
{
    float position[4];
    float color[4];
} vertex;

int main()
{
    dm_window window = { 0 }; 
    if(!dm_window_create(0,0,1280,720, "test", DM_WINDOW_CREATE_FLAG_CENTER, &window)) return 0;

    dm_renderer renderer = { 0 };
    if(!dm_renderer_init(window, &renderer)) return -1;

    dm_renderpass_handle pass;
    dm_pipeline_handle pipeline;

    dm_renderpass_desc pass_desc = { .type=DM_RENDERPASS_TYPE_DEFAULT };
    if(!dm_renderer_create_renderpass(pass_desc, &pass, &renderer)) goto END;

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

    if(!dm_renderer_create_raster_pipeline(pipe_desc, &pipeline, &renderer)) goto END;

    vertex vertices[] = {
        { { -0.5f,-0.5f,0.f }, {1,0,0,1} },
        { {  0.5f,-0.5f,0.f }, {0,1,0,1} },
        { {  0.f,  0.5f,0.f }, {0,0,1,1} },
    };

    dm_resource_handle vb;

    dm_vertex_buffer_desc vb_desc = {
        .size=sizeof(vertices), .stride=sizeof(vertex),
        .data=vertices
    };

    if(!dm_renderer_create_vertex_buffer(vb_desc, &vb, &renderer)) goto END;

    //
    dm_timer timer = { 0 };
    uint16_t frame_count = 0;
    double frame_time = 0;

    dm_timer_start(&timer);

    while(true)
    {
        dm_timer frame_timer = { 0 };
        dm_timer_start(&frame_timer);

        if(!dm_window_poll_events(&window)) break;
        if(dm_window_should_close(window)) break;
        if(dm_input_is_key_pressed(DM_KEY_ESCAPE, window)) { dm_log(DM_LOG_WARN, "window closed"); break; }

        if(dm_window_resized(window))
        {
            if(!dm_renderer_resize(window, &renderer)) { dm_log(DM_LOG_FATAL, "Window resize failed"); break; }
        }

        if(!dm_renderer_begin_frame(&renderer)) { dm_log(DM_LOG_FATAL, "begin frame failed"); break; }

        dm_render_command_begin_render_pass(pass, 0.1f,0.3f,0.7f,1,1, &renderer);
            dm_render_command_bind_raster_pipeline(pipeline, &renderer);
            dm_render_command_bind_vertex_buffer(vb, 0, 0, &renderer);
            dm_render_command_draw_instanced(1,0,3,0, &renderer);
        dm_render_command_end_render_pass(pass, &renderer);

        if(!dm_renderer_submit_render_commands(&renderer)) { dm_log(DM_LOG_FATAL, "submit commands failed"); break; }
        if(!dm_renderer_end_frame(false, &renderer))   { dm_log(DM_LOG_FATAL, "end frame failed"); break; }

        if(dm_timer_elapsed(&timer) >= 1)
        {
            dm_log(DM_LOG_TRACE, "FPS: %d", frame_count);
            dm_timer_start(&timer);
            frame_count = 0;
        }
        else frame_count++;

        frame_time = dm_timer_elapsed(&frame_timer);
    }

END:
    dm_renderer_shutdown(&renderer);
    dm_window_shutdown(&window);

    return 0;
}
