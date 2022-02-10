#ifndef __DM_METAL_VIEW_H__
#define __DM_METAL_VIEW_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "core/math/dm_math_types.h"
#include "platform/dm_platform_apple.h"

@interface dm_metal_view : NSView

@property (nonatomic, strong) CAMetalLayer* metal_layer;

- (id) init;

@end

#endif

#endif