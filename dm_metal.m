#include "dm.h"

#ifdef DM_METAL

extern void* dm_window_get_internal(dm_context* context);

#include "RGFW/RGFW.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

typedef struct dm_metal_renderpass_t
{
    dm_resource_index render_target[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index depth_target[DM_MAX_FRAMES_IN_FLIGHT];
} dm_metal_renderpass;

typedef struct dm_metal_buffer_t
{
    dm_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index device[DM_MAX_FRAMES_IN_FLIGHT]; 
} dm_metal_buffer;

typedef struct dm_metal_texture_t
{
    dm_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index device[DM_MAX_FRAMES_IN_FLIGHT];
    MTLPixelFormat format;
} dm_metal_texture;

typedef struct dm_metal_index_buffer_t
{
    dm_metal_buffer buffer;
    MTLIndexType index_type;
} dm_metal_index_buffer;

typedef struct dm_metal_sampler_t
{
    id<MTLSamplerState> state;
} dm_metal_sampler;

typedef struct dm_metal_raster_pipeline_t
{
    id<MTLRenderPipelineState> pipeline_state;

    id<MTLLibrary>  vertex_library;
    id<MTLLibrary>  fragment_library;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;

    uint32_t uniform_offset;
    
    MTLCullMode cull_mode;
    MTLWinding  winding;

    MTLPrimitiveType primitive_type;
    MTLTriangleFillMode fill_mode;

    MTLViewport    viewport;
    MTLScissorRect scissor;

    uint32_t vertex_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t fragment_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];

    id<MTLArgumentEncoder> vertex_encoder;
    id<MTLArgumentEncoder> fragment_encoder;
} dm_metal_raster_pipeline;

