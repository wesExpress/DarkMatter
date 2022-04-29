#include "core/dm_defines.h"

#ifdef DM_METAL

#include "rendering/dm_renderer.h"
#include "rendering/dm_render_types.h"
#include "core/dm_math.h"
#include "core/dm_logger.h"
#include "core/dm_mem.h"
#include "platform/dm_platform_apple.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <stdbool.h>

typedef struct dm_internal_buffer
{
    id<MTLBuffer> buffer;
} dm_internal_buffer;

typedef struct dm_internal_texture
{
    id<MTLTexture> texture;
} dm_internal_texture;

typedef struct dm_internal_shader
{
	id<MTLLibrary> library;
	id<MTLFunction> vertex_func;
	id<MTLFunction> fragment_func;
} dm_internal_shader;

typedef struct dm_internal_pass
{
	id<MTLRenderPipelineState> pipeline_state;
	id<MTLRenderCommandEncoder> command_encoder;
	id<MTLBuffer> uniform_buffer;
} dm_internal_pass;

@interface dm_metal_view : NSView

@property (nonatomic, strong) CAMetalLayer* metal_layer;
@property (nonatomic, strong) id<CAMetalDrawable> drawable;
@property (nonatomic) dm_color clear_color;

- (id) init;
- (MTLRenderPassDescriptor*)currentRenderPassDescriptor;

@end

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

- (MTLRenderPassDescriptor*)currentRenderPassDescriptor
{
	MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];
	
	desc.colorAttachments[0].texture = [_drawable texture];
	desc.colorAttachments[0].storeAction = MTLStoreActionStore;
	desc.colorAttachments[0].loadAction = MTLLoadActionLoad;

	return desc;
}

@end

typedef struct dm_metal_renderer
{
	dm_metal_view* metal_view;

	id<MTLDevice> device;
	
	id<MTLCommandQueue> command_queue;
	id<MTLCommandBuffer> command_buffer;
	
	id<MTLDepthStencilState> depth_stencil_state;
	id<MTLSamplerState> sampler_state;

	dm_buffer* active_index_buffer;

	MTLViewport active_viewport;

	MTLCullMode cull_mode;
	MTLWinding winding;
} dm_metal_renderer;

/*******
GLOBALS
*********/

#define DM_METAL_BUFFER_ALIGNMENT 256

size_t dm_metal_align(size_t n, uint32_t alignment) 
{
    return ((n+alignment-1)/alignment)*alignment;
}

dm_metal_renderer renderer = { 0 };

/***************
ENUM CONVERSION
*****************/

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

/******
BUFFER
********/

dm_internal_buffer* dm_metal_create_buffer(dm_buffer* buffer, void* data)
{
	dm_internal_buffer* internal_buffer = dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);

	if(data)
	{
		internal_buffer->buffer = [renderer.device newBufferWithBytes:data length:buffer->desc.buffer_size options:MTLResourceOptionCPUCacheModeDefault];
	}
	else
	{
		internal_buffer->buffer = [renderer.device newBufferWithLength:buffer->desc.buffer_size options:MTLResourceOptionCPUCacheModeDefault];
	}

	if(!internal_buffer->buffer)
	{
		DM_LOG_FATAL("Could not create metal buffer!");
		return false;
	}

	if (buffer->desc.type == DM_BUFFER_TYPE_INDEX)
	{
		renderer.active_index_buffer = buffer;
	}

	return internal_buffer;
}

void dm_metal_destroy_buffer(dm_buffer* buffer)
{
	dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

/******
SHADER
********/

dm_internal_shader* dm_metal_create_shader(dm_shader* shader)
{
	char file_buffer[256];
	sprintf(file_buffer, "shaders/metal/%s.metallib", shader->pass);

	NSString* shader_file = [NSString stringWithUTF8String:file_buffer];
	dm_internal_shader* internal_shader = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDER_PASS);
		
	internal_shader->library = [renderer.device newLibraryWithFile:shader_file error:NULL];
	if(!internal_shader->library)
	{
		DM_LOG_FATAL("Could not create metal library from file %s", [shader_file UTF8String]);
		return NULL;
	}

	for(uint32_t i=0; i<shader->num_stages; i++)
	{
		dm_shader_desc stage = shader->stages[i];

		NSString* func_name = [[NSString alloc] initWithUTF8String:stage.source];

		switch(stage.type)
		{
			case DM_SHADER_TYPE_VERTEX:
				internal_shader->vertex_func = [internal_shader->library newFunctionWithName:func_name];
				break;
			case DM_SHADER_TYPE_PIXEL:
				internal_shader->fragment_func = [internal_shader->library newFunctionWithName:func_name];
				break;
			default:
				DM_LOG_ERROR("Unknown shader type, shouldn't be here...");
				break;
		}
	}

	if(!internal_shader->vertex_func)
	{
		DM_LOG_FATAL("Could not create vertex function from shader: %s", shader->pass);
		return NULL;
	}

	if(!internal_shader->fragment_func)
	{
		DM_LOG_FATAL("Could not create fragment function from shader: %s", shader->pass);
		return NULL;
	}

	return internal_shader;
}

