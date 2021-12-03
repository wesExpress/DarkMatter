#if DM_OPENGL

#include "dm_opengl_renderer.h"
#include "dm_logger.h"
#include "platform/dm_platform.h"

dm_renderer_data data = { 0 };

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_color clear_color)
{
    if (!dm_platform_init_opengl())
    {
        return false;
    }

    DM_INFO("OpenGL Info:");
    DM_INFO("       Vendor  : %s", glGetString(GL_VENDOR));
    DM_INFO("       Renderer: %s", glGetString(GL_RENDERER));
    DM_INFO("       Version : %s", glGetString(GL_VERSION));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, platform_data->window_width, platform_data->window_height);
    
    data.clear_color = clear_color;

    return true;
}

void dm_renderer_shutdown_impl()
{
    dm_platform_shutdown_opengl();
}

bool dm_renderer_resize_impl(int new_width, int new_height)
{
    return true;
}

void dm_renderer_begin_scene_impl()
{
    glClearColor(
        data.clear_color.x,
        data.clear_color.y,
        data.clear_color.z,
        data.clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void dm_renderer_end_scene_impl()
{
    dm_platform_swap_buffers();
}

#endif