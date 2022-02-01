#include "dm_platform.h"

#ifdef DM_PLATFORM_APPLE

#include "dm_platform_apple.h"
#include "core/dm_assert.h"
#include "core/dm_logger.h"
#include "core/dm_mem.h"
#include "core/dm_event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// handles window events and management
// bound to window with setDelegate message
@implementation dm_window_delegate

- (bool)windowShouldClose:(NSNotification*)notification
{
    dm_event_dispatch((dm_event) { DM_WINDOW_CLOSE_EVENT, NULL, NULL });
    return YES;
}

- (NSSize)windowWillResize: (NSWindow*)window : (NSSize)frameSize
{
    uint32_t rect[2] = { frameSize.width, frameSize.height };
    dm_event_dispatch((dm_event){DM_WINDOW_RESIZE_EVENT, NULL, (void*)rect});

    return frameSize;
}

@end

@implementation dm_content_view

- (instancetype)initWithWindow: (NSWindow*)window_in
{
    self = [super init];

    window = window_in;

    return self;
}

- (NSRect) getWindowFrame
{
    return window.frame;
}

// needed but not sure why
- (BOOL)canBecomeKeyView { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)wantsUpdateLayer { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }

// input events
- (void) mouseDown: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_L;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
}

- (void) mouseUp: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_L;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
}

- (void) rightMouseDown: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_R;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
}

- (void) rightMouseUp: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_R;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
}

- (void) otherMouseDown: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_M;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
}

- (void) otherMouseUp: (NSEvent*) event
{
    dm_mousebutton_code button = DM_MOUSEBUTTON_M;
    dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
}

- (void) mouseMoved: (NSEvent*) event
{
    const NSPoint point = [event locationInWindow];
    uint32_t pos[2] = { point.x, point.y };
    dm_event_dispatch((dm_event){ DM_MOUSE_MOVED_EVENT, NULL, (void*)(intptr_t)pos });
}

- (void) keyDown: (NSEvent*) event
{
    dm_key_code key = dm_translate_key_code((uint32_t)[event keyCode]);
    dm_event_dispatch((dm_event){ DM_KEY_DOWN_EVENT, NULL, (void*)(intptr_t)key });
}

- (void) keyUp: (NSEvent*) event
{
    dm_key_code key = dm_translate_key_code((uint32_t)[event keyCode]);
    dm_event_dispatch((dm_event){ DM_KEY_UP_EVENT, NULL, (void*)(intptr_t)key});
}

- (void) scrollWheel: (NSEvent*) event
{
    dm_event_dispatch((dm_event){ DM_MOUSE_SCROLLED_EVENT, NULL, (void*)(intptr_t)(int8_t)[event scrollingDeltaY]});
}

// must be implemented for the protocol to shut up in the compiler
- (NSRange) markedRange { return (NSRange) { NSNotFound, 0 }; }
- (NSRange) selectedRange { return (NSRange) { NSNotFound, 0 }; }
- (BOOL) hasMarkedText { return FALSE; }
- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return nil;}
- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {return [NSArray array];}
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {return NSMakeRect(0, 0, 0, 0);}
- (NSUInteger)characterIndexForPoint:(NSPoint)point {return 0;}
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {}
- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {}
- (void)unmarkText {}

@end

@implementation dm_app_delegate

- (void) applicationDidFinishLaunching: (NSNotification*)notification
{
    @autoreleasepool
    {
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                            location:NSMakePoint(0, 0)
                            modifierFlags:0
                            timestamp:0
                            windowNumber:0
                            context:nil
                            subtype:0
                            data1:0
                            data2:0];
        [NSApp postEvent:event atStart:YES];
    }

    [NSApp stop: nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    return YES;
}

@end

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

        // app delegate
        internal_data->app_delegate = [[dm_app_delegate alloc] init];
        [NSApp setDelegate: internal_data->app_delegate];

        // window delegate
        internal_data->window_delegate = [[dm_window_delegate alloc] init];

        // window creation
        internal_data->window = [[NSWindow alloc] 
            initWithContentRect: NSMakeRect(start_x, start_y, window_width, window_height)
            styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            backing: NSBackingStoreBuffered
            defer: NO
        ];

        // input view
        internal_data->content_view = [[dm_content_view alloc] initWithWindow: internal_data->window];

        // metal view
        //internal_data->metal_view = [[dm_metal_view alloc] initWithFrame: CGRectMake(start_x, start_y, window_width, window_height)];

        // window memebers
        [internal_data->window setAcceptsMouseMovedEvents: YES];
        [internal_data->window setDelegate: internal_data->window_delegate];
        [internal_data->window setContentView: internal_data->content_view];
        [internal_data->window makeFirstResponder: internal_data->content_view];
        [internal_data->window setAcceptsMouseMovedEvents:YES];
        [internal_data->window setLevel:NSNormalWindowLevel];
        [internal_data->window setTitle: @(window_title)];

        if (![[NSRunningApplication currentApplication] isFinishedLaunching]) [NSApp run];

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

