#include "core/dm_defines.h"

#ifdef DM_PLATFORM_LINUX

#include "platform/dm_platform.h"

#include "core/dm_assert.h"
#include "core/dm_logger.h"
#include "core/dm_event.h"
#include "core/dm_mem.h"

#include "input/dm_input.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

#ifdef DM_OPENGL
#include <glad/glad.h>
#include <GL/glx.h>
typedef GLXContext (*glXCreateContextAttribARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
#endif

typedef struct dm_internal_linux_data
{
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} dm_internal_linux_data;

dm_internal_linux_data* linux_data = NULL;

dm_key_code translate_key_code(uint32_t x_keycode);

bool dm_platform_init(dm_platform_data* platform_data, const char* window_name)
{
    linux_data = dm_alloc(sizeof(dm_internal_linux_data), DM_MEM_PLATFORM);
    platform_data->internal_data = linux_data;

    linux_data->display = XOpenDisplay(NULL);

    //XAutoRepeatOff(linux_data->display);

    int screen_p = 0;
    linux_data->connection = XGetXCBConnection(linux_data->display);

    if(xcb_connection_has_error(linux_data->connection))
    {
        DM_LOG_FATAL("Failed to connect ot X server via XCB!");
        return false;
    }

    const struct xcb_setup_t* setup = xcb_get_setup(linux_data->connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    for(uint32_t s=screen_p;s>0;s--)
    {
        xcb_screen_next(&it);    
    }

    linux_data->screen = it.data;

    linux_data->window = xcb_generate_id(linux_data->connection);

    uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t event_values = FocusChangeMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
                            PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                            EnterWindowMask | LeaveWindowMask;

    uint32_t value_list[] = {linux_data->screen->black_pixel, event_values};

    xcb_void_cookie_t cookie = xcb_create_window(
        linux_data->connection,
        XCB_COPY_FROM_PARENT,
        linux_data->window,
        linux_data->screen->root,
        platform_data->x, platform_data->y,
        platform_data->window_width, platform_data->window_height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        linux_data->screen->root_visual,
        event_mask,
        value_list
    );

    xcb_change_property(
        linux_data->connection,
        XCB_PROP_MODE_REPLACE,
        linux_data->window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(window_name),
        window_name
    );

    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
        linux_data->connection,
        0,
        strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW"
    );
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        linux_data->connection,
        0,
        strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS"
    );
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(
        linux_data->connection,
        wm_delete_cookie,
        NULL
    );
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(
        linux_data->connection,
        wm_protocols_cookie,
        NULL
    );

    linux_data->wm_delete_win = wm_delete_reply->atom;
    linux_data->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(
        linux_data->connection,
        XCB_PROP_MODE_REPLACE,
        linux_data->window,
        wm_protocols_reply->atom,
        4, 32, 1,
        &wm_delete_reply->atom
    );

    xcb_map_window(linux_data->connection, linux_data->window);

    int stream_result = xcb_flush(linux_data->connection);
    if(stream_result<=0)
    {
        DM_LOG_FATAL("Error occured when flushing the screen: %d", stream_result);
        return false;
    }

    return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
    //XAutoRepeatOn(linux_data->display);
    xcb_destroy_window(linux_data->connection, linux_data->window);

    dm_free(linux_data, sizeof(dm_internal_linux_data), DM_MEM_PLATFORM);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    while((event=xcb_poll_for_event(linux_data->connection)))
    {
        switch(event->response_type & ~0x80)
        {
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t* kb_event = (xcb_key_press_event_t*)event;
            xcb_keycode_t code = kb_event->detail;

            KeySym key_sym = XkbKeycodeToKeysym(
                linux_data->display,
                (KeyCode)code,
                0,
                code & ShiftMask ? 1 : 0
            );

            dm_key_code key = translate_key_code(key_sym);
            dm_event_dispatch((dm_event) {DM_KEY_DOWN_EVENT, NULL, &key});

        } break;
        case XCB_KEY_RELEASE:
        {
            xcb_key_release_event_t* kb_event = (xcb_key_release_event_t*)event;
            xcb_keycode_t code = kb_event->detail;

            KeySym key_sym = XkbKeycodeToKeysym(
                linux_data->display,
                (KeyCode)code,
                0,
                code & ShiftMask ? 1 : 0
            );

            dm_key_code key = translate_key_code(key_sym);
            dm_event_dispatch((dm_event) {DM_KEY_UP_EVENT, NULL, &key});

        } break;
        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t* m_event = (xcb_button_press_event_t*)event;
            
            dm_mousebutton_code button = DM_MOUSEBUTTON_UNKNOWN;

            switch (m_event->detail)
            {
                case XCB_BUTTON_INDEX_1: 
                    button = DM_MOUSEBUTTON_L;
                    break;
                case XCB_BUTTON_INDEX_2:
                    button = DM_MOUSEBUTTON_M;
                    break;
                case XCB_BUTTON_INDEX_3:
                    button = DM_MOUSEBUTTON_R;
                    break;
            }

            if (button!=DM_MOUSEBUTTON_UNKNOWN) dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_DOWN_EVENT, NULL, &button });
        } break;
        case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t* m_event = (xcb_button_release_event_t*)event;
            
            dm_mousebutton_code button = DM_MOUSEBUTTON_UNKNOWN;

            switch (m_event->detail)
            {
                case XCB_BUTTON_INDEX_1: 
                    button = DM_MOUSEBUTTON_L;
                    break;
                case XCB_BUTTON_INDEX_2:
                    button = DM_MOUSEBUTTON_M;
                    break;
                case XCB_BUTTON_INDEX_3:
                    button = DM_MOUSEBUTTON_R;
                    break;
            }

            if (button!=DM_MOUSEBUTTON_UNKNOWN) dm_event_dispatch((dm_event){ DM_MOUSEBUTTON_UP_EVENT, NULL, &button });
        } break;
        case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t* m_event = (xcb_motion_notify_event_t*)event;
            int32_t coords[2] = { m_event->event_x, m_event->event_y };

            dm_event_dispatch((dm_event){ DM_MOUSE_MOVED_EVENT, NULL,  &coords});
        } break;
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t* c_event = (xcb_configure_notify_event_t*)event;

            uint32_t new_rect[2] = { c_event->width, c_event->height };

            dm_event_dispatch((dm_event){ DM_WINDOW_RESIZE_EVENT, NULL, &new_rect });
        } break;
        case XCB_CLIENT_MESSAGE:
        {
            cm = (xcb_client_message_event_t*)event;

            if(cm->data.data32[0]==linux_data->wm_delete_win) dm_event_dispatch((dm_event){ DM_WINDOW_CLOSE_EVENT, NULL, NULL });
        } break;

        default:
            break;
        }

        free(event);
    }

    return true;
}

