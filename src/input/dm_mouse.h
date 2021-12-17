#ifndef __DM_MOUSE_H__
#define __DM_MOUSE_H__

#include <stdbool.h>

typedef enum dm_mousebutton_code
{
    DM_MOUSEBUTTON_L,
    DM_MOUSEBUTTON_R,
    DM_MOUSEBUTTON_M
} dm_mousebutton_code;

typedef struct dm_mouse_state
{
    bool buttons[3];
    int x, y;
    int scroll;
} dm_mouse_state;

#endif