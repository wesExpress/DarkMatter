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
    id<MTLDepthStencilState>   depth_state;

    id<MTLArgumentEncoder> vertex_encoder;
    id<MTLArgumentEncoder> fragment_encoder;
    id<MTLBuffer> argument_buffer[DM_FRAMES_IN_FLIGHT];

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

typedef struct dm_metal_sampler_t
{
    id<MTLSamplerState> state;
} dm_metal_sampler;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;
    
    dm_metal_swapchain swapchain;

    id<MTLCommandQueue> queue;
    id<MTLCommandBuffer> cmd;
    id<MTLRenderCommandEncoder> render_encoder;
    id<MTLComputeCommandEncoder> compute_encoder;

    id<MTLHeap> resource_heap;

    u32 frame_index;

    // resources
    dm_metal_render_target rts[DM_MAX_TEXTURES];
    u32 rt_count;

    dm_metal_raster_pipe rps[DM_MAX_PIPES];
    u32 rp_count;

    dm_metal_buffer buffers[DM_MAX_BUFFERS];
    u32 buffer_count;

    dm_metal_texture textures[DM_MAX_TEXTURES];
    u32 texture_count;

    dm_metal_sampler samplers[DM_MAX_SAMPLERS];
    u32 sampler_count;

    id<MTLBuffer> active_index_buffer;
    dm_pipeline active_pipeline;
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
    for(u32 i=0; i<renderer->sampler_count; i++)
    {
        [renderer->samplers[i].state release];
    }
    for(u32 i=0; i<renderer->rp_count; i++)
    {
        [renderer->rps[i].vertex_encoder release];
        [renderer->rps[i].fragment_encoder release];
        for(u8 j=0; j<DM_FRAMES_IN_FLIGHT; j++)
        {
            if(renderer->rps[i].argument_buffer[j]) [renderer->rps[i].argument_buffer[j] release];
        }
        [renderer->rps[i].pipeline release];
        [renderer->rps[i].depth_state release];
    }
    for(u32 i=0; i<renderer->rt_count; i++)
    {
        if(renderer->rts[i].swapchain) continue;

        [renderer->rts[i].color_texture release];
    }

    if(renderer->resource_heap) [renderer->resource_heap release];

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

    renderer->frame_index++;
    renderer->frame_index %= DM_FRAMES_IN_FLIGHT;
    context->renderer.current_frame = renderer->frame_index;
    
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

id<MTLLibrary> dm_metal_create_shader(id<MTLDevice> device, const char *path)
{
    LOG_DEBUG("Creating shader from %s", path);

    NSString* file = [NSString stringWithUTF8String:path];

    id<MTLLibrary> library = NULL;
    NSURL* library_url = [NSURL URLWithString:file];
    NSError* library_error = NULL;

    library = [device newLibraryWithURL:library_url error:&library_error];
    if(!library)
    {
        LOG_ERROR("newLibraryWithURL failed");
        LOG_ERROR("%s", [library_error.localizedDescription UTF8String]);

        [file release];
        [library_url release];

        return NULL;
    }

    return library;
}

id<MTLFunction> dm_metal_create_shader_function(id<MTLDevice> device, id<MTLLibrary> library, const char* entry)
{
    NSString* func_name = [[NSString alloc] initWithUTF8String:entry];

    id<MTLFunction> function = [library newFunctionWithName:func_name];

    if(!function) 
    { 
        LOG_ERROR("newFunctionWithName failed");
        return NULL;
    }

    [func_name release];
    return function;
}

MTLBlendOperation dm_metal_convert_blend_op(dm_blend_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported blend operation");
            LOG_WARN("Returning MTLBlendOperationAdd");
        case DM_BLEND_OP_ADD:
            return MTLBlendOperationAdd;
        case DM_BLEND_OP_SUBTRACT:
            return MTLBlendOperationSubtract;
        case DM_BLEND_OP_MIN:
            return MTLBlendOperationMin;
        case DM_BLEND_OP_MAX:
            return MTLBlendOperationMax;
    }
}

