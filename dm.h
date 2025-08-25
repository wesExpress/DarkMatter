#ifndef __DM_H__
#define __DM_H__

#include "dm_defines.h"

#include "lib/cglm/include/cglm/cglm.h"

/**********
 * MEMORY * 
 **********/
void* dm_alloc(size_t size);
void  dm_free(void** ptr);
void* dm_realloc(void* ptr, size_t size);
void  dm_memcpy(void* dst, void* src, size_t size);
void* dm_memset(void* dst, int value, size_t size);
void* dm_memzero(void* dst, size_t size);

/*********
* WINDOW *
**********/
typedef enum dm_window_create_flag_t
{
    DM_WINDOW_CREATE_FLAG_NONE      = 0,
    DM_WINDOW_CREATE_FLAG_CENTER    = 1,
    DM_WINDOW_CREATE_FLAG_NO_RESIZE = 2
} dm_window_create_flag;

typedef enum dm_window_flag_t
{
    DM_WINDOW_FLAG_NONE   = 0,
    DM_WINDOW_FLAG_CLOSE  = 1,
    DM_WINDOW_FLAG_RESIZE = 2,
} dm_window_flag;

typedef struct dm_window_t dm_window; 
bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_window* window);
bool dm_window_should_close(dm_window* window);
bool dm_window_poll_events(dm_window* window);

