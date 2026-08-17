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
    size_t size, stride;
} dm_metal_buffer;

typedef struct dm_metal_texture_t
{
    id<MTLTexture> host;
    id<MTLTexture> device;
    size_t size;
} dm_metal_texture;

typedef struct dm_metal_render_target_t
{
    id<MTLTexture> texture;
    size_t size;

    u16 width, height;

    bool swapchain, depth;
} dm_metal_render_target;

typedef struct dm_metal_sampler_t
{
    id<MTLSamplerState> state;
} dm_metal_sampler;

typedef struct dm_metal_frame_data_t
{
    id<MTLCommandBuffer> gfx_cmd, compute_cmd;
    id<MTLRenderCommandEncoder> gfx_encoder;
    id<MTLComputeCommandEncoder> compute_encoder;

    id<MTLCommandBuffer> blit_cmd;
    id<MTLBlitCommandEncoder> blit_encoder;
} dm_metal_frame_data;

typedef struct dm_metal_event_t
{
    id<MTLEvent> event;
    u64 value;
} dm_metal_event;

typedef struct dm_metal_heap_t
{
    id<MTLHeap> heap;
    size_t size, upload_size;
} dm_metal_heap;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;
    
    dm_metal_swapchain swapchain;

    id<MTLCommandQueue> gfx_queue;
    id<MTLCommandQueue> compute_queue;
    dm_metal_frame_data frame_data[DM_FRAMES_IN_FLIGHT];

    dm_metal_heap resource_heap;

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

    //
    dm_metal_buffer active_index_buffer;
    dm_pipeline active_pipeline;

    dm_metal_event events[DM_MAX_SYNCHRONIZATIONS * DM_FRAMES_IN_FLIGHT];
    u32 event_count;
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

    renderer->gfx_queue     = [renderer->device newCommandQueue];
    renderer->compute_queue = [renderer->device newCommandQueue];

    renderer->swapchain.width = context->window.width * context->window.scale_w;
    renderer->swapchain.height = context->window.height * context->window.scale_h;

    MTLTextureDescriptor *depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:DM_DEPTH_FORMAT width:renderer->swapchain.width height:renderer->swapchain.height mipmapped:NO];
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

        [renderer->rts[i].texture release];
    }

    for(u8 i=0; i<renderer->event_count; i++)
    {
        [renderer->events[i].event release];
    }

    [renderer->resource_heap.heap release];
    [renderer->compute_queue release];
    [renderer->gfx_queue release];
    [renderer->swapchain.depth_texture release];
    [renderer->swapchain.layer release];
    [renderer->device release];
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    int width = context->window.width * context->window.scale_w;
    int height = context->window.height * context->window.scale_h;
    renderer->swapchain.layer.drawableSize = CGSizeMake(width, height);
    renderer->swapchain.drawable = [renderer->swapchain.layer nextDrawable];
    if(!renderer->swapchain.drawable)
    {
        LOG_ERROR("nextDrawable failed");
        return false;
    }

    frame_data->gfx_cmd     = [renderer->gfx_queue commandBuffer];
    frame_data->compute_cmd = [renderer->compute_queue commandBuffer];

    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->gfx_cmd presentDrawable:renderer->swapchain.drawable];
    [frame_data->gfx_cmd commit];

    [frame_data->compute_cmd commit];

    renderer->frame_index++;
    renderer->frame_index %= DM_FRAMES_IN_FLIGHT;
    context->renderer.current_frame = renderer->frame_index;

    renderer->active_pipeline.type = DM_PIPELINE_TYPE_INVALID;
    renderer->active_index_buffer.device = NULL;
    
    return true;
}

bool dm_renderer_resize(dm_context *context, u16 width, u16 height)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    width  *= context->window.scale_w;
    height *= context->window.scale_h;

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
    LOG_DEBUG("Creating shader: %s", path);

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
        case DM_BLEND_OP_ADD:      return MTLBlendOperationAdd;
        case DM_BLEND_OP_SUBTRACT: return MTLBlendOperationSubtract;
        case DM_BLEND_OP_MIN:      return MTLBlendOperationMin;
        case DM_BLEND_OP_MAX:      return MTLBlendOperationMax;
    }
}