typedef struct dm_metal_heap_t
{
    id<MTLHeap> heap;
    size_t size;
} dm_metal_heap;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;

    id<MTLCommandQueue>          command_queue;
    
    id<MTLCommandBuffer>         render_command_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLRenderCommandEncoder>  render_command_encoder[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLBlitCommandEncoder>    render_blit_encoder[DM_MAX_FRAMES_IN_FLIGHT];

    id<MTLCommandBuffer>         compute_command_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLComputeCommandEncoder> compute_command_encoder[DM_MAX_FRAMES_IN_FLIGHT];

    CAMetalLayer* swapchain;
    id<CAMetalDrawable> render_target;
    uint32_t depth_target[DM_MAX_FRAMES_IN_FLIGHT];

    id<MTLDepthStencilState> depth_stencil_state;
    
    dispatch_semaphore_t frame_semaphore;

    dm_metal_heap resource_heap[DM_MAX_FRAMES_IN_FLIGHT];

    // regular buffers, 3 argument for raster pipes, times frames in flight
    id<MTLBuffer> buffers[(DM_MAX_BUFFERS + DM_MAX_RASTER_PIPES * 2 + 1) * DM_MAX_FRAMES_IN_FLIGHT];     
    uint32_t      buffer_count;

    // regular textures, 2 for renderpasses, depth target, time frames in flight
    id<MTLTexture> textures[(DM_MAX_TEXTURES + DM_MAX_RENDERPASS + 1) * DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t       texture_count;

    dm_metal_renderpass renderpasses[DM_MAX_RENDERPASS];
    dm_metal_raster_pipeline raster_pipes[DM_MAX_RASTER_PIPES];
    dm_metal_buffer vertex_buffers[DM_MAX_VBS];
    dm_metal_index_buffer index_buffers[DM_MAX_IBS];
    dm_metal_buffer constant_buffers[DM_MAX_CBS];
    dm_metal_buffer storage_buffers[DM_MAX_SBS];
    dm_metal_texture metal_textures[10];
    dm_metal_sampler samplers[DM_MAX_SAMPLERS];

    uint32_t renderpass_count, raster_pipe_count, vb_count, ib_count, cb_count, sb_count, metal_texture_count, sampler_count;

    dm_pipeline_handle active_pipeline;
    dm_resource_handle active_index_buffer;

    uint8_t current_frame;

    // TODO: not sure if I like this
    uint32_t texture_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLArgumentEncoder> texture_encoder;
} dm_metal_renderer;

// === backend ===
void* dm_renderer_init_backend(dm_context* context)
{
    dm_metal_renderer backend = { 0 };

#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing Metal backend...");
#endif
    backend.device = MTLCreateSystemDefaultDevice();
    if(!backend.device) { dm_log(DM_LOG_FATAL, "Could not create Metal device"); return NULL; }

    // check feature support
    if(backend.device.argumentBuffersSupport!=MTLArgumentBuffersTier2) { dm_log(DM_LOG_FATAL, "ArgumentBuffersTier2 not supported"); return NULL; }
#ifdef DM_HARDWARE_RAYTRACING
    if(!backend->device.supportsRaytracing) { DM_LOG_FATAL("Device does not support ray tracing"); return false; }
#endif // DM_HARDWARE_RAYTRACING

    // swapchain
    backend.swapchain        = [CAMetalLayer layer];
    backend.swapchain.device = backend.device;
    backend.swapchain.opaque = YES;

    backend.command_queue = [backend.device newCommandQueue];
    if(!backend.command_queue) { dm_log(DM_LOG_FATAL, "newCommandQueue failed"); return false; }

    // must set the content view's layer to our metal layer
    MTKView* view = dm_window_get_internal(context);
    [view setWantsLayer: YES];
    [view setLayer:backend.swapchain];
    
    uint32_t w = dm_get_window_width(context);
    uint32_t h = dm_get_window_height(context);
    CGSize drawable_size = CGSizeMake(w,h);
    backend.swapchain.drawableSize = drawable_size;
    
    // depth texture 
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        backend.textures[backend.texture_count] = [backend.device newTextureWithDescriptor:descriptor];

        backend.depth_target[i] = backend.texture_count++;
    }

    [descriptor release];
    
    MTLDepthStencilDescriptor* depth_descriptor = [MTLDepthStencilDescriptor new];
    depth_descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    depth_descriptor.depthWriteEnabled = YES;
    backend.depth_stencil_state = [backend.device newDepthStencilStateWithDescriptor:depth_descriptor];

    [depth_descriptor release];

    // command queue 
    backend.command_queue = [backend.device newCommandQueue];

    // synchronization 
    backend.frame_semaphore = dispatch_semaphore_create(DM_MAX_FRAMES_IN_FLIGHT);

    dm_metal_renderer* b = dm_alloc(sizeof(dm_metal_renderer));
    dm_memcpy(b, &backend, sizeof(backend));

    return b;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_copy_buffer_to_heap(dm_metal_buffer* buffer, id<MTLBlitCommandEncoder> blit_encoder, dm_metal_renderer* backend)
{
    id<MTLBuffer> host_buffer;
    id<MTLBuffer> device_buffer;
    size_t size;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        host_buffer = backend->buffers[buffer->host[i]];
        size = host_buffer.length;

        device_buffer = [backend->resource_heap[i].heap newBufferWithLength:size options:MTLResourceStorageModePrivate];
        if(!device_buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }
        [blit_encoder copyFromBuffer:host_buffer sourceOffset:0 toBuffer:device_buffer destinationOffset:0 size:size];

        backend->buffers[backend->buffer_count] = device_buffer;
        
        buffer->device[i] = backend->buffer_count++;
    }

    return true;
}