MTLBlendFactor dm_metal_convert_blend_factor(dm_blend_factor factor)
{
    switch(factor)
    {
        default:
            LOG_WARN("Unknown/unsupported blend factor");
            LOG_WARN("Returning MTLBlendFactorOne");
        case DM_BLEND_FACTOR_ONE:
            return MTLBlendFactorOne;
        case DM_BLEND_FACTOR_ZERO:
            return MTLBlendFactorZero;
        case DM_BLEND_FACTOR_SRC_ALPHA:
            return MTLBlendFactorSourceAlpha;
        case DM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
            return MTLBlendFactorOneMinusSourceAlpha;
    }
}

bool dm_renderer_create_raster_pipeline(dm_context *context, dm_raster_pipe_desc desc, dm_pipeline *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_raster_pipe pipeline = { 0 };

    dm_raster_shader vertex_shader = desc.shaders[DM_RASTER_SHADER_STAGE_VERTEX];
    dm_raster_shader fragment_shader = desc.shaders[DM_RASTER_SHADER_STAGE_FRAGMENT];

    char vertex_path[512];
    sprintf(vertex_path, "%s.metallib", vertex_shader.path);
    char fragment_path[512];
    sprintf(fragment_path, "%s.metallib", fragment_shader.path);

    id<MTLLibrary> vertex_library = dm_metal_create_shader(renderer->device, vertex_path);
    if(!vertex_library)
    {
        LOG_ERROR("Could not create shader from %s", vertex_path);
        return false;
    }
    id<MTLFunction> vertex_function = dm_metal_create_shader_function(renderer->device, vertex_library, vertex_shader.entry);
    if(!vertex_function) return false;

    id<MTLLibrary> fragment_library = dm_metal_create_shader(renderer->device, fragment_path);
    if(!fragment_library)
    {
        [vertex_library release];

        LOG_ERROR("Could not create shader from %s", fragment_path);
        return false;
    }
    id<MTLFunction> fragment_function = dm_metal_create_shader_function(renderer->device, fragment_library, fragment_shader.entry);
    if(!fragment_function) return false;

    // argument buffer
    // TODO: werid approach here
    pipeline.vertex_encoder = [vertex_function newArgumentEncoderWithBufferIndex:0];
    pipeline.fragment_encoder = [fragment_function newArgumentEncoderWithBufferIndex:0];

    size_t size = pipeline.vertex_encoder.encodedLength;
    if(size!=pipeline.fragment_encoder.encodedLength)
    {
        LOG_ERROR("Vertex and fragment shaders have different sized argument buffers");
        return false;
    }

    for(u8 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        pipeline.argument_buffer[i] = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        if(!pipeline.argument_buffer[i])
        {
            LOG_ERROR("newBufferWithLength failed");
            return false;
        }
    }

    // pipeline state
    MTLRenderPipelineDescriptor *pipe_desc = [MTLRenderPipelineDescriptor new];

    pipe_desc.rasterSampleCount=1;

    pipe_desc.vertexFunction = vertex_function;
    pipe_desc.fragmentFunction = fragment_function;

    pipe_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipe_desc.colorAttachments[0].writeMask = MTLColorWriteMaskAll;

    pipe_desc.colorAttachments[0].blendingEnabled = desc.blend;
    if(desc.blend)
    {
        pipe_desc.colorAttachments[0].rgbBlendOperation    = dm_metal_convert_blend_op(desc.color_blend_op);
        pipe_desc.colorAttachments[0].sourceRGBBlendFactor = dm_metal_convert_blend_factor(desc.color_src_factor);
        pipe_desc.colorAttachments[0].destinationRGBBlendFactor = dm_metal_convert_blend_factor(desc.color_dst_factor);

        pipe_desc.colorAttachments[0].alphaBlendOperation = dm_metal_convert_blend_op(desc.alpha_blend_op);
        pipe_desc.colorAttachments[0].sourceAlphaBlendFactor = dm_metal_convert_blend_factor(desc.alpha_src_factor);
        pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = dm_metal_convert_blend_factor(desc.alpha_dst_factor);
    }

    pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    MTLDepthStencilDescriptor *depth_desc = [MTLDepthStencilDescriptor new];

    depth_desc.depthWriteEnabled = YES;
    depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;

    pipeline.depth_state = [renderer->device newDepthStencilStateWithDescriptor:depth_desc];

    NSError *error = NULL;
    pipeline.pipeline = [renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];

    [vertex_library release];
    [vertex_function release];
    [fragment_library release];
    [fragment_function release];
    [depth_desc release];
    [pipe_desc release];

    if(!pipeline.pipeline)
    {
        LOG_ERROR("newRenderPipelineStateWithDescriptor failed");
        LOG_ERROR("NSError: %s", [error.localizedDescription UTF8String]);

        return false;
    }

    //
    renderer->rps[renderer->rp_count] = pipeline;
    handle->type = DM_PIPELINE_TYPE_RASTER;
    handle->index = renderer->rp_count++;

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
    texture_desc.storageMode = MTLStorageModeShared;
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

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_resource *handle)
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
    handle->type = DM_RESOURCE_TYPE_RENDER_TARGET;
    handle->index = renderer->rt_count++;

    return true;
}

bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_resource *handle)
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
    handle->type = DM_RESOURCE_TYPE_BUFFER;
    handle->index = renderer->buffer_count++;

    return true;
}

bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_texture texture = { 0 };

    MTLPixelFormat format = MTLPixelFormatRGBA8Unorm;
    texture.size = desc.size;
    texture.host = dm_metal_create_texture(renderer->device, format, desc.width, desc.height, desc.data, &texture.size);
    if(!texture.host) return false;

    //
    renderer->textures[renderer->texture_count] = texture;
    handle->type = DM_RESOURCE_TYPE_TEXTURE;
    handle->index = renderer->texture_count++;

    return true;
}

bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_sampler sampler = { 0 };

    MTLSamplerDescriptor *sampler_desc = [MTLSamplerDescriptor new];

    sampler_desc.rAddressMode = MTLSamplerAddressModeRepeat;
    sampler_desc.sAddressMode = MTLSamplerAddressModeRepeat;
    sampler_desc.tAddressMode = MTLSamplerAddressModeRepeat;

    sampler_desc.minFilter = MTLSamplerMinMagFilterLinear;
    sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;

    sampler_desc.supportArgumentBuffers = YES;

    sampler.state = [renderer->device newSamplerStateWithDescriptor:sampler_desc];
    if(!sampler.state)
    {
        LOG_ERROR("newSamplerStateWithDescriptor failed");
        return false;
    }
    
    [sampler_desc release];

    //
    renderer->samplers[renderer->sampler_count] = sampler;
    handle->type = DM_RESOURCE_TYPE_SAMPLER;
    handle->index = renderer->sampler_count++;

    return true;
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_resource *resources[], u32 count)
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
        dm_resource *resource = resources[i];

        switch(resource->type)
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
        dm_resource *resource = resources[i];

        dm_metal_buffer *buffer;
        dm_metal_texture *texture;
        MTLTextureDescriptor *texture_desc = NULL;

        switch(resource->type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                buffer = &renderer->buffers[resource->index];

                buffer->device = [renderer->resource_heap newBufferWithLength:buffer->size options:MTLResourceStorageModePrivate];
                if(!buffer->device) 
                {
                    LOG_ERROR("newBufferWithLength failed");
                    return false;
                }

                resource->gpu_index = buffer->device.gpuAddress;

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

                // TODO: YIKES
                {
                    MTLResourceID id = texture->device.gpuResourceID;
                    resource->gpu_index = *(u64*)&id;
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

bool dm_renderer_upload_samplers_to_heap(dm_context *context, dm_resource *samplers[], u32 count)
{
    return true;
}

u64 dm_renderer_get_buffer_address(dm_context *context, dm_resource handle)
{
    return handle.gpu_index;
}

bool dm_renderer_create_compute_pipeline(dm_context *context, dm_pipeline *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_resource handle, float r, float g, float b, float a, float d)
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
}

void dm_render_command_end_rendering(dm_context *context, dm_resource handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    [renderer->render_encoder endEncoding];
}

void dm_render_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];

    id<MTLRenderCommandEncoder> encoder = renderer->render_encoder;

    [encoder setRenderPipelineState:pipeline.pipeline];
    [encoder setDepthStencilState:pipeline.depth_state];
    [encoder setCullMode:MTLCullModeBack];
    [encoder setFrontFacingWinding:MTLWindingClockwise];
    [encoder setTriangleFillMode:MTLTriangleFillModeFill];

    renderer->active_pipeline = handle;
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_resource handle, size_t offset)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    renderer->active_index_buffer = renderer->buffers[handle.index].device;
}