MTLBlendFactor dm_metal_convert_blend_factor(dm_blend_factor factor)
{
    switch(factor)
    {
        default:
            LOG_WARN("Unknown/unsupported blend factor");
            LOG_WARN("Returning MTLBlendFactorOne");
        case DM_BLEND_FACTOR_ONE:                 return MTLBlendFactorOne;
        case DM_BLEND_FACTOR_ZERO:                return MTLBlendFactorZero;
        case DM_BLEND_FACTOR_SRC_ALPHA:           return MTLBlendFactorSourceAlpha;
        case DM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
    }
}

MTLWinding dm_metal_convert_winding(dm_winding_order winding)
{
    switch(winding)
    {
        default:
            LOG_WARN("Unknown/unsupported winding order");
            LOG_WARN("Returning MTLWindingCounterClockwise");
        case DM_WINDING_COUNTERCLOCKWISE: return MTLWindingCounterClockwise;
        case DM_WINDING_CLOCKWISE:        return MTLWindingClockwise;
    }
}

MTLCullMode dm_metal_convert_cull(dm_cull_mode culling)
{
    switch(culling)
    {
        default:
            LOG_WARN("Unknown/unsupported cull mode");
            LOG_WARN("Returning MTLCullModeNone");
        case DM_CULL_NONE:  return MTLCullModeNone;
        case DM_CULL_FRONT: return MTLCullModeFront;
        case DM_CULL_BACK:  return MTLCullModeBack;
    }
}

MTLTriangleFillMode dm_metal_convert_fill(dm_fill_mode fill)
{
    switch(fill)
    {
        default:
            LOG_WARN("Unknown/unsupported fill mode");
            LOG_WARN("Returning MTLTriangleFillModeFill");
        case DM_FILL_FULL:  return MTLTriangleFillModeFill;
        case DM_FILL_LINES: return MTLTriangleFillModeLines;
    }
}

MTLPrimitiveType dm_metal_convert_primitive(dm_primitive_type primitive)
{
    switch(primitive)
    {
        default:
            LOG_WARN("Unknown/unsupported primitive type");
            LOG_WARN("Returning MTLPrimitiveTypeTriangle");
        case DM_PRIMITIVE_TRIANGLE_LIST: return MTLPrimitiveTypeTriangle;
        case DM_PRIMITIVE_POINT_LIST:    return MTLPrimitiveTypePoint;
        case DM_PRIMITIVE_LINE_LIST:     return MTLPrimitiveTypeLine;
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

    pipe_desc.colorAttachments[0].blendingEnabled = desc.blend ? YES : NO;
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

    depth_desc.depthWriteEnabled    = desc.depth ? YES : NO;
    depth_desc.depthCompareFunction = desc.depth ? MTLCompareFunctionLessEqual : MTLCompareFunctionAlways;

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

    // TODO: needs to be configurable
    pipeline.cull_mode      = dm_metal_convert_cull(desc.culling);
    pipeline.winding        = dm_metal_convert_winding(desc.winding);
    pipeline.fill_mode      = dm_metal_convert_fill(desc.fill);
    pipeline.primitive_type = dm_metal_convert_primitive(desc.primitive_type);

    //
    renderer->rps[renderer->rp_count] = pipeline;
    handle->type = DM_PIPELINE_TYPE_RASTER;
    handle->index = renderer->rp_count++;

    return true;
}

MTLLoadAction dm_metal_convert_load(dm_render_load_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported load action");
            LOG_WARN("Returning MTLLoadActionLoad");
        case DM_RENDER_LOAD_OP_LOAD:      return MTLLoadActionLoad;
        case DM_RENDER_LOAD_OP_CLEAR:     return MTLLoadActionClear;
        case DM_RENDER_LOAD_OP_DONT_CARE: return MTLLoadActionDontCare;
    }
}