bool dm_renderer_finish_init_backend(void* b)
{
    dm_metal_renderer* backend = b;

    id<MTLCommandBuffer> command_buffer = [backend->command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = command_buffer.blitCommandEncoder;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        MTLHeapDescriptor* descriptor = [MTLHeapDescriptor new];
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.size        = backend->resource_heap[i].size;
#ifdef DM_DEBUG 
        dm_log(DM_LOG_DEBUG, "Resource heap (frame %u) size: %u bytes", i, descriptor.size); 
#endif  

        backend->resource_heap[i].heap = [backend->device newHeapWithDescriptor:descriptor];

        [descriptor release];
    }

    // move resources to the heap
    for(uint32_t i=0; i<backend->vb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&backend->vertex_buffers[i], blit_encoder, backend)) return false;
    }
    for(uint32_t i=0; i<backend->ib_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&backend->index_buffers[i].buffer, blit_encoder, backend)) return false;
    }
    for(uint32_t i=0; i<backend->cb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&backend->constant_buffers[i], blit_encoder, backend)) return false;
    }
    for(uint32_t i=0; i<backend->sb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&backend->storage_buffers[i], blit_encoder, backend)) return false;
    }

    for(uint32_t i=0; i<backend->metal_texture_count; i++)
    {
        dm_metal_texture t = backend->metal_textures[i];

        for(uint8_t j=0; j<DM_MAX_FRAMES_IN_FLIGHT; j++)
        {
            MTLTextureDescriptor* desc = [MTLTextureDescriptor new];

            id<MTLTexture> host_texture = backend->textures[t.host[j]];

            desc.width  = host_texture.width;
            desc.height = host_texture.height;
            desc.pixelFormat = t.format;
            desc.storageMode = backend->resource_heap[j].heap.storageMode;

            id<MTLTexture> heap_texture = [backend->resource_heap[j].heap newTextureWithDescriptor:desc];
            if(!heap_texture) { dm_log(DM_LOG_FATAL, "newTextureWithDescriptor failed"); return false; }

            MTLRegion region = MTLRegionMake2D(0,0, host_texture.width,host_texture.height);

            [blit_encoder copyFromTexture:host_texture sourceSlice:0 sourceLevel:0 sourceOrigin:region.origin sourceSize:region.size toTexture:heap_texture destinationSlice:0 destinationLevel:0 destinationOrigin:region.origin];

            [desc release];

            backend->textures[backend->texture_count] = heap_texture;
            backend->metal_textures[i].device[j] = backend->texture_count++;
        }
    }

    // argument encoder
    MTLArgumentDescriptor* descriptors[DM_MAX_TEXTURES + DM_MAX_SAMPLERS];

    uint32_t index = 0;
    for(uint32_t i=0; i<DM_MAX_TEXTURES; i++)
    {
        descriptors[index] = [MTLArgumentDescriptor new];
        descriptors[index].dataType    = MTLDataTypeTexture;
        descriptors[index].textureType = MTLTextureType2D;
        descriptors[index].index       = index;
        index++;
    }
    for(uint32_t i=0; i<DM_MAX_SAMPLERS; i++)
    {
        descriptors[index] = [MTLArgumentDescriptor new];
        descriptors[index].dataType = MTLDataTypeSampler;
        descriptors[index].index    = index;
        index++;
    }

    NSArray* argument_array = [NSArray arrayWithObjects:descriptors count:(DM_MAX_TEXTURES + DM_MAX_SAMPLERS)];
    backend->texture_encoder = [backend->device newArgumentEncoderWithArguments:argument_array];

    size_t size = backend->texture_encoder.encodedLength;
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        id<MTLBuffer> buffer = [backend->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        backend->buffers[backend->buffer_count] = buffer;
        backend->texture_argument_buffer[i] = backend->buffer_count++;
    }

    for(uint32_t i=0; i<DM_MAX_SAMPLERS+DM_MAX_TEXTURES; i++)
    {
        [descriptors[i] release];
    }

    [blit_encoder endEncoding];
    [command_buffer commit];

    return true;
}

void dm_renderer_shutdown_backend(void* b)
{
    dm_metal_renderer* backend = b;

    for(uint32_t i=0; i<backend->raster_pipe_count; i++)
    {
        [backend->raster_pipes[i].fragment_encoder release];
        [backend->raster_pipes[i].vertex_encoder release];

        [backend->raster_pipes[i].fragment_func release];
        [backend->raster_pipes[i].vertex_func release];

        [backend->raster_pipes[i].pipeline_state release];
    }

    for(uint32_t i=0; i<backend->sampler_count; i++)
    {
        [backend->samplers[i].state release];
    }

    for(uint32_t i=0; i<backend->buffer_count; i++)
    {
        [backend->buffers[i] release];
    }
    for(uint32_t i=0; i<backend->texture_count; i++)
    {
        [backend->textures[i] release];
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        [backend->compute_command_encoder[i] release];
        [backend->resource_heap[i].heap release];
    }

    [backend->texture_encoder release];
    [backend->depth_stencil_state release];
    [backend->swapchain release];
    [backend->command_queue release];
    [backend->device release];
}

bool dm_renderer_resize_backend(uint32_t width, uint32_t height, void* b)
{
    dm_metal_renderer* backend = b;

    backend->swapchain.drawableSize = CGSizeMake(width, height);
    
    // depth texture
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        [backend->textures[backend->depth_target[i]] release];
        backend->textures[backend->depth_target[i]] = [backend->device newTextureWithDescriptor:descriptor];
    }

    return true;
}

