#ifndef __DM_METAL_RENDERER_H__
#define __DM_METAL_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "rendering/dm_renderer.h"
#include "core/dm_logger.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// metal view base class
@interface dm_metal_view : NSView <CALayerDelegate>
    @property (readonly) CAMetalLayer* metalLayer;
    @property (readonly) id<MTLDevice> metalDevice;
@end

// implementations
@implementation dm_metal_view
    - (instancetype) initWithCoder: (NSCoder*)aDecoder
    {
        if((self = [super initWithCoder: aDecoder]))
        {
            _metalLayer = (CAMetalLayer*)[self layer];
            _metalDevice = MTLCreateSystemDefaultDevice();
            _metalLayer.device = _metalDevice;
            _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        }

        return self;
    }

    - (void) didMoveToWindow
    {
        [self redraw];
    }

    - (void) redraw
    {
        id <CAMetalDrawable> drawable = [self.metalLayer nextDrawable];
        id <MTLTexture> texture = drawable.texture;

        MTLRenderPassDescriptor* pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        pass_descriptor.colorAttachments[0].texture = texture;
        pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 0, 0 ,1);
    }

    + (id) layerClass
    {
        return [CAMetalLayer class];
    }

    -(CAMetalLayer*) metal_layer
    {
        return (CAMetalLayer*)self.layer;
    }
@end

#endif

#endif