void* dm_platform_alloc(size_t size)
{
    void* temp = malloc(size);
	DM_ASSERT_MSG(temp, "Malloc returned null pointer!");
	if (!temp) return NULL;
	dm_memzero(temp, size);
	return temp;
}

void* dm_platform_calloc(size_t count, size_t size)
{
    void* temp = calloc(count, size);
	DM_ASSERT_MSG(temp, "Calloc returned NULL pointer!");
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

float dm_platform_get_time()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec*0.000000001;
}

#ifdef DM_OPENGL
bool dm_platform_init_opengl()
{
    int visual_attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    int num_fbc = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(linux_data->display, DefaultScreen(linux_data->display), visual_attribs, &num_fbc);
    if(!fbc)
    {
        DM_LOG_FATAL("glxChooseFBConfig failed!");
        return false;
    }

    glXCreateContextAttribARBProc glxCreateContextAttribsARB = 0;
    glxCreateContextAttribsARB = (glXCreateContextAttribARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
    if(!glxCreateContextAttribsARB)
    {
        DM_LOG_FATAL("glxCreateContextAttribsARB failed!");
        return false;
    }

    int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, DM_OPENGL_MAJOR,
        GLX_CONTEXT_MINOR_VERSION_ARB, DM_OPENGL_MINOR,
        None
    };

    GLXContext context = glxCreateContextAttribsARB(
        linux_data->display,
        fbc[0],
        NULL,
        true,
        context_attribs
    );

    if(!context)
    {
        DM_LOG_FATAL("Failed to create OpenGL context!");
        return false;
    }

    glXMakeCurrent(linux_data->display, linux_data->window, context);

    if(!gladLoadGL())
    {
        DM_LOG_FATAL("Failed to initialize GLAD!");
        return false;
    }

    XFree(fbc);

    return true;
}

void dm_platform_shutdown_opengl()
{

}

void dm_platform_swap_buffers()
{
    glXSwapBuffers(
        linux_data->display,
        linux_data->window
    );
}
#endif

