#include "dm_metal_view.h"

#ifdef DM_METAL

#include "core/dm_logger.h"

@implementation dm_metal_view

+ (id) layerClass
{
    return [CAMetalLayer class];
}

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    
    if(self)
    {
        self.wantsLayer = true;

        self.layer = [CAMetalLayer layer];
        _device = MTLCreateSystemDefaultDevice();

        CAMetalLayer* metal_layer = (CAMetalLayer*)self.layer;
        metal_layer.device = _device;
        metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }

    return self;
}

@end

#endif