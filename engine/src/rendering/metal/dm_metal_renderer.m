#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "core/dm_assert.h"
#include "core/dm_mem.h"

#include "platform/dm_platform_apple.h"

#include "dm_metal_view.h"

#include "dm_metal_view.h"
#include "dm_metal_buffer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

dm_metal_renderer* metal_renderer = NULL;

demo_vertex test_vertices[] = {
    {.position={-1, 1, 1,1}, .color={0,1,1,1}}, 
    {.position={-1,-1, 1,1}, .color={0,0,1,1}}, 
    {.position={ 1,-1, 1,1}, .color={1,0,1,1}}, 
    {.position={ 1, 1, 1,1}, .color={1,1,1,1}}, 
    {.position={-1, 1,-1,1}, .color={0,1,0,1}}, 
    {.position={-1,-1,-1,1}, .color={0,0,0,1}}, 
    {.position={ 1,-1,-1,1}, .color={1,0,0,1}}, 
    {.position={ 1, 1,-1,1}, .color={1,1,0,1}}
};

dm_index_t test_indices[] = {
    3,2,6,6,7,3, 
    4,5,1,1,0,4, 
    4,0,3,3,7,4,
    1,5,6,6,2,1, 
    0,1,2,2,3,0, 
    7,6,5,5,4,7
};

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing Metal render backend...");

    @autoreleasepool
    {
        dm_internal_data* internal_data = platform_data->internal_data;
        metal_renderer = dm_alloc(sizeof(dm_metal_renderer), DM_MEM_RENDERER);
        
        metal_renderer->device =  MTLCreateSystemDefaultDevice();

        metal_renderer->view = [[dm_metal_view alloc] init];
        if(!metal_renderer->view)
        {
            DM_LOG_FATAL("Could not create metal view!");
            return false;
        }
        metal_renderer->view.metal_layer.device = metal_renderer->device;
        metal_renderer->view.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        // set the frame for view and layer in view
        NSRect frame = [internal_data->content_view getWindowFrame];
        [metal_renderer->view setFrame: frame];
        [metal_renderer->view.metal_layer setFrame: frame];

        // content view is the main view for our NSWindow
        // must add our view to the subviews
        [internal_data->content_view addSubview: metal_renderer->view];

        // must set the content view's layer to our metal layer
        [internal_data->content_view setWantsLayer: YES];
        [internal_data->content_view setLayer: metal_renderer->view.metal_layer];

        // TODO: hack to get clear color for now
        metal_renderer->clear_color = renderer_data->clear_color;
    }

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_free(metal_renderer, sizeof(dm_metal_renderer), DM_MEM_RENDERER);
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{
    @autoreleasepool
    {
        //[metal_renderer->view redrawWithColor:renderer_data->clear_color];
    }
}

bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        pipeline->interal_pipeline = dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
        dm_internal_pipeline* internal_pipe = pipeline->interal_pipeline;

        //internal_pipe->pipeline_state = 

        id<MTLLibrary> library = [metal_renderer->device newLibraryWithFile:@"shaders/metal/shaders.metallib" error:NULL];

        id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragFunc = [library newFunctionWithName:@"fragment_main"];

        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
        pipe_desc.vertexFunction = vertexFunc;
        pipe_desc.fragmentFunction = fragFunc;
        pipe_desc.colorAttachments[0].pixelFormat = metal_renderer->view.metal_layer.pixelFormat;

        internal_pipe->pipeline_state = [metal_renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];

        metal_renderer->command_queue = [metal_renderer->device newCommandQueue];
    }

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        //dm_metal_destroy_buffer(pipeline->render_packet.vertex_buffer);
        //dm_metal_destroy_buffer(pipeline->render_packet.index_buffer);

        dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    }
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, void* mvp_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        // buffers
        dm_internal_pipeline* internal_pipe = pipeline->interal_pipeline;

        internal_pipe->vertex_buffer = [metal_renderer->device newBufferWithBytes:test_vertices
                                                               length:sizeof(test_vertices)
                                                               options:MTLResourceOptionCPUCacheModeDefault];

        internal_pipe->index_buffer = [metal_renderer->device newBufferWithBytes:test_indices
                                                              length:sizeof(test_indices)
                                                              options:MTLResourceOptionCPUCacheModeDefault];

        //if(!dm_metal_create_buffer(pipeline->render_packet.vertex_buffer, vb_data, metal_renderer)) return false;
        //if(!dm_metal_create_buffer(pipeline->render_packet.index_buffer, ib_data, metal_renderer)) return false;
    }

    return true;
}

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline)
{
    id<CAMetalDrawable> drawable = [metal_renderer->view.metal_layer nextDrawable];

    if(drawable)
    {
        dm_internal_pipeline* internal_pipe = pipeline->interal_pipeline;

        id<MTLTexture> texture = drawable.texture;

        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(metal_renderer->clear_color.x, metal_renderer->clear_color.y, metal_renderer->clear_color.z, metal_renderer->clear_color.w);

        id<MTLCommandBuffer> commandBuffer = [metal_renderer->command_queue commandBuffer];

        id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

        [commandEncoder setRenderPipelineState:internal_pipe->pipeline_state];
        [commandEncoder setVertexBuffer:internal_pipe->vertex_buffer offset:0 atIndex:0];
        [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [commandEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
    else
    {
        DM_LOG_ERROR("Drawable was null");
    }
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