MTLStoreAction dm_metal_convert_store(dm_render_store_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported store action");
            LOG_WARN("Returning MTLStoreActionStore");
        case DM_RENDER_STORE_OP_STORE:     return MTLStoreActionStore;
        case DM_RENDER_STORE_OP_DONT_CARE: return MTLStoreActionDontCare;
    }
}

id<MTLTexture> dm_metal_create_texture(id<MTLDevice> device, MTLPixelFormat format, dm_texture2d_type type, u16 width, u16 height, void *data, size_t *size)
{
    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:width height:height mipmapped:NO];
    switch(type)
    {
        default:
        case DM_TEXTURE2D_TYPE_COMBINED_SAMPLER:
        case DM_TEXTURE2D_TYPE_SAMPLED: texture_desc.usage = MTLTextureUsageShaderRead; break;
        case DM_TEXTURE2D_TYPE_STORAGE: texture_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite; break;
    }

    texture_desc.storageMode = MTLStorageModeShared;
    MTLSizeAndAlign size_align = [device heapTextureSizeAndAlignWithDescriptor:texture_desc];
    size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;
    *size = size_align.size;

    id<MTLTexture> texture = [device newTextureWithDescriptor:texture_desc];
    [texture_desc release];

    if(data)
    {
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);

        size_t bytes_per_row = width;
        if(format == MTLPixelFormatRGBA8Unorm) bytes_per_row *= 4;

        [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytes_per_row];
    }

    return texture;
}

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    u16 width = desc.color_attachment.width * context->window.scale_w;
    u16 height = desc.color_attachment.height * context->window.scale_h;

    dm_metal_render_target render_target = { 
        .depth=desc.depth,
        .swapchain=desc.swapchain,
        .width=width,
        .height=height
    };

    if(!desc.swapchain)
    {
        MTLPixelFormat format = DM_SWAPCHAIN_FORMAT;

        MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:width height:height mipmapped:NO];
        texture_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;

        MTLSizeAndAlign size_align = [renderer->device heapTextureSizeAndAlignWithDescriptor:texture_desc];
        size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;

        render_target.size = size_align.size;
        
        renderer->resource_heap.size += size_align.size;
        LOG_DEBUG("Texture size: %zu, Heap size: %zu", render_target.size, renderer->resource_heap.size);
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

    buffer.stride = desc.stride;

    MTLSizeAndAlign size_align = [renderer->device heapBufferSizeAndAlignWithLength:desc.size options:MTLResourceStorageModePrivate];
    size_align.size += (size_align.size & (size_align.align - 1)) + size_align.align;
    buffer.size = size_align.size;
    renderer->resource_heap.size += size_align.size;
    LOG_DEBUG("Buffer size: %zu, Heap size: %zu", buffer.size, renderer->resource_heap.size);

    if(desc.data)
    {
        buffer.host = [renderer->device newBufferWithBytes:desc.data length:buffer.size options:MTLResourceCPUCacheModeDefaultCache];
        if(!buffer.host)
        {
            LOG_ERROR("newBufferWithBytes failed");
            return false;
        }
    }
    else
    {
        buffer.host = [renderer->device newBufferWithLength:buffer.size options:MTLResourceCPUCacheModeDefaultCache];
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

MTLPixelFormat dm_metal_convert_format(dm_texture2d_format format)
{
    switch(format)
    {
        default:
            LOG_WARN("No texture format specified, or unsupported");
            LOG_WARN("Returning MTLPixelFormatRGBA8Unorm");
        case DM_TEXTURE2D_FORMAT_R8G8B8A8_UNORM: return MTLPixelFormatRGBA8Unorm;
        case DM_TEXTURE2D_FORMAT_A8_UNORM:       return MTLPixelFormatA8Unorm;
    }
}

bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    dm_metal_texture texture = { 0 };

    MTLPixelFormat format = dm_metal_convert_format(desc.format);
    texture.host = dm_metal_create_texture(renderer->device, format, desc.type, desc.width, desc.height, desc.data, &texture.size);
    if(!texture.host) return false;

    renderer->resource_heap.size += texture.size;
    LOG_DEBUG("Texture size: %zu, Heap size: %zu", texture.size, renderer->resource_heap.size);

    //
    renderer->textures[renderer->texture_count] = texture;
    handle->type = DM_RESOURCE_TYPE_TEXTURE;
    handle->index = renderer->texture_count++;

    return true;
}

