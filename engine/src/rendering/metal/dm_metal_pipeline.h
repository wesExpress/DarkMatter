#ifndef __DM_METAL_PIPELINE__
#define __DM_METAL_PIPELINE__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"

@interface dm_metal_pipeline : NSObject

@property (strong, nonatomic) id<MTLDepthStencilState> depth_stencil;

- (id) initWithRenderer:(dm_metal_renderer*)renderer AndPipeline:(dm_render_pipeline*)pipeline;

- (BOOL) bind:(dm_render_pipeline*)pipeline Renderer:(dm_metal_renderer*)renderer;
@end

#endif 

#endif