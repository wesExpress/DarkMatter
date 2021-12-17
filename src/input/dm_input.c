#include "dm_input.h"
#include "platform/dm_platform.h"

typedef struct dm_input_state
{
	dm_keyboard_state keyboard;
	dm_mouse_state mouse;
} dm_input_state;

static dm_input_state dm_current_input = { 0 };
static dm_input_state dm_prev_input = { 0 };

void dm_input_update_state()
{
	//dm_prev_input = dm_current_input;
	dm_platform_memcpy(&dm_prev_input, &dm_current_input, sizeof(dm_input_state));
	dm_platform_memzero(&dm_current_input, sizeof(dm_input_state));
}

bool dm_input_is_key_pressed(dm_key_code key)
{
	return dm_current_input.keyboard.keys[key];
}

bool dm_input_is_mousebutton_pressed(dm_mousebutton_code button)
{
	return dm_current_input.mouse.buttons[button];
}

bool dm_input_key_just_pressed(dm_key_code key)
{
	return ((dm_current_input.keyboard.keys[key] == 1) && (dm_prev_input.keyboard.keys[key] == 0));
}

bool dm_input_key_just_released(dm_key_code key)
{
	return ((dm_current_input.keyboard.keys[key] == 0) && (dm_prev_input.keyboard.keys[key] == 1));
}

bool dm_input_mousebutton_just_pressed(dm_mousebutton_code button)
{
	return ((dm_current_input.mouse.buttons[button] == 1) && (dm_prev_input.mouse.buttons[button] == 0));
}

bool dm_input_mousebutton_just_released(dm_mousebutton_code button)
{
	return ((dm_current_input.mouse.buttons[button] == 0) && (dm_prev_input.mouse.buttons[button] == 1));
}

bool dm_input_mouse_has_moved()
{
	return ((dm_current_input.mouse.x != dm_prev_input.mouse.x) || (dm_current_input.mouse.y != dm_prev_input.mouse.y));
}

int dm_input_get_mouse_x()
{
	return dm_current_input.mouse.x;
}

int dm_input_get_mouse_y()
{
	return dm_current_input.mouse.y;
}

void dm_input_get_mouse_pos(int* x, int* y)
{
	*x = dm_current_input.mouse.x;
	*y = dm_current_input.mouse.y;
}

int dm_input_get_prev_mouse_x()
{
	return dm_prev_input.mouse.x;
}

int dm_input_get_prev_mouse_y()
{
	return dm_prev_input.mouse.y;
}

void dm_input_get_prev_mouse_pos(int* x, int* y)
{
	*x = dm_prev_input.mouse.x;
	*y = dm_prev_input.mouse.y;
}

int dm_input_get_mouse_scroll()
{
	return dm_current_input.mouse.scroll;
}

int dm_input_get_prev_mouse_scroll()
{
	return dm_prev_input.mouse.scroll;
}

// internal functions
void dm_input_clear_keyboard()
{
	for (int i = 0; i < 256; i++)
	{
		dm_current_input.keyboard.keys[i] = 0;
	}
}

void dm_input_reset_mouse_x()
{
	dm_current_input.mouse.x = 0;
}

void dm_input_reset_mouse_y()
{
	dm_current_input.mouse.y = 0;
}

void dm_input_reset_mouse_pos()
{
	dm_input_reset_mouse_x();
	dm_input_reset_mouse_y();
}

void dm_input_reset_mouse_scroll()
{
	dm_current_input.mouse.scroll = 0;
}

void dm_input_clear_mousebuttons()
{
	for (int i = 0; i < 3; i++)
	{
		dm_current_input.mouse.buttons[i] = 0;
	}
}

void dm_input_set_key_pressed(dm_key_code key)
{
	dm_current_input.keyboard.keys[key] = 1;
}

void dm_input_set_key_released(dm_key_code key)
{
	dm_current_input.keyboard.keys[key] = 0;
}

void dm_input_set_mousebutton_pressed(dm_mousebutton_code button)
{
	dm_current_input.mouse.buttons[button] = 1;
}

void dm_input_set_mousebutton_released(dm_mousebutton_code button)
{
	dm_current_input.mouse.buttons[button] = 0;
}

void dm_input_set_mouse_x(int x)
{
	dm_current_input.mouse.x = x;
}

void dm_input_set_mouse_y(int y)
{
	dm_current_input.mouse.y = y;
}

void dm_input_set_mouse_scroll(int delta)
{
	dm_current_input.mouse.scroll += delta;
}