MTLSamplerMinMagFilter dm_metal_convert_min_mag_filter(dm_sampler_filter filter)
{
    switch(filter)
    {
        default:
            LOG_WARN("Unknown/unsupported filter");
            LOG_WARN("Returning MTLSamplerMinMagFilterLinear");
        case DM_SAMPLER_FILTER_LINEAR:  return MTLSamplerMinMagFilterLinear;
        case DM_SAMPLER_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
    }
}

MTLSamplerMipFilter dm_metal_convert_mip_filter(dm_sampler_filter filter)
{
    switch(filter)
    {
        default:
            LOG_WARN("Unknown/unsupported filter");
            LOG_WARN("Returning MTLSamplerMinMagFilterLinear");
        case DM_SAMPLER_FILTER_LINEAR:  return MTLSamplerMipFilterLinear;
        case DM_SAMPLER_FILTER_NEAREST: return MTLSamplerMipFilterNearest;
    }
}

bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    dm_metal_sampler sampler = { 0 };

    MTLSamplerDescriptor *sampler_desc = [MTLSamplerDescriptor new];

    // TODO: configurable
    sampler_desc.rAddressMode = MTLSamplerAddressModeRepeat;
    sampler_desc.sAddressMode = MTLSamplerAddressModeRepeat;
    sampler_desc.tAddressMode = MTLSamplerAddressModeRepeat;

    sampler_desc.minFilter = dm_metal_convert_min_mag_filter(desc.min);
    sampler_desc.magFilter = dm_metal_convert_min_mag_filter(desc.mag);
    sampler_desc.mipFilter = dm_metal_convert_mip_filter(desc.mip);

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

bool dm_metal_upload_buffer_to_heap(dm_metal_buffer *buffer, dm_metal_heap *heap, id<MTLBlitCommandEncoder> blit_cmd)
{
    buffer->device = [heap->heap newBufferWithLength:buffer->size options:MTLResourceStorageModePrivate];

    if(!buffer->device) 
    {
        LOG_ERROR("newBufferWithLength failed");
        LOG_ERROR("Upload buffer to heap failed");
        LOG_ERROR("Heap size: %zu, Upload size: %zu, Buffer size: %zu", heap->size, heap->upload_size, buffer->size);
        return false;
    }
    heap->upload_size += buffer->size;

    if(!buffer->host.contents) return true;
    [blit_cmd copyFromBuffer:buffer->host sourceOffset:0 toBuffer:buffer->device destinationOffset:0 size:buffer->size];

    return true;
}

bool dm_metal_upload_texture_to_heap(dm_metal_texture *texture, dm_metal_heap *heap, id<MTLBlitCommandEncoder> blit_cmd)
{
    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor new];
    texture_desc.textureType = texture->host.textureType;
    texture_desc.pixelFormat = texture->host.pixelFormat;
    texture_desc.width = texture->host.width;
    texture_desc.height = texture->host.height;
    texture_desc.depth  = texture->host.depth;
    texture_desc.mipmapLevelCount = texture->host.mipmapLevelCount;
    texture_desc.arrayLength = texture->host.arrayLength;
    texture_desc.sampleCount = texture->host.sampleCount;
    texture_desc.storageMode = heap->heap.storageMode;

    texture->device = [heap->heap newTextureWithDescriptor:texture_desc];
    [texture_desc release];

    if(!texture->device)
    {
        LOG_ERROR("newTextureWithDescriptor failed");
        LOG_ERROR("Upload texture to heap failed");
        LOG_ERROR("Heap size: %zu, Upload size: %zu, Texture size: %zu", heap->size, heap->upload_size, texture->size);
        return false;
    }
    heap->upload_size += texture->size;

    [blit_cmd copyFromTexture:texture->host toTexture:texture->device];

    return true;
}

