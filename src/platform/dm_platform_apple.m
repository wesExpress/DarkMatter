#include "dm_platform.h"

#ifdef DM_PLATFORM_APPLE

#include "dm_assert.h"
#include "dm_logger.h"
#include "dm_mem.h"
#include "dm_event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Cocoa/Cocoa.h>

@interface dm_window_delegate : NSObject <NSWindowDelegate>
@end

// handles window events and management
// bound to window with setDelegate message
@implementation dm_window_delegate

- (bool)windowShouldClose:(NSNotification*)notification
{
    dm_event_dispatch((dm_event) { DM_WINDOW_CLOSE_EVENT, NULL });
    return YES;
}

- (void)windowDidResize: (NSNotification*)notification
{

}

@end

typedef struct dm_internal_data
{
    NSWindow* window;
    dm_window_delegate* window_delegate;
    NSView* view;
    bool should_close;
} dm_internal_data;

bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title, int start_x, int start_y)
{
    e_data->platform_data = (dm_platform_data*)dm_alloc(sizeof(dm_platform_data), DM_MEM_PLATFORM);
    e_data->platform_data->window_width = window_width;
    e_data->platform_data->window_height = window_height;
    e_data->platform_data->window_title = window_title;
    
    e_data->platform_data->internal_data = (dm_internal_data*)dm_alloc(sizeof(dm_internal_data), DM_MEM_PLATFORM);
    dm_internal_data* internal_data = (dm_internal_data*)e_data->platform_data->internal_data;

    @autoreleasepool
    {
        // main app
        [NSApplication sharedApplication];

        // window delegate
        internal_data->window_delegate = [[dm_window_delegate alloc] init];

        // window creation
        internal_data->window = [[NSWindow alloc] 
            initWithContentRect: NSMakeRect(start_x, start_y, window_width, window_height)
            styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            backing: NSBackingStoreBuffered
            defer: NO
        ];
        [internal_data->window setTitle: @(window_title)];
        [internal_data->window setAcceptsMouseMovedEvents: YES];
        [internal_data->window setDelegate: internal_data->window_delegate];

        // last housekeeping
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        [NSApp activateIgnoringOtherApps:YES];
        [internal_data->window makeKeyAndOrderFront:nil];
    }
    return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
    DM_LOG_WARN("Platform shutdown called...");

    dm_internal_data* internal_data = (dm_internal_data*)e_data->platform_data->internal_data;

    dm_free(e_data->platform_data->internal_data, sizeof(dm_internal_data), DM_MEM_PLATFORM);
    dm_free(e_data->platform_data, sizeof(dm_platform_data), DM_MEM_PLATFORM);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
    @autoreleasepool
    {
        NSEvent* event;

        while((event = [NSApp 
            nextEventMatchingMask: NSEventMaskAny
            untilDate: [NSDate distantPast]
            inMode: NSDefaultRunLoopMode
            dequeue: YES
        ]))
        {
            [NSApp sendEvent: event];
        }
    }
    return true;
}

void* dm_platform_alloc(size_t size)
{
    void* temp = malloc(size);
	DM_ASSERT_MSG(temp, "Malloc returned null pointer!");
	if (!temp) return NULL;
	dm_platform_memzero(temp, size);
	return temp;
}

void* dm_platform_realloc(void* block, size_t size)
{
    void* temp = realloc(block, size);
	DM_ASSERT_MSG(temp, "Realloc returned null pointer!");
	if (temp) block = temp;
	else DM_LOG_FATAL("Realloc returned NULL ptr!");
	return block;
}

void dm_platform_free(void* block)
{
    free(block);
}

void* dm_platform_memzero(void* block, size_t size)
{
    return memset(block, 0, size);
}

void* dm_platform_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest, src, size);
}

void* dm_platform_memset(void* dest, int value, size_t size)
{
    return memset(dest, value, size);
}

void dm_platform_memmove(void* dest, const void* src, size_t size)
{
    memmove(dest, src, size);
}

void dm_platform_write(const char* message, uint8_t color)
{
    static char* levels[6] = {
        "1;30",   // white
        "1;34",   // blue
        "1;32",   // green
        "1;33",   // yellow
        "1;31",   // red
        "0;41"    // highlighted red
    };

    char out[5000];
    sprintf(
        out,
        "\033[%sm%s \033[0m",
        levels[color], message
    );

    printf("%s", out);
}

void dm_platform_write_error(const char* message, uint8_t color)
{
    static char* levels[6] = {
        "1;30",   // white
        "1;34",   // blue
        "1;32",   // green
        "1;33",   // yellow
        "1;31",   // red
        "0;41"    // highlighted red
    };

    char out[5000];
    sprintf(
        out,
        "\033[%sm%s \033[0m",
        levels[color], message
    );

    fprintf(stderr, "%s", out);
}

void dm_platform_swap_buffers()
{

}

#endif