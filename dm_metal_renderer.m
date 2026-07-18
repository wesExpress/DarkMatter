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

typedef struct dm_metal_compute_pipe_t
{
    id<MTLComputePipelineState> pipeline;

    id<MTLArgumentEncoder> encoder;
    id<MTLBuffer> argument_buffer[DM_FRAMES_IN_FLIGHT];

    u16 grp_x, grp_y, grp_z;
} dm_metal_compute_pipe;

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

typedef struct dm_metal_render_target_t
{
    id<MTLTexture> render_texture;
    id<MTLTexture> sample_texture;
    size_t size;

    u16 width, height;

    MTLLoadAction color_load_op, depth_load_op;
    MTLStoreAction color_store_op, depth_store_op;

    bool swapchain, depth;
} dm_metal_render_target;

typedef struct dm_metal_sampler_t
{
    id<MTLSamplerState> state;
} dm_metal_sampler;

typedef struct dm_metal_frame_data_t
{
    id<MTLCommandBuffer> cmd, padding;
    id<MTLRenderCommandEncoder> gfx_encoder;
    id<MTLComputeCommandEncoder> compute_encoder;

    id<MTLCommandBuffer> blit_cmd;
    id<MTLBlitCommandEncoder> blit_encoder;
} dm_metal_frame_data;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;
    
    dm_metal_swapchain swapchain;

    id<MTLCommandQueue> queue;
    dm_metal_frame_data frame_data[DM_FRAMES_IN_FLIGHT];

    id<MTLHeap> resource_heap;

    u32 frame_index;

    // pipelines
    dm_metal_raster_pipe rps[DM_MAX_PIPES];
    u32 rp_count;

    dm_metal_compute_pipe cps[DM_MAX_PIPES];
    u32 cp_count;

    // resources
    dm_metal_render_target rts[DM_MAX_TEXTURES];
    u32 rt_count;

    dm_metal_buffer buffers[DM_MAX_BUFFERS];
    u32 buffer_count;

    dm_metal_texture textures[DM_MAX_TEXTURES];
    u32 texture_count;

    dm_metal_sampler samplers[DM_MAX_SAMPLERS];
    u32 sampler_count;

    id<MTLBuffer> active_index_buffer;
    dm_pipeline active_pipeline;

    id<MTLFence> fences[DM_MAX_SYNCHRONIZATIONS];
    u32 fence_count;
} dm_metal_renderer;

#define DM_SWAPCHAIN_FORMAT MTLPixelFormatBGRA8Unorm
#define DM_DEPTH_FORMAT     MTLPixelFormatDepth32Float

extern void *dm_window_get_native_window(dm_context *context);

bool dm_renderer_init(dm_context* context)
{
    LOG_INFO("Initializing metal backend...");

    context->renderer.internal_renderer = dm_arena_alloc(&context->arena, sizeof(dm_metal_renderer));
    if(!context->renderer.internal_renderer) return false;

    dm_metal_renderer *renderer = context->renderer.internal_renderer;

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

    MTLTextureDescriptor *depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:DM_DEPTH_FORMAT width:context->window.width height:context->window.height mipmapped:NO];
    depth_desc.storageMode = MTLStorageModePrivate;
    depth_desc.usage = MTLTextureUsageRenderTarget;

    renderer->swapchain.depth_texture = [renderer->device newTextureWithDescriptor:depth_desc];

    [depth_desc release];

    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

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
    for(u32 i=0; i<renderer->cp_count; i++)
    {
        [renderer->cps[i].encoder release];
        for(u8 j=0; j<DM_FRAMES_IN_FLIGHT; j++)
        {
            [renderer->cps[i].argument_buffer[j] release];
        }
        [renderer->cps[i].pipeline release];
    }
    for(u32 i=0; i<renderer->rt_count; i++)
    {
        if(renderer->rts[i].swapchain) continue;

        [renderer->rts[i].render_texture release];
        [renderer->rts[i].sample_texture release];
    }

    for(u8 i=0; i<renderer->fence_count; i++)
    {
        [renderer->fences[i] release];
    }

    if(renderer->resource_heap) [renderer->resource_heap release];

    [renderer->queue release];
    [renderer->swapchain.depth_texture release];
    [renderer->swapchain.layer release];
    [renderer->device release];
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    renderer->swapchain.drawable = [renderer->swapchain.layer nextDrawable];
    if(!renderer->swapchain.drawable)
    {
        LOG_ERROR("nextDrawable failed");
        return false;
    }

    frame_data->cmd = [renderer->queue commandBuffer];

    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->cmd presentDrawable:renderer->swapchain.drawable];
    [frame_data->cmd commit];

    renderer->frame_index++;
    renderer->frame_index %= DM_FRAMES_IN_FLIGHT;
    context->renderer.current_frame = renderer->frame_index;

    renderer->active_pipeline.type = DM_PIPELINE_TYPE_INVALID;
    renderer->active_index_buffer  = NULL;
    
    return true;
}

