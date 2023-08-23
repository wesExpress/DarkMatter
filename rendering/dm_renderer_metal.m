#include "dm.h"
#include "dm_platform_mac.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <stdbool.h>

typedef enum dm_metal_buffer_type_t
{
	DM_METAL_BUFFER_TYPE_VERTEX,
	DM_METAL_BUFFER_TYPE_INDEX,
	DM_METAL_BUFFER_TYPE_UNKNOWN
} dm_metal_buffer_type;

typedef struct dm_metal_buffer_t
{
	id<MTLBuffer>        buffer;
	uint32_t             index;
	dm_metal_buffer_type type;
} dm_metal_buffer;

typedef struct dm_metal_texture_t
{
	id<MTLTexture> texture;
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
	id<MTLRenderCommandEncoder> command_encoder;
	id<MTLLibrary>              library;
	id<MTLFunction>             vertex_func;
	id<MTLFunction>             fragment_func;

	uint32_t                    uniform_offset;
	dm_render_handle            index_buffer;
} dm_metal_shader;

@interface dm_metal_view : NSView
{
@public
	float clear_color[4];
}

@property (nonatomic, strong) CAMetalLayer*       metal_layer;
@property (nonatomic, strong) id<CAMetalDrawable> drawable;
@property (nonatomic) MTLViewport                 viewport;
@property (nonatomic, strong) id<MTLTexture>      depth_texture;

- (id) init;
- (MTLRenderPassDescriptor*)currentRenderPassDescriptor: (bool*)new_frame;
- (void) nextDrawable;
- (bool) hasDrawable;
- (void) presentDrawable : (id<MTLCommandBuffer>)command_buffer;
- (void) setViewport: (MTLViewport)viewport_in;
@end

typedef struct dm_metal_renderer_t
{
	id<MTLDevice>        device;
	id<MTLCommandQueue>  command_queue;
	id<MTLCommandBuffer> command_buffer;

	MTLViewport          active_viewport;
	
	dm_metal_view* 	  metal_view;
	dm_render_handle     active_shader;
	dm_render_handle     active_pipeline;
	MTLPrimitiveType     active_primitive;

	bool                 new_frame, renew_encoder;
	
	dm_metal_buffer      buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_shader      shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_texture     textures[DM_RENDERER_MAX_RESOURCE_COUNT];
	dm_metal_pipeline    pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
	
	uint32_t buffer_count, shader_count, texture_count, pipeline_count;
} dm_metal_renderer;


#define DM_METAL_GET_RENDERER dm_metal_renderer* metal_renderer = renderer->internal_renderer

@implementation dm_metal_view

+ (id) layerClass
{
	return [CAMetalLayer class];
}

- (id) init
{
	self = [super init];

	if(self)
	{
		self.wantsLayer = true;
		self.metal_layer = [CAMetalLayer layer];
	}

	return self;
}

- (MTLRenderPassDescriptor*)currentRenderPassDescriptor: (bool*)new_frame
{
	MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];
	
	desc.colorAttachments[0].texture = [_drawable texture];
	desc.colorAttachments[0].storeAction = MTLStoreActionStore;
	
	if(*new_frame)
	{
		desc.colorAttachments[0].loadAction = MTLLoadActionClear;
		desc.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
	}
	else desc.colorAttachments[0].loadAction = MTLLoadActionLoad;

	MTLRenderPassDepthAttachmentDescriptor* depth_attachment = desc.depthAttachment;
	depth_attachment.texture = _depth_texture;
	depth_attachment.clearDepth = 1.0;
	depth_attachment.storeAction = MTLStoreActionDontCare;
    if(*new_frame) depth_attachment.loadAction = MTLLoadActionClear;
    else depth_attachment.loadAction = MTLLoadActionLoad;

    if(*new_frame) *new_frame = false;
    
	return desc;
}

- (void) nextDrawable
{
	_drawable = [_metal_layer nextDrawable];
}

- (bool) hasDrawable
{
	return _drawable ? 1 : 0;
}

