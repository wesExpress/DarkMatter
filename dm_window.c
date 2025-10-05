#include "dm.h"

#define RGFW_IMPLEMENTATION
#include "RGFW/RGFW.h"

typedef struct dm_keyboard_t
{
    bool keys[256];
} dm_keyboard;

typedef struct dm_mouse_t
{
    bool buttons[3];
    uint16_t x,y;
    float scroll;
} dm_mouse;

typedef struct dm_input_state_t
{
    dm_keyboard keyboard;
    dm_mouse    mouse;
} dm_input_state;

// === window ===
typedef enum dm_window_flag_t
{
    DM_WINDOW_FLAG_NONE   = 0,
    DM_WINDOW_FLAG_CLOSE  = 1,
    DM_WINDOW_FLAG_RESIZE = 2,
} dm_window_flag;

typedef struct dm_window_t 
{
    dm_window_flag flags;

    dm_input_state current_input;
    dm_input_state previous_input;

    RGFW_window* window;
} dm_window; 

bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_context* context)
{
    context->window = dm_alloc(sizeof(dm_window));
    dm_window* window = context->window;

    RGFW_windowFlags window_flags = 0;
    if(flags & DM_WINDOW_CREATE_FLAG_CENTER)    window_flags |= RGFW_windowCenter;
    if(flags & DM_WINDOW_CREATE_FLAG_NO_RESIZE) window_flags |= RGFW_windowNoResize;

    window->window = RGFW_createWindow(title, x,y,width,height, window_flags);
    if(!window->window) { return false; }

    return true;
}

void dm_window_shutdown(dm_window* window)
{
    RGFW_window_close(window->window);
}

bool dm_window_should_close(dm_window window)
{
    return window.flags & DM_WINDOW_FLAG_CLOSE;
}

bool dm_window_resized(dm_window window)
{
    return window.flags & DM_WINDOW_FLAG_RESIZE;
}

uint32_t dm_get_window_width(dm_context* context)
{
    dm_window* window = context->window;
    return window->window->w;
}

uint32_t dm_get_window_height(dm_context* context)
{
    dm_window* window = context->window;
    return window->window->h;
}
#ifndef DM_DEBUG
DM_INLINE
#endif
dm_key_code dm_convert_key_code(RGFW_key key)
{
    switch(key)
    {
        case RGFW_F1:  return DM_KEY_F1;
        case RGFW_F2:  return DM_KEY_F2;
        case RGFW_F3:  return DM_KEY_F3;
        case RGFW_F4:  return DM_KEY_F4;
        case RGFW_F5:  return DM_KEY_F5;
        case RGFW_F6:  return DM_KEY_F6;
        case RGFW_F7:  return DM_KEY_F7;
        case RGFW_F8:  return DM_KEY_F8;
        case RGFW_F9:  return DM_KEY_F9;
        case RGFW_F10: return DM_KEY_F10;
        case RGFW_F11: return DM_KEY_F11;
        case RGFW_F12: return DM_KEY_F12;
        case RGFW_F13: return DM_KEY_F13;
        case RGFW_F14: return DM_KEY_F14;
        case RGFW_F15: return DM_KEY_F15;
        case RGFW_F16: return DM_KEY_F16;

        case RGFW_a: return DM_KEY_A;
        case RGFW_b: return DM_KEY_B;
        case RGFW_c: return DM_KEY_C;
        case RGFW_d: return DM_KEY_D;
        case RGFW_e: return DM_KEY_E;
        case RGFW_f: return DM_KEY_F;
        case RGFW_h: return DM_KEY_H;
        case RGFW_i: return DM_KEY_I;
        case RGFW_j: return DM_KEY_J;
        case RGFW_k: return DM_KEY_K;
        case RGFW_l: return DM_KEY_L;
        case RGFW_m: return DM_KEY_M;
        case RGFW_n: return DM_KEY_N;
        case RGFW_o: return DM_KEY_O;
        case RGFW_p: return DM_KEY_P;
        case RGFW_q: return DM_KEY_Q;
        case RGFW_r: return DM_KEY_R;
        case RGFW_s: return DM_KEY_S;
        case RGFW_t: return DM_KEY_T;
        case RGFW_u: return DM_KEY_U;
        case RGFW_v: return DM_KEY_V;
        case RGFW_w: return DM_KEY_W;
        case RGFW_x: return DM_KEY_X;
        case RGFW_y: return DM_KEY_Y;
        case RGFW_z: return DM_KEY_Z;

        case RGFW_left:  return DM_KEY_LEFT;
        case RGFW_right: return DM_KEY_RIGHT;
        case RGFW_down:  return DM_KEY_DOWN;
        case RGFW_up:    return DM_KEY_UP;

        case RGFW_home:        return DM_KEY_HOME;
        case RGFW_printScreen: return DM_KEY_PRINT;
        case RGFW_pageUp:      return DM_KEY_PAGEUP;
        case RGFW_pageDown:    return DM_KEY_PAGEDOWN;
        case RGFW_numLock:     return DM_KEY_NUMLCK;

        case RGFW_tab:      return DM_KEY_TAB;
        case RGFW_capsLock: return DM_KEY_CAPSLOCK;
        case RGFW_escape:   return DM_KEY_ESCAPE;

        case RGFW_1: return DM_KEY_1;
        case RGFW_2: return DM_KEY_2;
        case RGFW_3: return DM_KEY_3;
        case RGFW_4: return DM_KEY_4;
        case RGFW_5: return DM_KEY_5;
        case RGFW_6: return DM_KEY_6;
        case RGFW_7: return DM_KEY_7;
        case RGFW_8: return DM_KEY_8;
        case RGFW_9: return DM_KEY_9;
        case RGFW_0: return DM_KEY_0;

        case RGFW_minus:  return DM_KEY_MINUS;
        case RGFW_kpPlus: return DM_KEY_PLUS;

        case RGFW_controlR:
        case RGFW_controlL: return DM_KEY_CTRL;
        case RGFW_altR:     return DM_KEY_RALT;
        case RGFW_altL:     return DM_KEY_LALT;
        case RGFW_shiftL:   return DM_KEY_LSHIFT;
        case RGFW_shiftR:   return DM_KEY_RSHIFT;
        case RGFW_space:    return DM_KEY_SPACE;
        case RGFW_enter:    return DM_KEY_ENTER;

        case RGFW_bracket:   return DM_KEY_LBRACE;
        case RGFW_comma:     return DM_KEY_COMMA;
        case RGFW_period:    return DM_KEY_PERIOD;
        case RGFW_backSpace: return DM_KEY_BACKSPACE;
        case RGFW_semicolon: return DM_KEY_COLON;

        default: return DM_KEY_NONE;
    }
}

