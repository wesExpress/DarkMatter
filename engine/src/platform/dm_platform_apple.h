#ifndef __DM_PLATFORM_APPLE_H__
#define __DM_PLATFORM_APPLE_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "input/dm_input.h"
#include "rendering/metal/dm_metal_view.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <mach/mach_time.h>

dm_key_code dm_translate_key_code(uint32_t cocoa_key);

// window delegate
@interface dm_window_delegate : NSObject <NSWindowDelegate>
@end

// for mouse and keyboard input
@interface dm_content_view : NSView <NSTextInputClient>
{
    NSWindow* window;
}

@property (readonly) id<MTLDevice> device;
@property (readonly) CAMetalLayer* metal_layer;

- (instancetype)initWithWindow: (NSWindow*)window_in;
- (BOOL) initMetalDevice;
@end

@interface dm_app_delegate : NSObject<NSApplicationDelegate>
@end

typedef struct dm_internal_data
{
    NSWindow* window;
    dm_app_delegate* app_delegate;
    dm_window_delegate* window_delegate;
    dm_content_view* content_view;
    dm_metal_view* metal_view;
} dm_internal_data;

#endif

#endif