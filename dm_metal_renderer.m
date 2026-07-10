#include "dm.h"
#include <stdlib.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/NSWindow.h>

typedef struct dm_metal_swapchain_t
{
    CAMetalLayer *layer;
    id<CAMetalDrawable> drawable;
    id<MTLTexture> depth_texture;

    u16 width, height;
} dm_metal_swapchain;

typedef struct dm_metal_render_target_t
{
    id<MTLTexture> color_texture;

    MTLLoadAction color_load_op, depth_load_op;
    MTLStoreAction color_store_op, depth_store_op;

    bool swapchain, depth;
} dm_metal_render_target;

typedef struct dm_metal_raster_pipe_t
{
    id<MTLRenderPipelineState> pipeline;

    MTLPrimitiveType primitive_type;
    MTLTriangleFillMode fill_mode;
    MTLCullMode cull_mode;
    MTLWinding winding;
} dm_metal_raster_pipe;

typedef struct dm_metal_buffer_t
{
    id<MTLBuffer> host;
    id<MTLBuffer> device;
    size_t size;
} dm_metal_buffer;

typedef struct dm_metal_texture_t
{
    id<MTLTexture> host;
    id<MTLTexture> device;
    size_t size;
} dm_metal_texture;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;
    
    dm_metal_swapchain swapchain;

    id<MTLCommandQueue> queue;
    id<MTLCommandBuffer> cmd;
    id<MTLRenderCommandEncoder> render_encoder;
    id<MTLComputeCommandEncoder> compute_encoder;

    id<MTLHeap> resource_heap;
    id<MTLHeap> sampler_heap;

    // resources
    dm_metal_render_target rts[DM_MAX_TEXTURES];
    u32 rt_count;

    dm_metal_raster_pipe rps[DM_MAX_PIPES];
    u32 rp_count;

    dm_metal_buffer buffers[DM_MAX_BUFFERS];
    u32 buffer_count;

    dm_metal_texture textures[DM_MAX_TEXTURES];
    u32 texture_count;

    u32 active_pipeline, active_index_buffer;
} dm_metal_renderer;

extern void *dm_window_get_native_window(dm_context *context);

bool dm_renderer_init(dm_context* context)
{
    LOG_INFO("Initializing metal backend...");

    dm_metal_renderer *renderer = dm_arena_alloc(&context->arena, sizeof(dm_metal_renderer), &context->renderer.offset);
    if(!renderer) return false;

    renderer->device = MTLCreateSystemDefaultDevice();

    renderer->swapchain.layer = [CAMetalLayer layer];
    renderer->swapchain.layer.device = renderer->device;
    renderer->swapchain.layer.opaque = YES;

    NSWindow *window = dm_window_get_native_window(context);
    window.contentView.layer = renderer->swapchain.layer;
    window.contentView.wantsLayer = YES;

    renderer->queue = [renderer->device newCommandQueue];

    renderer->swapchain.width = context->window.width;
    renderer->swapchain.height = context->window.height;

    MTLTextureDescriptor *depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:context->window.width height:context->window.height mipmapped:NO];
    depth_desc.storageMode = MTLStorageModePrivate;
    depth_desc.usage = MTLTextureUsageRenderTarget;

    renderer->swapchain.depth_texture = [renderer->device newTextureWithDescriptor:depth_desc];

    [depth_desc release];

    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    for(u32 i=0; i<renderer->buffer_count; i++)
    {
        [renderer->buffers[i].host release];
        [renderer->buffers[i].device release];
    }
    for(u32 i=0; i<renderer->texture_count; i++)
    {
        [renderer->textures[i].host release];
        [renderer->textures[i].device release];
    }
    for(u32 i=0; i<renderer->rp_count; i++)
    {
        [renderer->rps[i].pipeline release];
    }
    for(u32 i=0; i<renderer->rt_count; i++)
    {
        if(renderer->rts[i].swapchain) continue;

        [renderer->rts[i].color_texture release];
    }

    if(renderer->resource_heap) [renderer->resource_heap release];
    if(renderer->sampler_heap)  [renderer->sampler_heap  release];

    [renderer->queue release];
    [renderer->swapchain.depth_texture release];
    [renderer->swapchain.layer release];
    [renderer->device release];
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    renderer->swapchain.drawable = [renderer->swapchain.layer nextDrawable];
    if(!renderer->swapchain.drawable)
    {
        LOG_ERROR("nextDrawable failed");
        return false;
    }

    renderer->cmd = [renderer->queue commandBuffer];

    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    [renderer->cmd presentDrawable:renderer->swapchain.drawable];
    [renderer->cmd commit];
    
    return true;
}

bool dm_renderer_resize(dm_context *context, u16 width, u16 height)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    renderer->swapchain.width = width;
    renderer->swapchain.height = height;

    renderer->swapchain.layer.drawableSize = CGSizeMake(width, height);

    return true;
}

size_t dm_renderer_get_internal_size()
{
    return sizeof(dm_metal_renderer);
}

bool dm_renderer_create_raster_pipeline(dm_context *context, dm_raster_pipe_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_raster_pipe pipeline = { 0 };

    return true;
}