/*******
TEXTURE
*********/

dm_internal_texture* dm_metal_create_texture(dm_image* image)
{
	dm_internal_texture* internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);

	MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

	texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
	texture_desc.width = image->desc.width;
	texture_desc.height = image->desc.height;
	texture_desc.usage = MTLTextureUsageShaderRead;

	internal_texture->texture = [renderer.device newTextureWithDescriptor:texture_desc];
	if(!internal_texture)
    {
		DM_LOG_FATAL("Could not create metal texture from image: %s", image->desc.path);
	return false;
	}

	MTLRegion region = MTLRegionMake2D(0, 0, image->desc.width, image->desc.height);

	NSUInteger bytes_per_row = 4 * image->desc.width;

	[internal_texture->texture replaceRegion:region mipmapLevel:0 withBytes:image->data bytesPerRow:bytes_per_row];

	id <MTLCommandBuffer> command_buffer = [renderer.command_queue commandBuffer];
	id <MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
	[blit_encoder generateMipmapsForTexture: internal_texture->texture];
	[blit_encoder endEncoding];
	[command_buffer commit];
	[command_buffer waitUntilCompleted];

	return internal_texture;
}

void dm_metal_destroy_texture(dm_image* image)
{
	dm_free(image->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

/*************
MAIN RENDERER
***************/

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_render_pipeline* pipeline)
{
	DM_LOG_DEBUG("Initializing Metal render backend...");

	dm_internal_apple_data* internal_data = platform_data->internal_data;
	NSRect frame = [internal_data->content_view getWindowFrame];

	renderer.device = MTLCreateSystemDefaultDevice();
	renderer.metal_view = [[dm_metal_view alloc] init];
	if(!renderer.metal_view)
	{
		DM_LOG_FATAL("Could not create metal view!");
		return false;
	}

	renderer.metal_view.metal_layer.device = renderer.device;
	renderer.metal_view.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

	[renderer.metal_view setFrame:frame];
	[renderer.metal_view.metal_layer setFrame:frame];

	renderer.command_queue = [renderer.device newCommandQueue];
	if(!renderer.command_queue)
	{
		DM_LOG_FATAL("Could not create metal command queue!");
		return false;
	}

	// content view is the main view for our NSWindow
    // must add our view to the subviews
    [internal_data->content_view addSubview:renderer.metal_view];

    // must set the content view's layer to our metal layer
    [internal_data->content_view setWantsLayer: YES];
    [internal_data->content_view setLayer:renderer.metal_view.metal_layer];
	
	// depth stencil
    MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
    depth_stencil_desc.depthCompareFunction = dm_compare_to_metal_compare(pipeline->depth_desc.comparison);
    depth_stencil_desc.depthWriteEnabled = YES;
    renderer.depth_stencil_state = [renderer.device newDepthStencilStateWithDescriptor:depth_stencil_desc];
    if(!renderer.depth_stencil_state)
    {
    	DM_LOG_FATAL("Could not create metal depth stencil state!");
		return false;
	}

	// sampler
    MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
    sampler_desc.minFilter = dm_minmagfilter_to_metal_minmagfilter(pipeline->sampler_desc.filter);
    sampler_desc.magFilter = dm_minmagfilter_to_metal_minmagfilter(pipeline->sampler_desc.filter);
    sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
    sampler_desc.sAddressMode = dm_tex_mode_to_metal_sampler_mode(pipeline->sampler_desc.u);
    sampler_desc.tAddressMode = dm_tex_mode_to_metal_sampler_mode(pipeline->sampler_desc.v);

    renderer.sampler_state = [renderer.device newSamplerStateWithDescriptor:sampler_desc];
    if(!renderer.sampler_state)
    {
        DM_LOG_FATAL("Could not create metal sampler state!");
        return false;
    }

	// raster stuff
	renderer.winding = dm_winding_to_metal_winding(pipeline->raster_desc.winding_order);
	renderer.cull_mode = dm_cull_to_metal_cull(pipeline->raster_desc.cull_mode);

	return true;
}

void dm_renderer_shutdown_impl()
{
}

bool dm_renderer_begin_frame_impl()
{
	MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
	desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
	renderer.command_buffer = [renderer.command_queue commandBufferWithDescriptor:desc];
	renderer.metal_view.drawable = [renderer.metal_view.metal_layer nextDrawable];

	return true;
}

bool dm_renderer_end_frame_impl()
{
	if(renderer.metal_view.drawable)
	{
		[renderer.command_buffer presentDrawable:renderer.metal_view.drawable];
	}

	[renderer.command_buffer commit];
	
return true;
}

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout layout)
{
	render_pass->internal_pass = dm_alloc(sizeof(dm_internal_pass), DM_MEM_RENDER_PASS);
	dm_internal_pass* internal_pass = render_pass->internal_pass;

	render_pass->shader.internal_shader = dm_metal_create_shader(&render_pass->shader);
	//if(!dm_metal_create_shader(&render_pass->shader)) return false;
	if (!render_pass->shader.internal_shader) return false;

	dm_internal_shader* internal_shader = render_pass->shader.internal_shader;

	// pipeline state
	MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
	pipe_desc.vertexFunction = internal_shader->vertex_func;
	pipe_desc.fragmentFunction = internal_shader->fragment_func;
	pipe_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

	NSError* error = NULL;
	internal_pass->pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
	if(!internal_pass->pipeline_state)
	{
		DM_LOG_FATAL("Could not create metal pipeline state");
		return NULL;
	}

	// uniform buffer
	size_t buffer_size = 0;
	dm_for_map_item(render_pass->uniforms)
	{
		dm_uniform* uniform = item->value;
		buffer_size += uniform->data_size;
	}
	
	size_t aligned_size = dm_metal_align(buffer_size, DM_METAL_BUFFER_ALIGNMENT);
	internal_pass->uniform_buffer = [renderer.device newBufferWithLength:aligned_size options:MTLResourceOptionCPUCacheModeDefault];

	return true;
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
	dm_free(render_pass->shader.internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDER_PASS);
	dm_free(render_pass->internal_pass, sizeof(dm_internal_pass), DM_MEM_RENDER_PASS);
}

bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data)
{
	buffer->internal_buffer = dm_metal_create_buffer(buffer, data);

	return (buffer->internal_buffer != NULL);
}

void dm_renderer_delete_buffer_impl(dm_buffer* buffer)
{
	dm_metal_destroy_buffer(buffer);
}

bool dm_create_texture_impl(dm_image* image)
{
	image->internal_texture = dm_metal_create_texture(image);
	
	return (image->internal_texture != NULL);
}

void dm_destroy_texture_impl(dm_image* image)
{
	dm_metal_destroy_texture(image);
}

/*
bool dm_renderer_test_func()
{
	// vertices
	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f, 1.0f,
		 0.0f,  0.5f, 0.0f, 1.0f
	};
	id<MTLBuffer> vertex_buffer = [renderer.device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceOptionCPUCacheModeDefault];

	// uniform
	typedef struct uniforms
	{
		dm_vec3 blah;
	} uniforms;
	uniforms uniform = { .blah = {1,2,3} };
	id<MTLBuffer> uniform_buffer = [renderer.device newBufferWithLength:sizeof(uniforms) options:MTLResourceOptionCPUCacheModeDefault];

	// shader library
	NSError* error = NULL;
	id<MTLLibrary> lib = [renderer.device newLibraryWithFile:@"shaders/metal/test.metallib" error:&error];
	if(!lib)
	{
		DM_LOG_FATAL("%d", error.code);
	}

	// drawable
	renderer.metal_view.drawable = [renderer.metal_view.metal_layer nextDrawable];
 
	// command buffer
	MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
	desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
	renderer.command_buffer = [renderer.command_queue commandBufferWithDescriptor:desc];

	// pass descriptor
	MTLRenderPassDescriptor* pass_desc = [renderer.metal_view currentRenderPassDescriptor];
	renderer.command_encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:pass_desc];

	// pipeline
	MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.vertexFunction = [lib newFunctionWithName:@"vertex_main"];
    pipelineDescriptor.fragmentFunction = [lib newFunctionWithName:@"fragment_main"];

	id<MTLRenderPipelineState> pipeline = [renderer.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

	[renderer.command_encoder setRenderPipelineState:pipeline];
	//[renderer.command_encoder setDepthStencilState:renderer.depth_stencil_state];
	[renderer.command_encoder setFrontFacingWinding:MTLWindingCounterClockwise]; 
	[renderer.command_encoder setCullMode:MTLCullModeBack];
	[renderer.command_encoder setViewport:renderer.active_viewport];

	// bind the vertex buffers
	[renderer.command_encoder setVertexBuffer:vertex_buffer offset:0 atIndex:0];
	dm_memcpy([uniform_buffer contents], &uniform, sizeof(uniforms));
	[renderer.command_encoder setVertexBuffer:uniform_buffer offset:0 atIndex:1];

	//dm_internal_buffer* index_buffer = renderer.active_index_buffer->internal_buffer;
	//[renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:36 indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:0];
	[renderer.command_encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
	[renderer.command_encoder endEncoding];

	[renderer.command_buffer presentDrawable:renderer.metal_view.drawable];
	[renderer.command_buffer commit];

	return true;
}
*/