bool dm_renderer_resize(dm_context *context, u16 width, u16 height)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    renderer->swapchain.width = width;
    renderer->swapchain.height = height;

    renderer->swapchain.layer.drawableSize = CGSizeMake(width, height);

    [renderer->swapchain.depth_texture release];

    MTLTextureDescriptor *depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:DM_DEPTH_FORMAT width:width height:height mipmapped:NO];
    depth_desc.usage = MTLTextureUsageRenderTarget;
    depth_desc.storageMode = MTLStorageModePrivate;
    renderer->swapchain.depth_texture = [renderer->device newTextureWithDescriptor:depth_desc];

    [depth_desc release];

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
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

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

    pipe_desc.colorAttachments[0].pixelFormat = DM_SWAPCHAIN_FORMAT;
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

    pipe_desc.depthAttachmentPixelFormat = DM_DEPTH_FORMAT;

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

    *size = heap_size;

    [texture_desc release];

    return texture;
}

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    u16 width = desc.color_attachment.width;
    u16 height = desc.color_attachment.height;

    dm_metal_render_target render_target = { 
        .color_load_op=dm_metal_convert_load(desc.color_attachment.load_op),
        .color_store_op=dm_metal_convert_store(desc.color_attachment.store_op),
        .depth_load_op=dm_metal_convert_load(desc.depth_attachment.load_op),
        .depth_store_op=dm_metal_convert_store(desc.depth_attachment.store_op),
        .depth=desc.depth,
        .swapchain=desc.swapchain,
        .width=width,
        .height=height
    };

    if(!desc.swapchain)
    {
        size_t heap_size = 4 * desc.color_attachment.width * desc.color_attachment.height;

        MTLPixelFormat format = DM_SWAPCHAIN_FORMAT;

        MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:width height:height mipmapped:NO];
        MTLSizeAndAlign size_align = [renderer->device heapTextureSizeAndAlignWithDescriptor:texture_desc];
        size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;
        heap_size = size_align.size;

        render_target.size = heap_size;
    }

    //
    renderer->rts[renderer->rt_count] = render_target;
    handle->type = DM_RESOURCE_TYPE_RENDER_TARGET;
    handle->index = renderer->rt_count++;

    return true;
}

bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

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
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    dm_metal_texture texture = { 0 };

    MTLPixelFormat format = DM_SWAPCHAIN_FORMAT;
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
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

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

