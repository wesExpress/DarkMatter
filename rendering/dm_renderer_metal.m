#include "dm.h"

#include "platform/dm_platform_mac.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

#include <stdbool.h>

#define DM_METAL_NUM_FRAMES 3

typedef struct dm_metal_vertex_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        stride, count;
} dm_metal_vertex_buffer;

typedef struct dm_metal_index_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        index_size, count;
} dm_metal_index_buffer;

typedef struct dm_metal_structured_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        stride, count;
} dm_metal_structured_buffer;

typedef struct dm_metal_constant_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        size;
} dm_metal_constant_buffer;

typedef struct dm_metal_texture_t
{
    id<MTLTexture> texture[DM_METAL_NUM_FRAMES];
    uint32_t       width, height;
} dm_metal_texture;

#ifdef DM_RAYTRACING
typedef struct dm_metal_as_t {
    id<MTLBuffer> scratch_buffer;
    id<MTLBuffer> result_buffer;
    
    id<MTLAccelerationStructure> as;
    id<MTLAccelerationStructure> compact_as;
} dm_metal_as;

typedef struct dm_metal_blas_t
{
    dm_metal_as as[DM_METAL_NUM_FRAMES];
} dm_metal_blas;

typedef struct dm_metal_tlas_t
{
    dm_metal_as as[DM_METAL_NUM_FRAMES];
    
    id<MTLBuffer> instance_buffer[DM_METAL_NUM_FRAMES];
} dm_metal_tlas;

#define DM_METAL_MAX_BLAS 10
typedef struct dm_metal_acceleration_structure_t
{
    dm_metal_blas blas[DM_METAL_MAX_BLAS];
    uint8_t       blas_count;
    
    dm_metal_tlas tlas;
    
    NSMutableArray* primitive_as[DM_METAL_NUM_FRAMES];
} dm_metal_acceleration_structure;

typedef struct dm_metal_shader_binding_table_t
{
    id<MTLIntersectionFunctionTable> sbt[DM_METAL_NUM_FRAMES];
    
    uint32_t record_count, miss_count, hit_group_count, max_instance_count;
    size_t   record_size;
} dm_metal_shader_binding_table;

typedef struct dm_metal_raytracing_pipeline_t
{
    dm_metal_shader_binding_table sbt;
    
    id<MTLComputePipelineState> state;
    id<MTLLibrary>              library;
} dm_metal_raytracing_pipeline;
#endif

typedef struct dm_metal_pipeline_t
{
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLDepthStencilState>   depth_stencil_state;
    id<MTLSamplerState>        sampler_state;

    id<MTLLibrary>  library;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;

    uint32_t uniform_offset;
    
    MTLCullMode cull_mode;
    MTLWinding  winding;

    MTLPrimitiveType primitive_type;
    bool             wireframe;
} dm_metal_pipeline;

#define DM_METAL_MAX_RESOURCE 1000
typedef struct dm_metal_renderer_t
{
    id<MTLDevice>        device;

    id<MTLCommandQueue>          command_queue;
    
    id<MTLCommandBuffer>         command_buffer;
    id<MTLRenderCommandEncoder>  command_encoder;

    id<MTLCommandBuffer>         compute_command_buffer;
    id<MTLComputeCommandEncoder> compute_command_encoder;

    MTLViewport          active_viewport;

    CAMetalLayer*       swapchain;
    id<MTLTexture>      depth_texture;
    id<CAMetalDrawable> render_target;
    
    dispatch_semaphore_t semaphore;
    
    uint8_t current_frame_index;

    dm_vec4 clear_color;

    dm_render_handle     active_shader;
    dm_compute_handle    active_compute_shader;
    dm_render_handle     active_pipeline;
    MTLPrimitiveType     active_primitive;

    dm_metal_vertex_buffer   vertex_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_index_buffer    index_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_constant_buffer constant_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_texture         textures[DM_METAL_MAX_RESOURCE];
    dm_metal_pipeline        pipelines[DM_METAL_MAX_RESOURCE];

    uint32_t vb_count, ib_count, cb_count, texture_count, pipeline_count;
    
#ifdef DM_RAYTRACING
    dm_metal_acceleration_structure accel_structs[10];
    dm_metal_raytracing_pipeline    rt_pipes[10];
    
    uint32_t as_count, rt_pipe_count;
#endif
} dm_metal_renderer;