/***************
RENDER COMMANDS
*****************/

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;

		// render pass descriptor
		MTLRenderPassDescriptor* passDescriptor = [renderer.metal_view currentRenderPassDescriptor];

		internal_pass->command_encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];
	
		[internal_pass->command_encoder setRenderPipelineState:internal_pass->pipeline_state];
		[internal_pass->command_encoder setDepthStencilState:renderer.depth_stencil_state];
		[internal_pass->command_encoder setFrontFacingWinding:renderer.winding]; 
		[internal_pass->command_encoder setCullMode:renderer.cull_mode];
		[internal_pass->command_encoder setFragmentSamplerState:renderer.sampler_state atIndex:0];
	}
	return true;
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;

		[internal_pass->command_encoder endEncoding];
	}
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
{
	if(renderer.metal_view.drawable)
	{
		MTLViewport new_viewport;
		new_viewport.originX = viewport.x;
		new_viewport.originY = viewport.y;
		new_viewport.width = viewport.width;
		new_viewport.height = viewport.height;
		new_viewport.znear = viewport.min_depth;
		new_viewport.zfar = viewport.max_depth;

		MTLRenderPassDescriptor* pass_descriptor = [renderer.metal_view currentRenderPassDescriptor];
		id<MTLRenderCommandEncoder> encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:pass_descriptor];
		[encoder setViewport:new_viewport];
		[encoder endEncoding];
	}
}

void dm_renderer_clear_impl(dm_color clear_color)
{
	if(renderer.metal_view.drawable)
	{
		MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
		passDescriptor.colorAttachments[0].texture = [renderer.metal_view.drawable texture];
		passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
		passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
		passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

		id<MTLRenderCommandEncoder> encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];
		[encoder endEncoding];
	}
}

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;

		[internal_pass->command_encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:start vertexCount:count];
	}
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;
		dm_internal_buffer* index_buffer = renderer.active_index_buffer->internal_buffer;

		[internal_pass->command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset*sizeof(dm_index_t)];
	}
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;
		dm_internal_buffer* index_buffer = renderer.active_index_buffer->internal_buffer;

		[internal_pass->command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset instanceCount:num_insts];
	}
}

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size)
{
	dm_internal_buffer* internal_buffer = buffer->internal_buffer;

	dm_memcpy([internal_buffer->buffer contents], data, data_size);

	return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;
		dm_internal_buffer* internal_buffer = buffer->internal_buffer;
		
		id<MTLBuffer> bind_buffer = [renderer.device newBufferWithBytes:[internal_buffer->buffer contents] length:buffer->desc.buffer_size options:MTLResourceOptionCPUCacheModeDefault];
		[internal_pass->command_encoder setVertexBuffer:bind_buffer offset:0 atIndex:slot];
	}
	return true;
}

bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;
		dm_internal_texture* internal_texture = image->internal_texture;
		
		[internal_pass->command_encoder setFragmentTexture:internal_texture->texture atIndex:slot];
	}

	return true;
}

bool dm_renderer_bind_uniforms_impl(uint32_t slot, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_pass* internal_pass = render_pass->internal_pass;

		size_t buffer_size = 0;
		void* buffer_data = NULL;
		dm_for_map_item(render_pass->uniforms)
		{
			dm_uniform* uniform = item->value;
		
			buffer_data = dm_realloc(buffer_data, buffer_size + uniform->data_size);
			void* dest = (char*)buffer_data + buffer_size;
			dm_memcpy(dest, uniform->data, uniform->data_size);
			buffer_size += uniform->data_size;
		}

		size_t aligned_size = dm_metal_align(buffer_size, DM_METAL_BUFFER_ALIGNMENT);
		dm_memcpy([internal_pass->uniform_buffer contents], buffer_data, aligned_size);
		[internal_pass->command_encoder setVertexBuffer:internal_pass->uniform_buffer offset:0 atIndex:slot];
		[internal_pass->command_encoder setFragmentBuffer:internal_pass->uniform_buffer offset:0 atIndex:0];
	}

	return true;
}

#endif