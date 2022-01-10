#include "dm_engine.h"
#include "platform/dm_platform.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "rendering/dm_renderer.h"
#include "input/dm_input.h"

// TODO: remove
#include "structures/dm_list.h"
#include "structures/dm_map.h"

dm_engine_data* e_data = NULL;
static bool initialized = false;

bool dm_engine_create()
{
    if (initialized)
    {
        DM_LOG_FATAL("Engine create called more than once!");
        return false;
    }

    e_data = (dm_engine_data*)dm_alloc(sizeof(dm_engine_data));
    
    dm_event_set_callback(dm_engine_on_event);

    if(!dm_platform_startup(e_data, 1280, 720, "CEngine", 100, 100))
    {
        DM_LOG_FATAL("Platform could not be initialized!");
        return false;
    }

    if (!dm_renderer_init(e_data->platform_data, (dm_color) { 0.2f, 0.5f, 0.8f, 1.0f }))
    {
        DM_LOG_FATAL("Renderer could not be initialized!");
        return false;
    }

    e_data->is_running = true;
    e_data->is_suspended = false;

    //dm_map* map = dm_map_create(50000);
    //dm_map_insert(map, "1", "First");
    //dm_map_insert(map, "2", "Second");
    //dm_map_insert(map, "Hel", "Third");
    //dm_map_insert(map, "Cau", "Fourth");
    //dm_map_search_print(map, "1");
    //dm_map_search_print(map, "2");
    //dm_map_search_print(map, "3");
    //dm_map_search_print(map, "Hel");
    //dm_map_search_print(map, "Cau");
    //dm_map_print(map);
    //dm_map_delete(map);

    return true;
}

void dm_engine_shutdown()
{
    free(e_data);
}

bool dm_engine_run()
{
    while (e_data->is_running)
    {
        if (!dm_platform_pump_messages(e_data))
        {
            e_data->is_running = false;
        }

        if (!e_data->is_suspended)
        {
            dm_renderer_begin_scene();

            dm_renderer_end_scene();
        }
    }
    
    e_data->is_running = false;

    dm_renderer_shutdown();
    dm_platform_shutdown(e_data);

    return true;
}

bool dm_engine_on_event(dm_event_type type, void* data)
{
    switch (type)
    {
    case DM_WINDOW_CLOSE_EVENT:
    {
        DM_LOG_DEBUG("Window close event received. Shutting down...");
        e_data->is_running = false;
        return true;
    } break;
    case DM_KEY_UP_EVENT:
    {
        dm_key_code key = (dm_key_code)data;
        dm_input_set_key_released(key);

        DM_LOG_DEBUG("Key up event received: %c", key);
    } break;
    case DM_KEY_DOWN_EVENT:
    {
        dm_key_code key = (dm_key_code)data;
        dm_input_set_key_pressed(key);

        DM_LOG_DEBUG("Key down event received: %c", key);
    } break;
    case DM_KEY_TYPE_EVENT:
    {
        // TODO: figure out what to put here
    } break;
    case DM_MOUSEBUTTON_UP_EVENT:
    {
        dm_mousebutton_code button = (dm_mousebutton_code)data;
        dm_input_set_mousebutton_released(button);

        DM_LOG_DEBUG("Mousebutton up event received");
    } break;
    case DM_MOUSEBUTTON_DOWN_EVENT:
    {
        dm_mousebutton_code button = (dm_mousebutton_code)data;
        dm_input_set_mousebutton_pressed(button);

        DM_LOG_DEBUG("Mousebutton down event received");
    } break;
    case DM_MOUSE_MOVED_EVENT:
    {
        int x = *(int*)data;
        int y = *((int*)data + 1);

        dm_input_set_mouse_x(x);
        dm_input_set_mouse_y(y);
    } break;
    case DM_MOUSE_SCROLLED_EVENT:
    {
        // TODO 
    } break;
    case DM_WINDOW_RESIZE_EVENT:
    {
        if (initialized)
        {
            int width = *(int*)data;
            int height = *((int*)data + 1);

            dm_renderer_resize(width, height);
        }
    } break;
    }

    return false;
}