#include "dm.h"
#include "dm_platform_mac.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dm_key_code dm_translate_key_code(uint32_t cocoa_key);

extern void dm_add_window_close_event(dm_event_list* event_list);
extern void dm_add_window_resize_event(uint32_t new_widt, uint32_t new_height, dm_event_list* event_list);
extern void dm_add_mousebutton_down_event(dm_mousebutton_code button, dm_event_list* event_list);
extern void dm_add_mousebutton_up_event(dm_mousebutton_code button, dm_event_list* event_list);
extern void dm_add_mouse_move_event(uint32_t mouse_x, uint32_t mouse_y, dm_event_list* event_list);
extern void dm_add_mouse_scroll_event(uint32_t delta, dm_event_list* event_list);
extern void dm_add_key_down_event(dm_key_code key, dm_event_list* event_list);
extern void dm_add_key_up_event(dm_key_code key, dm_event_list* event_list);

// handles window events and management
// bound to window with setDelegate message
@implementation dm_window_delegate

- (instancetype)initWithPlatformData : (dm_platform_data*)platform_data_in
{
	self = [super init];

	platform_data = platform_data_in;

	return self;
}

- (BOOL)windowShouldClose:(NSNotification*)notification
{
    dm_add_window_close_event(&platform_data->event_list);
	return YES;
}

- (NSSize)windowWillResize: (NSWindow*)window : (NSSize)frameSize
{
	dm_add_window_resize_event(frameSize.width, frameSize.height, &platform_data->event_list);

    return frameSize;
}

@end

@implementation dm_content_view

- (instancetype)initWithWindow: (NSWindow*)window_in AndPlatformData: (dm_platform_data*)platform_data_in
{
    self = [super init];

    window = window_in;
	platform_data = platform_data_in;

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
	dm_add_mousebutton_down_event(DM_MOUSEBUTTON_L, &platform_data->event_list);
}

- (void) mouseUp: (NSEvent*) event
{
	dm_add_mousebutton_up_event(DM_MOUSEBUTTON_L, &platform_data->event_list);
}

- (void) rightMouseDown: (NSEvent*) event
{
	dm_add_mousebutton_down_event(DM_MOUSEBUTTON_R, &platform_data->event_list);
}

- (void) rightMouseUp: (NSEvent*) event
{
	dm_add_mousebutton_up_event(DM_MOUSEBUTTON_R, &platform_data->event_list);
}

- (void) otherMouseDown: (NSEvent*) event
{
	dm_add_mousebutton_down_event(DM_MOUSEBUTTON_M, &platform_data->event_list);
}

- (void) otherMouseUp: (NSEvent*) event
{
	dm_add_mousebutton_up_event(DM_MOUSEBUTTON_M, &platform_data->event_list);
}

- (void) mouseMoved: (NSEvent*) event
{
    const NSPoint point = [event locationInWindow];
	dm_add_mouse_move_event(point.x, point.y, &platform_data->event_list);
}

- (void) keyDown: (NSEvent*) event
{
	dm_key_code key = dm_translate_key_code((uint32_t)[event keyCode]);
	dm_add_key_down_event(key, &platform_data->event_list);
}

- (void) keyUp: (NSEvent*) event
{
    dm_key_code key = dm_translate_key_code((uint32_t)[event keyCode]);
	dm_add_key_up_event(key, &platform_data->event_list);
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

bool dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, uint32_t window_w, uint32_t window_h, const char* window_title, dm_platform_data* platform_data)
{
	platform_data->internal_data = dm_alloc(sizeof(dm_internal_apple_data));
	dm_internal_apple_data* apple_data = platform_data->internal_data;

	// app delegate
	[NSApplication sharedApplication];
	apple_data->app_delegate = [[dm_app_delegate alloc] init];
	[NSApp setDelegate: apple_data->app_delegate];

	// window delegate
	apple_data->window_delegate = [[dm_window_delegate alloc] initWithPlatformData: platform_data];

	// window creation
	apple_data->window = [[NSWindow alloc] 
		initWithContentRect: NSMakeRect(window_x_pos, window_y_pos, window_w, window_h) 
		styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing: NSBackingStoreBuffered
		defer: NO
	];

	// input view
	apple_data->content_view = [[dm_content_view alloc] initWithWindow: apple_data->window AndPlatformData: platform_data];

	// window memebers
	[apple_data->window setAcceptsMouseMovedEvents: YES];
	[apple_data->window setDelegate: apple_data->window_delegate];
	[apple_data->window setContentView: apple_data->content_view];
	[apple_data->window makeFirstResponder: apple_data->content_view];
	[apple_data->window setAcceptsMouseMovedEvents:YES];
	[apple_data->window setLevel:NSNormalWindowLevel];
	[apple_data->window setTitle: @(window_title)];

	if (![[NSRunningApplication currentApplication] isFinishedLaunching]) [NSApp run];

	// last housekeeping
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	[NSApp activateIgnoringOtherApps:YES];
	[apple_data->window makeKeyAndOrderFront:nil];

	return true;
}

void dm_platform_shutdown(dm_platform_data* platform_data)
{
	dm_internal_apple_data* apple_data = platform_data->internal_data;

	[apple_data->app_delegate release];
	[apple_data->window release];
	[apple_data->content_view release];
	[apple_data->window_delegate release];

	dm_free(platform_data->internal_data);
}

bool dm_platform_pump_events(dm_platform_data* platform_data)
{
	NSEvent* event;

	while((event = [NSApp 
        			nextEventMatchingMask: NSEventMaskAny
            		untilDate: [NSDate distantPast]
            		inMode: NSDefaultRunLoopMode
            		dequeue: YES]))
	{
		[NSApp sendEvent: event];
	}
	
    return true;
}

double dm_platform_get_time(dm_platform_data* platform_data)
{
	const uint64_t factor = 1000000;
    static mach_timebase_info_data_t s_timebase_info;
    static dispatch_once_t once_token;

    dispatch_once(&once_token, ^{
        (void) mach_timebase_info(&s_timebase_info);
    });

    float mili = (float)((mach_absolute_time() * s_timebase_info.numer) / (factor * s_timebase_info.denom));

    return mili / 1000.0;
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