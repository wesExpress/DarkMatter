#include "dm_metal_renderer.h"

#ifdef DM_METAL

#include "dm_metal_view.h"
#include "dm_metal_buffer.h"
#include "dm_metal_shader.h"
#include "dm_metal_texture.h"
#include "dm_metal_render_pass.h"

#include "rendering/dm_image.h"

#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "core/dm_string.h"

#include "platform/dm_platform_apple.h"

#include "structures/dm_list.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

@implementation dm_metal_renderer

-(id) initWithFrame: (NSRect)frame
{
    self = [super init];

    if(self)
    {
        _device = MTLCreateSystemDefaultDevice();

        _view = [[dm_metal_view alloc] init];
        if(!_view)
        {
            DM_LOG_FATAL("Could not create metal view!");
            return NULL;
        }

        _view.metal_layer.device = _device;
        _view.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        [_view setFrame: frame];
        [_view.metal_layer setFrame: frame];

        _command_queue = [_device newCommandQueue];
        if(!_command_queue)
        {
            DM_LOG_FATAL("Could not create metal command queue!");
            return false;
        }
    }

    return self;
}

@end

#define DM_METAL_BUFFER_ALIGNMENT 256

size_t dm_metal_align(size_t n, uint32_t alignment) 
{
    return ((n+alignment-1)/alignment)*alignment;
}

dm_metal_renderer* metal_renderer = NULL;

/*************
MAIN RENDERER
***************/

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_render_pipeline* pipeline)
{
    DM_LOG_DEBUG("Initializing Metal render backend...");

    @autoreleasepool
    {
        dm_internal_apple_data* internal_data = platform_data->internal_data;
        NSRect frame = [internal_data->content_view getWindowFrame];

        metal_renderer = [[dm_metal_renderer alloc] initWithFrame: frame];

        // content view is the main view for our NSWindow
        // must add our view to the subviews
        [internal_data->content_view addSubview: metal_renderer.view];

        // must set the content view's layer to our metal layer
        [internal_data->content_view setWantsLayer: YES];
        [internal_data->content_view setLayer: metal_renderer.view.metal_layer];
		
		// depth stencil
        MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
        depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
        depth_stencil_desc.depthWriteEnabled = YES;
        metal_renderer.depth_stencil = [metal_renderer.device newDepthStencilStateWithDescriptor:depth_stencil_desc];
        if(!metal_renderer.depth_stencil)
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

        metal_renderer.sampler_state = [metal_renderer.device newSamplerStateWithDescriptor:sampler_desc];
        if(!metal_renderer.sampler_state)
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
	metal_renderer.drawable = [metal_renderer.view.metal_layer nextDrawable];

	metal_renderer.command_buffer = [metal_renderer.command_queue commandBuffer];

	return true;
}

bool dm_renderer_end_frame_impl()
{
	[metal_renderer.command_buffer presentDrawable:metal_renderer.drawable];
	[metal_renderer.command_buffer commit];

    return true;
}

bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data)
{
    return dm_metal_create_buffer(buffer, data, metal_renderer);
}

extern void dm_renderer_delete_buffer_impl(dm_buffer* buffer)
{
	dm_internal_buffer* internal_buffer = buffer->internal_buffer;
	[internal_buffer->buffer release];
}

/***********
RENDER PASS
*************/

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass)
{
    render_pass->internal_pass = [[dm_metal_render_pass alloc] initWithRenderer: metal_renderer andPass: render_pass];
		
    if(!render_pass->internal_pass) return false;

	return true;
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
	dm_metal_render_pass* internal_pass = render_pass->internal_pass;
	dm_metal_shader_library* internal_shader = render_pass->shader.internal_shader;

	[internal_shader release];
	[internal_pass.uniform_buffer release];
	[internal_pass release];
}

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
	if(!metal_renderer.drawable)
	{
		metal_renderer.drawable = [metal_renderer.view.metal_layer nextDrawable];
	}

	if(metal_renderer.drawable)
	{
    	dm_metal_render_pass* internal_pass = render_pass->internal_pass;

		return [internal_pass beginWithRenderer:metal_renderer];
	}

	return true;
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
	if(metal_renderer.drawable)
	{
		dm_metal_render_pass* internal_pass = render_pass->internal_pass;

		[internal_pass endWithRenderer:metal_renderer];
	}
}

/***************
RENDER COMMANDS
*****************/
void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline)
{
	if(metal_renderer.drawable)
	{
    	MTLViewport new_viewport;
    	new_viewport.originX = viewport.x;
    	new_viewport.originY = viewport.y;
    	new_viewport.width = viewport.width;
    	new_viewport.height = viewport.height;
    	new_viewport.znear = viewport.min_depth;
    	new_viewport.zfar = viewport.max_depth;

    	[metal_renderer.command_encoder setViewport:new_viewport];
	}
}

void dm_renderer_clear_impl(dm_color clear_color)
{
	metal_renderer.clear_color = clear_color;
}

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count)
{

}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
	if(metal_renderer.drawable)
	{
		dm_internal_buffer* index_buffer = render_pass->index_buffer->internal_buffer;

    	[metal_renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:num_indices indexType:MTLIndexTypeUInt32 indexBuffer:index_buffer->buffer indexBufferOffset: index_offset];
	}
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
	if(metal_renderer.drawable)
    {
        [metal_renderer.command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount: num_indices indexType: MTLIndexTypeUInt32 indexBuffer: metal_renderer.index_buffer indexBufferOffset: index_offset instanceCount: num_insts];
    }
}

bool dm_renderer_update_buffer_impl(dm_buffer* cb, void* data, size_t data_size)
{
    dm_internal_buffer* internal_buffer = cb->internal_buffer;

    dm_memcpy([internal_buffer->buffer contents], data, dm_metal_align(data_size, DM_METAL_BUFFER_ALIGNMENT));

    return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot)
{
	if(metal_renderer.drawable)
	{
		dm_internal_buffer* internal_buffer = buffer->internal_buffer;
		[metal_renderer.command_encoder setVertexBuffer:internal_buffer->buffer offset:0 atIndex:slot];
	}
    return true;
}

bool dm_create_texture_impl(dm_image* image)
{
	return true;
}

void dm_destroy_texture_impl(dm_image* image)
{
	
}

bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot)
{
	//[metal_renderer->command_encoder setFragmentTexture:
	return true;
}

bool dm_renderer_bind_uniform_impl(dm_uniform* uniform)
{
	return true;
}

#endif