#include "dm.h"
#include "platform/dm_platform_mac.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

#include <stdbool.h>

typedef enum dm_metal_buffer_type_t
{
	DM_METAL_BUFFER_TYPE_VERTEX,
	DM_METAL_BUFFER_TYPE_INDEX,
    DM_METAL_BUFFER_TYPE_COMPUTE,
	DM_METAL_BUFFER_TYPE_UNKNOWN
} dm_metal_buffer_type;

typedef struct dm_metal_buffer_t
{
	id<MTLBuffer>        buffer;
	uint32_t             index;
    size_t               elem_size;
	dm_metal_buffer_type type;
} dm_metal_buffer;

typedef struct dm_metal_texture_t
{
	id<MTLTexture> texture;
    uint32_t width, height;
} dm_metal_texture;

typedef struct dm_metal_pipeline_t
{
	id<MTLRenderPipelineState> pipeline_state;
	id<MTLDepthStencilState>   depth_stencil_state;
	id<MTLSamplerState>        sampler_state;

	MTLCullMode                cull_mode;
	MTLWinding                 winding;

	MTLPrimitiveType           primitive_type;
	bool                       wireframe;
} dm_metal_pipeline;

typedef struct dm_metal_shader_t
{
	id<MTLLibrary>    library;
	id<MTLFunction>   vertex_func;
	id<MTLFunction>   fragment_func;

	uint32_t          uniform_offset;
	dm_render_handle  index_buffer;
} dm_metal_shader;

typedef struct dm_metal_compute_shader_t
{
    id<MTLLibrary>              library;
    id<MTLFunction>             function;
    id<MTLComputePipelineState> compute_pipeline;
} dm_metal_compute_shader;

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
    
    dm_vec4 clear_color;
    
	dm_render_handle     active_shader;
    dm_compute_handle    active_compute_shader;
	dm_render_handle     active_pipeline;
	MTLPrimitiveType     active_primitive;
	
	dm_metal_buffer         buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_shader         shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_metal_compute_shader compute_shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_texture        textures[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_pipeline       pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
	
	uint32_t buffer_count, shader_count, compute_count, texture_count, pipeline_count;
} dm_metal_renderer;


#define DM_METAL_GET_RENDERER dm_metal_renderer* metal_renderer = renderer->internal_renderer

#define DM_METAL_BUFFER_ALIGNMENT 512

size_t dm_metal_align(size_t n, uint32_t alignment) 
{
    return ((n+alignment-1)/alignment)*alignment;
}

#define DM_METAL_INVALID_RESOURCE UINT_MAX

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

/******
BUFFER
********/
bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	dm_metal_buffer internal_buffer = { 0 };

	if(data) internal_buffer.buffer = [metal_renderer->device newBufferWithBytes:data length:desc.buffer_size options:MTLResourceOptionCPUCacheModeDefault];
	else internal_buffer.buffer = [metal_renderer->device newBufferWithLength:desc.buffer_size options:MTLResourceOptionCPUCacheModeDefault];

    internal_buffer.elem_size = desc.elem_size;
    
	if(!internal_buffer.buffer)
	{
		DM_LOG_FATAL("Could not create metal buffer");
		return false;
	}

	switch(desc.type)
	{
		case DM_BUFFER_TYPE_VERTEX: 
		internal_buffer.type = DM_METAL_BUFFER_TYPE_VERTEX;
		break;
		case DM_BUFFER_TYPE_INDEX: 
		internal_buffer.type = DM_METAL_BUFFER_TYPE_INDEX;
		break;
		default:
		DM_LOG_FATAL("Unknown buffer type");
		return false;		
	}

	dm_memcpy(metal_renderer->buffers + metal_renderer->buffer_count, &internal_buffer, sizeof(dm_metal_buffer));
	*handle = metal_renderer->buffer_count++;
	
	return true;
}