id<MTLTexture> dm_metal_create_rt_texture(id<MTLDevice> device, id<MTLHeap> heap, u16 width, u16 height, MTLTextureUsage usage)
{
    id<MTLTexture> texture = NULL;

    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:DM_SWAPCHAIN_FORMAT width:width height:height mipmapped:NO];
    texture_desc.usage = usage;
    texture_desc.storageMode = MTLStorageModePrivate;

    texture = [heap newTextureWithDescriptor:texture_desc];
    if(!texture)
    {
        LOG_ERROR("newTextureWithDescriptor failed");
        return NULL;
    }

    [texture_desc release];

    return texture;
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_resource *resources[], u32 count)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    id<MTLCommandBuffer> cmd       = [renderer->queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];

    MTLHeapDescriptor *heap_desc = [MTLHeapDescriptor new];
    heap_desc.storageMode = MTLStorageModePrivate;
    heap_desc.size = 100 * DM_MEGABYTE;

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

            // twice, for target and sampled
            case DM_RESOURCE_TYPE_RENDER_TARGET:
                heap_desc.size += renderer->rts[resource->index].size;
                heap_desc.size += renderer->rts[resource->index].size;
                break;

            case DM_RESOURCE_TYPE_SAMPLER:
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
        dm_metal_render_target *rt;

        MTLTextureDescriptor *texture_desc = NULL;
        id<MTLTexture> target = NULL;
        id<MTLTexture> sampled = NULL;

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

                if(!buffer->host.contents) LOG_ERROR("No data");
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

            case DM_RESOURCE_TYPE_SAMPLER:
                break;

            case DM_RESOURCE_TYPE_RENDER_TARGET:
                rt = &renderer->rts[resource->index];

                rt->render_texture = dm_metal_create_rt_texture(renderer->device, renderer->resource_heap, rt->width, rt->height, MTLTextureUsageRenderTarget);
                if(!rt->render_texture) return false;
                rt->sample_texture = dm_metal_create_rt_texture(renderer->device, renderer->resource_heap, rt->width, rt->height, MTLTextureUsageShaderRead);
                if(!rt->sample_texture) return false;

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

bool dm_renderer_create_compute_pipeline(dm_context *context, dm_compute_pipeline_desc desc, dm_pipeline *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    dm_metal_compute_pipe pipeline = { 0 };

    dm_compute_shader shader = desc.shader;

    char shader_path[512];
    sprintf(shader_path, "%s.metallib", shader.path);

    id<MTLLibrary> shader_library = dm_metal_create_shader(renderer->device, shader_path);
    if(!shader_library)
    {
        LOG_ERROR("Could not create shader from %s", shader_path);
        return false;
    }
    id<MTLFunction> shader_function = dm_metal_create_shader_function(renderer->device, shader_library, shader.entry);
    if(!shader_function) return false;

    pipeline.encoder = [shader_function newArgumentEncoderWithBufferIndex:0];

    size_t size = pipeline.encoder.encodedLength;

    for(u8 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        pipeline.argument_buffer[i] = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        if(!pipeline.argument_buffer[i])
        {
            LOG_ERROR("newBufferWithLength failed");
            return false;
        }
    }

    NSError *error = NULL;
    pipeline.pipeline = [renderer->device newComputePipelineStateWithFunction:shader_function error:&error];

    [shader_function release];
    [shader_library release];
    
    if(!pipeline.pipeline)
    {
        LOG_ERROR("newComputePipelineStateWithFunction failed");
        LOG_ERROR("%s", [error.localizedDescription UTF8String]);
        return false;
    }

    pipeline.grp_x = desc.grp_x;
    pipeline.grp_y = desc.grp_y;
    pipeline.grp_z = desc.grp_z;

    //
    renderer->cps[renderer->cp_count] = pipeline;
    handle->index = renderer->cp_count++;
    handle->type = DM_PIPELINE_TYPE_COMPUTE;

    return true;
}

bool dm_renderer_create_synchronization(dm_context *context, dm_synchronization_desc desc, dm_synchronization *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    renderer->fences[renderer->fence_count] = [renderer->device newFence];
    *handle = renderer->fence_count++;

    return true;
}

// commands
void dm_render_command_update_begin(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    frame_data->blit_cmd     = [renderer->queue commandBuffer];
    frame_data->blit_encoder = [frame_data->blit_cmd blitCommandEncoder];
}

void dm_render_command_update_end(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->blit_encoder endEncoding];
    [frame_data->blit_cmd     commit];
}

void dm_render_command_begin_rendering(dm_context *context, dm_resource handle, float r, float g, float b, float a, float d)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_render_target *target = &renderer->rts[handle.index];

    id<MTLTexture> color_texture = target->swapchain ? [renderer->swapchain.drawable texture] : target->render_texture;

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

    frame_data->gfx_encoder = [frame_data->cmd renderCommandEncoderWithDescriptor:desc];

    MTLRenderStages resource_stages = MTLRenderStageVertex | MTLRenderStageFragment;
    MTLRenderStages sampler_stages  = MTLRenderStageFragment;

    if(renderer->resource_heap) [frame_data->gfx_encoder useHeap:renderer->resource_heap stages:resource_stages];

    MTLViewport viewport = {
        .width=renderer->swapchain.width,
        .height=renderer->swapchain.height,
        .zfar=1.f
    };

    MTLScissorRect scissor = {
        .width=renderer->swapchain.width,
        .height=renderer->swapchain.height
    };

    [frame_data->gfx_encoder setViewport:viewport];
    [frame_data->gfx_encoder setScissorRect:scissor];
}