#ifndef DM_DEBUG
DM_INLINE
#endif
dm_mousebutton_code dm_convert_mousebutton(RGFW_mouseButton button)
{
    switch(button)
    {
        case RGFW_mouseLeft:   return DM_MOUSEBUTTON_L;
        case RGFW_mouseRight:  return DM_MOUSEBUTTON_R;
        case RGFW_mouseMiddle: return DM_MOUSEBUTTON_M;

        default: return DM_MOUSEBUTTON_UNKNOWN;
    }   
}

bool dm_window_poll_events(void* w)
{
    dm_window* window = w;

    window->flags = DM_WINDOW_FLAG_NONE;

    window->previous_input = window->current_input;

    //RGFW_waitForEvent(RGFW_eventWaitNext);
    RGFW_event event;
    while (RGFW_window_checkEvent(window->window, &event)) 
    {
        dm_key_code key;
        dm_mousebutton_code button;
        switch(event.type)
        {
            case RGFW_quit:
            window->flags |= DM_WINDOW_FLAG_CLOSE;
            break;

            case RGFW_keyPressed:
            case RGFW_keyReleased:
            key = dm_convert_key_code(event.key.value);
            if(key==DM_KEY_NONE) continue;

            window->current_input.keyboard.keys[key] = event.type==RGFW_keyPressed ? 1 : 0;
            break;

            case RGFW_mouseButtonPressed:
            case RGFW_mouseButtonReleased:
            button = dm_convert_mousebutton(event.button.value);
            if(button==DM_MOUSEBUTTON_UNKNOWN) continue;

            window->current_input.mouse.buttons[button] = event.type==RGFW_mouseButtonPressed ? 1 : 0;
            break;

            case RGFW_mousePosChanged:
            window->current_input.mouse.x = event.mouse.x;
            window->current_input.mouse.y = event.mouse.y;
            break;

            case RGFW_windowResized:
            window->flags |= DM_WINDOW_FLAG_RESIZE;
            break;

            default:
            break;
        }
    }

    return true;
} 

void* dm_window_get_internal(dm_context* context)
{
    dm_window* window = context->window;
#ifdef DM_PLATFORM_APPLE
    return RGFW_window_getView_OSX(window->window);
#elif defined(DM_PLATFORM_WIN32)
    return RGFW_window_getHWND(window->window);
#endif
}

bool dm_input_mouse_moved(dm_context* context)
{
    dm_window* window = context->window;

    bool move_x = window->current_input.mouse.x != window->previous_input.mouse.x;
    bool move_y = window->current_input.mouse.y != window->previous_input.mouse.y;
    
    return move_x || move_y;
}

bool dm_input_is_key_pressed(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.keyboard.keys[key];
}

bool dm_input_key_just_pressed(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.keyboard.keys[key]==1 && window->previous_input.keyboard.keys[key]==0);
}

bool dm_input_key_just_released(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.keyboard.keys[key]==0 && window->previous_input.keyboard.keys[key]==1);
}

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.buttons[button];
}

bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.mouse.buttons[button]==1 && window->previous_input.mouse.buttons[button]==0);
}

bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.mouse.buttons[button]==0 && window->previous_input.mouse.buttons[button]==1);
}

uint16_t dm_input_get_mouse_pos_x(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.x;
}

uint16_t dm_input_get_mouse_pos_y(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.y;
}

void dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_context* context)
{
    *x = dm_input_get_mouse_pos_x(context);
    *y = dm_input_get_mouse_pos_y(context);
}

int dm_input_get_mouse_delta_x(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.x - window->previous_input.mouse.x;
}

int dm_input_get_mouse_delta_y(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.y - window->previous_input.mouse.y;
}

void dm_input_get_mouse_delta(int* x, int* y, dm_context* context)
{
    *x = dm_input_get_mouse_delta_x(context);
    *y = dm_input_get_mouse_delta_y(context);
}
