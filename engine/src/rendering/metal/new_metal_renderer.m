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
	id<MTLBuffer> uniform_buffer;
} dm_internal_pass;

@interface dm_metal_view : NSView

@property (nonatomic, strong) CAMetalLayer* metal_layer;
@property (nonatomic, strong) id<CAMetalDrawable> drawable;

- (id) init;

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

@end

typedef struct dm_metal_renderer
{
	dm_metal_view* metal_view;

	id<MTLDevice> device;
	
	id<MTLCommandQueue> command_queue;
	id<MTLCommandBuffer> command_buffer;
	id<MTLRenderCommandEncoder> command_encoder;

	id<MTLRenderPipelineState> pipeline_state;
	id<MTLDepthStencilState> depth_stencil_state;
	id<MTLSamplerState> sampler_state;

	dm_buffer* active_index_buffer;

	MTLViewport active_viewport;

	dm_color clear_color;
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

/******
BUFFER
********/

bool dm_metal_create_buffer(dm_buffer* buffer, void* data)
{
	@autoreleasepool
	{
		buffer->internal_buffer = dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
		dm_internal_buffer* internal_buffer = buffer->internal_buffer;

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
	}

	return true;
}

void dm_metal_destroy_buffer(dm_buffer* buffer)
{
	dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

/******
SHADER
********/

bool dm_metal_create_shader(dm_shader* shader)
{
	@autoreleasepool
	{
		char file_buffer[256];
		sprintf(file_buffer, "shaders/metal/%s.metallib", shader->pass);

		NSString* shader_file = [NSString stringWithUTF8String:file_buffer];
		shader->internal_shader = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDER_PASS);
		dm_internal_shader* internal_shader = shader->internal_shader;
		
		internal_shader->library = [renderer.device newLibraryWithFile:shader_file error:NULL];
		if(!internal_shader->library)
		{
			DM_LOG_FATAL("Could not create metal library from file %s", [shader_file UTF8String]);
			return false;
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
			return false;
		}

		if(!internal_shader->fragment_func)
		{
			DM_LOG_FATAL("Could not create fragment function from shader: %s", shader->pass);
			return false;
		}
	}

	return true;
}

/*******
TEXTURE
*********/

bool dm_metal_create_texture(dm_image* image)
{
	@autoreleasepool
	{
		image->internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
		dm_internal_texture* internal_texture = image->internal_texture;

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

        [internal_texture->texture replaceRegion: region
                                   mipmapLevel: 0
                                   withBytes: image->data
                                   bytesPerRow: bytes_per_row];

        id <MTLCommandBuffer> command_buffer = [renderer.command_queue commandBuffer];
        id <MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        [blit_encoder generateMipmapsForTexture: internal_texture->texture];
        [blit_encoder endEncoding];
        [command_buffer commit];
	}

	return true;
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
	@autoreleasepool
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
    	depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
    	depth_stencil_desc.depthWriteEnabled = YES;
    	renderer.depth_stencil_state = [renderer.device newDepthStencilStateWithDescriptor:depth_stencil_desc];
    	if(!renderer.depth_stencil_state)
    	{
        	DM_LOG_FATAL("Could not create metal depth stencil state!");
			return false;
		}

		// sampler
    	MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
    	sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
    	sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;
    	sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
    	sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    	sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;

    	renderer.sampler_state = [renderer.device newSamplerStateWithDescriptor:sampler_desc];
    	if(!renderer.sampler_state)
    	{
        	DM_LOG_FATAL("Could not create metal sampler state!");
        	return false;
    	}
	}

	return true;
}

void dm_renderer_shutdown_impl()
{
}

bool dm_renderer_begin_frame_impl()
{
	renderer.metal_view.drawable = [renderer.metal_view.metal_layer nextDrawable];
	renderer.command_buffer = [renderer.command_queue commandBuffer];

	return true;
}

bool dm_renderer_end_frame_impl()
{
	[renderer.command_buffer presentDrawable:renderer.metal_view.drawable];
	[renderer.command_buffer commit];

	return true;
}

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout layout)
{
	@autoreleasepool
	{
		render_pass->internal_pass = dm_alloc(sizeof(dm_internal_pass), DM_MEM_RENDER_PASS);
		dm_internal_pass* internal_pass = render_pass->internal_pass;

		if(!dm_metal_create_shader(&render_pass->shader)) return false;
		dm_internal_shader* internal_shader = render_pass->shader.internal_shader;

		// pipeline state
		MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
		pipe_desc.vertexFunction = internal_shader->vertex_func;
		pipe_desc.fragmentFunction = internal_shader->fragment_func;

        pipe_desc.colorAttachments[0].pixelFormat = renderer.metal_view.metal_layer.pixelFormat;
        pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        renderer.pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!renderer.pipeline_state)
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
		internal_pass->uniform_buffer = [renderer.device newBufferWithLength:buffer_size options:MTLResourceOptionCPUCacheModeDefault];
	}

	return true;
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
	@autoreleasepool
	{
		dm_free(render_pass->shader.internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDER_PASS);
		dm_free(render_pass->internal_pass, sizeof(dm_internal_pass), DM_MEM_RENDER_PASS);
	}
}

bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data)
{
	return dm_metal_create_buffer(buffer, data);
}

void dm_renderer_delete_buffer_impl(dm_buffer* buffer)
{
	dm_metal_destroy_buffer(buffer);
}

bool dm_create_texture_impl(dm_image* image)
{
	return dm_metal_create_texture(image);
}

void dm_destroy_texture_impl(dm_image* image)
{
}

/***************
RENDER COMMANDS
*****************/

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
	id<MTLTexture> texture = renderer.metal_view.drawable.texture;

    // render pass descriptor
    MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
    passDescriptor.colorAttachments[0].texture = texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(renderer.clear_color.x, renderer.clear_color.y, renderer.clear_color.z, renderer.clear_color.w);

    renderer.command_encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];
	
	[renderer.command_encoder setRenderPipelineState:renderer.pipeline_state];
	[renderer.command_encoder setDepthStencilState:renderer.depth_stencil_state];
	[renderer.command_encoder setFrontFacingWinding:MTLWindingCounterClockwise]; 
    [renderer.command_encoder setCullMode:MTLCullModeBack];
	[renderer.command_encoder setFragmentSamplerState:renderer.sampler_state atIndex:0];
	[renderer.command_encoder setViewport:renderer.active_viewport];

	return true;
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
	@autoreleasepool
	{
		[renderer.command_encoder endEncoding];
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
		
		renderer.active_viewport = new_viewport;
	}
}

void dm_renderer_clear_impl(dm_color clear_color)
{
	renderer.clear_color = clear_color;
}

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
	
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_buffer* index_buffer = renderer.active_index_buffer->internal_buffer;

		[renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset];
	}
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
	if(renderer.metal_view.drawable)
	{
		dm_internal_buffer* index_buffer = renderer.active_index_buffer->internal_buffer;

		[renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset:index_offset instanceCount:num_insts];
	}
}

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size)
{
	dm_internal_buffer* internal_buffer = buffer->internal_buffer;

	dm_memcpy([internal_buffer->buffer contents], data, dm_metal_align(data_size, DM_METAL_BUFFER_ALIGNMENT));

	return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot)
{
	dm_internal_buffer* internal_buffer = buffer->internal_buffer;
	[renderer.command_encoder setVertexBuffer:internal_buffer->buffer offset:0 atIndex:slot];

	return true;
}

bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot)
{
	return true;
}

bool dm_renderer_bind_uniform_impl(dm_uniform* uniform)
{
	return true;
}

#endif