/************
* RESOURCES *
*************/
bool dm_create_renderpass_backend(dm_renderpass_desc desc, dm_renderpass_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_renderpass renderpass = { 0 };

    switch(desc.type)
    {
        case DM_RENDERPASS_TYPE_DEFAULT:
        break;

        case DM_RENDERPASS_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom renderpasses not supported yet");
        default:
        return false;
    }

    backend->renderpasses[backend->renderpass_count] = renderpass;
    *handle = backend->renderpass_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_shader(const char* path, id<MTLLibrary>* library, id<MTLDevice> device)
{
    NSString* file = [NSString stringWithUTF8String:path];

    NSURL* library_url = [NSURL URLWithString:file];
    NSError* library_error = NULL;

    *library = [device newLibraryWithURL:library_url error:&library_error];
    if(!*library)
    {
        dm_log(DM_LOG_FATAL, "Creating Metal shader library failed");
        dm_log(DM_LOG_ERROR, "%s", [library_error.localizedDescription UTF8String]);
        [file release];
        [library_url release];

        return false;
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_shader_function(const char* shader_function, id<MTLFunction>* function, id<MTLLibrary> library, id<MTLDevice> device)
{
    NSString* func_name = [[NSString alloc] initWithUTF8String:shader_function];

    *function = [library newFunctionWithName:func_name];

    if(!*function) { dm_log(DM_LOG_FATAL, "Failed to create Metal function"); return false; }

    [func_name release];
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_argument_buffer(uint32_t* index, id<MTLArgumentEncoder> encoder, dm_metal_renderer* backend)
{
    size_t size = encoder.encodedLength;
    id<MTLBuffer> buffer = [backend->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
    if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }

    backend->buffers[backend->buffer_count] = buffer;
    *index = backend->buffer_count++;

    return true;
}

bool dm_create_raster_pipeline_backend(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_raster_pipeline pipeline = { 0 };

    // shaders 
    if(!dm_metal_create_shader(desc.rasterizer.vertex_shader_desc.path, &pipeline.vertex_library, backend->device))      return false;
    if(!dm_metal_create_shader_function("vertex_main", &pipeline.vertex_func, pipeline.vertex_library, backend->device)) return false;

    if(!dm_metal_create_shader(desc.rasterizer.pixel_shader_desc.path, &pipeline.fragment_library, backend->device))           return false;
    if(!dm_metal_create_shader_function("fragment_main", &pipeline.fragment_func, pipeline.fragment_library, backend->device)) return false;

    // pipeline state
    MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
    pipe_desc.rasterSampleCount = 1;

    pipe_desc.vertexFunction   = pipeline.vertex_func;
    pipe_desc.fragmentFunction = pipeline.fragment_func;

    //pipe_desc.colorAttachments[0].pixelFormat                 = MTLPixelFormatRGBA8Unorm;
    pipe_desc.colorAttachments[0].pixelFormat                 = MTLPixelFormatBGRA8Unorm;
    pipe_desc.colorAttachments[0].blendingEnabled             = YES;
    pipe_desc.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
    pipe_desc.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    pipe_desc.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    pipe_desc.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;
    pipe_desc.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
    pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    NSError* error = NULL;
    pipeline.pipeline_state = [backend->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
    if(!pipeline.pipeline_state)
    {
        dm_log(DM_LOG_FATAL, "Creating Metal pipeline state failed");
        dm_log(DM_LOG_ERROR, "%s", [error.localizedDescription UTF8String]);
        [pipeline.vertex_library release];
        [pipeline.fragment_library release];
        [pipeline.vertex_func release];
        [pipeline.fragment_func release];

        return false;
    }

    // misc
    switch(desc.rasterizer.cull_mode)
    {
        case DM_RASTERIZER_CULL_MODE_BACK:
        pipeline.cull_mode = MTLCullModeBack;
        break;

        case DM_RASTERIZER_CULL_MODE_FRONT:
        pipeline.cull_mode = MTLCullModeFront;
        break;

        case DM_RASTERIZER_CULL_MODE_NONE:
        pipeline.cull_mode = MTLCullModeNone;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported cull mode");
        return false;
    }

    switch(desc.rasterizer.front_face)
    {
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pipeline.winding = MTLWindingCounterClockwise;
        break;

        case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
        pipeline.winding = MTLWindingClockwise;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported winding");
        return false;
    }

    switch(desc.input_assembler.topology)
    {
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.primitive_type = MTLPrimitiveTypeTriangle;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.primitive_type = MTLPrimitiveTypeLine;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported topology");
        return false;
    }

    pipeline.fill_mode = desc.rasterizer.polygon_fill==DM_RASTERIZER_POLYGON_FILL_WIREFRAME ? MTLTriangleFillModeLines : MTLTriangleFillModeFill;

    pipeline.vertex_encoder   = [pipeline.vertex_func newArgumentEncoderWithBufferIndex:0];
    if(!pipeline.vertex_encoder) { dm_log(DM_LOG_FATAL, "newArgumentEncoderWithBufferIndex failed"); return false; }
    pipeline.fragment_encoder = [pipeline.fragment_func newArgumentEncoderWithBufferIndex:1];
    if(!pipeline.fragment_encoder) { dm_log(DM_LOG_FATAL, "newArgumentEncoderWithBufferIndex failed"); return false; }

    // argument buffer
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_metal_create_argument_buffer(&pipeline.vertex_argument_buffer[i], pipeline.vertex_encoder, backend)) return false;
        if(!dm_metal_create_argument_buffer(&pipeline.fragment_argument_buffer[i], pipeline.fragment_encoder, backend)) return false;
    }

    //
    backend->raster_pipes[backend->raster_pipe_count] = pipeline;
    handle->index = backend->raster_pipe_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_buffer(void* data, size_t size, dm_metal_buffer* buffer, dm_metal_renderer* backend) 
{
    id<MTLBuffer> host_buffer;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(data) 
        {
            host_buffer = [backend->device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache]; 
            if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithBytes failed"); return false; }
        }
        else
        {
            host_buffer = [backend->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
            if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }
        }

        backend->buffers[backend->buffer_count] = host_buffer;
        buffer->host[i] = backend->buffer_count++;

        // heap size
        size_t size = host_buffer.length; 
        MTLSizeAndAlign size_and_align = [backend->device heapBufferSizeAndAlignWithLength:size options:MTLResourceStorageModePrivate];
        size_and_align.size += (size_and_align.size & (size_and_align.align - 1)) + size_and_align.align;
        backend->resource_heap[i].size += size_and_align.size;
    }

    return true;
}

bool dm_create_vertex_buffer_backend(dm_vertex_buffer_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, backend)) return false;

    backend->vertex_buffers[backend->vb_count] = buffer;
    handle->index = backend->vb_count++;

    return true;
}

bool dm_create_index_buffer_backend(dm_index_buffer_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_type = MTLIndexTypeUInt16;
        break;

        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_type = MTLIndexTypeUInt32;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported index type");
        return false;
    }
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer.buffer, backend)) return false; 
    
    //
    backend->index_buffers[backend->ib_count] = buffer;
    handle->index = backend->ib_count++;

    return true;
}