void dm_metal_push_raster_data(dm_metal_renderer *renderer, dm_pipeline handle, dm_resource *resources, u32 count)
{
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];

    id<MTLRenderCommandEncoder> encoder = renderer->render_encoder;
    id<MTLBuffer> argument_buffer = pipeline.argument_buffer[renderer->frame_index];

    id<MTLArgumentEncoder> vertex_encoder = pipeline.vertex_encoder;
    id<MTLArgumentEncoder> fragment_encoder = pipeline.fragment_encoder;

    [vertex_encoder setArgumentBuffer:argument_buffer offset:0];
    [fragment_encoder setArgumentBuffer:argument_buffer offset:0];

    for(u32 i=0; i<count; i++)
    {
        dm_resource resource = resources[i];

        switch(resource.type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                [vertex_encoder setBuffer:renderer->buffers[resource.index].device offset:0 atIndex:i];
                [fragment_encoder setBuffer:renderer->buffers[resource.index].device offset:0 atIndex:i];
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                [vertex_encoder setTexture:renderer->textures[resource.index].device atIndex:i];
                [fragment_encoder setTexture:renderer->textures[resource.index].device atIndex:i];
                break;
            case DM_RESOURCE_TYPE_SAMPLER:
                [vertex_encoder setSamplerState:renderer->samplers[resource.index].state atIndex:i];
                [fragment_encoder setSamplerState:renderer->samplers[resource.index].state atIndex:i];
                break;
            default:
                LOG_WARN("Unknown/unsupported resource type");
                continue;
        }
    }

    [encoder setVertexBuffer:argument_buffer offset:0 atIndex:0];
    [encoder setFragmentBuffer:argument_buffer offset:0 atIndex:0];
}

void dm_render_command_push_resources(dm_context *context, dm_resource *resources, u32 count)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    switch(renderer->active_pipeline.type)
    {
        case DM_PIPELINE_TYPE_RASTER:
            dm_metal_push_raster_data(renderer, renderer->active_pipeline, resources, count);
            break;
        case DM_PIPELINE_TYPE_COMPUTE:
            break;
        default:
            LOG_ERROR("No active pipeline");
            return;
    }
}

void dm_render_command_draw(dm_context *context, u32 index_count, u32 instance_count)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    id<MTLRenderCommandEncoder> encoder = renderer->render_encoder;

    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:index_count indexType:MTLIndexTypeUInt32 indexBuffer:renderer->active_index_buffer indexBufferOffset:0 instanceCount:instance_count];
}

void dm_render_command_update_buffer(dm_context *context, dm_resource handle, void *data, size_t size)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_buffer buffer = renderer->buffers[handle.index];

    id<MTLCommandBuffer> cmd = [renderer->queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];

    memcpy(buffer.host.contents, data, size);

    [blit copyFromBuffer:buffer.host sourceOffset:0 toBuffer:buffer.device destinationOffset:0 size:size];

    [blit endEncoding];
    [cmd commit];
}

bool dm_render_command_update_texture(dm_context *context, dm_resource handle, void* data, size_t size, u16 width, u16 height)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    return true;
}

void dm_render_command_copy_texture(dm_context *context, dm_resource src, dm_resource dst)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

// compute commands
void dm_compute_command_push_data(dm_context *context, void *data, size_t size)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_compute_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}

void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
}