#define DM_METAL_GET_RENDERER dm_metal_renderer* metal_renderer = renderer->internal_renderer

#define DM_METAL_BUFFER_ALIGNMENT 512

#define DM_METAL_ALIGN(VAL, ALIGNMENT) ((VAL + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT

void dm_metal_destroy_vertex_buffer(dm_metal_vertex_buffer* buffer);
void dm_metal_destroy_index_buffer(dm_metal_index_buffer* buffer);
void dm_metal_destroy_constant_buffer(dm_metal_constant_buffer* buffer);
void dm_metal_destroy_texture(dm_metal_texture* texture);
void dm_metal_destroy_pipeline(dm_metal_pipeline* pipeline);

/*********************
METAL ENUM CONVERSION
***********************/
MTLWinding dm_winding_to_metal_winding(dm_winding_order winding)
{
	switch(winding)
	{
		case DM_WINDING_CLOCK:         return MTLWindingClockwise;
		case DM_WINDING_COUNTER_CLOCK: return MTLWindingCounterClockwise;
		default:
			DM_LOG_WARN("Unknown winding order. Returning \"MTLWindingCounterClockwise\"");
			return MTLWindingCounterClockwise;
	}
}

MTLCullMode dm_cull_to_metal_cull(dm_cull_mode cull)
{
	switch(cull)
	{
		case DM_CULL_FRONT_BACK:
		case DM_CULL_FRONT: return MTLCullModeFront;
		case DM_CULL_BACK:  return MTLCullModeBack;
        case DM_CULL_NONE:  return MTLCullModeNone;
		default:
			DM_LOG_ERROR("Unknown cull mode. Returning \"MTLCullModeBack\"");
			return MTLCullModeBack;
	}
}

MTLCompareFunction dm_compare_to_metal_compare(dm_comparison comp)
{
	switch(comp)
	{
		case DM_COMPARISON_ALWAYS:   return MTLCompareFunctionAlways;
		case DM_COMPARISON_NEVER:    return MTLCompareFunctionNever;
		case DM_COMPARISON_EQUAL:    return MTLCompareFunctionEqual;
		case DM_COMPARISON_NOTEQUAL: return MTLCompareFunctionNotEqual;
		case DM_COMPARISON_LESS:     return MTLCompareFunctionLess;
		case DM_COMPARISON_LEQUAL:   return MTLCompareFunctionLessEqual;
		case DM_COMPARISON_GREATER:  return MTLCompareFunctionGreater;
		case DM_COMPARISON_GEQUAL:   return MTLCompareFunctionGreaterEqual;
		default:
			DM_LOG_ERROR("Unknown comparison function. Returning \"MTLCompareFunctionLess\"");
			return MTLCompareFunctionLess;
	}
}

MTLSamplerAddressMode dm_tex_mode_to_metal_sampler_mode(dm_texture_mode mode)
{
	switch(mode)
	{
		case DM_TEXTURE_MODE_WRAP:          return MTLSamplerAddressModeRepeat;
		case DM_TEXTURE_MODE_EDGE:          return MTLSamplerAddressModeClampToEdge;
		case DM_TEXTURE_MODE_BORDER:        return MTLSamplerAddressModeClampToBorderColor;
		case DM_TEXTURE_MODE_MIRROR_REPEAT: return MTLSamplerAddressModeMirrorRepeat;
		case DM_TEXTURE_MODE_MIRROR_EDGE:   return MTLSamplerAddressModeMirrorClampToEdge;
		default:
			DM_LOG_ERROR("Unknown sampler mode. Returning \"MTLSamplerAddressModeRepeat\"");
			return MTLSamplerAddressModeRepeat;
	}
}

MTLSamplerMinMagFilter dm_minmagfilter_to_metal_minmagfilter(dm_filter filter)
{
	switch(filter)
	{
		case DM_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
		case DM_FILTER_LINEAR:  return MTLSamplerMinMagFilterLinear;
		default:
			DM_LOG_ERROR("Unknown min mag filter. Returning \"MTLSamplerMinMagFilterNearest\"");
			return MTLSamplerMinMagFilterNearest;
	}
}