bool dm_compute_backend_create_buffer(size_t data_size, size_t elem_size, dm_compute_buffer_type type, dm_compute_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_buffer internal_buffer = { 0 };

    size_t aligned_size = dm_metal_align(data_size, DM_METAL_BUFFER_ALIGNMENT);
    internal_buffer.buffer = [metal_renderer->device newBufferWithLength:data_size options:MTLResourceStorageModeShared];

    internal_buffer.elem_size = elem_size;
    internal_buffer.type = DM_METAL_BUFFER_TYPE_COMPUTE;
    
    if(!internal_buffer.buffer)
    {
        DM_LOG_FATAL("Could not create metal buffer");
        return false;
    }
    
    dm_memcpy(metal_renderer->buffers + metal_renderer->buffer_count, &internal_buffer, sizeof(dm_metal_buffer));
    *handle = metal_renderer->buffer_count++;
    
    return true;
}

void dm_renderer_backend_destroy_buffer(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(handle > metal_renderer->buffer_count) { DM_LOG_ERROR("Trying to delete invalid Metal buffer"); return; }

	[metal_renderer->buffers[handle].buffer release];
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	dm_metal_buffer internal_uniform = { 0 };
		
	size_t aligned_size = dm_metal_align(size, DM_METAL_BUFFER_ALIGNMENT);
	internal_uniform.buffer = [metal_renderer->device newBufferWithLength:aligned_size options:MTLResourceOptionCPUCacheModeDefault];
	if(!internal_uniform.buffer)
	{
		DM_LOG_FATAL("Could not create uniform buffer!");
		return false;
	}

	dm_memcpy(metal_renderer->buffers + metal_renderer->buffer_count, &internal_uniform, sizeof(dm_metal_buffer));
	*handle = metal_renderer->buffer_count++;

	return true;
}

void dm_renderer_backend_destroy_uniform(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(handle > metal_renderer->buffer_count) { DM_LOG_ERROR("Trying to destroy invalid Metal uniform"); return; }

	[metal_renderer->buffers[handle].buffer release];
}

bool dm_compute_backend_create_uniform(size_t data_size, dm_compute_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_buffer internal_uniform = { 0 };
    
    size_t aligned_size = dm_metal_align(data_size, DM_METAL_BUFFER_ALIGNMENT);
    internal_uniform.buffer = [metal_renderer->device newBufferWithLength:data_size options:MTLResourceOptionCPUCacheModeDefault];
    
    if(!internal_uniform.buffer)
    {
        DM_LOG_FATAL("Could not create Metal compute uniform");
        return false;
    }
    
    dm_memcpy(metal_renderer->buffers + metal_renderer->buffer_count, &internal_uniform, sizeof(dm_metal_buffer));
    metal_renderer->buffer_count++;
    
    return true;
}