/********
* INPUT *
*********/
#define MAKE_KEYCODE(NAME, CODE) DM_KEY_##NAME = CODE
typedef enum dm_key_code
{
	MAKE_KEYCODE(BACKSPACE, 0x08),
	MAKE_KEYCODE(TAB,       0x09),
	MAKE_KEYCODE(ENTER,     0x0D),
	MAKE_KEYCODE(SHIFT,     0x10),
	MAKE_KEYCODE(CTRL,      0x11),
	MAKE_KEYCODE(ESCAPE,    0x1B),
	MAKE_KEYCODE(SPACE,     0x20),
    
	// keys above arrows
	MAKE_KEYCODE(END,       0x23),
	MAKE_KEYCODE(HOME,      0x24),
	MAKE_KEYCODE(PRINT,     0x2A),
	MAKE_KEYCODE(INSERT,    0x2D),
	MAKE_KEYCODE(DELETE,    0x2E),
	MAKE_KEYCODE(NUMLCK,    0x90),
	MAKE_KEYCODE(SCRLLCK,   0x91),
	MAKE_KEYCODE(PAUSE,     0x13),
	MAKE_KEYCODE(PAGEUP,    0x21),
	MAKE_KEYCODE(PAGEDOWN,  0x22),
    
	MAKE_KEYCODE(LEFT,      0x25),
	MAKE_KEYCODE(UP,        0x26),
	MAKE_KEYCODE(RIGHT,     0x27),
	MAKE_KEYCODE(DOWN,      0x28),
    
	// numbers
	MAKE_KEYCODE(0,         0x30),
	MAKE_KEYCODE(1,         0x31),
	MAKE_KEYCODE(2,         0x32),
	MAKE_KEYCODE(3,         0x33),
	MAKE_KEYCODE(4,         0x34),
	MAKE_KEYCODE(5,         0x35),
	MAKE_KEYCODE(6,         0x36),
	MAKE_KEYCODE(7,         0x37),
	MAKE_KEYCODE(8,         0x38),
	MAKE_KEYCODE(9,         0x39),
    
	// letters
	MAKE_KEYCODE(A,         0x41),
	MAKE_KEYCODE(B,         0x42),
	MAKE_KEYCODE(C,         0x43),
	MAKE_KEYCODE(D,         0x44),
	MAKE_KEYCODE(E,         0x45),
	MAKE_KEYCODE(F,         0x46),
	MAKE_KEYCODE(G,         0x47),
	MAKE_KEYCODE(H,         0x48),
	MAKE_KEYCODE(I,         0x49),
	MAKE_KEYCODE(J,         0x4A),
	MAKE_KEYCODE(K,         0x4B),
	MAKE_KEYCODE(L,         0x4C),
	MAKE_KEYCODE(M,         0x4D),
	MAKE_KEYCODE(N,         0x4E),
	MAKE_KEYCODE(O,         0x4F),
	MAKE_KEYCODE(P,         0x50),
	MAKE_KEYCODE(Q,         0x51),
	MAKE_KEYCODE(R,         0x52),
	MAKE_KEYCODE(S,         0x53),
	MAKE_KEYCODE(T,         0x54),
	MAKE_KEYCODE(U,         0x55),
	MAKE_KEYCODE(V,         0x56),
	MAKE_KEYCODE(W,         0x57),
	MAKE_KEYCODE(X,         0x58),
	MAKE_KEYCODE(Y,         0x59),
	MAKE_KEYCODE(Z,         0x5A),
    
	// numpad
	MAKE_KEYCODE(NUMPAD_0,  0x60),
	MAKE_KEYCODE(NUMPAD_1,  0x61),
	MAKE_KEYCODE(NUMPAD_2,  0x62),
	MAKE_KEYCODE(NUMPAD_3,  0x63),
	MAKE_KEYCODE(NUMPAD_4,  0x64),
	MAKE_KEYCODE(NUMPAD_5,  0x65),
	MAKE_KEYCODE(NUMPAD_6,  0x66),
	MAKE_KEYCODE(NUMPAD_7,  0x67),
	MAKE_KEYCODE(NUMPAD_8,  0x68),
	MAKE_KEYCODE(NUMPAD_9,  0x69),
    
	MAKE_KEYCODE(MULTIPLY,  0x6A),
	MAKE_KEYCODE(ADD,       0x6B),
	MAKE_KEYCODE(SUBTRACT,  0x6C),
	MAKE_KEYCODE(DECIMAL,   0x6D),
	MAKE_KEYCODE(DIVIDE,    0x6E),
    
	// function
	MAKE_KEYCODE(F1,        0x70),
	MAKE_KEYCODE(F2,        0x71),
	MAKE_KEYCODE(F3,        0x72),
	MAKE_KEYCODE(F4,        0x73),
	MAKE_KEYCODE(F5,        0x74),
	MAKE_KEYCODE(F6,        0x75),
	MAKE_KEYCODE(F7,        0x76),
	MAKE_KEYCODE(F8,        0x77),
	MAKE_KEYCODE(F9,        0x78),
	MAKE_KEYCODE(F10,       0x79),
	MAKE_KEYCODE(F11,       0x7A),
	MAKE_KEYCODE(F12,       0x7B),
	MAKE_KEYCODE(F13,       0x7C),
	MAKE_KEYCODE(F14,       0x7D),
	MAKE_KEYCODE(F15,       0x7E),
	MAKE_KEYCODE(F16,       0x7F),
	MAKE_KEYCODE(F17,       0x80),
	MAKE_KEYCODE(F18,       0x81),
	MAKE_KEYCODE(F19,       0x82),
	MAKE_KEYCODE(F20,       0x83),
	MAKE_KEYCODE(F21,       0x84),
	MAKE_KEYCODE(F22,       0x85),
	MAKE_KEYCODE(F23,       0x86),
	MAKE_KEYCODE(F24,       0x87),
	
	// modifiers
	MAKE_KEYCODE(LSHIFT,    0xA0),
	MAKE_KEYCODE(RSHIFT,    0xA1),
	MAKE_KEYCODE(RCTRL,     0xA2),
	MAKE_KEYCODE(LCTRL,     0xA3),
	MAKE_KEYCODE(ALT,       0x12),
    MAKE_KEYCODE(LALT,      0xA4),
    MAKE_KEYCODE(RALT,      0xA5),
	MAKE_KEYCODE(CAPSLOCK,  0x14),
    MAKE_KEYCODE(LSUPER,    0x5B),
    MAKE_KEYCODE(RSUPER,    0x5C),
    
	// misc
	MAKE_KEYCODE(COMMA,     0xBC),
	MAKE_KEYCODE(PERIOD,    0xBE),
	MAKE_KEYCODE(PLUS,      0xBB),
	MAKE_KEYCODE(EQUAL,     0xBB),
	MAKE_KEYCODE(MINUS,     0xBD),
	MAKE_KEYCODE(COLON,     0xBA),
	MAKE_KEYCODE(TILDE,     0xC0),
    MAKE_KEYCODE(LBRACE,    0xDB),
	MAKE_KEYCODE(RBRACE,    0xDD),
	MAKE_KEYCODE(LSLASH,    0xDC),
	MAKE_KEYCODE(RSLASH,    0xBF),
	MAKE_KEYCODE(QUOTE,     0xDE),

    MAKE_KEYCODE(NONE, 0xFF)
} dm_key_code;

typedef enum dm_mousebutton_code_t
{
    DM_MOUSEBUTTON_L,
    DM_MOUSEBUTTON_R,
    DM_MOUSEBUTTON_M,
    DM_MOUSEBUTTON_DOUBLE,
    DM_MOUSEBUTTON_UNKNOWN
} dm_mousebutton_code;

bool dm_input_is_key_pressed(dm_key_code key, dm_window window);
bool dm_input_key_just_pressed(dm_key_code key, dm_window window);
bool dm_input_key_just_released(dm_key_code key, dm_window window);

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_window window);
bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_window window);
bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_window window);

bool     dm_input_mouse_moved(dm_window window);
uint16_t dm_input_get_mouse_pos_x(dm_window window);
uint16_t dm_input_get_mouse_pos_y(dm_window window);
void     dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_window window);
int      dm_input_get_mouse_delta_x(dm_window window);
int      dm_input_get_mouse_delta_y(dm_window window);
void     dm_input_get_mouse_delta(int* x, int* y, dm_window window);

