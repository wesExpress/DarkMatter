#include "dm_metal_view.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"
#include "core/dm_logger.h"

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