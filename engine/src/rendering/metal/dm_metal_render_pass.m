#include "dm_metal_render_pass.h"

#ifdef DM_METAL

#include "core/dm_logger.h"
#include "dm_metal_shader.h"
#include <stdio.h>

@implementation dm_metal_render_pass

- (id) initWithRenderer: (dm_metal_renderer*)renderer AndPass:(dm_render_pass*)pass
{
    self = [super init];

    if(self)
    {
        // sampler
        MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
        sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
        sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;
        sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
        sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;

        _sampler_state = [renderer.device newSamplerStateWithDescriptor:sampler_desc];
        if(!_sampler_state)
        {
            DM_LOG_FATAL("Could not create metal sampler state!");
            return NULL;
        }

        // shader
        char buffer[512];
        snprintf(buffer, sizeof buffer, "shaders/metal/%s.metallib", pass->shader->name);
        NSString* shader_file = [NSString stringWithUTF8String: buffer];
        pass->shader->internal_shader = [[dm_metal_shader_library alloc] initWithShader: pass->shader AndPath: shader_file AndRenderer: renderer];
        if(!pass->shader->internal_shader) return NULL;

        dm_metal_shader_library* shader_lib = pass->shader->internal_shader;

        // pipeline state
        NSString* vertex_name = [NSString stringWithUTF8String: pass->shader->vertex_desc.path];
        NSString* frag_name = [NSString stringWithUTF8String: pass->shader->pixel_desc.path];

        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
        pipe_desc.colorAttachments[0].pixelFormat = renderer.view.metal_layer.pixelFormat;
        pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        pipe_desc.vertexFunction = [shader_lib.library newFunctionWithName: vertex_name];
        pipe_desc.fragmentFunction = [shader_lib.library newFunctionWithName: frag_name];

        _pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!_pipeline_state)
        {
            DM_LOG_FATAL("Could not create metal pipeline state");
            return NULL;
        }
    }

    return self;
}

@end

#endif