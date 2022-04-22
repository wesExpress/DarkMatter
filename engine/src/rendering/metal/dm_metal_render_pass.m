#include "dm_metal_render_pass.h"

#ifdef DM_METAL

#include "core/dm_logger.h"
#include "dm_metal_shader.h"

@implementation dm_metal_render_pass

- (id) initWithRenderer:(dm_metal_renderer*)renderer andPass:(dm_render_pass*)pass
{
    self = [super init];

    if(self)
    {
        // shader
        NSString* shader_file = [NSString stringWithUTF8String: pass->name];
        pass->shader.internal_shader = [[dm_metal_shader_library alloc] createShader: &pass->shader withPath: shader_file andRenderer: renderer];
        if(!pass->shader.internal_shader) return NULL;
        [shader_file release];
    }

    return self;
}

@end

#endif