/**************
METAL PIPELINE
****************/
bool dm_metal_create_pipeline(dm_pipeline_desc desc, dm_render_handle shader_handle, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(shader_handle > metal_renderer->shader_count) { DM_LOG_ERROR("Trying to create Metal pipeline with invalid Metal shader"); return false; }

	dm_metal_shader* shader = &metal_renderer->shaders[shader_handle];

	dm_metal_pipeline internal_pipe = { 0 };

	// pipeline state
	MTLRenderPipelineDescriptor* pipe_desc    = [MTLRenderPipelineDescriptor new];
	pipe_desc.vertexFunction                  = shader->vertex_func;
	pipe_desc.fragmentFunction                = shader->fragment_func;
	pipe_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	pipe_desc.depthAttachmentPixelFormat      = MTLPixelFormatDepth32Float_Stencil8;

	// blending
	if(desc.blend)
	{
		MTLBlendOperation blend_op = dm_blend_eq_to_metal_blend_op(desc.blend_eq);
		MTLBlendFactor dest_factor = dm_blend_func_to_metal_blend_factor(desc.blend_dest_f);
		MTLBlendFactor src_factor  = dm_blend_func_to_metal_blend_factor(desc.blend_src_f);

		pipe_desc.colorAttachments[0].blendingEnabled             = YES;
		pipe_desc.colorAttachments[0].rgbBlendOperation           = blend_op;
		pipe_desc.colorAttachments[0].alphaBlendOperation         = blend_op;
		pipe_desc.colorAttachments[0].sourceRGBBlendFactor        = src_factor;
		pipe_desc.colorAttachments[0].sourceAlphaBlendFactor      = src_factor;
		pipe_desc.colorAttachments[0].destinationRGBBlendFactor   = dest_factor;
		pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = dest_factor;
	}

	NSError* error = NULL;
	internal_pipe.pipeline_state = [metal_renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
	if(!internal_pipe.pipeline_state)
	{
		NSString* str_error = [NSString stringWithFormat:@"%@", error];
		const char* c = [str_error cStringUsingEncoding:NSUTF8StringEncoding];
		DM_LOG_FATAL("Could not create metal pipeline state: %s", c);
        [pipe_desc release];
        
		return false;
	}

	// depth stencil
	MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
	if(desc.depth)
	{
		depth_stencil_desc.depthCompareFunction = dm_compare_to_metal_compare(desc.depth_comp);
		depth_stencil_desc.depthWriteEnabled    = YES;
	}
	else
	{
		depth_stencil_desc.depthWriteEnabled = NO;
	}
	
	internal_pipe.depth_stencil_state = [metal_renderer->device newDepthStencilStateWithDescriptor:depth_stencil_desc];
	if(!internal_pipe.depth_stencil_state)
	{
		DM_LOG_FATAL("Could not create metal depth stencil state!");
        [depth_stencil_desc release];
        [pipe_desc release];
        
		return false;
	}

	// sampler
	MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
	sampler_desc.minFilter    = dm_minmagfilter_to_metal_minmagfilter(desc.sampler_filter);
	sampler_desc.magFilter    = dm_minmagfilter_to_metal_minmagfilter(desc.sampler_filter);
	sampler_desc.mipFilter    = MTLSamplerMipFilterLinear;
	sampler_desc.sAddressMode = dm_tex_mode_to_metal_sampler_mode(desc.u_mode);
	sampler_desc.tAddressMode = dm_tex_mode_to_metal_sampler_mode(desc.v_mode);

	internal_pipe.sampler_state = [metal_renderer->device newSamplerStateWithDescriptor:sampler_desc];
	if(!internal_pipe.sampler_state)
	{
		DM_LOG_FATAL("Could not create metal sampler state!");
        [sampler_desc release];
        [depth_stencil_desc release];
        [pipe_desc release];
        
		return false;
	}

	// raster stuff
	internal_pipe.winding   = dm_winding_to_metal_winding(desc.winding_order);
	internal_pipe.cull_mode = dm_cull_to_metal_cull(desc.cull_mode);

	internal_pipe.primitive_type = dm_topology_to_metal_primitive(desc.primitive_topology);
    internal_pipe.wireframe = desc.wireframe;
    
	[depth_stencil_desc release];
	[sampler_desc release];
	[pipe_desc release];
	
	dm_memcpy(metal_renderer->pipelines + metal_renderer->pipeline_count, &internal_pipe, sizeof(dm_metal_pipeline));
	*handle = metal_renderer->pipeline_count++;

	return true;
}

void dm_renderer_backend_destroy_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;
	
	if(handle > metal_renderer->pipeline_count) { DM_LOG_ERROR("Trying to destroy invalid Metal pipeline"); return; }

	dm_metal_pipeline* pipeline = &metal_renderer->pipelines[handle];

	[pipeline->sampler_state release];
	[pipeline->depth_stencil_state release];
	[pipeline->pipeline_state release];
}

