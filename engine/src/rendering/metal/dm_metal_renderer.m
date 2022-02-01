#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_metal_view.h"
#include "dm_metal_buffer.h"
#include "dm_metal_shader.h"
#include "dm_metal_texture.h"

#include "rendering/dm_image.h"

#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "core/dm_string.h"

#include "platform/dm_platform_apple.h"

#include "structures/dm_list.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

dm_metal_renderer* metal_renderer = NULL;

#define DM_METAL_BUFFER_ALIGNMENT 256

size_t dm_metal_align(size_t n, uint32_t alignment) 
{
    return ((n+alignment-1)/alignment)*alignment;
}

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

        metal_renderer->command_queue = [metal_renderer->device newCommandQueue];
        if(!metal_renderer->command_queue)
        {
            DM_LOG_FATAL("Could not create metal command queue!");
            return false;
        }

        // depth stencil
        MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
        depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
        depth_stencil_desc.depthWriteEnabled = YES;
        internal_pipe->depth_stencil = [metal_renderer->device newDepthStencilStateWithDescriptor:depth_stencil_desc];
        if(!internal_pipe->depth_stencil)
        {
            DM_LOG_FATAL("Could not create metal depth stencil state!");
            return false;
        }

        // sampler
        MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
        sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
        sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;
        sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
        sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;

        internal_pipe->sampler_state = [metal_renderer->device newSamplerStateWithDescriptor:sampler_desc];
        if(!internal_pipe->sampler_state)
        {
            DM_LOG_FATAL("Could not create metal sampler state!");
            return false;
        }
    }

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        // shader
        dm_metal_destroy_shader_library(pipeline->raster_desc.shader);
        
        // buffers
        dm_metal_destroy_buffer(pipeline->render_packet.vertex_buffer);
        dm_metal_destroy_buffer(pipeline->render_packet.index_buffer);
        dm_metal_destroy_buffer(pipeline->render_packet.mvp);

        // textures
        for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        {
            dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
            dm_image* image = dm_image_get(key->string);

            dm_metal_destroy_texture(image);
        }

        dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    }
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, void* mvp_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        dm_internal_pipeline* internal_pipe = pipeline->interal_pipeline;

        // shader
        if(!dm_metal_create_shader_library(pipeline->raster_desc.shader, @"shaders/metal/object_shader.metallib", metal_renderer)) return false;

        /// pipeline state
        dm_internal_shader* internal_shader = pipeline->raster_desc.shader->internal_shader;

        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
        pipe_desc.vertexFunction = internal_shader->vertex_func;
        pipe_desc.fragmentFunction = internal_shader->fragment_func;
        pipe_desc.colorAttachments[0].pixelFormat = metal_renderer->view.metal_layer.pixelFormat;

        internal_pipe->pipeline_state = [metal_renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!internal_pipe->pipeline_state)
        {
            DM_LOG_FATAL("Could not create metal pipeline state");
            return false;
        }

        // buffers
        if(!dm_metal_create_buffer(pipeline->render_packet.vertex_buffer, vb_data, metal_renderer)) return false;
        if(!dm_metal_create_buffer(pipeline->render_packet.index_buffer, ib_data, metal_renderer)) return false;

        // mvp uniform
        pipeline->render_packet.mvp->internal_buffer = dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
        dm_internal_buffer* internal_buffer = pipeline->render_packet.mvp->internal_buffer;

        dm_mat4* test = (dm_mat4*)mvp_data;
        internal_buffer->buffer = [metal_renderer->device newBufferWithLength: dm_metal_align(sizeof(dm_mat4), DM_METAL_BUFFER_ALIGNMENT)
                                                          options: MTLResourceOptionCPUCacheModeDefault];

        //if(!dm_metal_create_buffer(pipeline->render_packet.mvp, mvp_data, metal_renderer)) return false;

        // textures
        for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        {
            dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
            dm_image* image = dm_image_get(key->string);

            if(!dm_metal_create_texture(image, metal_renderer)) return false;
        }
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
        id<MTLCommandBuffer> commandBuffer = [metal_renderer->command_queue commandBuffer];

        // render pass descriptor
        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(metal_renderer->clear_color.x, metal_renderer->clear_color.y, metal_renderer->clear_color.z, metal_renderer->clear_color.w);

        id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

        // pipeline state
        [commandEncoder setRenderPipelineState:internal_pipe->pipeline_state];

        // depth stencil
        [commandEncoder setDepthStencilState:internal_pipe->depth_stencil]; 
        [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise]; 
        [commandEncoder setCullMode:MTLCullModeBack];

        // sampler
        [commandEncoder setFragmentSamplerState:internal_pipe->sampler_state atIndex:0];

        // buffers
        dm_internal_buffer* internal_vb = pipeline->render_packet.vertex_buffer->internal_buffer;
        dm_internal_buffer* internal_ib = pipeline->render_packet.index_buffer->internal_buffer;
        dm_internal_buffer* internal_mvp = pipeline->render_packet.mvp->internal_buffer;

        [commandEncoder setVertexBuffer:internal_vb->buffer offset:0 atIndex:0];

        NSUInteger uniform_offset = 0;
        [commandEncoder setVertexBuffer:internal_mvp->buffer offset:uniform_offset atIndex:1];

        // textures
        for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        {
            dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
            dm_image* image = dm_image_get(key->string);
            dm_internal_texture* internal_texture = image->internal_texture;

            [commandEncoder setFragmentTexture:internal_texture->texture atIndex:i];
        }

        // draw call
        [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle 
                        indexCount: [internal_ib->buffer length] / sizeof(dm_index_t)
                        indexType: MTLIndexTypeUInt32
                        indexBuffer: internal_ib->buffer
                        indexBufferOffset: 0];

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