void dm_render_command_end_rendering(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_render_target target = renderer->rts[handle.index];

    [frame_data->gfx_encoder endEncoding];

    if(target.swapchain) return;

    // copy over to sampled image
    frame_data->blit_encoder = [frame_data->cmd blitCommandEncoder];

    [frame_data->blit_encoder copyFromTexture:target.render_texture toTexture:target.sample_texture];
    [frame_data->blit_encoder endEncoding];
}

void dm_render_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    DM_ASSERT(handle.type==DM_PIPELINE_TYPE_RASTER, "Not a raster pipeline");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];

    [frame_data->gfx_encoder setRenderPipelineState:pipeline.pipeline];
    [frame_data->gfx_encoder setDepthStencilState:pipeline.depth_state];
    [frame_data->gfx_encoder setCullMode:MTLCullModeBack];
    [frame_data->gfx_encoder setFrontFacingWinding:MTLWindingClockwise];
    [frame_data->gfx_encoder setTriangleFillMode:MTLTriangleFillModeFill];

    renderer->active_pipeline = handle;
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_resource handle, size_t offset)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Not a buffer");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    renderer->active_index_buffer = renderer->buffers[handle.index].device;
}

void dm_metal_push_raster_data(dm_metal_renderer *renderer, dm_pipeline handle, dm_resource *resources, u32 count)
{
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

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
            case DM_RESOURCE_TYPE_RENDER_TARGET:
                [vertex_encoder setTexture:renderer->rts[resource.index].sample_texture atIndex:i];
                [fragment_encoder setTexture:renderer->rts[resource.index].sample_texture atIndex:i];
                break;
            case DM_RESOURCE_TYPE_SAMPLER:
                [vertex_encoder setSamplerState:renderer->samplers[resource.index].state atIndex:i];
                [fragment_encoder setSamplerState:renderer->samplers[resource.index].state atIndex:i];
                break;
            default:
                LOG_FATAL("Unknown/unsupported resource type");
                return;
        }
    }

    [frame_data->gfx_encoder setVertexBuffer:argument_buffer offset:0 atIndex:0];
    [frame_data->gfx_encoder setFragmentBuffer:argument_buffer offset:0 atIndex:0];
}

void dm_render_command_push_resources(dm_context *context, dm_resource *resources, u32 count)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    switch(renderer->active_pipeline.type)
    {
        case DM_PIPELINE_TYPE_RASTER:
            dm_metal_push_raster_data(renderer, renderer->active_pipeline, resources, count);
            break;

        default:
            LOG_ERROR("Invalid graphics pipeline");
            return;
    }
}

void dm_render_command_draw(dm_context *context, u32 index_count, u32 instance_count)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_index_buffer, "No active index buffer");
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->gfx_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:index_count indexType:MTLIndexTypeUInt32 indexBuffer:renderer->active_index_buffer indexBufferOffset:0 instanceCount:instance_count];
}

void dm_render_command_update_buffer(dm_context *context, dm_resource handle, void *data, size_t size)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Not a buffer");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_buffer buffer = renderer->buffers[handle.index];

    memcpy(buffer.host.contents, data, size);

    [frame_data->blit_encoder copyFromBuffer:buffer.host sourceOffset:0 toBuffer:buffer.device destinationOffset:0 size:size];
}

bool dm_render_command_update_texture(dm_context *context, dm_resource handle, void* data, size_t size, u16 width, u16 height)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_TEXTURE, "Not a texture");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    switch(handle.type)
    {
        case DM_RESOURCE_TYPE_TEXTURE: 
            return true;

        default:
            LOG_ERROR("Invalid resource");
            return false;
    }

    return true;
}

