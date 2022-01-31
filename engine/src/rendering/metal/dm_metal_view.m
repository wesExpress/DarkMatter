#include "dm_metal_view.h"

#ifdef DM_METAL

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

        _metal_layer = [CAMetalLayer layer];
        _metal_device = MTLCreateSystemDefaultDevice();

        _metal_layer.device = _metal_device;
        _metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }

    return self;
}

@end

#endif