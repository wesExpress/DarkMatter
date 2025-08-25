#define DM_IMPLEMENTATION
#include "dm.h"

#include <stdio.h>

int main()
{
    dm_window window = { 0 }; 
    if(!dm_window_create(0,0,1280,720, "test", DM_WINDOW_CREATE_FLAG_CENTER, &window)) return 0;

    while(true)
    {
        if(!dm_window_poll_events(&window)) break;
        if(dm_window_should_close(&window)) break;
        if(dm_input_is_key_pressed(DM_KEY_ESCAPE, window)) { printf("window closed\n"); break; }
    }

    return 0;
}
