#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_metal_view.h"

#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "core/math/dm_math.h"

#include "platform/dm_platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

@class dm_app_delegate;
@class dm_window_delegate;
@class dm_input_view;

typedef struct dm_internal_data
{
    NSWindow* window;
    dm_app_delegate* app_delegate;
    dm_window_delegate* window_delegate;
    dm_input_view* input_view;
} dm_internal_data;

typedef struct dm_metal_renderer
{
    dm_metal_view* view;
} dm_metal_renderer;

dm_metal_renderer* metal_renderer = NULL;

/*
// render functions
*/
bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing Metal render backend...");

    dm_internal_data* internal_data = platform_data->internal_data;
    
    @autoreleasepool
    {
        metal_renderer = dm_alloc(sizeof(dm_metal_renderer), DM_MEM_RENDERER);

        NSRect rect = [internal_data->window frame];

        metal_renderer->view = [[dm_metal_view alloc] initWithFrame: CGRectMake(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)];

        CAMetalLayer* metal_layer = (CAMetalLayer*)metal_renderer->view.layer;
        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
        id<MTLTexture> texture = drawable.texture;

        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(renderer_data->clear_color.x, renderer_data->clear_color.y, renderer_data->clear_color.z, renderer_data->clear_color.w);

        id<MTLCommandQueue> commandQueue = [metal_renderer->view.device newCommandQueue];

        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

        id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
        [commandEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_free(metal_renderer, sizeof(dm_metal_renderer), DM_MEM_RENDERER);
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