MTLLoadAction dm_metal_convert_load(dm_render_attachment_load_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported load action");
            LOG_WARN("Returning MTLLoadActionLoad");
        case DM_RENDER_ATTACHMENT_LOAD_OP_LOAD:
            return MTLLoadActionLoad;
        case DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR:
            return MTLLoadActionClear;
        case DM_RENDER_ATTACHMENT_LOAD_OP_DONT_CARE:
            return MTLLoadActionDontCare;
    }
}

MTLStoreAction dm_metal_convert_store(dm_render_attachment_store_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported store action");
            LOG_WARN("Returning MTLStoreActionStore");
        case DM_RENDER_ATTACHMENT_STORE_OP_STORE:
            return MTLStoreActionStore;
        case DM_RENDER_ATTACHMENT_STORE_OP_DONT_CARE:
            return MTLStoreActionDontCare;
    }
}

id<MTLTexture> dm_metal_create_texture(id<MTLDevice> device, MTLPixelFormat format, u16 width, u16 height, void *data, size_t *size)
{
    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:width height:height mipmapped:NO];

    id<MTLTexture> texture = [device newTextureWithDescriptor:texture_desc];
    if(data)
    {
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:(4 * width)];
    }

    size_t heap_size;
    MTLSizeAndAlign size_align = [device heapTextureSizeAndAlignWithDescriptor:texture_desc];
    size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;
    heap_size = size_align.size;

    LOG_INFO("%zu", heap_size);
    *size = heap_size;

    [texture_desc release];

    return texture;
}

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_render_target render_target = { 
        .color_load_op=dm_metal_convert_load(desc.color_attachment.load_op),
        .color_store_op=dm_metal_convert_store(desc.color_attachment.store_op),
        .depth_load_op=dm_metal_convert_load(desc.depth_attachment.load_op),
        .depth_store_op=dm_metal_convert_store(desc.depth_attachment.store_op),
        .depth=desc.depth,
        .swapchain=desc.swapchain
    };

    if(!desc.swapchain)
    {
        size_t color_size = 4 * desc.color_attachment.width * desc.color_attachment.height;

        render_target.color_texture = dm_metal_create_texture(renderer->device, MTLPixelFormatRGBA8Unorm, desc.color_attachment.width, desc.color_attachment.height, NULL, &color_size);
        if(!render_target.color_texture) return false;
    }

    //
    renderer->rts[renderer->rt_count] = render_target;
    handle->r_type = DM_RESOURCE_TYPE_RENDER_TARGET;
    handle->index = renderer->rt_count++;

    return true;
}

bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_buffer buffer = { 0 };

    size_t heap_size = desc.size;
    MTLSizeAndAlign size_align = [renderer->device heapBufferSizeAndAlignWithLength:heap_size options:MTLResourceStorageModePrivate];
    size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;
    heap_size = size_align.size;
    buffer.size = heap_size;

    if(desc.data)
    {
        buffer.host = [renderer->device newBufferWithBytes:desc.data length:heap_size options:MTLResourceCPUCacheModeDefaultCache];
        if(!buffer.host)
        {
            LOG_ERROR("newBufferWithBytes failed");
            return false;
        }
    }
    else
    {
        buffer.host = [renderer->device newBufferWithLength:heap_size options:MTLResourceCPUCacheModeDefaultCache];
        if(!buffer.host)
        {
            LOG_ERROR("newBufferWithLength failed");
            return false;
        }
    }

    //
    renderer->buffers[renderer->buffer_count] = buffer;
    handle->r_type = DM_RESOURCE_TYPE_BUFFER;
    handle->index = renderer->buffer_count++;

    return true;
}

bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_texture texture = { 0 };

    MTLPixelFormat format = MTLPixelFormatRGBA8Unorm;
    texture.size = desc.size;
    texture.host = dm_metal_create_texture(renderer->device, format, desc.width, desc.height, desc.data, &texture.size);
    if(!texture.host) return false;

    //
    renderer->textures[renderer->texture_count] = texture;
    handle->r_type = DM_RESOURCE_TYPE_TEXTURE;
    handle->index = renderer->texture_count++;

    return true;
}

bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_handle *resources[], u32 count)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    id<MTLCommandBuffer> cmd = [renderer->queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];

    MTLHeapDescriptor *heap_desc = [MTLHeapDescriptor new];
    heap_desc.storageMode = MTLStorageModePrivate;
    heap_desc.size = 0;

    // get heap size
    for(u32 i=0; i<count; i++)
    {
        dm_handle *resource = resources[i];

        switch(resource->r_type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                heap_desc.size += renderer->buffers[resource->index].size;
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                heap_desc.size += renderer->textures[resource->index].size;
                break;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                return false;
        }
    }
    LOG_INFO("Heap size: %zu", heap_desc.size);

    renderer->resource_heap = [renderer->device newHeapWithDescriptor:heap_desc];
    if(!renderer->resource_heap)
    {
        LOG_ERROR("newHeapWithDescriptor failed");
        return false;
    }

    // actually upload to heap
    for(u32 i=0; i<count; i++)
    {
        dm_handle *resource = resources[i];

        dm_metal_buffer *buffer;
        dm_metal_texture *texture;
        MTLTextureDescriptor *texture_desc = NULL;

        switch(resource->r_type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                buffer = &renderer->buffers[resource->index];

                buffer->device = [renderer->resource_heap newBufferWithLength:buffer->size options:MTLResourceStorageModePrivate];
                if(!buffer->device) 
                {
                    LOG_ERROR("newBufferWithLength failed");
                    return false;
                }

                [blit copyFromBuffer:buffer->host sourceOffset:0 toBuffer:buffer->device destinationOffset:0 size:buffer->size];
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                texture = &renderer->textures[resource->index];

                texture_desc = [MTLTextureDescriptor new];
                texture_desc.textureType = texture->host.textureType;
                texture_desc.pixelFormat = texture->host.pixelFormat;
                texture_desc.width = texture->host.width;
                texture_desc.height = texture->host.height;
                texture_desc.depth  = texture->host.depth;
                texture_desc.mipmapLevelCount = texture->host.mipmapLevelCount;
                texture_desc.arrayLength = texture->host.arrayLength;
                texture_desc.sampleCount = texture->host.sampleCount;
                texture_desc.storageMode = renderer->resource_heap.storageMode;

                texture->device = [renderer->resource_heap newTextureWithDescriptor:texture_desc];
                if(!texture->device)
                {
                    LOG_ERROR("newTextureWithDescriptor failed");
                    return false;
                }

                [blit copyFromTexture:texture->host toTexture:texture->device];

                [texture_desc release];
                break;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                return false;
        }
    }

    [blit endEncoding];
    [cmd commit];

    [heap_desc release];

    return true;
}

bool dm_renderer_upload_samplers_to_heap(dm_context *context, dm_handle *samplers[], u32 count)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

u64 dm_renderer_get_buffer_address(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return 0;
}

bool dm_renderer_create_compute_pipeline(dm_context *context, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_handle handle, float r, float g, float b, float a, float d)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_render_target *target = &renderer->rts[handle.index];

    id<MTLTexture> color_texture = target->swapchain ? [renderer->swapchain.drawable texture] : target->color_texture;

    MTLClearColor clear = MTLClearColorMake(r, g, b, a);

    MTLRenderPassDescriptor *desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.colorAttachments[0].clearColor  = clear;
    desc.colorAttachments[0].loadAction  = target->color_load_op;
    desc.colorAttachments[0].storeAction = target->color_store_op;
    desc.colorAttachments[0].texture     = color_texture;

    if(target->depth)
    {
        desc.depthAttachment.clearDepth  = d;
        desc.depthAttachment.loadAction  = target->depth_load_op;
        desc.depthAttachment.storeAction = target->depth_store_op;
        desc.depthAttachment.texture     = renderer->swapchain.depth_texture;
    }

    renderer->render_encoder = [renderer->cmd renderCommandEncoderWithDescriptor:desc];

    MTLRenderStages resource_stages = MTLRenderStageVertex | MTLRenderStageFragment;
    MTLRenderStages sampler_stages  = MTLRenderStageFragment;

    if(renderer->resource_heap) [renderer->render_encoder useHeap:renderer->resource_heap stages:resource_stages];
    if(renderer->sampler_heap)  [renderer->render_encoder useHeap:renderer->sampler_heap  stages:sampler_stages];
}

void dm_render_command_end_rendering(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    [renderer->render_encoder endEncoding];
}

void dm_render_command_bind_pipeline(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];

    id<MTLRenderCommandEncoder> encoder = renderer->render_encoder;

#if 0
    [encoder setRenderPipelineState:pipeline.pipeline];
    [encoder setCullMode:pipeline.cull_mode];
    [encoder setFrontFacingWinding:pipeline.winding];
    [encoder setTriangleFillMode:pipeline.fill_mode];
#endif
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_handle handle, size_t offset)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    renderer->active_index_buffer = handle.index;
}

void dm_render_command_push_constants(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_render_command_push_data(dm_context *context, void *data, size_t size)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_render_command_draw(dm_context *context, u32 index_count, u32 instance_count)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    id<MTLRenderCommandEncoder> encoder = renderer->render_encoder;

    //[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:index_count indexType:MTLIndexTypeUInt32 indexBuffer:renderer->buffers[renderer->active_index_buffer].device indexBufferOffset:0 instanceCount:instance_count];
}

void dm_render_command_update_buffer(dm_context *context, dm_handle handle, void *data, size_t size)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_render_command_copy_buffer(dm_context *context, dm_handle src, dm_handle dst)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

bool dm_render_command_update_texture(dm_context *context, dm_handle handle, void* data, size_t size, u16 width, u16 height)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

void dm_render_command_copy_texture(dm_context *context, dm_handle src, dm_handle dst)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

// compute commands
void dm_compute_command_push_data(dm_context *context, void *data, size_t size)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_compute_command_bind_pipeline(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}
