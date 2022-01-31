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
{
    NSWindow* window;
}

- (id) initWithView: (dm_content_view*)view_in;

- (void) makeDevice;
- (void) makeBuffers;
- (void) makePipeline;
- (void) redrawWithColor:(dm_vec4)color;

@end

typedef struct demo_vertex
{
    dm_vec4 position;
    dm_vec4 color;
} demo_vertex;

#endif

#endif