/************
METAL SHADER
**************/
bool dm_metal_create_shader(dm_shader_desc shader_desc, dm_vertex_attrib_desc* layout, uint32_t num_attribs, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	dm_metal_shader internal_shader = { 0 };

	char file_buffer[256];
	strcpy(file_buffer, shader_desc.master);

	NSString* shader_file = [NSString stringWithUTF8String:file_buffer];
		
	internal_shader.library = [metal_renderer->device newLibraryWithFile:shader_file error:NULL];
	if(!internal_shader.library)
	{
		DM_LOG_FATAL("Could not create metal library from file %s", [shader_file UTF8String]);
		return false;
	}

	// vertex
	NSString* func_name = [[NSString alloc] initWithUTF8String:shader_desc.vertex];
	internal_shader.vertex_func = [internal_shader.library newFunctionWithName:func_name];
	
	if(!internal_shader.vertex_func)
	{
		DM_LOG_FATAL("Could not create vertex function \'%s\' from shader \'%s\'", shader_desc.vertex, shader_desc.master);
        [func_name release];
		return false;
	}

	// fragment
    [func_name release];
	func_name = [[NSString alloc] initWithUTF8String:shader_desc.pixel];
	internal_shader.fragment_func = [internal_shader.library newFunctionWithName:func_name];

	if(!internal_shader.fragment_func)
	{
		DM_LOG_FATAL("Could not create fragment function \'%s\' from shader \'%s\'", shader_desc.pixel, shader_desc.master);
        [func_name release];
		return false;
	}
	
	[func_name release];
	
	dm_memcpy(metal_renderer->shaders + metal_renderer->shader_count, &internal_shader, sizeof(dm_metal_shader));
	*handle = metal_renderer->shader_count++;

	return true;
}

void dm_renderer_backend_destroy_shader(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(handle > metal_renderer->shader_count) { DM_LOG_ERROR("Trying to destroy invalid Metal shader"); return; }

	dm_metal_shader* internal_shader = &metal_renderer->shaders[handle];

	[internal_shader->library release];
	[internal_shader->vertex_func release];
	[internal_shader->fragment_func release];
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
	if(!dm_metal_create_shader(shader_desc, attrib_descs, num_attribs, shader_handle, renderer)) return false;

	if(!dm_metal_create_pipeline(pipe_desc, *shader_handle, pipe_handle, renderer)) return false;

	return true;
}

bool dm_compute_backend_create_shader(dm_compute_shader_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_compute_shader internal_shader = { 0 };
    
    // library
    char file_buffer[256];
    strcpy(file_buffer, desc.path);

    NSString* shader_file = [NSString stringWithUTF8String:file_buffer];
        
    internal_shader.library = [metal_renderer->device newLibraryWithFile:shader_file error:NULL];
    if(!internal_shader.library)
    {
        DM_LOG_FATAL("Could not create metal library from file %s", [shader_file UTF8String]);
        return false;
    }
    
    // function
    char function_buffer[256];
    strcpy(function_buffer, desc.function);
    
    NSString* function_string = [NSString stringWithUTF8String:function_buffer];
    
    internal_shader.function = [internal_shader.library newFunctionWithName:function_string];
    if(!internal_shader.function)
    {
        DM_LOG_FATAL("Could not create compute shader function '%s' from compute shader '%s'", [function_string UTF8String], [shader_file UTF8String]);
        return false;
    }
    
    // compute pipeline
    NSError* error = NULL;
    internal_shader.compute_pipeline = [metal_renderer->device newComputePipelineStateWithFunction:internal_shader.function error:&error];
    if(!internal_shader.compute_pipeline)
    {
        NSString* str_error = [NSString stringWithFormat:@"%@", error];
        const char* c = [str_error cStringUsingEncoding:NSUTF8StringEncoding];
        DM_LOG_FATAL("Could not create metal compute shader pipeline state: %s", c);
        
        return false;
    }
    
    // write it out
    dm_memcpy(metal_renderer->compute_shaders + metal_renderer->compute_count, &internal_shader, sizeof(dm_metal_compute_shader));
    *handle = metal_renderer->compute_count++;
    
    return true;
}

void dm_compute_backend_destroy_shader(dm_compute_handle handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;

    if(handle > metal_renderer->compute_count) { DM_LOG_ERROR("Trying to destroy invalid Metal compute shader"); return; }

    dm_metal_compute_shader* internal_shader = &metal_renderer->compute_shaders[handle];

    [internal_shader->library release];
    [internal_shader->function release];
    [internal_shader->compute_pipeline release];
}

