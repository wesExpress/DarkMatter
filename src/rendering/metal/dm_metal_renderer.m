#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{

}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{

}

bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{

}

bool dm_renderer_init_pipeline_data_impl(dm_buffer_desc vb_desc, void* vb_data, dm_buffer_desc ib_desc, void* ib_data, dm_shader_desc vs_desc, dm_shader_desc ps_desc, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    return true;
}

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline)
{

}

void dm_renderer_end_rederpass_impl()
{

}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline)
{

}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{

}

void dm_renderer_draw_arrays_impl(int first, size_t count)
{

}

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{

}

#endif