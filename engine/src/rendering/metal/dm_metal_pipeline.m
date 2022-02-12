#include "dm_metal_pipeline.h"

#ifdef DM_METAL

#include "core/dm_logger.h"

@implementation dm_metal_pipeline

- (id) initWithRenderer: (dm_metal_renderer*)renderer AndPipeline:(dm_render_pipeline*)pipeline
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
    }

    return self;
}

- (id) init
{
    return [self initWithRenderer: NULL AndPipeline: NULL];
}

- (BOOL) bind:(dm_render_pipeline*)pipeline Renderer:(dm_metal_renderer*)renderer
{
    [renderer.command_encoder setDepthStencilState:_depth_stencil];
    [renderer.command_encoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderer.command_encoder setCullMode:MTLCullModeBack];

    dm_internal_buffer* vertex_buffer = pipeline->vertex_buffer->internal_buffer;
    dm_internal_buffer* inst_buffer = pipeline->inst_buffer->internal_buffer;

    [renderer.command_encoder setVertexBuffer:vertex_buffer->buffer offset:0 atIndex:0];
    [renderer.command_encoder setVertexBuffer:inst_buffer->buffer offset:0 atIndex:1];

    return YES;
}

@end

#endif