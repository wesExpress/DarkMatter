#include "dm.h"

bool dm_input_key_is_pressed(dm_context *context, dm_key_code key)
{
    return context->window.input_states[0].keys[key]==1;
}

bool dm_input_key_just_released(dm_context *context, dm_key_code key)
{
    return (context->window.input_states[0].keys[key]==0 && context->window.input_states[1].keys[key]==1);
}

bool dm_input_mouse_button_is_pressed(dm_context *context, dm_mouse_button button)
{
    return context->window.input_states[0].buttons[button]==1;
}

bool dm_input_mouse_button_just_released(dm_context *context, dm_mouse_button button)
{
    return (context->window.input_states[0].buttons[button]==0 && context->window.input_states[1].buttons[button]==1);
}

int dm_input_get_mouse_x(dm_context *context)
{
    return context->window.input_states[0].mouse_x;
}

int dm_input_get_mouse_y(dm_context *context)
{
    return context->window.input_states[0].mouse_y;
}