- (void) presentDrawable: (id<MTLCommandBuffer>)command_buffer
{
	[command_buffer presentDrawable:_drawable];
}

- (void) setViewport: (MTLViewport)viewport_in
{
	_viewport = viewport_in;
}

@end

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
		case DM_WINDING_CLOCK: return MTLWindingClockwise;
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
		case DM_CULL_BACK: return MTLCullModeBack;
		default:
			DM_LOG_ERROR("Unknown cull mode. Returning \"MTLCullModeBack\"");
			return MTLCullModeBack;
	}
}

MTLCompareFunction dm_compare_to_metal_compare(dm_comparison comp)
{
	switch(comp)
	{
		case DM_COMPARISON_ALWAYS: return MTLCompareFunctionAlways;
		case DM_COMPARISON_NEVER: return MTLCompareFunctionNever;
		case DM_COMPARISON_EQUAL: return MTLCompareFunctionEqual;
		case DM_COMPARISON_NOTEQUAL: return MTLCompareFunctionNotEqual;
		case DM_COMPARISON_LESS: return MTLCompareFunctionLess;
		case DM_COMPARISON_LEQUAL: return MTLCompareFunctionLessEqual;
		case DM_COMPARISON_GREATER: return MTLCompareFunctionGreater;
		case DM_COMPARISON_GEQUAL: return MTLCompareFunctionGreaterEqual;
		default:
			DM_LOG_ERROR("Unknown comparison function. Returning \"MTLCompareFunctionLess\"");
			return MTLCompareFunctionLess;
	}
}

MTLSamplerAddressMode dm_tex_mode_to_metal_sampler_mode(dm_texture_mode mode)
{
	switch(mode)
	{
		case DM_TEXTURE_MODE_WRAP: return MTLSamplerAddressModeRepeat;
		case DM_TEXTURE_MODE_EDGE: return MTLSamplerAddressModeClampToEdge;
		case DM_TEXTURE_MODE_BORDER: return MTLSamplerAddressModeClampToBorderColor;
		case DM_TEXTURE_MODE_MIRROR_REPEAT: return MTLSamplerAddressModeMirrorRepeat;
		case DM_TEXTURE_MODE_MIRROR_EDGE: return MTLSamplerAddressModeMirrorClampToEdge;
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
		case DM_FILTER_LINEAR: return MTLSamplerMinMagFilterLinear;
		default:
			DM_LOG_ERROR("Unknown min mag filter. Returning \"MTLSamplerMinMagFilterNearest\"");
			return MTLSamplerMinMagFilterNearest;
	}
}

MTLBlendOperation dm_blend_eq_to_metal_blend_op(dm_blend_equation eq)
{
	switch(eq)
	{
		case DM_BLEND_EQUATION_ADD: return MTLBlendOperationAdd;
		case DM_BLEND_EQUATION_SUBTRACT: return MTLBlendOperationSubtract;
		case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return MTLBlendOperationReverseSubtract;
		case DM_BLEND_EQUATION_MIN: return MTLBlendOperationMin;
		case DM_BLEND_EQUATION_MAX: return MTLBlendOperationMax;
		default:
		DM_LOG_ERROR("Unknown blend equation, returning MTLBlendOperationAdd");
		return MTLBlendOperationAdd;
	}
}

