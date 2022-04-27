#include "dm_metal_render_pass.h"

#ifdef DM_METAL

#include "dm_metal_shader.h"
#include "core/dm_logger.h"
#include "structures/dm_map.h"

@implementation dm_metal_render_pass

- (id) initWithRenderer:(dm_metal_renderer*)renderer andPass:(dm_render_pass*)pass
{
    self = [super init];

    if(self)
    {
        // shader
		char file_buffer[256];
		sprintf(file_buffer, "shaders/metal/%s.metallib", pass->shader.pass);

        NSString* shader_file = [NSString stringWithUTF8String: file_buffer];
        pass->shader.internal_shader = [[dm_metal_shader_library alloc] createShader: &pass->shader withPath: shader_file andRenderer: renderer];
        if(!pass->shader.internal_shader) return NULL;
        [shader_file release];

		// pipeline state
		dm_metal_shader_library* shader_lib = pass->shader.internal_shader;

        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
		pipe_desc.vertexFunction = shader_lib.vertex_func;
		pipe_desc.fragmentFunction = shader_lib.fragment_func;

        pipe_desc.colorAttachments[0].pixelFormat = renderer.view.metal_layer.pixelFormat;
        pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        _pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!_pipeline_state)
        {
            DM_LOG_FATAL("Could not create metal pipeline state");
            return NULL;
        }
    
		// uniform buffer
		size_t buffer_size = 0;
		dm_for_map_item(pass->uniforms)
		{
			dm_uniform* uniform = item->value;
			buffer_size += uniform->data_size;
		}
		_uniform_buffer = [renderer.device newBufferWithLength:buffer_size options:MTLResourceOptionCPUCacheModeDefault];
	}

    return self;
}

- (BOOL) beginWithRenderer:(dm_metal_renderer*)renderer
{
	[renderer.command_encoder setRenderPipelineState:_pipeline_state];
	[renderer.command_encoder setDepthStencilState:renderer.depth_stencil];

    id<MTLTexture> texture = renderer.drawable.texture;

    // render pass descriptor
    MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
    passDescriptor.colorAttachments[0].texture = texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(renderer.clear_color.x, renderer.clear_color.y, renderer.clear_color.z, renderer.clear_color.w);

    renderer.command_encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];

	[renderer.command_encoder setDepthStencilState:renderer.depth_stencil];

	[renderer.command_encoder setFrontFacingWinding:MTLWindingCounterClockwise]; 
    [renderer.command_encoder setCullMode:MTLCullModeBack];

	[renderer.command_encoder setFragmentSamplerState:renderer.sampler_state atIndex:0];

	return YES;
}

- (void) endWithRenderer:(dm_metal_renderer*)renderer
{
    [renderer.command_encoder endEncoding];
}

@end

#endif