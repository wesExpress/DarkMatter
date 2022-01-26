#ifndef __DM_INPUT_H__
#define __DM_INPUT_H__

#include <stdint.h>
#include "dm_keyboard.h"
#include "dm_mouse.h"
#include "core/dm_defines.h"

// keyboard
DM_API bool dm_input_is_key_pressed(dm_key_code key);
DM_API bool dm_input_is_mousebutton_pressed(dm_mousebutton_code button);
DM_API bool dm_input_key_just_pressed(dm_key_code key);
DM_API bool dm_input_key_just_released(dm_key_code key);
DM_API bool dm_input_mousebutton_just_pressed(dm_mousebutton_code button);
DM_API bool dm_input_mousebutton_just_released(dm_mousebutton_code button);

// getters for mouse
DM_API int  dm_input_get_mouse_x();
DM_API int  dm_input_get_mouse_y();
DM_API void dm_input_get_mouse_pos(int* x, int* y);
DM_API int  dm_input_get_mouse_scroll();
DM_API bool dm_input_mouse_has_moved();
DM_API int  dm_input_get_prev_mouse_x();
DM_API int  dm_input_get_prev_mouse_y();
DM_API void dm_input_get_prev_mouse_pos(int* x, int* y);
DM_API int  dm_input_get_mouse_scroll();

// clear/resetting functions
DM_API void dm_input_reset_mouse_x();
DM_API void dm_input_reset_mouse_y();
DM_API void dm_input_reset_mouse_pos();
DM_API void dm_input_reset_mouse_scroll();
DM_API void dm_input_clear_keyboard();
DM_API void dm_input_clear_mousebuttons();

void dm_input_update_state();

void dm_input_set_key_pressed(dm_key_code key);
void dm_input_set_key_released(dm_key_code key);

void dm_input_set_mousebutton_pressed(dm_mousebutton_code button);
void dm_input_set_mousebutton_released(dm_mousebutton_code button);
void dm_input_set_mouse_x(int x);
void dm_input_set_mouse_y(int y);
void dm_input_set_mouse_scroll(int delta);
#endif