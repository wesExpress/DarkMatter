#include "dm_metal_render_pass.h"

#ifdef DM_METAL

#include "core/dm_logger.h"
#include "dm_metal_shader.h"

@implementation dm_metal_render_pass

- (id) initWithRendererAndPass: (dm_metal_renderer*)renderer :(dm_render_pass*)pass
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
        //NSString* shader_file = [NSString stringWithUTF8String: pass->shader->file_name];
        //pass->shader->internal_shader = [[dm_metal_shader_library alloc] create: pass->shader path: shader_file renderer: renderer];
        //if(!pass->shader->internal_shader) return NULL;
        //[shader_file release];
    }

    return self;
}

@end

#endif