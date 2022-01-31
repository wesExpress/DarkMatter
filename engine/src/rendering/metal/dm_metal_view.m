#include "dm_metal_view.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"
#include "core/dm_logger.h"

@interface dm_metal_view ()
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
@property (nonatomic, strong) id<MTLRenderPipelineState> metal_pipeline;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation dm_metal_view

+ (id) layerClass
{
    return [CAMetalLayer class];
}

- (id) init
{
    self = [super init];

    if(self)
    {
        self.wantsLayer = true;
        self.metal_layer = [CAMetalLayer layer];
        
        self.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }
    
    return self;
}

@end

#endif