MTLBlendOperation dm_blend_eq_to_metal_blend_op(dm_blend_equation eq)
{
	switch(eq)
	{
		case DM_BLEND_EQUATION_ADD:              return MTLBlendOperationAdd;
		case DM_BLEND_EQUATION_SUBTRACT:         return MTLBlendOperationSubtract;
		case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return MTLBlendOperationReverseSubtract;
		case DM_BLEND_EQUATION_MIN:              return MTLBlendOperationMin;
		case DM_BLEND_EQUATION_MAX:              return MTLBlendOperationMax;
		default:
		DM_LOG_ERROR("Unknown blend equation, returning MTLBlendOperationAdd");
		return MTLBlendOperationAdd;
	}
}

MTLBlendFactor dm_blend_func_to_metal_blend_factor(dm_blend_func func)
{
	switch(func)
	{
		case DM_BLEND_FUNC_ZERO:                  return MTLBlendFactorZero;
		case DM_BLEND_FUNC_ONE:                   return MTLBlendFactorOne;
		case DM_BLEND_FUNC_SRC_COLOR:             return MTLBlendFactorSourceColor;
		case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR:   return MTLBlendFactorOneMinusSourceColor;
		case DM_BLEND_FUNC_DST_COLOR:             return MTLBlendFactorDestinationColor;
		case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR:   return MTLBlendFactorOneMinusDestinationColor;
		case DM_BLEND_FUNC_SRC_ALPHA:             return MTLBlendFactorSourceAlpha;
		case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA:   return MTLBlendFactorOneMinusSourceAlpha;
		case DM_BLEND_FUNC_DST_ALPHA:             return MTLBlendFactorDestinationAlpha;
		case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA:   return MTLBlendFactorOneMinusDestinationAlpha;
		case DM_BLEND_FUNC_CONST_COLOR:           return MTLBlendFactorBlendColor;
		case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR: return MTLBlendFactorOneMinusBlendColor;
		case DM_BLEND_FUNC_CONST_ALPHA:           return MTLBlendFactorBlendAlpha;
		case DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA: return MTLBlendFactorOneMinusBlendAlpha;
		default:
		DM_LOG_ERROR("Unknown blend func, returning MTLBlendFactorSourceAlpha");
		return MTLBlendFactorSourceAlpha;
	}
}

MTLPrimitiveType dm_topology_to_metal_primitive(dm_primitive_topology topology)
{
	switch(topology)
	{
		case DM_TOPOLOGY_POINT_LIST:     return MTLPrimitiveTypePoint;
		case DM_TOPOLOGY_LINE_LIST:      return MTLPrimitiveTypeLine;
		case DM_TOPOLOGY_LINE_STRIP:     return MTLPrimitiveTypeLineStrip;
		case DM_TOPOLOGY_TRIANGLE_LIST:  return MTLPrimitiveTypeTriangle;
		case DM_TOPOLOGY_TRIANGLE_STRIP: return MTLPrimitiveTypeTriangleStrip;
		default: 
		DM_LOG_ERROR("Unknown DM primitive type! Returning MTLPrimitiveTypeTriangle...");
		return MTLPrimitiveTypeTriangle;
	}
}

