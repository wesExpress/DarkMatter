#ifndef DM_PLATFORM_MAC_H
#define DM_PLATFORM_MAC_H

#import <Cocoa/Cocoa.h>

// window delegate
@interface dm_window_delegate : NSObject <NSWindowDelegate>
{
	dm_platform_data* platform_data;
}

- (instancetype)initWithPlatformData : (dm_platform_data*)platform_data_in;
@end

// for mouse and keyboard input
@interface dm_content_view : NSView <NSTextInputClient>
{
    NSWindow*         window;
	dm_platform_data* platform_data;
}

- (instancetype)initWithWindow: (NSWindow*)window_in AndPlatformData:(dm_platform_data*)platform_data_in;
- (NSRect)getWindowFrame;
@end

@interface dm_app_delegate : NSObject<NSApplicationDelegate>
@end

typedef struct dm_internal_apple_data_t
{
    NSWindow*           window;
    dm_app_delegate*    app_delegate;
    dm_window_delegate* window_delegate;
    dm_content_view*    content_view;
} dm_internal_apple_data;

#endif //DM_PLATFORM_MAC_H