/*************
METAL TEXTURE
***************/
bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;
	
	dm_metal_texture internal_texture = { 0 };

    internal_texture.width  = width;
    internal_texture.height = height;
    
	MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

	texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
	texture_desc.width       = width;
	texture_desc.height      = height;
	texture_desc.usage       = MTLTextureUsageShaderRead;

	internal_texture.texture = [metal_renderer->device newTextureWithDescriptor:texture_desc];
	if(!internal_texture.texture)
	{
		DM_LOG_FATAL("Could not create metal texture from image: %s", name);
        [texture_desc release];
        
		return false;
	}

	MTLRegion region = MTLRegionMake2D(0, 0, width, height);
	
	NSUInteger bytes_per_row = 4 * width;

	[internal_texture.texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytes_per_row];

	id <MTLCommandBuffer> command_buffer = [metal_renderer->command_queue commandBuffer];
	id <MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
	//[blit_encoder generateMipmapsForTexture: internal_texture.texture];
	[blit_encoder endEncoding];
	[command_buffer commit];
	[command_buffer waitUntilCompleted];

	[texture_desc release];

	dm_memcpy(metal_renderer->textures + metal_renderer->texture_count, &internal_texture, sizeof(dm_metal_texture));
	*handle = metal_renderer->texture_count++;
	
	return true;
}

bool dm_renderer_backend_create_dynamic_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_texture internal_texture = { 0 };

    internal_texture.width  = width;
    internal_texture.height = height;
    
    MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

    texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_desc.width = width;
    texture_desc.height = height;
    texture_desc.usage = MTLTextureUsageShaderRead;

    internal_texture.texture = [metal_renderer->device newTextureWithDescriptor:texture_desc];
    if(!internal_texture.texture)
    {
        DM_LOG_FATAL("Could not create metal texture from image: %s", name);
        [texture_desc release];
        
        return false;
    }

    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    
    NSUInteger bytes_per_row = 4 * width;

    [internal_texture.texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytes_per_row];

    id <MTLCommandBuffer> command_buffer = [metal_renderer->command_queue commandBuffer];
    id <MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    //[blit_encoder generateMipmapsForTexture: internal_texture.texture];
    [blit_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    [texture_desc release];

    dm_memcpy(metal_renderer->textures + metal_renderer->texture_count, &internal_texture, sizeof(dm_metal_texture));
    *handle = metal_renderer->texture_count++;
    
    return true;
}

void dm_renderer_backend_destroy_texture(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(handle > metal_renderer->texture_count) { DM_LOG_ERROR("Trying to destroy invalid Metal texture"); return; }

	[metal_renderer->textures[handle].texture release];
}

void* dm_renderer_backend_get_internal_texture_ptr(dm_render_handle handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->texture_count) { DM_LOG_FATAL("Trying to retrieve invalid Metal texture"); return NULL; }
    
    return metal_renderer->textures[handle].texture;
}