bool dm_metal_upload_render_target_to_heap(dm_metal_render_target *target, dm_metal_heap *heap)
{
    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:DM_SWAPCHAIN_FORMAT width:target->width height:target->height mipmapped:NO];
    texture_desc.storageMode = heap->heap.storageMode;
    texture_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;

    target->texture = [heap->heap newTextureWithDescriptor:texture_desc];
    [texture_desc release];

    if(!target->texture)
    {
        LOG_ERROR("newTextureWithDescriptor failed");
        LOG_ERROR("Upload render target to heap failed");
        LOG_ERROR("Heap size: %zu, Upload size: %zu, Texture size: %zu", heap->size, heap->upload_size, target->size);
        return false;
    }
    heap->upload_size += target->size;

    return true;
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_resource *resources[], u32 count)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    if(!renderer->resource_heap.heap)
    {
        renderer->resource_heap.size += DM_MEGABYTE;

        MTLHeapDescriptor *heap_desc = [MTLHeapDescriptor new];
        heap_desc.storageMode = MTLStorageModePrivate;
        heap_desc.size = renderer->resource_heap.size;

        LOG_INFO("Heap size: %zu", heap_desc.size);

        renderer->resource_heap.heap = [renderer->device newHeapWithDescriptor:heap_desc];

        [heap_desc release];

        if(!renderer->resource_heap.heap)
        {
            LOG_ERROR("newHeapWithDescriptor failed");
            return false;
        }
    }

    id<MTLCommandBuffer> cmd       = [renderer->gfx_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    dm_metal_heap *heap = &renderer->resource_heap;

    // upload to heap
    for(u32 i=0; i<count; i++)
    {
        dm_resource *resource = resources[i];

        switch(resource->type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                if(!dm_metal_upload_buffer_to_heap(&renderer->buffers[resource->index], heap, blit)) return false;
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                if(!dm_metal_upload_texture_to_heap(&renderer->textures[resource->index], heap, blit)) return false;
                break;

            case DM_RESOURCE_TYPE_SAMPLER:
                break;

            case DM_RESOURCE_TYPE_RENDER_TARGET:
                if(!dm_metal_upload_render_target_to_heap(&renderer->rts[resource->index], heap)) return false;
                break;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                return false;
        }
    }

    [blit endEncoding];
    [cmd commit];

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

bool dm_renderer_create_synchronization(dm_context *context, dm_synchronization_desc desc, dm_resource *handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    dm_metal_event event = { 0 };

    event.event = [renderer->device newEvent];

    renderer->events[renderer->event_count] = event;
    handle->index = renderer->event_count++;
    handle->type = DM_RESOURCE_TYPE_SYNCHRONIZATION;

    return true;
}

// commands
void dm_render_command_update_begin(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    frame_data->blit_cmd     = [renderer->gfx_queue commandBuffer];
    frame_data->blit_encoder = [frame_data->blit_cmd blitCommandEncoder];
}

void dm_render_command_update_end(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->blit_encoder endEncoding];
    [frame_data->blit_cmd     commit];
}

