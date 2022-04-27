#ifndef __DM_METAL_RENDER_PASS_H__
#define __DM_METAL_RENDER_PASS_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"
#include "rendering/dm_render_types.h"

@interface dm_metal_render_pass : NSObject

@property (strong, nonatomic) id<MTLBuffer> uniform_buffer;
@property (strong, nonatomic) id<MTLRenderPipelineState> pipeline_state;

- (id) initWithRenderer:(dm_metal_renderer*)renderer andPass:(dm_render_pass*)pass;
- (BOOL) beginWithRenderer:(dm_metal_renderer*)renderer;
- (void) endWithRenderer:(dm_metal_renderer*)renderer;

@end

#endif

#endif