/***********************
 RENDERER BACKEND
*****************************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Metal backend...");
    
    dm_internal_apple_data* internal_data = context->platform_data.internal_data;
    NSRect frame = [internal_data->content_view getWindowFrame];

    context->renderer.internal_renderer = dm_alloc(sizeof(dm_metal_renderer));
    dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;

    metal_renderer->device = MTLCreateSystemDefaultDevice();
    if(!metal_renderer->device)
    {
        DM_LOG_FATAL("Could not create Metal device");
        return false;
    }

    // swapchain
    metal_renderer->swapchain        = [CAMetalLayer layer];
    metal_renderer->swapchain.device = metal_renderer->device;
    metal_renderer->swapchain.opaque = YES;

    metal_renderer->command_queue = [metal_renderer->device newCommandQueue];
    if(!metal_renderer->command_queue)
    {
        DM_LOG_FATAL("Could not create metal command queue.");
        return false;
    }

    // must set the content view's layer to our metal layer
    [internal_data->content_view setWantsLayer: YES];
    [internal_data->content_view setLayer:metal_renderer->swapchain];
    
    NSSize layer_size = internal_data->content_view.layer.frame.size;
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    metal_renderer->swapchain.contentsScale = scale;
    NSSize drawable_size = NSMakeSize(layer_size.width * scale, layer_size.height * scale);
    metal_renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    metal_renderer->depth_texture       = [metal_renderer->device newTextureWithDescriptor:descriptor];
    metal_renderer->depth_texture.label = @"DepthStencil";
    if(!metal_renderer->depth_texture)
    {
        DM_LOG_FATAL("Could not create depth texture");
        return false;
    }
    
    [descriptor release];
    
    // semaphore
    metal_renderer->semaphore = dispatch_semaphore_create(DM_METAL_NUM_FRAMES);
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;
    
    // resource destruction
    for(uint32_t i=0; i<metal_renderer->vb_count; i++)
    {
        dm_metal_destroy_vertex_buffer(&metal_renderer->vertex_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->ib_count; i++)
    {
        dm_metal_destroy_index_buffer(&metal_renderer->index_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->cb_count; i++)
    {
        dm_metal_destroy_constant_buffer(&metal_renderer->constant_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->texture_count; i++)
    {
        dm_metal_destroy_texture(&metal_renderer->textures[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->pipeline_count; i++)
    {
        dm_metal_destroy_pipeline(&metal_renderer->pipelines[i]);
    }
    
    // renderer destruction
    [metal_renderer->swapchain release];
    [metal_renderer->depth_texture release];
    [metal_renderer->command_encoder release];
    [metal_renderer->compute_command_encoder release];
    [metal_renderer->command_queue release];
    [metal_renderer->device release];
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
#if 0
    dispatch_semaphore_wait(metal_renderer->semaphore, DISPATCH_TIME_FOREVER);
    
    [metal_renderer->command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(metal_renderer->semaphore);
    }];
#endif
    
    metal_renderer->current_frame_index = (metal_renderer->current_frame_index + 1) % DM_METAL_NUM_FRAMES;
    
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    return true;
}

/******************************
 RESOURCE CREATION
*****************************/
bool dm_renderer_backend_create_vertex_buffer(const dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_vertex_buffer internal_buffer = { 0 };
    
    internal_buffer.count  = desc.count;
    internal_buffer.stride = desc.stride;
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        if(desc.data)
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:desc.data length:desc.size options:MTLResourceCPUCacheModeDefaultCache];
        }
        else
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithLength:desc.size options:MTLResourceCPUCacheModeDefaultCache];
        }
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal vertex buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->vertex_buffers + metal_renderer->vb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->vb_count++;
    
    return true;
}

void dm_metal_destroy_vertex_buffer(dm_metal_vertex_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_index_buffer internal_buffer = { 0 };
    
    internal_buffer.count      = desc.count;
    internal_buffer.index_size = desc.size / desc.count;
    
    if(!desc.data)
    {
        DM_LOG_FATAL("Need data to create index buffer");
        return false;
    }
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:desc.data length:desc.size options:MTLResourceCPUCacheModeDefaultCache];
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal vertex buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->index_buffers + metal_renderer->ib_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->ib_count++;
    
    return true;
}

void dm_metal_destroy_index_buffer(dm_metal_index_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_structured_buffer(dm_structured_buffer_desc buffer_desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_create_constant_buffer(const void* data, size_t data_size, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_constant_buffer internal_buffer = { 0 };
    
    internal_buffer.size = data_size;
    
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        if(data)
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:data length:data_size options:MTLResourceCPUCacheModeDefaultCache];
        }
        else
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithLength:data_size options:MTLResourceCPUCacheModeDefaultCache];
        }
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal constant buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->constant_buffers + metal_renderer->cb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->cb_count++;
    
    return true;
}