MTLBlendFactor dm_blend_func_to_metal_blend_factor(dm_blend_func func)
{
	switch(func)
	{
		case DM_BLEND_FUNC_ZERO: return MTLBlendFactorZero;
		case DM_BLEND_FUNC_ONE: return MTLBlendFactorOne;
		case DM_BLEND_FUNC_SRC_COLOR: return MTLBlendFactorSourceColor;
		case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR: return MTLBlendFactorOneMinusSourceColor;
		case DM_BLEND_FUNC_DST_COLOR: return MTLBlendFactorDestinationColor;
		case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
		case DM_BLEND_FUNC_SRC_ALPHA: return MTLBlendFactorSourceAlpha;
		case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
		case DM_BLEND_FUNC_DST_ALPHA: return MTLBlendFactorDestinationAlpha;
		case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA: return MTLBlendFactorOneMinusDestinationAlpha;
		case DM_BLEND_FUNC_CONST_COLOR: return MTLBlendFactorBlendColor;
		case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR: return MTLBlendFactorOneMinusBlendColor;
		case DM_BLEND_FUNC_CONST_ALPHA: return MTLBlendFactorBlendAlpha;
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

/************
METAL SHADER
**************/
bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
	return true;
}

bool dm_renderer_backend_create_shader(const char* shader_src, dm_vertex_attrib_desc* layout, uint32_t num_attribs, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	dm_metal_shader internal_shader = { 0 };

	char file_buffer[256];
	strcpy(file_buffer, shader_src);

	NSString* shader_file = [NSString stringWithUTF8String:file_buffer];
		
	internal_shader.library = [metal_renderer->device newLibraryWithFile:shader_file error:NULL];
	if(!internal_shader.library)
	{
		DM_LOG_FATAL("Could not create metal library from file %s", [shader_file UTF8String]);
		return false;
	}

	// vertex
	NSString* func_name = [[NSString alloc] initWithUTF8String:"vertex_main"];
	internal_shader.vertex_func = [internal_shader.library newFunctionWithName:func_name];
	
	if(!internal_shader.vertex_func)
	{
		DM_LOG_FATAL("Could not create vertex function from shader: %s", shader_src);
		return false;
	}

	// fragment
	func_name = [[NSString alloc] initWithUTF8String:"fragment_main"];
	internal_shader.fragment_func = [internal_shader.library newFunctionWithName:func_name];

	if(!internal_shader.fragment_func)
	{
		DM_LOG_FATAL("Could not create fragment function from shader: %s", shader_src);
		return false;
	}
	
	[shader_file release];
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
	[internal_shader->command_encoder release];
}

/*************
METAL TEXTURE
***************/
bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;
	
	dm_metal_texture internal_texture = { 0 };

	MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

	texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
	texture_desc.width = width;
	texture_desc.height = height;
	texture_desc.usage = MTLTextureUsageShaderRead;

	internal_texture.texture = [metal_renderer->device newTextureWithDescriptor:texture_desc];
	if(!internal_texture.texture)
	{
		DM_LOG_FATAL("Could not create metal texture from image: %s", name);
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

	[command_buffer release];
	[texture_desc release];
	[blit_encoder release];

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

/**************
METAL PIPELINE
****************/
bool dm_renderer_backend_create_pipeline(dm_pipeline_desc desc, dm_render_handle shader_handle, dm_render_handle* handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(shader_handle > metal_renderer->shader_count) { DM_LOG_ERROR("Trying to create Metal pipeline with invalid Metal shader"); return false; }

	dm_metal_shader* shader = &metal_renderer->shaders[shader_handle];

	dm_metal_pipeline internal_pipe = { 0 };

	// pipeline state
	MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
	pipe_desc.vertexFunction = shader->vertex_func;
	pipe_desc.fragmentFunction = shader->fragment_func;
	pipe_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

	// blending
	if(desc.blend)
	{
		MTLBlendOperation blend_op = dm_blend_eq_to_metal_blend_op(desc.blend_eq);
		MTLBlendFactor dest_factor = dm_blend_func_to_metal_blend_factor(desc.blend_dest_f);
		MTLBlendFactor src_factor = dm_blend_func_to_metal_blend_factor(desc.blend_src_f);

		pipe_desc.colorAttachments[0].blendingEnabled = YES;
		pipe_desc.colorAttachments[0].rgbBlendOperation = blend_op;
		pipe_desc.colorAttachments[0].alphaBlendOperation = blend_op;
		pipe_desc.colorAttachments[0].sourceRGBBlendFactor = src_factor;
		pipe_desc.colorAttachments[0].sourceAlphaBlendFactor = src_factor;
		pipe_desc.colorAttachments[0].destinationRGBBlendFactor = dest_factor;
		pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = dest_factor;
	}

	NSError* error = NULL;
	internal_pipe.pipeline_state = [metal_renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
	if(!internal_pipe.pipeline_state)
	{
		NSString* str_error = [NSString stringWithFormat:@"%@", error];
		const char* c = [str_error cStringUsingEncoding:NSUTF8StringEncoding];
		DM_LOG_FATAL("Could not create metal pipeline state: %s", c);
		return false;
	}

	// depth stencil
	MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
	if(desc.depth)
	{
		depth_stencil_desc.depthCompareFunction = dm_compare_to_metal_compare(desc.depth_comp);
		depth_stencil_desc.depthWriteEnabled = YES;
	}
	else
	{
		depth_stencil_desc.depthWriteEnabled = NO;
	}
	
	internal_pipe.depth_stencil_state = [metal_renderer->device newDepthStencilStateWithDescriptor:depth_stencil_desc];
	if(!internal_pipe.depth_stencil_state)
	{
		DM_LOG_FATAL("Could not create metal depth stencil state!");
		return false;
	}

	// sampler
	MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
	sampler_desc.minFilter = dm_minmagfilter_to_metal_minmagfilter(desc.sampler_filter);
	sampler_desc.magFilter = dm_minmagfilter_to_metal_minmagfilter(desc.sampler_filter);
	sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
	sampler_desc.sAddressMode = dm_tex_mode_to_metal_sampler_mode(desc.u_mode);
	sampler_desc.tAddressMode = dm_tex_mode_to_metal_sampler_mode(desc.v_mode);

	internal_pipe.sampler_state = [metal_renderer->device newSamplerStateWithDescriptor:sampler_desc];
	if(!internal_pipe.sampler_state)
	{
		DM_LOG_FATAL("Could not create metal sampler state!");
		return false;
	}

	// raster stuff
	internal_pipe.winding = dm_winding_to_metal_winding(desc.winding_order);
	internal_pipe.cull_mode = dm_cull_to_metal_cull(desc.cull_mode);

	internal_pipe.primitive_type = dm_topology_to_metal_primitive(desc.primitive_topology);

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
	metal_renderer->metal_view = [[dm_metal_view alloc] init];
	if(!metal_renderer->metal_view)
	{
		DM_LOG_FATAL("Could not create metal view.");
		return false;
	}

	metal_renderer->metal_view.metal_layer.device = metal_renderer->device;
	metal_renderer->metal_view.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

	[metal_renderer->metal_view setFrame:frame];
	[metal_renderer->metal_view.metal_layer setFrame:frame];

	metal_renderer->command_queue = [metal_renderer->device newCommandQueue];
	if(!metal_renderer->command_queue)
	{
		DM_LOG_FATAL("Could not create metal command queue.");
		return false;
	}

	// content view is the main view for our NSWindow
    // must add our view to the subviews
	[internal_data->content_view addSubview:metal_renderer->metal_view];

	// must set the content view's layer to our metal layer
	[internal_data->content_view setWantsLayer: YES];
	[internal_data->content_view setLayer:metal_renderer->metal_view.metal_layer];

	CGFloat contents_scale = internal_data->content_view.layer.contentsScale;
    NSSize layer_size = internal_data->content_view.layer.frame.size;
    metal_renderer->metal_view.metal_layer.contentsScale = contents_scale;
    metal_renderer->metal_view.metal_layer.drawableSize = NSMakeSize(layer_size.width * contents_scale, layer_size.height * contents_scale);
        
	// create the depth texture
    CGSize drawableSize = metal_renderer->metal_view.metal_layer.drawableSize;
    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:drawableSize.width height:drawableSize.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    metal_renderer->metal_view.depth_texture = [metal_renderer->device newTextureWithDescriptor:descriptor];
    metal_renderer->metal_view.depth_texture.label = @"DepthStencil";

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
    
    // textures
    for(uint32_t i=0; i<metal_renderer->texture_count; i++)
    {
        dm_renderer_backend_destroy_texture(i, &context->renderer);
    }
    
    /*
    // framebuffers
    for(uint32_t i=0; i<metal_renderer->framebuffer_count; i++)
    {
        dm_renderer_backend_destroy_framebuffer(i, &context->renderer);
    }
    */
    
    // pipelines
    for(uint32_t i=0; i<metal_renderer->pipeline_count; i++)
    {
        dm_renderer_backend_destroy_pipeline(i, &context->renderer);
    }
    
    [metal_renderer->metal_view.depth_texture release];
	[metal_renderer->metal_view release];
    if(metal_renderer->command_buffer) [metal_renderer->command_buffer release];
    [metal_renderer->command_queue release];
	[metal_renderer->device release];
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
	desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
	metal_renderer->command_buffer = [metal_renderer->command_queue commandBufferWithDescriptor:desc];
	[metal_renderer->metal_view nextDrawable];

	metal_renderer->new_frame = true;

	[desc release];

	return true;
}

bool dm_renderer_backend_end_frame(bool vsync, dm_context* context)
{
	dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;

	if([metal_renderer->metal_view hasDrawable]) [metal_renderer->metal_view presentDrawable:metal_renderer->command_buffer];

	[metal_renderer->command_buffer commit];
	[metal_renderer->command_buffer release];

	metal_renderer->active_shader = DM_METAL_INVALID_RESOURCE;
	metal_renderer->active_pipeline = DM_METAL_INVALID_RESOURCE;

	return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
	return true;
}

void dm_metal_renew_encoder(dm_metal_renderer* metal_renderer)
{
	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];

	// renew the command encoder
	MTLRenderPassDescriptor* passDescriptor = [metal_renderer->metal_view currentRenderPassDescriptor:&metal_renderer->new_frame];

	active_shader->command_encoder = [metal_renderer->command_buffer renderCommandEncoderWithDescriptor:passDescriptor];
	
	[active_shader->command_encoder setViewport:metal_renderer->metal_view.viewport];

	// rebind the pipeline
	dm_metal_pipeline pipeline = metal_renderer->pipelines[metal_renderer->active_pipeline];

	[active_shader->command_encoder setRenderPipelineState:pipeline.pipeline_state];
	[active_shader->command_encoder setDepthStencilState:pipeline.depth_stencil_state];
	[active_shader->command_encoder setFrontFacingWinding:pipeline.winding];
	[active_shader->command_encoder setCullMode:pipeline.cull_mode];
	[active_shader->command_encoder setFragmentSamplerState:pipeline.sampler_state atIndex:0];

	[passDescriptor release];
}

/********
COMMANDS
**********/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	metal_renderer->metal_view->clear_color[0] = r;
	metal_renderer->metal_view->clear_color[1] = g;
	metal_renderer->metal_view->clear_color[2] = b;
	metal_renderer->metal_view->clear_color[3] = a;
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	MTLViewport new_viewport;
	new_viewport.originX = 0.0f;
	new_viewport.originY = 0.0f;
	new_viewport.width = width;
	new_viewport.height = height;
	new_viewport.znear = 0.01f;
	new_viewport.zfar = 1000.0f;

	[metal_renderer->metal_view setViewport : new_viewport];
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;
	
	if(handle > metal_renderer->pipeline_count) { DM_LOG_FATAL("Trying to bind invalid Metal pipeline"); return false; }

	dm_metal_shader active_shader = metal_renderer->shaders[metal_renderer->active_shader];
	dm_metal_pipeline pipeline = metal_renderer->pipelines[handle];

	[active_shader.command_encoder setRenderPipelineState:pipeline.pipeline_state];
	[active_shader.command_encoder setDepthStencilState:pipeline.depth_stencil_state];
	[active_shader.command_encoder setFrontFacingWinding:pipeline.winding];
	[active_shader.command_encoder setCullMode:pipeline.cull_mode];
	[active_shader.command_encoder setFragmentSamplerState:pipeline.sampler_state atIndex:0];

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

	if(![metal_renderer->metal_view hasDrawable]) return false;

	if(handle > metal_renderer->shader_count) { DM_LOG_FATAL("Trying to bind invalid Metal shader"); return false; }

	// finish any active shaders
	if(metal_renderer->active_shader != DM_METAL_INVALID_RESOURCE) [metal_renderer->shaders[metal_renderer->active_shader].command_encoder endEncoding];

	dm_metal_shader* internal_shader = &metal_renderer->shaders[handle];

	MTLRenderPassDescriptor* passDescriptor = [metal_renderer->metal_view currentRenderPassDescriptor:&metal_renderer->new_frame];

	internal_shader->command_encoder = [metal_renderer->command_buffer renderCommandEncoderWithDescriptor:passDescriptor];
	
	[internal_shader->command_encoder setViewport:metal_renderer->metal_view.viewport];

	metal_renderer->active_shader = handle;

	[passDescriptor release];

	return true;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return false;

	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal buffer"); return false; }

	dm_metal_buffer* internal_buffer = &metal_renderer->buffers[handle];

	internal_buffer->index = slot;

	if(internal_buffer->type == DM_METAL_BUFFER_TYPE_INDEX) metal_renderer->shaders[metal_renderer->active_shader].index_buffer = handle;
	else if(internal_buffer->type == DM_METAL_BUFFER_TYPE_VERTEX) 
	{
		dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
		[active_shader->command_encoder setVertexBuffer:internal_buffer->buffer offset:0 atIndex:slot];
		active_shader->uniform_offset++;
	}

	return true;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return false;

	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid Metal buffer"); return false; }

	dm_memcpy([metal_renderer->buffers[handle].buffer contents]+offset, data, data_size);

	return true;
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return false;

	if(handle > metal_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid Metal uniform"); return false; }

	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
	dm_metal_buffer* internal_uniform = &metal_renderer->buffers[handle];

	if(stage!=DM_UNIFORM_STAGE_PIXEL) [active_shader->command_encoder setVertexBuffer:internal_uniform->buffer offset:0 atIndex:slot+offset];
	if(stage!=DM_UNIFORM_STAGE_VERTEX) [active_shader->command_encoder setFragmentBuffer:internal_uniform->buffer offset:0 atIndex:slot];

	return true;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
	return dm_render_command_backend_update_buffer(handle, data, data_size, 0, renderer);
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return false;

	if(handle > metal_renderer->texture_count) { DM_LOG_FATAL("Trying to bind invalid Metal texture"); return false; }

	[metal_renderer->shaders[metal_renderer->active_shader].command_encoder setFragmentTexture:metal_renderer->textures[handle].texture atIndex:slot];

	return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
	DM_LOG_FATAL("Not supported yet");
	return false;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
	DM_LOG_FATAL("Not supported yet");
	return false;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
	DM_LOG_FATAL("Not supported yet");
	return false;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
	DM_LOG_FATAL("Not supported yet");
	return false;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return;

	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];
	
	[active_shader->command_encoder drawPrimitives:metal_renderer->pipelines[metal_renderer->active_pipeline].primitive_type vertexStart:start vertexCount:count];

	[active_shader->command_encoder endEncoding];
	if(metal_renderer->renew_encoder) dm_metal_renew_encoder(metal_renderer);
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return;

	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];

	[active_shader->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:metal_renderer->buffers[active_shader->index_buffer].buffer indexBufferOffset:index_offset*sizeof(uint32_t)];

	[active_shader->command_encoder endEncoding];
	if(metal_renderer->renew_encoder) dm_metal_renew_encoder(metal_renderer);
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
	DM_METAL_GET_RENDERER;

	if(![metal_renderer->metal_view hasDrawable]) return;

	dm_metal_shader* active_shader = &metal_renderer->shaders[metal_renderer->active_shader];

	[active_shader->command_encoder drawIndexedPrimitives:metal_renderer->active_primitive indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:metal_renderer->buffers[active_shader->index_buffer].buffer indexBufferOffset:index_offset*sizeof(uint32_t) instanceCount:num_insts];

	[active_shader->command_encoder endEncoding];
	if(metal_renderer->renew_encoder) dm_metal_renew_encoder(metal_renderer);
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
	DM_LOG_ERROR("Not supported yet");
}
