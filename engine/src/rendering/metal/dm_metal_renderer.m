#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_metal_view.h"
#include "dm_metal_buffer.h"
#include "dm_metal_shader.h"
#include "dm_metal_texture.h"
#include "dm_metal_pipeline.h"
#include "dm_metal_render_pass.h"

#include "rendering/dm_image.h"

#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "core/dm_string.h"

#include "platform/dm_platform_apple.h"

#include "structures/dm_list.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

@implementation dm_metal_renderer

-(id) initWithFrame: (NSRect)frame
{
    self = [super init];

    if(self)
    {
        _device = MTLCreateSystemDefaultDevice();

        _view = [[dm_metal_view alloc] init];
        if(!_view)
        {
            DM_LOG_FATAL("Could not create metal view!");
            return NULL;
        }

        _view.metal_layer.device = _device;
        _view.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        [_view setFrame: frame];
        [_view.metal_layer setFrame: frame];

        _command_queue = [_device newCommandQueue];
        if(!_command_queue)
        {
            DM_LOG_FATAL("Could not create metal command queue!");
            return false;
        }
    }

    return self;
}

@end

#define DM_METAL_BUFFER_ALIGNMENT 256

size_t dm_metal_align(size_t n, uint32_t alignment) 
{
    return ((n+alignment-1)/alignment)*alignment;
}

dm_metal_renderer* metal_renderer = NULL;

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing Metal render backend...");

    @autoreleasepool
    {
        dm_internal_apple_data* internal_data = platform_data->internal_data;
        NSRect frame = [internal_data->content_view getWindowFrame];

        metal_renderer = [[dm_metal_renderer alloc] initWithFrame: frame];

        // content view is the main view for our NSWindow
        // must add our view to the subviews
        [internal_data->content_view addSubview: metal_renderer.view];

        // must set the content view's layer to our metal layer
        [internal_data->content_view setWantsLayer: YES];
        [internal_data->content_view setLayer: metal_renderer.view.metal_layer];

        // TODO: hack to get clear color for now
        metal_renderer.clear_color = renderer_data->clear_color;
    }

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    
}

bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data)
{
    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        pipeline->internal_pipeline = [[dm_metal_pipeline alloc] init];
        if(!pipeline->internal_pipeline) return false;        

        dm_metal_pipeline* internal_pipe = pipeline->internal_pipeline;
        if(![internal_pipe initPipeline: metal_renderer pipeline: pipeline]) return false;
    }

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {        
        // buffers
        dm_metal_destroy_buffer(pipeline->vertex_buffer);
        dm_metal_destroy_buffer(pipeline->index_buffer);
        dm_metal_destroy_buffer(pipeline->inst_buffer);

        // textures
        for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        {
            dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
            dm_image* image = dm_image_get(key->string);

            dm_metal_destroy_texture(image);
        }
    }
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        // buffers
        if(!dm_metal_create_buffer(pipeline->vertex_buffer, vb_data, metal_renderer)) return false;
        if(!dm_metal_create_buffer(pipeline->index_buffer, ib_data, metal_renderer)) return false;
        if(!dm_metal_create_buffer(pipeline->inst_buffer, NULL, metal_renderer)) return false;

        metal_renderer.index_buffer = pipeline->index_buffer->internal_buffer;

        // textures
        //for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        //{
        //    dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        //    dm_image* image = dm_image_get(key->string);
        //
        //    if(!dm_metal_create_texture(image, metal_renderer)) return false;
        //}
    }

    return true;
}

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass)
{
    @autoreleasepool
    {
        render_pass->internal_render_pass = [[dm_metal_render_pass alloc] initWithRendererAndPass: metal_renderer pass: render_pass];

        if(!render_pass->internal_render_pass) return false;
    }
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{

}

void dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
    @autoreleasepool
    {
        dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;
        internal_pass.drawable = [metal_renderer.view.metal_layer nextDrawable];

        if(internal_pass.drawable)
        {
            id<MTLTexture> texture = internal_pass.drawable.texture;
            metal_renderer.command_buffer = [metal_renderer.command_queue commandBuffer];

            // render pass descriptor
            MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
            passDescriptor.colorAttachments[0].texture = texture;
            passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(metal_renderer.clear_color.x, metal_renderer.clear_color.y, metal_renderer.clear_color.z, metal_renderer.clear_color.w);

            metal_renderer.command_encoder = [metal_renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];

            // sampler
            [metal_renderer.command_encoder setFragmentSamplerState:internal_pass.sampler_state atIndex:0];
        }
        else
        {
            DM_LOG_ERROR("Drawable was null");
        }
    }
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
    @autoreleasepool
    {
        dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;

        [metal_renderer.command_encoder endEncoding];

        [metal_renderer.command_buffer presentDrawable:internal_pass.drawable];
        [metal_renderer.command_buffer commit];

        [metal_renderer.command_encoder release];
        [metal_renderer.command_buffer release];
    }
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        dm_metal_pipeline* internal_pipe = pipeline->internal_pipeline;

         // pipeline state
        [metal_renderer.command_encoder setRenderPipelineState:internal_pipe.pipeline_state];

        // depth stencil
        [metal_renderer.command_encoder setDepthStencilState:internal_pipe.depth_stencil]; 
        [metal_renderer.command_encoder setFrontFacingWinding:MTLWindingCounterClockwise]; 
        [metal_renderer.command_encoder setCullMode:MTLCullModeBack];

        // buffers
        dm_internal_buffer* internal_vb = pipeline->vertex_buffer->internal_buffer;
        dm_internal_buffer* internal_inst = pipeline->inst_buffer->internal_buffer;

        [metal_renderer.command_encoder setVertexBuffer:internal_vb->buffer offset:0 atIndex:0];
        [metal_renderer.command_encoder setVertexBuffer:internal_inst->buffer offset:0 atIndex:1];

        // textures
        //for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        //{
        //   dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        //    dm_image* image = dm_image_get(key->string);
        //    dm_internal_texture* internal_texture = image->internal_texture;
        //
        //    [metal_renderer->command_encoder setFragmentTexture:internal_texture->texture atIndex:i];
        //}
    }

    return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        MTLViewport new_viewport;
        new_viewport.originX = viewport.x;
        new_viewport.originY = viewport.y;
        new_viewport.width = viewport.width;
        new_viewport.height = viewport.height;
        new_viewport.znear = viewport.min_depth;
        new_viewport.zfar = viewport.max_depth;

        [metal_renderer.command_encoder setViewport:new_viewport];
    }
   
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
    @autoreleasepool
    {
        metal_renderer.clear_color = *clear_color;
    }
}

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count)
{

}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
    @autoreleasepool
    {
        dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;

        [metal_renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle 
                                        indexCount: num_indices
                                        indexType: MTLIndexTypeUInt32
                                        indexBuffer: metal_renderer.index_buffer
                                        indexBufferOffset: index_offset];
    }
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
    @autoreleasepool
    {
        dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;

        [metal_renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle 
                                        indexCount: num_indices
                                        indexType: MTLIndexTypeUInt32
                                        indexBuffer: metal_renderer.index_buffer
                                        indexBufferOffset: index_offset
                                        instanceCount: num_insts];
    }
    
}

bool dm_renderer_update_buffer_impl(dm_buffer* cb, void* data, size_t data_size)
{
    dm_internal_buffer* internal_buffer = cb->internal_buffer;

    dm_memcpy([internal_buffer->buffer contents], data, dm_metal_align(data_size, DM_METAL_BUFFER_ALIGNMENT));

    return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    return true;
}

#endif