dm_key_code translate_key_code(uint32_t x_keycode)
{
    switch(x_keycode)
    {
        case XK_BackSpace: return DM_KEY_BACKSPACE;
        case XK_Return: return DM_KEY_ENTER;
        case XK_Tab: return DM_KEY_TAB;
        case XK_Escape: return DM_KEY_ESCAPE;
        case XK_space: return DM_KEY_SPACE;
        case XK_End: return DM_KEY_END;
        case XK_Home: return DM_KEY_HOME;
        case XK_Left: return DM_KEY_LEFT;
        case XK_Right: return DM_KEY_RIGHT;
        case XK_Up: return DM_KEY_UP;
        case XK_Down: return DM_KEY_DOWN;
        case XK_Print: return DM_KEY_PRINT;
        case XK_Insert: return DM_KEY_INSERT;
        case XK_Delete: return DM_KEY_DELETE;
        
        case XK_KP_0: return DM_KEY_NUMPAD_0;
        case XK_KP_1: return DM_KEY_NUMPAD_1;
        case XK_KP_2: return DM_KEY_NUMPAD_2;
        case XK_KP_3: return DM_KEY_NUMPAD_3;
        case XK_KP_4: return DM_KEY_NUMPAD_4;
        case XK_KP_5: return DM_KEY_NUMPAD_5;
        case XK_KP_6: return DM_KEY_NUMPAD_6;
        case XK_KP_7: return DM_KEY_NUMPAD_7;
        case XK_KP_8: return DM_KEY_NUMPAD_8;
        case XK_KP_9: return DM_KEY_NUMPAD_9;
        case XK_minus: return DM_KEY_MINUS;
        case XK_KP_Add: return DM_KEY_ADD;
        case XK_KP_Subtract: return DM_KEY_SUBTRACT;
        case XK_KP_Decimal: return DM_KEY_DECIMAL;
        case XK_KP_Divide: return DM_KEY_DIVIDE;
        case XK_KP_Multiply: return DM_KEY_MULTIPLY;
        
        case XK_F1: return DM_KEY_F1;
        case XK_F2: return DM_KEY_F1;
        case XK_F3: return DM_KEY_F1;
        case XK_F4: return DM_KEY_F1;
        case XK_F5: return DM_KEY_F1;
        case XK_F6: return DM_KEY_F1;
        case XK_F7: return DM_KEY_F1;
        case XK_F8: return DM_KEY_F1;
        case XK_F9: return DM_KEY_F1;
        case XK_F10: return DM_KEY_F1;
        case XK_F11: return DM_KEY_F1;
        case XK_F12: return DM_KEY_F1;

        case XK_Num_Lock: return DM_KEY_NUMLCK;
        case XK_Scroll_Lock: return DM_KEY_SCRLLCK;

        case XK_Shift_L: return DM_KEY_LSHIFT;
        case XK_Shift_R: return DM_KEY_RSHIFT;
        case XK_Control_L: return DM_KEY_LCTRL;
        case XK_Control_R: return DM_KEY_RCTRL;
        
        case XK_plus: return DM_KEY_PLUS;
        case XK_comma: return DM_KEY_COMMA;
        case XK_period: return DM_KEY_PERIOD;
        case XK_slash: return DM_KEY_LSLASH;

        case XK_a:
        case XK_A: return DM_KEY_A;
        case XK_b:
        case XK_B: return DM_KEY_B;
        case XK_c:
        case XK_C: return DM_KEY_C;
        case XK_d:
        case XK_D: return DM_KEY_D;
        case XK_e:
        case XK_E: return DM_KEY_E;
        case XK_f:
        case XK_F: return DM_KEY_F;
        case XK_g:
        case XK_G: return DM_KEY_G;
        case XK_h:
        case XK_H: return DM_KEY_H;
        case XK_i:
        case XK_I: return DM_KEY_I;
        case XK_j:
        case XK_J: return DM_KEY_J;
        case XK_k:
        case XK_K: return DM_KEY_K;
        case XK_l:
        case XK_L: return DM_KEY_L;
        case XK_m:
        case XK_M: return DM_KEY_M;
        case XK_n:
        case XK_N: return DM_KEY_N;
        case XK_o:
        case XK_O: return DM_KEY_O;
        case XK_p:
        case XK_P: return DM_KEY_P;
        case XK_q:
        case XK_Q: return DM_KEY_Q;
        case XK_r:
        case XK_R: return DM_KEY_R;
        case XK_s:
        case XK_S: return DM_KEY_S;
        case XK_t:
        case XK_T: return DM_KEY_T;
        case XK_u:
        case XK_U: return DM_KEY_U;
        case XK_v:
        case XK_V: return DM_KEY_V;
        case XK_w:
        case XK_W: return DM_KEY_W;
        case XK_x:
        case XK_X: return DM_KEY_X;
        case XK_y:
        case XK_Y: return DM_KEY_Y;
        case XK_z:
        case XK_Z: return DM_KEY_Z;

        default: return 0;
    }
}

#endif