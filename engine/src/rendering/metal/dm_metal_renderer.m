#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_metal_buffer.h"
#include "dm_metal_texture.h"
#include "dm_metal_pipeline.h"
#include "dm_metal_render_pass.h"

#include "rendering/dm_image.h"

#include "platform/dm_platform_apple.h"

#include "core/dm_assert.h"
#include "core/dm_mem.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

@implementation dm_metal_renderer

-(id) initWithFrame: (NSRect)frame
{
    self = [super initWithFrame:frame];

    if(self)
    {
        _device = MTLCreateSystemDefaultDevice();
        if(!_device)
        {
            DM_LOG_FATAL("Could not create metal device!");
            return NULL;
        }

        self.wantsLayer = true;
        _metal_layer = [CAMetalLayer layer];
        if(!_metal_layer)
        {
            DM_LOG_FATAL("Could not create metal layer!");
            return false;
        }

        _metal_layer.device = _device;
        _metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        [_metal_layer setFrame: frame];

        _command_queue = [_device newCommandQueue];
        if(!_command_queue)
        {
            DM_LOG_FATAL("Could not create metal command queue!");
            return false;
        }
    }

    return self;
}

- (void) setIndexBuffer: (dm_buffer*)buffer
{
    dm_metal_buffer* index_buffer = buffer->internal_buffer;

    _index_buffer = index_buffer.buffer;
}

- (BOOL) beginFrame
{
    _drawable = [_metal_layer nextDrawable];
    if(!_drawable)
    {
        DM_LOG_FATAL("Metal drawable is NULL!");
        return NO;
    }

    _texture = [_drawable texture];
    if(!_texture)
    {
        DM_LOG_FATAL("Metal texture is NULL!");
        return NO;
    }

    return YES;
}

- (void) endFrame
{
    [_command_buffer presentDrawable:_drawable];
    [_command_buffer commit];
}

- (void) setViewport:(dm_viewport)viewport
{
    MTLViewport new_viewport;
    new_viewport.originX = viewport.x;
    new_viewport.originY = viewport.y;
    new_viewport.width = viewport.width;
    new_viewport.height = viewport.height;
    new_viewport.znear = viewport.min_depth;
    new_viewport.zfar = viewport.max_depth;

    [_command_encoder setViewport:new_viewport];
}

- (void) clearScreen: (dm_color)color
{
    _clear_color = color;
}

- (void) drawArrays: (uint32_t)first Count: (uint32_t)count
{
    return;
}

- (void) drawIndexed: (uint32_t)num Offset:(uint32_t)offset
{
    [_command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                 indexCount: num
                                  indexType: MTLIndexTypeUInt32
                                indexBuffer: _index_buffer
                          indexBufferOffset: offset];
}

- (void) drawInstanced: (uint32_t)num_indices Offset:(uint32_t)offset NumInstances:(uint32_t)num_insts
{
    [_command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                 indexCount: num_indices
                                  indexType: MTLIndexTypeUInt32
                                indexBuffer: _index_buffer
                          indexBufferOffset: offset
                              instanceCount: num_insts];
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

    //@autoreleasepool
    {
        dm_internal_apple_data* internal_data = platform_data->internal_data;
        NSRect frame = [internal_data->content_view getWindowFrame];

        metal_renderer = [[dm_metal_renderer alloc] initWithFrame: frame];

        // content view is the main view for our NSWindow
        // must add our view to the subviews
        [internal_data->content_view addSubview: metal_renderer];

        // must set the content view's layer to our metal layer
        [internal_data->content_view setWantsLayer: YES];
        [internal_data->content_view setLayer: metal_renderer.metal_layer];

        // TODO: hack to get clear color for now
        metal_renderer.clear_color = renderer_data->clear_color;
    }

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    return;
}

bool dm_renderer_begin_frame_impl(dm_renderer_data* renderer_data)
{
    //@autoreleasepool
    {
        return [metal_renderer beginFrame];
    }
}

bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data)
{
    //@autoreleasepool
    {
        [metal_renderer endFrame];
    }

    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    //@autoreleasepool
    {
        pipeline->internal_pipeline = [[dm_metal_pipeline alloc] initWithRenderer:metal_renderer AndPipeline:pipeline];
        if(!pipeline->internal_pipeline) return false;
    }

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    //@autoreleasepool
    {
        // textures
        //for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
        //{
        //    dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        //    dm_image* image = dm_image_get(key->string);
        //
        //    dm_metal_destroy_texture(image);
        //}
    }
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline)
{
    //@autoreleasepool
    {
        // buffers
        pipeline->vertex_buffer->internal_buffer = [[dm_metal_buffer alloc] initWithData:vb_data AndLength:pipeline->vertex_buffer->desc.buffer_size AndRenderer:metal_renderer];
        if(!pipeline->vertex_buffer->internal_buffer) {
            DM_LOG_FATAL("Could not create internal vertex buffer!");
            return false;
        }

        pipeline->index_buffer->internal_buffer = [[dm_metal_buffer alloc] initWithData:ib_data AndLength:pipeline->index_buffer->desc.buffer_size AndRenderer:metal_renderer];
        if(!pipeline->index_buffer->internal_buffer) {
            DM_LOG_FATAL("Could not create internal index buffer!");
            return false;
        }

        pipeline->inst_buffer->internal_buffer = [[dm_metal_buffer alloc] initWithLength:pipeline->inst_buffer->desc.buffer_size AndRenderer:metal_renderer];
        if(!pipeline->inst_buffer->internal_buffer) {
            DM_LOG_FATAL("Could not create internal instance buffer!");
            return false;
        }

        [metal_renderer setIndexBuffer: pipeline->index_buffer];

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
    //@autoreleasepool
    {
        render_pass->internal_render_pass = [[dm_metal_render_pass alloc] initWithRenderer:metal_renderer AndPass:render_pass];

        if(!render_pass->internal_render_pass) return false;
    }

    return true;
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
    return;
}

bool dm_renderer_begin_render_pass_impl(dm_render_pass* render_pass)
{
    //@autoreleasepool
    {
        dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;
        [internal_pass updateUniforms:render_pass];
        return [internal_pass beginPass:metal_renderer];
    }
}

void dm_renderer_end_render_pass_impl(dm_render_pass* render_pass)
{
    //@autoreleasepool
    {
        [metal_renderer.command_encoder endEncoding];
        //dm_metal_render_pass* internal_pass = render_pass->internal_render_pass;

        //[internal_pass endPass:metal_renderer];
    }
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    //@autoreleasepool
    {
        dm_metal_pipeline* internal_pipe = pipeline->internal_pipeline;

        [internal_pipe bind:pipeline Renderer:metal_renderer];

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
    //@autoreleasepool
    {
        [metal_renderer setViewport:viewport];
    }
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
    //@autoreleasepool
    {
        [metal_renderer clearScreen:*clear_color];
    }
}

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count)
{
    //@autoreleasepool
    {
        [metal_renderer drawArrays: first Count: count];
    }
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
    //@autoreleasepool
    {
        [metal_renderer drawIndexed: num_indices Offset: index_offset];
    }
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
    //@autoreleasepool
    {
        [metal_renderer drawInstanced:num_indices Offset:index_offset NumInstances:num_insts];
    }
}

bool dm_renderer_update_buffer_impl(dm_buffer* cb, void* data, size_t data_size)
{
    //@autoreleasepool
    {
        dm_metal_buffer* internal_buffer = cb->internal_buffer;

        dm_memcpy([internal_buffer.buffer contents], data, data_size);
    }

    return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    return true;
}

#endif