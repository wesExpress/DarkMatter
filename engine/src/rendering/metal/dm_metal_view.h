#ifndef __DM_METAL_VIEW_H__
#define __DM_METAL_VIEW_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface dm_metal_view : NSView
    @property (readonly) id<MTLDevice> metal_device;
    @property (readonly) CAMetalLayer* metal_layer;
@end

#endif

#endif