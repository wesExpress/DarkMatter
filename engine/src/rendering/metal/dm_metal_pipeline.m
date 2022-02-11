#include "dm_metal_pipeline.h"

#ifdef DM_METAL

#include "core/dm_logger.h"

@implementation dm_metal_pipeline

- (id) initWithRenderer: (dm_metal_renderer*)renderer :(dm_render_pipeline*)pipeline
{
    self = [super init];

    if(self)
    {
        // depth stencil
        MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
        depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
        depth_stencil_desc.depthWriteEnabled = YES;
        _depth_stencil = [renderer.device newDepthStencilStateWithDescriptor:depth_stencil_desc];
        if(!_depth_stencil)
        {
            DM_LOG_FATAL("Could not create metal depth stencil state!");
            return NULL;
        }

        // pipeline state
        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
        pipe_desc.colorAttachments[0].pixelFormat = renderer.view.metal_layer.pixelFormat;
        pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        _pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!_pipeline_state)
        {
            DM_LOG_FATAL("Could not create metal pipeline state");
            return NULL;
        }
    }

    return self;
}

- (id) init
{
    return [self initWithRenderer: NULL pipeline: NULL];
}

@end

#endif