void dm_metal_destroy_constant_buffer(dm_metal_constant_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_pipeline(const dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;

    dm_metal_pipeline internal_pipe = { 0 };

    // shader
    NSString* shader_file = [NSString stringWithUTF8String:desc.shaders[DM_PIPELINE_SHADER_STAGE_MASTER].shader_file];

    NSURL* library_url = [NSURL URLWithString:shader_file];

    NSError* library_error = NULL;

    internal_pipe.library = [metal_renderer->device newLibraryWithURL:library_url error:&library_error];
    if(!internal_pipe.library)
    {
        DM_LOG_FATAL("Creating Metal shader library failed");
        DM_LOG_ERROR("%s", [library_error.localizedDescription UTF8String]);
        return false;
    }

    NSString* func_name = [[NSString alloc] initWithUTF8String:desc.shaders[DM_PIPELINE_SHADER_STAGE_VERTEX].shader_function];

    internal_pipe.vertex_func = [internal_pipe.library newFunctionWithName:func_name];

    if(!internal_pipe.vertex_func)
    {
        DM_LOG_FATAL("Failed to create Metal vertex function");
        return false;
    }

    [func_name release];

    func_name = [[NSString alloc] initWithUTF8String:desc.shaders[DM_PIPELINE_SHADER_STAGE_PIXEL].shader_function];

    internal_pipe.fragment_func = [internal_pipe.library newFunctionWithName:func_name];
    if(!internal_pipe.fragment_func)
    {
        DM_LOG_FATAL("Failed to create Metal fragment function");
        return false;
    }

    [func_name release];

    // pipeline state
    MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor alloc];
    pipe_desc.vertexFunction                  = internal_pipe.vertex_func;
    pipe_desc.fragmentFunction                = internal_pipe.fragment_func;
    pipe_desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
    pipe_desc.depthAttachmentPixelFormat      = MTLPixelFormatDepth32Float_Stencil8;

    if(desc.flags && DM_PIPELINE_FLAG_BLEND)
    {
        MTLBlendOperation blend_op    = dm_blend_eq_to_metal_blend_op(desc.blend_desc.eq);
        MTLBlendFactor    src_factor  = dm_blend_func_to_metal_blend_factor(desc.blend_desc.src_func);
        MTLBlendFactor    dest_factor = dm_blend_func_to_metal_blend_factor(desc.blend_desc.dest_func);

        MTLBlendOperation blend_alpha_op    = dm_blend_eq_to_metal_blend_op(desc.blend_alpha_desc.eq);
        MTLBlendFactor    src_alpha_factor  = dm_blend_func_to_metal_blend_factor(desc.blend_alpha_desc.src_func);
        MTLBlendFactor    dest_alpha_factor = dm_blend_func_to_metal_blend_factor(desc.blend_alpha_desc.dest_func);

        pipe_desc.colorAttachments[0].blendingEnabled             = YES;
        pipe_desc.colorAttachments[0].rgbBlendOperation           = blend_op;
        pipe_desc.colorAttachments[0].sourceRGBBlendFactor        = src_factor;
        pipe_desc.colorAttachments[0].destinationRGBBlendFactor   = dest_factor;
        pipe_desc.colorAttachments[0].alphaBlendOperation         = blend_alpha_op;
        pipe_desc.colorAttachments[0].sourceAlphaBlendFactor      = src_alpha_factor;
        pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = dest_alpha_factor;
    }

    NSError* error = NULL;
    internal_pipe.pipeline_state = [metal_renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
    if(!internal_pipe.pipeline_state)
    {
        DM_LOG_FATAL("Creating Metal pipeline state failed");
        DM_LOG_ERROR("%s", [error.localizedDescription UTF8String]);
        return false;
    }

    //
    dm_memcpy(metal_renderer->pipelines + metal_renderer->pipeline_count, &internal_pipe, sizeof(internal_pipe));
    DM_RENDER_HANDLE_SET_INDEX(*handle, metal_renderer->pipeline_count++);

    return true;
}

void dm_metal_destroy_pipeline(dm_metal_pipeline* pipeline)
{
    
}

bool dm_renderer_backend_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_create_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_resize_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

void dm_metal_destroy_texture(dm_metal_texture* texture)
{
    
}

#ifdef DM_RAYTRACING
bool dm_metal_create_as(MTLAccelerationStructureDescriptor* descriptor, dm_metal_as* as, dm_metal_renderer* metal_renderer)
{
    id<MTLCommandBuffer>                       command_buffer  = [metal_renderer->command_queue commandBuffer];
    id<MTLAccelerationStructureCommandEncoder> command_encoder = [command_buffer accelerationStructureCommandEncoder];
    
    MTLAccelerationStructureSizes sizes = [metal_renderer->device accelerationStructureSizesWithDescriptor:descriptor];

    // allocate memory for the acceleration structure
    as->as = [metal_renderer->device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
    if(!as->as)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure");
        return false;
    }
    
    // build the scratch buffer
    as->scratch_buffer = [metal_renderer->device newBufferWithLength:sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
    if(!as->scratch_buffer)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure scratch buffer");
        return false;
    }
    
    as->result_buffer = [metal_renderer->device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];
    if(!as->result_buffer)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure result buffer");
        return false;
    }
    
    // actually build the acceleration structure
    [command_encoder buildAccelerationStructure:as->as
                                     descriptor:descriptor
                                  scratchBuffer:as->scratch_buffer
                            scratchBufferOffset:0];
    if(!as->as)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure");
        return false;
    }
    
    [command_encoder writeCompactedAccelerationStructureSize:as->as toBuffer:as->result_buffer offset:0];
    
    [command_encoder endEncoding];
    
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    
    // compact acceleration structure
    const uint32_t compacted_size = *(uint32_t *)as->result_buffer.contents;
    
    as->compact_as = [metal_renderer->device newAccelerationStructureWithSize:compacted_size];
    if(!as->compact_as)
    {
        DM_LOG_FATAL("Could not create Metal compact acceleration structure");
        return false;
    }
    
    command_buffer = [metal_renderer->command_queue commandBuffer];

    command_encoder = [command_buffer accelerationStructureCommandEncoder];
    
    [command_encoder copyAndCompactAccelerationStructure:as->as toAccelerationStructure:as->compact_as];
    
    [command_encoder endEncoding];
    [command_buffer commit];
    
    return true;
}