/*************
MAIN RENDERER
***************/
bool dm_renderer_backend_init(dm_context* context)
{
	DM_LOG_DEBUG("Initializing metal render backend...");

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
    
	return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
	dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;

	// buffers
    for(uint32_t i=0; i<metal_renderer->buffer_count; i++)
    {
        dm_renderer_backend_destroy_buffer(i, &context->renderer);
    }
    
    // shaders
    for(uint32_t i=0; i<metal_renderer->shader_count; i++)
    {
        dm_renderer_backend_destroy_shader(i, &context->renderer);
    }
    
    for(uint32_t i=0; i<metal_renderer->compute_count; i++)
    {
        dm_compute_backend_destroy_shader(i, &context->renderer);
    }
    
    // textures
    for(uint32_t i=0; i<metal_renderer->texture_count; i++)
    {
        dm_renderer_backend_destroy_texture(i, &context->renderer);
    }
    
    // pipelines
    for(uint32_t i=0; i<metal_renderer->pipeline_count; i++)
    {
        dm_renderer_backend_destroy_pipeline(i, &context->renderer);
    }
    
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

    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    metal_renderer->swapchain.contentsScale = scale;
    CGSize size = metal_renderer->swapchain.bounds.size;
    size.width *= scale;
    size.height *= scale;
    metal_renderer->swapchain.drawableSize = size;
    
    metal_renderer->render_target = [metal_renderer->swapchain nextDrawable];
    if(!metal_renderer->render_target) { DM_LOG_FATAL("Could not get next Metal drawable!"); }

	MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
	desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
	metal_renderer->command_buffer = [metal_renderer->command_queue commandBufferWithDescriptor:desc];

    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    
    pass_desc.colorAttachments[0].texture     = [metal_renderer->render_target texture];
    pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass_desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
    pass_desc.colorAttachments[0].clearColor  = MTLClearColorMake(
                                                                 metal_renderer->clear_color[0],
                                                                 metal_renderer->clear_color[1],
                                                                 metal_renderer->clear_color[2],
                                                                 metal_renderer->clear_color[3]
                                                                  );
    
    MTLRenderPassDepthAttachmentDescriptor* depth_attachment = pass_desc.depthAttachment;
    depth_attachment.texture     = metal_renderer->depth_texture;
    depth_attachment.clearDepth  = 1.0;
    depth_attachment.storeAction = MTLStoreActionDontCare;
    depth_attachment.loadAction  = MTLLoadActionClear;
    
    metal_renderer->command_encoder = [metal_renderer->command_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if(!metal_renderer->command_encoder) { DM_LOG_FATAL("Could not get Metal render command encoder"); return false; }
    
	[desc release];

	return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
	dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;

	metal_renderer->swapchain.displaySyncEnabled = context->renderer.vsync;
    
    if(metal_renderer->render_target) [metal_renderer->command_buffer presentDrawable:metal_renderer->render_target];
    else { DM_LOG_FATAL("Render target is NULL!"); return false; }

    [metal_renderer->command_encoder endEncoding];
	[metal_renderer->command_buffer commit];
	[metal_renderer->command_buffer release];

	metal_renderer->active_shader = DM_METAL_INVALID_RESOURCE;
	metal_renderer->active_pipeline = DM_METAL_INVALID_RESOURCE;

	return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    metal_renderer->swapchain.contentsScale = scale;
    CGSize drawable_size;
    drawable_size.width  = width * scale;
    drawable_size.height = height * scale;
    metal_renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture
    [metal_renderer->depth_texture release];
    
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    metal_renderer->depth_texture       = [metal_renderer->device newTextureWithDescriptor:descriptor];
    metal_renderer->depth_texture.label = @"DepthStencil";
    
	return true;
}

/********
COMMANDS
**********/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	metal_renderer->clear_color[0] = r;
	metal_renderer->clear_color[1] = g;
	metal_renderer->clear_color[2] = b;
	metal_renderer->clear_color[3] = a;
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    
	MTLViewport new_viewport;
	new_viewport.originX = 0.0f;
	new_viewport.originY = 0.0f;
	new_viewport.width = width * scale;
	new_viewport.height = height * scale;
	new_viewport.znear = 0;
	new_viewport.zfar = 1.0f;

    [metal_renderer->command_encoder setViewport:new_viewport];
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;
	
	if(handle > metal_renderer->pipeline_count) { DM_LOG_FATAL("Trying to bind invalid Metal pipeline"); return false; }

	dm_metal_pipeline pipeline = metal_renderer->pipelines[handle];
	if(!pipeline.pipeline_state) { DM_LOG_FATAL("Invalid Metal pipeline state"); return false; }

    metal_renderer->active_primitive = pipeline.primitive_type;
    
	[metal_renderer->command_encoder setRenderPipelineState:pipeline.pipeline_state];
	[metal_renderer->command_encoder setDepthStencilState:pipeline.depth_stencil_state];
	[metal_renderer->command_encoder setFrontFacingWinding:pipeline.winding];
	[metal_renderer->command_encoder setCullMode:pipeline.cull_mode];
	[metal_renderer->command_encoder setFragmentSamplerState:pipeline.sampler_state atIndex:0];
    
    if(pipeline.wireframe) [metal_renderer->command_encoder setTriangleFillMode:MTLTriangleFillModeLines];
    else                   [metal_renderer->command_encoder setTriangleFillMode:MTLTriangleFillModeFill];

	metal_renderer->active_pipeline = handle;

	return true;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	metal_renderer->active_primitive = dm_topology_to_metal_primitive(topology);

	return true;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(!metal_renderer->render_target) return false;

	if(handle > metal_renderer->shader_count) { DM_LOG_FATAL("Trying to bind invalid Metal shader"); return false; }

	dm_metal_shader* internal_shader = &metal_renderer->shaders[handle];

    internal_shader->uniform_offset = 0;
    
	metal_renderer->active_shader = handle;

	return true;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal buffer"); return false; }

	dm_metal_buffer* internal_buffer = &metal_renderer->buffers[handle];

	internal_buffer->index = slot;

	if(internal_buffer->type == DM_METAL_BUFFER_TYPE_INDEX) metal_renderer->shaders[metal_renderer->active_shader].index_buffer = handle;
	else if(internal_buffer->type == DM_METAL_BUFFER_TYPE_VERTEX) 
	{
		dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
		[metal_renderer->command_encoder setVertexBuffer:internal_buffer->buffer offset:0 atIndex:slot];
		active_shader->uniform_offset++;
	}

	return true;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid Metal buffer"); return false; }

	dm_memcpy([metal_renderer->buffers[handle].buffer contents]+offset, data, data_size);

	return true;
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal uniform"); return false; }

	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
	dm_metal_buffer* internal_uniform = &metal_renderer->buffers[handle];

	if(stage!=DM_UNIFORM_STAGE_PIXEL) [metal_renderer->command_encoder setVertexBuffer:internal_uniform->buffer offset:0 atIndex:slot+active_shader->uniform_offset];
	if(stage!=DM_UNIFORM_STAGE_VERTEX) [metal_renderer->command_encoder setFragmentBuffer:internal_uniform->buffer offset:0 atIndex:slot];

	return true;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
	return dm_render_command_backend_update_buffer(handle, data, data_size, 0, renderer);
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	if(handle > metal_renderer->texture_count) { DM_LOG_FATAL("Trying to bind invalid Metal texture"); return false; }

	[metal_renderer->command_encoder setFragmentTexture:metal_renderer->textures[handle].texture atIndex:slot];

	return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->texture_count) { DM_LOG_FATAL("Trying to update invalid Metal texture"); return false; }
    
    dm_metal_texture* internal_texture = &metal_renderer->textures[handle];
    
    if(internal_texture->width != width || internal_texture->height != height)
    {
        [internal_texture->texture release];
        
        MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

        texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_desc.width = width;
        texture_desc.height = height;
        texture_desc.usage = MTLTextureUsageShaderRead;

        internal_texture->texture = [metal_renderer->device newTextureWithDescriptor:texture_desc];
        if(!internal_texture->texture)
        {
            DM_LOG_FATAL("Updating Metal texture failed");
            [texture_desc release];
            
            return false;
        }
        
        [texture_desc release];
    }

    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    
    NSUInteger bytes_per_row = 4 * width;

    [internal_texture->texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytes_per_row];
    
    return true;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
	DM_LOG_FATAL("Bind default framebuffer not supported yet");
	return false;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
	DM_LOG_FATAL("Bind framebuffer ot supported yet");
	return false;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_LOG_FATAL("Bind framebuffer texture not supported yet");
	return false;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    [metal_renderer->command_encoder drawPrimitives:metal_renderer->active_primitive vertexStart:start vertexCount:count];
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
    dm_metal_buffer* index_buffer  = &metal_renderer->buffers[active_shader->index_buffer];

    switch(index_buffer->elem_size)
    {
        case 2:
            [metal_renderer->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt16 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset*index_buffer->elem_size];
            break;
            
        case 4:
            [metal_renderer->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset*index_buffer->elem_size];
            break;
    }
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

    if(!metal_renderer->render_target) return false;
    
	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
    dm_metal_buffer* index_buffer  = &metal_renderer->buffers[active_shader->index_buffer];
    
    switch(index_buffer->elem_size)
    {
        case 2:
            [metal_renderer->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt16 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset*index_buffer->elem_size];
            break;
            
        case 4:
            [metal_renderer->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset*index_buffer->elem_size];
            break;
    }
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(wireframe) [metal_renderer->command_encoder setTriangleFillMode:MTLTriangleFillModeLines];
    else          [metal_renderer->command_encoder setTriangleFillMode:MTLTriangleFillModeFill];
}