void dm_render_command_begin_rendering(dm_context *context, dm_resource handle, float r, float g, float b, float a, float d, dm_render_load_op color_load, dm_render_store_op color_store, dm_render_load_op depth_load, dm_render_store_op depth_store)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_render_target *target = &renderer->rts[handle.index];

    id<MTLTexture> color_texture = target->swapchain ? [renderer->swapchain.drawable texture] : target->texture;

    MTLClearColor clear = MTLClearColorMake(r, g, b, a);

    MTLRenderPassDescriptor *desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.colorAttachments[0].clearColor  = clear;
    desc.colorAttachments[0].loadAction  = dm_metal_convert_load(color_load);
    desc.colorAttachments[0].storeAction = dm_metal_convert_store(color_store);
    desc.colorAttachments[0].texture     = color_texture;

    if(target->depth)
    {
        desc.depthAttachment.clearDepth  = d;
        desc.depthAttachment.loadAction  = dm_metal_convert_load(depth_load);
        desc.depthAttachment.storeAction = dm_metal_convert_store(depth_store);
        desc.depthAttachment.texture     = renderer->swapchain.depth_texture;
    }

    frame_data->gfx_encoder = [frame_data->gfx_cmd renderCommandEncoderWithDescriptor:desc];

    MTLRenderStages resource_stages = MTLRenderStageVertex | MTLRenderStageFragment;

    [frame_data->gfx_encoder useHeap:renderer->resource_heap.heap stages:resource_stages];
}

void dm_render_command_end_rendering(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_render_target target = renderer->rts[handle.index];

    [frame_data->gfx_encoder endEncoding];
}

void dm_render_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    DM_ASSERT(handle.type==DM_PIPELINE_TYPE_RASTER, "Not a raster pipeline");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_raster_pipe pipeline = renderer->rps[handle.index];

    [frame_data->gfx_encoder setRenderPipelineState:pipeline.pipeline];
    [frame_data->gfx_encoder setDepthStencilState:pipeline.depth_state];
    [frame_data->gfx_encoder setCullMode:pipeline.cull_mode];
    [frame_data->gfx_encoder setFrontFacingWinding:pipeline.winding];
    [frame_data->gfx_encoder setTriangleFillMode:pipeline.fill_mode];

    renderer->active_pipeline = handle;
}

void dm_render_command_set_viewport(dm_context *context, int x, int y, int w, int h, float d_min, float d_max)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    w *= context->window.scale_w;
    h *= context->window.scale_h;

    MTLViewport viewport = {
        .originX=x, .originY=y,
        .width=w, .height=h,
        .znear=d_min, .zfar=d_max,
    };

    [frame_data.gfx_encoder setViewport:viewport];
}

void dm_render_command_set_scissor(dm_context *context, int x, int y, int w, int h)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    w *= context->window.scale_w;
    h *= context->window.scale_h;

    MTLScissorRect scissor = {
        .x=x, .y=y,
        .width=w,.height=h
    };

    [frame_data.gfx_encoder setScissorRect:scissor];
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_resource handle, size_t offset)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Not a buffer");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;

    renderer->active_index_buffer = renderer->buffers[handle.index];
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
                [vertex_encoder setTexture:renderer->rts[resource.index].texture atIndex:i];
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

void dm_render_command_draw(dm_context *context, u32 index_count, u32 index_offset, u32 instance_count, u32 vertex_offset)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_index_buffer.device, "No active index buffer");
    DM_ASSERT(renderer->active_pipeline.type==DM_PIPELINE_TYPE_RASTER, "Not a valid raster pipeline");

    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_buffer index_buffer = renderer->active_index_buffer;
    dm_metal_raster_pipe pipeline = renderer->rps[renderer->active_pipeline.index];

    MTLIndexType index_type;
    switch(index_buffer.stride)
    {
        default:
            LOG_WARN("Index size is not 16 or 32");
            LOG_WARN("Using MTLIndexTypeUInt16");
        case sizeof(u16): 
            index_type = MTLIndexTypeUInt16; 
            index_offset *= sizeof(u16);
            break;
        case sizeof(u32): 
            index_type = MTLIndexTypeUInt32; 
            index_offset *= sizeof(u32);
            break;
    }

    [frame_data->gfx_encoder drawIndexedPrimitives:pipeline.primitive_type indexCount:index_count indexType:index_type indexBuffer:index_buffer.device indexBufferOffset:index_offset instanceCount:instance_count baseVertex:vertex_offset baseInstance:0];
}