bool dm_create_constant_buffer_backend(dm_constant_buffer_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, backend)) return false;

    backend->constant_buffers[backend->cb_count] = buffer;
    handle->index = backend->cb_count++;

    return true;
}

bool dm_create_storage_buffer_backend(dm_storage_buffer_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, backend)) return false;

    backend->storage_buffers[backend->sb_count] = buffer;
    handle->index = backend->sb_count++;

    return true;
}

bool dm_create_texture_backend(dm_texture_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_texture texture = { 0 };

    size_t bytes_per_pixel = 0;
    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        texture.format = MTLPixelFormatRGBA8Unorm;
        bytes_per_pixel = desc.n_channels * 1;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported texture format for Metal");
        return false;
    }

    MTLTextureDescriptor* texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:texture.format width:desc.width height:desc.height mipmapped:NO];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        id<MTLTexture> t = [backend->device newTextureWithDescriptor:texture_desc];
        
        MTLRegion region = MTLRegionMake2D(0,0, desc.width,desc.height);
        
        [t replaceRegion:region mipmapLevel:0 withBytes:desc.data bytesPerRow:(bytes_per_pixel * desc.width)];

        backend->textures[backend->texture_count] = t;
        texture.host[i] = backend->texture_count++;

        //
        MTLSizeAndAlign size_and_align = [backend->device heapTextureSizeAndAlignWithDescriptor:texture_desc];
        size_and_align.size += (size_and_align.size & (size_and_align.align - 1)) + size_and_align.align;
        backend->resource_heap[i].size += size_and_align.size;
    }

    //
    backend->metal_textures[backend->metal_texture_count] = texture;
    handle->index = backend->metal_texture_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif // DM_DEBUG