/*****************
* IMPLEMENTATION *
******************/
#define DM_IMPLEMENTATION
#ifdef DM_IMPLEMENTATION

#define RGFW_IMPLEMENTATION
#include "lib/RGFW/RGFW.h"

void* dm_alloc(size_t size)
{
    void* temp = malloc(size);
    return dm_memzero(temp, size);
}

void dm_free(void** ptr)
{
    free(*ptr);
    *ptr = NULL;
}

void* dm_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void dm_memcpy(void* dest, void* src, size_t size)
{
    memcpy(dest, src, size);
}

void* dm_memset(void* dst, int value, size_t size)
{
    return memset(dst, value, size);
}

void* dm_memzero(void* dst, size_t size)
{
    return dm_memset(dst, 0, size);
}

// === input ===
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
struct dm_window_t 
{
    RGFW_window* window;
    dm_window_flag flags;

    dm_input_state current_input;
    dm_input_state previous_input;
};

bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_window* window)
{
    RGFW_windowFlags window_flags = 0;
    if(flags & DM_WINDOW_CREATE_FLAG_CENTER) window_flags |= RGFW_windowCenter;
    if(flags & DM_WINDOW_CREATE_FLAG_NO_RESIZE) window_flags |= RGFW_windowNoResize;

    window->window = RGFW_createWindow(title, x,y,width,height, window_flags);
    if(!window->window) { return false; }

    return true;
}

bool dm_window_should_close(dm_window* window)
{
    return window->flags & DM_WINDOW_FLAG_CLOSE;
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

bool dm_window_poll_events(dm_window* window)
{
    window->flags = DM_WINDOW_FLAG_NONE;

    window->previous_input = window->current_input;

    RGFW_waitForEvent(RGFW_eventWaitNext);
    RGFW_event event;
    while (RGFW_window_checkEvent(window->window, &event)) 
    {
        switch(event.type)
        {
            case RGFW_quit:
            window->flags |= DM_WINDOW_FLAG_CLOSE;
            break;

            case RGFW_keyPressed:
            case RGFW_keyReleased:
            {
                dm_key_code key = dm_convert_key_code(event.key.value);
                if(key==DM_KEY_NONE) continue;

                window->current_input.keyboard.keys[key] = event.type==RGFW_keyPressed ? 1 : 0;
            } break;

            case RGFW_mouseButtonPressed:
            case RGFW_mouseButtonReleased:
            {
                dm_mousebutton_code button = dm_convert_mousebutton(event.button.value);
                if(button==DM_MOUSEBUTTON_UNKNOWN) continue;

                window->current_input.mouse.buttons[button] = event.type==RGFW_mouseButtonPressed ? 1 : 0;
            } break;

            case RGFW_mousePosChanged:
            window->current_input.mouse.x = event.mouse.x;
            window->current_input.mouse.y = event.mouse.y;
            break;

            default:
            break;
        }
    }

    return true;
} 

// === input ===
bool dm_input_mouse_moved(dm_window window)
{
    bool move_x = window.current_input.mouse.x != window.previous_input.mouse.x;
    bool move_y = window.current_input.mouse.y != window.previous_input.mouse.y;
    
    return move_x || move_y;
}

bool dm_input_is_key_pressed(dm_key_code key, dm_window window)
{
    return window.current_input.keyboard.keys[key];
}

bool dm_input_key_just_pressed(dm_key_code key, dm_window window)
{
    return (window.current_input.keyboard.keys[key]==1 && window.previous_input.keyboard.keys[key]==0);
}

bool dm_input_key_just_released(dm_key_code key, dm_window window)
{
    return (window.current_input.keyboard.keys[key]==0 && window.previous_input.keyboard.keys[key]==1);
}

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_window window)
{
    return window.current_input.mouse.buttons[button];
}

bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_window window)
{
    return (window.current_input.mouse.buttons[button]==1 && window.previous_input.mouse.buttons[button]==0);
}

bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_window window)
{
    return (window.current_input.mouse.buttons[button]==0 && window.previous_input.mouse.buttons[button]==1);
}

uint16_t dm_input_get_mouse_pos_x(dm_window window)
{
    return window.current_input.mouse.x;
}

uint16_t dm_input_get_mouse_pos_y(dm_window window)
{
    return window.current_input.mouse.y;
}

void dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_window window)
{
    *x = dm_input_get_mouse_pos_x(window);
    *y = dm_input_get_mouse_pos_y(window);
}

int dm_input_get_mouse_delta_x(dm_window window)
{
    return window.current_input.mouse.x - window.previous_input.mouse.x;
}

int dm_input_get_mouse_delta_y(dm_window window)
{
    return window.current_input.mouse.y - window.previous_input.mouse.y;
}

void dm_input_get_mouse_delta(int* x, int* y, dm_window window)
{
    *x = dm_input_get_mouse_delta_x(window);
    *y = dm_input_get_mouse_delta_y(window);
}

#endif // DM_IMPLEMENTATION

#endif // DM_H