void dm_render_command_update_buffer(dm_context *context, dm_resource handle, void *data, size_t size, size_t offset)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Not a buffer");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_buffer buffer = renderer->buffers[handle.index];

    memcpy(buffer.host.contents + offset, data, size);

    [frame_data->blit_encoder copyFromBuffer:buffer.host sourceOffset:offset toBuffer:buffer.device destinationOffset:offset size:size];
}

bool dm_render_command_update_texture(dm_context *context, dm_resource handle, void* data, size_t size)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_TEXTURE, "Not a texture");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    dm_metal_texture *texture = &renderer->textures[handle.index];

    MTLRegion region = MTLRegionMake2D(0, 0, texture->host.width, texture->host.height);
    size_t bytes_per_row = texture->host.width;
    if(texture->host.pixelFormat == MTLPixelFormatRGBA8Unorm) bytes_per_row *= 4;

    [texture->host replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytes_per_row];
    [frame_data->blit_encoder copyFromTexture:texture->host toTexture:texture->device];

    return true;
}

bool dm_render_command_resize_render_target(dm_context *context, dm_resource resource, u16 width, u16 height)
{
    DM_ASSERT(resource.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Not a render target");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_render_target *target = &renderer->rts[resource.index];

    [target->texture release];

    renderer->resource_heap.upload_size -= target->size;

    return dm_metal_upload_render_target_to_heap(target, &renderer->resource_heap);
}

void dm_render_command_copy_texture(dm_context *context, dm_resource src, dm_resource dst)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    DM_ASSERT(src.type==DM_RESOURCE_TYPE_TEXTURE, "Src is not a texture");
    DM_ASSERT(dst.type==DM_RESOURCE_TYPE_TEXTURE, "Dst is not a texture");

    id<MTLTexture> src_texture = renderer->textures[src.index].device;
    id<MTLTexture> dst_texture = renderer->textures[dst.index].device;

    [frame_data->blit_encoder copyFromTexture:src_texture toTexture:dst_texture];
}

void dm_render_command_signal(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_SYNCHRONIZATION, "Not a sync resource");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_event *event = &renderer->events[handle.index];

    [frame_data->gfx_cmd encodeSignalEvent:event->event value:++event->value];
}

void dm_render_command_wait(dm_context *context, dm_resource handle)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_event event = renderer->events[handle.index];

    [frame_data->gfx_cmd encodeWaitForEvent:event.event value:event.value];
}

// compute commands
void dm_compute_command_begin_recording(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    frame_data->compute_encoder = [frame_data->compute_cmd computeCommandEncoder];

    [frame_data->compute_encoder useHeap:renderer->resource_heap.heap];
}

void dm_compute_command_end_recording(dm_context *context)
{
    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    [frame_data->compute_encoder endEncoding];
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
                [argument_encoder setTexture:renderer->rts[resource.index].texture atIndex:i];
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
    MTLSize group_size  = MTLSizeMake(pipeline.grp_x, pipeline.grp_y, pipeline.grp_z);

    [frame_data->compute_encoder dispatchThreadgroups:thread_size threadsPerThreadgroup:group_size];
}

void dm_compute_command_signal(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_SYNCHRONIZATION, "Not a sync resource");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_event *event = &renderer->events[handle.index];

    [frame_data->compute_cmd encodeSignalEvent:event->event value:++event->value];
}

void dm_compute_command_wait(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_SYNCHRONIZATION, "Not a sync resource");

    dm_metal_renderer *renderer = context->renderer.internal_renderer;
    dm_metal_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_metal_event event = renderer->events[handle.index];

    [frame_data->compute_cmd encodeWaitForEvent:event.event value:event.value];
}