void* dm_platform_calloc(size_t count, size_t size)
{
    void* temp = calloc(count, size);
    DM_ASSERT_MSG(temp, "Calloc return null pointer!");
    if (!temp) return NULL;
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

float dm_platform_get_time()
{
    const uint64_t factor = 1000000;
    static mach_timebase_info_data_t s_timebase_info;
    static dispatch_once_t once_token;

    dispatch_once(&once_token, ^{
        (void) mach_timebase_info(&s_timebase_info);
    });

    float mili = (float)((mach_absolute_time() * s_timebase_info.numer) / (factor * s_timebase_info.denom));

    return mili / 1000.0f;
}

dm_key_code dm_translate_key_code(uint32_t cocoa_key)
{
    switch(cocoa_key)
    {
    case 0:  return DM_KEY_A;
    case 11: return DM_KEY_B;
    case 8:  return DM_KEY_C;
    case 2:  return DM_KEY_D;
    case 14: return DM_KEY_E;
    case 3:  return DM_KEY_F;
    case 5:  return DM_KEY_G;
    case 4:  return DM_KEY_H;
    case 34: return DM_KEY_I;
    case 38: return DM_KEY_J;
    case 40: return DM_KEY_K;
    case 37: return DM_KEY_L;
    case 46: return DM_KEY_M;
    case 45: return DM_KEY_N;
    case 31: return DM_KEY_O;
    case 35: return DM_KEY_P;
    case 12: return DM_KEY_Q;
    case 15: return DM_KEY_R;
    case 1:  return DM_KEY_S;
    case 17: return DM_KEY_T;
    case 32: return DM_KEY_U;
    case 9:  return DM_KEY_V;
    case 13: return DM_KEY_W;
    case 7:  return DM_KEY_X;
    case 16: return DM_KEY_Y;
    case 6:  return DM_KEY_Z;
    
    case 18: return DM_KEY_1;
    case 19: return DM_KEY_2;
    case 20: return DM_KEY_3;
    case 21: return DM_KEY_4;
    case 23: return DM_KEY_5;
    case 22: return DM_KEY_6;
    case 25: return DM_KEY_9;
    case 26: return DM_KEY_7;
    case 28: return DM_KEY_8;
    case 29: return DM_KEY_0;

    case 39: return DM_KEY_QUOTE;
    case 47: return DM_KEY_PERIOD;
    case 43: return DM_KEY_COMMA;
    case 42: return DM_KEY_RSLASH;
    case 44: return DM_KEY_LSLASH;
    case 36: return DM_KEY_ENTER;
    case 30: return DM_KEY_RBRACE;
    case 33: return DM_KEY_LBRACE;
    case 27: return DM_KEY_MINUS;
    case 24: return DM_KEY_EQUAL;
   
    case 49: return DM_KEY_SPACE;
    case 51: return DM_KEY_DELETE;
    case 53: return DM_KEY_ESCAPE;
    case 57: return DM_KEY_CAPSLOCK;
    case 48: return DM_KEY_TAB;
    case 56: return DM_KEY_LSHIFT;
    case 60: return DM_KEY_RSHIFT;
    case 59: return DM_KEY_LCTRL;
    case 62: return DM_KEY_RCTRL;
    
    case 65: return DM_KEY_DECIMAL;
    case 67: return DM_KEY_MULTIPLY;
    case 69: return DM_KEY_PLUS;
    case 75: return DM_KEY_DIVIDE;
    case 76: return DM_KEY_ENTER;
    case 78: return DM_KEY_MINUS;
    case 81: return DM_KEY_EQUAL;

    case 82: return DM_KEY_NUMPAD_0;
    case 83: return DM_KEY_NUMPAD_1;
    case 84: return DM_KEY_NUMPAD_2;
    case 85: return DM_KEY_NUMPAD_3;
    case 86: return DM_KEY_NUMPAD_4;
    case 87: return DM_KEY_NUMPAD_5;
    case 88: return DM_KEY_NUMPAD_6;
    case 89: return DM_KEY_NUMPAD_7;
    case 91: return DM_KEY_NUMPAD_8;
    case 92: return DM_KEY_NUMPAD_9;
    
    case 115: return DM_KEY_HOME;
    case 116: return DM_KEY_PAGEUP;
    case 121: return DM_KEY_PAGEDOWN;
    case 117: return DM_KEY_DELETE;
    case 119: return DM_KEY_END;
    
    case 122: return DM_KEY_F1;
    case 120: return DM_KEY_F2;
    case 99:  return DM_KEY_F3;
    case 118: return DM_KEY_F4;
    case 96:  return DM_KEY_F5;
    case 97:  return DM_KEY_F6;
    case 98:  return DM_KEY_F7;
    case 100: return DM_KEY_F8;
    case 101: return DM_KEY_F9;
    case 109: return DM_KEY_F10;
    case 103: return DM_KEY_F11;
    case 111: return DM_KEY_F12;
    case 105: return DM_KEY_F13;
    case 107: return DM_KEY_F14;
    case 113: return DM_KEY_F15;
    case 106: return DM_KEY_F16;
    case 64:  return DM_KEY_F17;
    case 79:  return DM_KEY_F18;
    case 80:  return DM_KEY_F19;
    case 90:  return DM_KEY_F20;

    case 123: return DM_KEY_LEFT;
    case 124: return DM_KEY_RIGHT;
    case 125: return DM_KEY_DOWN;
    case 126: return DM_KEY_UP;
    default:
        DM_LOG_ERROR("Unknown key code! Reeturning 'A'...");
        return DM_KEY_A;
    }
}

#endif