bool dm_metal_create_blas(dm_blas_desc desc, dm_metal_acceleration_structure* internal_as, dm_metal_renderer* metal_renderer)
{
    const uint8_t blas_index = internal_as->blas_count;
    dm_metal_blas* blas = &internal_as->blas[blas_index];
    
    const dm_render_handle vb_handle = desc.vertex_buffer;
    const dm_render_handle ib_handle = desc.index_buffer;
    
    const dm_metal_vertex_buffer* vb = &metal_renderer->vertex_buffers[vb_handle];
    const dm_metal_index_buffer*  ib = NULL;
    if(DM_RENDER_HANDLE_GET_TYPE(desc.index_buffer)!=DM_RENDER_RESOURCE_TYPE_INVALID) ib = &metal_renderer->index_buffers[ib_handle];
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        internal_as->primitive_as[i] = [[NSMutableArray alloc] init];
        
        MTLPrimitiveAccelerationStructureDescriptor *accel_descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        
        switch(desc.geom_type)
        {
            case DM_BLAS_GEOMETRY_TYPE_TRIANGLES:
            {
                MTLAccelerationStructureTriangleGeometryDescriptor *geom_descriptor = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
                
                geom_descriptor.vertexBuffer = vb->buffer[i];
                geom_descriptor.indexBuffer  = NULL;
                
                geom_descriptor.vertexStride  = vb->stride;
                geom_descriptor.triangleCount = vb->count / 3;
                
                if(DM_RENDER_HANDLE_GET_TYPE(desc.index_buffer)!=DM_RENDER_RESOURCE_TYPE_INVALID)
                {
                    geom_descriptor.indexBuffer = ib->buffer[i];
                    geom_descriptor.indexType   = MTLIndexTypeUInt32;
                }
                
                accel_descriptor.geometryDescriptors = @[ geom_descriptor ];
            } break;
                
            default:
                DM_LOG_FATAL("Unknown or unsupported Metal gemoetry type for BLAS");
                return false;
        }
        
        if(!dm_metal_create_as(accel_descriptor, &blas->as[i], metal_renderer)) return false;
        
        // add to the primitive blas list
        [internal_as->primitive_as[i] addObject:blas->as[i].compact_as];
        
        [accel_descriptor release];
    }
    
    internal_as->blas_count++;
    
    return true;
}