void dm_render_command_backend_set_scissor_rects(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    MTLScissorRect rect;
    
    rect.x = left;
    rect.y = top;
    rect.width = right - left;
    rect.height = bottom - top;
    
    [metal_renderer->command_encoder setScissorRect:rect];
}

bool dm_compute_backend_command_bind_buffer(dm_compute_handle handle, uint32_t offset, uint32_t slot, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal compute buffer"); return false; }
    
    dm_metal_buffer* internal_buffer = &metal_renderer->buffers[handle];
    
    [metal_renderer->compute_command_encoder setBuffer:internal_buffer->buffer offset:offset atIndex:slot];
    
    return true;
}

extern bool dm_compute_backend_command_update_buffer(dm_compute_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal compute buffer"); return false; }
    
    dm_metal_buffer* internal_buffer = &metal_renderer->buffers[handle];
    
    dm_memcpy(internal_buffer->buffer.contents + offset, data, data_size);
    
    return true;
}

void* dm_compute_backend_command_get_buffer_data(dm_compute_handle handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal compute buffer"); return NULL; }
    
    return metal_renderer->buffers[handle].buffer.contents;
}

bool dm_compute_backend_command_bind_shader(dm_compute_handle handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    if(handle > metal_renderer->compute_count) { DM_LOG_FATAL("Trying to bind invalid Metal compute shader"); return false; }
    
    dm_metal_compute_shader* internal_shader = &metal_renderer->compute_shaders[handle];
    
    metal_renderer->compute_command_buffer = [metal_renderer->command_queue commandBuffer];
    if(!metal_renderer->compute_command_buffer) { DM_LOG_FATAL("Could not get Metal compute command buffer"); return false; }
    
    metal_renderer->compute_command_encoder = [metal_renderer->compute_command_buffer computeCommandEncoder];
    if(!metal_renderer->compute_command_encoder) { DM_LOG_FATAL("Could not get Metal compute command encoder"); return false; }
    
    [metal_renderer->compute_command_encoder setComputePipelineState:internal_shader->compute_pipeline];
    
    metal_renderer->active_compute_shader = handle;
    
    return true;
}

bool dm_compute_backend_command_dispatch(uint32_t x_size, uint32_t y_size, uint32_t z_size, uint32_t x_thread_grps, uint32_t y_thread_grps, uint32_t z_thread_grps, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    NSInteger max_grp = metal_renderer->compute_shaders[metal_renderer->active_compute_shader].compute_pipeline.maxTotalThreadsPerThreadgroup;
    
    MTLSize grid_size = MTLSizeMake(x_size, y_size, z_size);
    MTLSize group_size = MTLSizeMake(x_thread_grps, y_thread_grps, z_thread_grps);
    
    [metal_renderer->compute_command_encoder dispatchThreads:grid_size threadsPerThreadgroup:group_size];
    
    [metal_renderer->compute_command_encoder endEncoding];
    [metal_renderer->compute_command_buffer commit];
    
    [metal_renderer->compute_command_buffer waitUntilCompleted];
    
    return true;
}