MTLSamplerAddressMode dm_convert_sampler_address(dm_sampler_address_mode mode)
{
    switch(mode)
    {
        default:
        case DM_SAMPLER_ADDRESS_MODE_WRAP:   return MTLSamplerAddressModeRepeat;
        case DM_SAMPLER_ADDRESS_MODE_BORDER: return MTLSamplerAddressModeClampToEdge;
    }
}

bool dm_create_sampler_backend(dm_sampler_desc desc, dm_resource_handle* handle, void* b)
{
    dm_metal_renderer* backend = b;

    dm_metal_sampler sampler = { 0 };

    MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];

    sampler_desc.rAddressMode = dm_convert_sampler_address(desc.address_u);
    sampler_desc.sAddressMode = dm_convert_sampler_address(desc.address_v);
    sampler_desc.tAddressMode = dm_convert_sampler_address(desc.address_w);

    sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
    sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;

    sampler_desc.supportArgumentBuffers = YES;

    sampler.state = [backend->device newSamplerStateWithDescriptor:sampler_desc];

    [sampler_desc release];
    
    //
    backend->samplers[backend->sampler_count] = sampler;
    handle->index = backend->sampler_count++;

    return true;
}

// === commands ===
bool dm_render_command_begin_frame_backend(void* b) 
{ 
    dm_metal_renderer* backend = b;

    dispatch_semaphore_wait(backend->frame_semaphore, DISPATCH_TIME_FOREVER);

    // new render target
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    CGSize size = backend->swapchain.bounds.size;
    backend->swapchain.contentsScale = scale;
    backend->swapchain.drawableSize = size;
    
    backend->render_target = [backend->swapchain nextDrawable];
    if(!backend->render_target) { dm_log(DM_LOG_FATAL, "nextDrawable failed"); return false; }

    // command buffer
    MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
    desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    backend->render_command_buffer[backend->current_frame] = [backend->command_queue commandBufferWithDescriptor:desc];
    [desc release];

    // pipeline
    backend->active_pipeline.type = DM_PIPELINE_TYPE_INVALID;

    return true; 
}

bool dm_render_command_end_frame_backend(bool vsync, void* b) 
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    backend->swapchain.displaySyncEnabled = vsync; 
    [backend->render_command_buffer[current_frame] presentDrawable:backend->render_target];

    // sync
    __block dispatch_semaphore_t block_semaphore = backend->frame_semaphore;
    [backend->render_command_buffer[current_frame] addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(block_semaphore);
    }];

    // final commit
    [backend->render_command_buffer[current_frame] commit];
    [backend->render_command_buffer[current_frame] waitUntilCompleted];

    // update frame
    backend->current_frame = (backend->current_frame + 1) % DM_MAX_FRAMES_IN_FLIGHT;

    return true; 
}

bool dm_render_command_begin_update_backend(void* b) 
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    backend->render_blit_encoder[current_frame] = [backend->render_command_buffer[current_frame] blitCommandEncoder];

    return true; 
}

bool dm_render_command_end_update_backend(void* b)
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    [backend->render_blit_encoder[current_frame] endEncoding];

    return true; 
}

bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, void* bknd)
{ 
    dm_metal_renderer* backend = bknd;

    const uint8_t current_frame = backend->current_frame;

    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];

    pass_desc.colorAttachments[0].texture     = [backend->render_target texture]; 
    pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass_desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
    pass_desc.colorAttachments[0].clearColor  = MTLClearColorMake(r,g,b,a);

    pass_desc.depthAttachment.texture     = backend->textures[backend->depth_target[current_frame]]; 
    pass_desc.depthAttachment.clearDepth  = depth;
    pass_desc.depthAttachment.storeAction = MTLStoreActionDontCare;
    pass_desc.depthAttachment.loadAction  = MTLLoadActionClear;

    dm_metal_renderpass* pass = &backend->renderpasses[handle];

    id<MTLCommandBuffer> command_buffer = backend->render_command_buffer[current_frame];
    id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if(!encoder) { dm_log(DM_LOG_FATAL, "renderCommandEncoderWithDescriptor failed"); return false; }

    backend->render_command_encoder[current_frame] = encoder;

    // bind heap
    [encoder useHeap:backend->resource_heap[current_frame].heap stages:MTLRenderStageVertex];
    [encoder useHeap:backend->resource_heap[current_frame].heap stages:MTLRenderStageFragment];

    // depth state
    [encoder setDepthStencilState:backend->depth_stencil_state];

    // argument buffer
    [backend->texture_encoder setArgumentBuffer:backend->buffers[backend->texture_argument_buffer[current_frame]] offset:0];

    for(uint32_t i=0; i<backend->metal_texture_count; i++)
    {
        [backend->texture_encoder setTexture:backend->textures[backend->metal_textures[i].device[current_frame]] atIndex:i];
    }
    for(uint32_t i=0; i<backend->sampler_count; i++)
    {
        [backend->texture_encoder setSamplerState:backend->samplers[i].state atIndex:(DM_MAX_TEXTURES + i)];
    }

    [encoder setFragmentBuffer:backend->buffers[backend->texture_argument_buffer[current_frame]] offset:0 atIndex:0];

    return true; 
}

bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, void* b) 
{ 
    dm_metal_renderer* backend = b;

    [backend->render_command_encoder[backend->current_frame] endEncoding];
    
    return true; 
}

bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, void* b) 
{
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    id<MTLRenderCommandEncoder> encoder = backend->render_command_encoder[current_frame];
    dm_metal_raster_pipeline* pipeline = &backend->raster_pipes[handle.index];

    pipeline->viewport.width  = backend->swapchain.drawableSize.width;
    pipeline->viewport.height = backend->swapchain.drawableSize.height;
    pipeline->viewport.zfar   = 1.f;

    pipeline->scissor.x = 0;
    pipeline->scissor.y = 0;
    pipeline->scissor.width  = backend->swapchain.drawableSize.width;
    pipeline->scissor.height = backend->swapchain.drawableSize.height;

    [encoder setRenderPipelineState:pipeline->pipeline_state];
    [encoder setFrontFacingWinding:pipeline->winding];
    [encoder setCullMode:pipeline->cull_mode];
    [encoder setViewport:pipeline->viewport];
    [encoder setScissorRect:pipeline->scissor];
    [encoder setTriangleFillMode:pipeline->fill_mode];

    backend->active_pipeline = handle;

    return true; 
}

bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, void* b) 
{
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    if(backend->active_pipeline.type==DM_PIPELINE_TYPE_INVALID) { dm_log(DM_LOG_FATAL, "No pipeline is set"); return false; }

    id<MTLRenderCommandEncoder> encoder = backend->render_command_encoder[current_frame];

    switch(backend->active_pipeline.type)
    { 
        case DM_PIPELINE_TYPE_RASTER:
        {
            dm_metal_raster_pipeline pipeline = backend->raster_pipes[backend->active_pipeline.index];

            id<MTLBuffer> vertex_argument_buffer   = backend->buffers[pipeline.vertex_argument_buffer[current_frame]];
            id<MTLBuffer> fragment_argument_buffer = backend->buffers[pipeline.fragment_argument_buffer[current_frame]];

            [pipeline.vertex_encoder setArgumentBuffer:vertex_argument_buffer offset:0];
            [pipeline.fragment_encoder setArgumentBuffer:fragment_argument_buffer offset:0];

            uint8_t index = 0;

            // set up user argument buffers
            for(uint16_t i=0; i<count; i++)
            {
                switch(handles[i].type)
                {
                    case DM_RESOURCE_TYPE_CONSTANT_BUFFER:
                    {
                        id<MTLBuffer> buffer = backend->buffers[backend->constant_buffers[handles[i].index].device[current_frame]];

                        // TODO: FIX!
                        [pipeline.vertex_encoder setBuffer:buffer offset:0 atIndex:index];
                        [pipeline.fragment_encoder setBuffer:buffer offset:0 atIndex:index++];
                    } break;
                    case DM_RESOURCE_TYPE_STORAGE_BUFFER:
                    {
                        id<MTLBuffer> buffer = backend->buffers[backend->storage_buffers[handles[i].index].device[current_frame]];

                        // TODO: FIX!
                        [pipeline.vertex_encoder setBuffer:buffer offset:0 atIndex:index];
                        [pipeline.fragment_encoder setBuffer:buffer offset:0 atIndex:index++];
                    } break;

                    case DM_RESOURCE_TYPE_TEXTURE:
                    {
                        id<MTLTexture> texture = backend->textures[backend->metal_textures[handles[i].index].device[current_frame]];

                        [pipeline.fragment_encoder setTexture:texture atIndex:index++];
                    } break;
                    case DM_RESOURCE_TYPE_SAMPLER:
                    {
                        id<MTLSamplerState> sampler = backend->samplers[handles[i].index].state;

                        [pipeline.fragment_encoder setSamplerState:sampler atIndex:index++];
                    } break;

                    default:
                    dm_log(DM_LOG_FATAL, "Unknown/unsupported resource type");
                    return false;
                }
            }

            [encoder setVertexBuffer:vertex_argument_buffer offset:0 atIndex:0];
            [encoder setFragmentBuffer:fragment_argument_buffer offset:0 atIndex:1];
        } break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported pipeline type");
        return false;
    }

    return true; 
}

bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, void* b)
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    id<MTLRenderCommandEncoder> encoder = backend->render_command_encoder[current_frame];

    dm_metal_buffer buffer = backend->vertex_buffers[handle.index];

    id<MTLBuffer> vertex_buffer = backend->buffers[buffer.device[current_frame]];

    // always have an argument buffer being bound
    // vertex buffer must be second buffer
    // TODO: theoretically could be more than second buffer. should have an increasing slot
    [encoder setVertexBuffer:vertex_buffer offset:offset atIndex:(slot+1)];

    return true; 
}

bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, void* b) 
{ 
    dm_metal_renderer* backend = b;

    backend->active_index_buffer = handle;

    return true; 
}

#ifndef DM_DEBUG
DM_INLINE
#endif
void dm_metal_update_buffer(void* data, size_t size, size_t offset, dm_metal_buffer buffer, dm_metal_renderer* backend)
{
    const uint8_t current_frame = backend->current_frame;

    id<MTLBlitCommandEncoder> blit_encoder = backend->render_blit_encoder[current_frame];

    dm_memcpy(backend->buffers[buffer.host[current_frame]].contents + offset, data, size);

    id<MTLBuffer> host_buffer   = backend->buffers[buffer.host[current_frame]];
    id<MTLBuffer> device_buffer = backend->buffers[buffer.device[current_frame]];

    [blit_encoder copyFromBuffer:host_buffer sourceOffset:offset toBuffer:device_buffer destinationOffset:offset size:size];
}

bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* b) 
{ 
    dm_metal_renderer* backend = b;

    dm_metal_update_buffer(data, size, offset, backend->vertex_buffers[handle.index], backend);

    return true; 
}

bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* b) 
{ 
    dm_metal_renderer* backend = b;

    dm_metal_update_buffer(data, size, offset, backend->index_buffers[handle.index].buffer, backend);

    return true; 
}

bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* b) 
{ 
    dm_metal_renderer* backend = b;

    dm_metal_update_buffer(data, size, offset, backend->constant_buffers[handle.index], backend);

    return true; 
}

bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* b)
{ 
    dm_metal_renderer* backend = b;

    dm_metal_update_buffer(data, size, offset, backend->storage_buffers[handle.index], backend);

    return true; 
}

bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, void* b) { return true; }

bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, void* b) 
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    id<MTLRenderCommandEncoder> encoder = backend->render_command_encoder[current_frame];
    dm_metal_raster_pipeline pipeline = backend->raster_pipes[backend->active_pipeline.index];

    [encoder drawPrimitives:pipeline.primitive_type vertexStart:vertex_offset vertexCount:vertex_count instanceCount:instance_count baseInstance:instance_offset];
    return true; 
}

bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, void* b)
{ 
    dm_metal_renderer* backend = b;

    const uint8_t current_frame = backend->current_frame;

    id<MTLRenderCommandEncoder> encoder = backend->render_command_encoder[current_frame];

    dm_metal_raster_pipeline pipeline = backend->raster_pipes[backend->active_pipeline.index];
    dm_metal_index_buffer buffer = backend->index_buffers[backend->active_index_buffer.index];

    id<MTLBuffer> index_buffer = backend->buffers[buffer.buffer.device[current_frame]];

    [encoder drawIndexedPrimitives:pipeline.primitive_type indexCount:index_count indexType:buffer.index_type indexBuffer:index_buffer indexBufferOffset:index_offset instanceCount:instance_count baseVertex:vertex_offset baseInstance:instance_offset];
    return true; 
}

#endif // DM_METAL