bool dm_metal_create_tlas(dm_acceleration_structure_desc desc, dm_metal_acceleration_structure* internal_as, dm_metal_renderer* metal_renderer)
{
    dm_metal_tlas* tlas = &internal_as->tlas;
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        tlas->instance_buffer[i] = [metal_renderer->device newBufferWithLength:sizeof(MTLAccelerationStructureInstanceDescriptor) * desc.tlas_desc.max_instance_count options:MTLResourceStorageModeManaged];
        
        MTLAccelerationStructureInstanceDescriptor *instance_descriptors = (MTLAccelerationStructureInstanceDescriptor *)tlas->instance_buffer[i].contents;
        
        uint32_t i_offset = 0;
        for(uint32_t j=0; j<desc.tlas_desc.instance_count; j++)
        {
            // associate the instance's mesh with the appropriate blas
            instance_descriptors[j].accelerationStructureIndex = desc.tlas_desc.instance_meshes[j];
            
            instance_descriptors[j].options = MTLAccelerationStructureInstanceOptionOpaque;
            
            instance_descriptors[j].mask = 0xFF;
            
            // InstanceContributionToHitgroupOffset
            instance_descriptors[j].intersectionFunctionTableOffset = i_offset;
            
            i_offset += desc.hit_group_count;
            
            if(!desc.tlas_desc.instance_transforms) continue;
            
            dm_memcpy(instance_descriptors[j].transformationMatrix.columns, desc.tlas_desc.instance_transforms[j], sizeof(float) * 4 * 3);
        }
        
        MTLInstanceAccelerationStructureDescriptor *accel_descriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];

        accel_descriptor.instancedAccelerationStructures = internal_as->primitive_as[i];
        accel_descriptor.instanceCount                   = desc.tlas_desc.max_instance_count;
        accel_descriptor.instanceDescriptorBuffer        = tlas->instance_buffer[i];
        
        if(!dm_metal_create_as(accel_descriptor, &tlas->as[i], metal_renderer)) return false;
        
        [accel_descriptor release];
    }
    
    return true;
}

bool dm_renderer_backend_create_acceleration_structure(dm_acceleration_structure_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_acceleration_structure internal_as = { 0 };
    
    // bottom level acceleration structures
    for(uint32_t i=0; i<desc.blas_count; i++)
    {
        if(!dm_metal_create_blas(desc.blas_descs[i], &internal_as, metal_renderer)) return false;
    }
    
    // top level
    if(!dm_metal_create_tlas(desc, &internal_as, metal_renderer)) return false;
    
    //
    dm_memcpy(metal_renderer->accel_structs + metal_renderer->as_count, &internal_as, sizeof(internal_as));
    *handle = metal_renderer->as_count++;
    
    return true;
}

void dm_metal_destroy_as(dm_metal_as* as)
{
    [as->as release];
    [as->compact_as release];
    [as->result_buffer release];
    [as->scratch_buffer release];
}

void dm_metal_destroy_acceleration_structure(dm_metal_acceleration_structure* as)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        for(uint32_t j=0; j<as->blas_count; j++)
        {
            dm_metal_destroy_as(&as->blas[j].as[i]);
        }
        
        dm_metal_destroy_as(&as->tlas.as[i]);
        
        [as->primitive_as[i] release];
    }
}

bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_raytracing_pipeline internal_pipe = { 0 };
    
    //
    dm_memcpy(metal_renderer->rt_pipes + metal_renderer->rt_pipe_count, &internal_pipe, sizeof(internal_pipe));
    *handle = metal_renderer->rt_pipe_count++;
    
    return true;
}
#endif

/*****************************
 RENDER COMMANDS
*********************************/
bool dm_render_command_backend_begin_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_end_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_clear_texture(dm_render_handle handle, dm_renderer* renderer)
{
    DM_LOG_FATAL("Not supported");
    return false;
}

bool dm_render_command_backend_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
    
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    
}

void dm_render_command_backend_draw_instanced(uint32_t index_count, uint32_t vertex_count, uint32_t inst_count, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    
}

#ifdef DM_RAYTRACING
bool dm_render_command_backend_update_acceleration_structure_instance(dm_render_handle handle, uint32_t instance_id, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_instance_range(dm_render_handle handle, uint32_t instance_start, uint32_t instance_end, void* data, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_tlas(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_raytracing_pipeline_dispatch_rays(uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_copy_texture_to_screen(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}
#endif
