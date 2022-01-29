#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "platform/dm_platform.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct dm_metal_renderer
{
    dm_metal_view* view;
} dm_metal_renderer;

static dm_metal_renderer* internal_renderer = NULL;

/*
// render functions
*/
bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing Metal render backend...");

    internal_renderer = (dm_metal_renderer*)dm_alloc(sizeof(dm_metal_renderer), DM_MEM_RENDERER);

    @autoreleasepool
    {
        internal_renderer->view = [[dm_metal_view alloc] init];
    }

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_free(internal_renderer, sizeof(dm_metal_renderer), DM_MEM_RENDERER);
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

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, void* mvp_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
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

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count)
{

}

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{

}

bool dm_renderer_update_buffer(dm_buffer* cb, void* data, size_t data_size)
{
    return true;
}

bool dm_renderer_bind_constant_buffer(dm_buffer* buffer)
{
    return true;
}

#endif