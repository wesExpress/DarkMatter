#ifndef __DM_METAL_RENDER_PASS_H__
#define __DM_METAL_RENDER_PASS_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"

@interface dm_metal_render_pass : NSObject

@property (strong, nonatomic) id<MTLRenderPipelineState> pipeline_state;
@property (strong, nonatomic) id<CAMetalDrawable> drawable;
@property (strong, nonatomic) id<MTLSamplerState> sampler_state;

- (id)initWithRenderer:(dm_metal_renderer*)renderer AndPass:(dm_render_pass*)pass;

@end

#endif

#endif