bool dm_render_command_resize_render_target(dm_context *context, dm_resource resource, u16 width, u16 height)
{
    DM_ASSERT(resource.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_render_target *target = &renderer->rts[resource.index];

    [target->render_texture release];
    [target->sample_texture release];

    target->render_texture = dm_metal_create_rt_texture(renderer->device, renderer->resource_heap, width, height, MTLTextureUsageRenderTarget);
    if(!target->render_texture) return false;
    target->sample_texture = dm_metal_create_rt_texture(renderer->device, renderer->resource_heap, width, height, MTLTextureUsageShaderRead);
    if(!target->sample_texture) return false;

    return true;
}

void dm_render_command_copy_texture(dm_context *context, dm_resource src, dm_resource dst)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    id<MTLTexture> src_texture = renderer->textures[src.index].device;
    id<MTLTexture> dst_texture = renderer->textures[dst.index].device;

    [frame_data->blit_encoder copyFromTexture:src_texture toTexture:dst_texture];
}

void dm_render_command_update_synchronization(dm_context *context, dm_synchronization synchronization)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    id<MTLFence> fence = renderer->fences[synchronization];

    [frame_data->gfx_encoder updateFence:fence afterStages:MTLRenderStageFragment];
}

void dm_render_command_wait_synchronization(dm_context *context, dm_synchronization synchronization)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    id<MTLFence> fence = renderer->fences[synchronization];

    [frame_data->gfx_encoder waitForFence:fence beforeStages:MTLRenderStageVertex];
}

// compute commands
void dm_compute_command_begin_recording(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    //frame_data->cmd     = [renderer->queue commandBuffer];
    frame_data->compute_encoder = [frame_data->cmd computeCommandEncoder];
}

void dm_compute_command_end_recording(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->compute_encoder endEncoding];
    //[frame_data->compute_cmd     commit];
}

void dm_compute_command_push_resources(dm_context *context, dm_resource *resources, u32 count)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_pipeline.type==DM_PIPELINE_TYPE_COMPUTE, "Active pipeline is not compute");
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    dm_metal_compute_pipe pipeline = renderer->cps[renderer->active_pipeline.index];

    id<MTLBuffer> argument_buffer = pipeline.argument_buffer[renderer->frame_index];

    id<MTLArgumentEncoder> argument_encoder = pipeline.encoder;

    [argument_encoder setArgumentBuffer:argument_buffer offset:0];

    for(u32 i=0; i<count; i++)
    {
        dm_resource resource = resources[i];

        switch(resource.type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                [argument_encoder setBuffer:renderer->buffers[resource.index].device offset:0 atIndex:i];
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                [argument_encoder setTexture:renderer->textures[resource.index].device atIndex:i];
                break;
            case DM_RESOURCE_TYPE_RENDER_TARGET:
                [argument_encoder setTexture:renderer->rts[resource.index].sample_texture atIndex:i];
                break;

            default:
                LOG_WARN("Unknown/unsupported resource type");
                continue;
        }
    }

    [frame_data->compute_encoder setBuffer:argument_buffer offset:0 atIndex:0];
}

void dm_compute_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    DM_ASSERT(handle.type==DM_PIPELINE_TYPE_COMPUTE, "Not a compute pipeline");
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    dm_metal_compute_pipe pipeline = renderer->cps[handle.index];

    [frame_data->compute_encoder setComputePipelineState:pipeline.pipeline];

    renderer->active_pipeline = handle;
}

void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_pipeline.type==DM_PIPELINE_TYPE_COMPUTE, "Active pipeline is not compute");
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_compute_pipe pipeline = renderer->cps[renderer->active_pipeline.index];

    MTLSize thread_size = MTLSizeMake(x, y, z);
    MTLSize group_size = MTLSizeMake(pipeline.grp_x, pipeline.grp_y, pipeline.grp_z);

    [frame_data->compute_encoder dispatchThreadgroups:thread_size threadsPerThreadgroup:group_size];
}

void dm_compute_command_update_synchronization(dm_context *context, dm_synchronization synchronization)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    id<MTLFence> fence = renderer->fences[synchronization];

    [frame_data->compute_encoder updateFence:fence];
}

void dm_compute_command_wait_synchronization(dm_context *context, dm_synchronization synchronization)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    id<MTLFence> fence = renderer->fences[synchronization];

    [frame_data->compute_encoder waitForFence:fence];
}
