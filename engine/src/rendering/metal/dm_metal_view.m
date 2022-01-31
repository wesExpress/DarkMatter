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

- (id) initWithWindow: (NSWindow*)window_in
{
    self = [super init];
    
    if(self)
    {
        window = window_in;

        self.wantsLayer = true;

        _metal_layer = [CAMetalLayer layer];
        _metal_device = MTLCreateSystemDefaultDevice();

        _metal_layer.device = _metal_device;
        _metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

        // must set the size of the layer manually
        NSRect frame = window.frame;
        [self.metal_layer setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height)];
    }

    return self;
}

@end

#endif