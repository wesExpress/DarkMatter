#include "dm_engine.h"
#include "platform/dm_platform.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "rendering/dm_renderer.h"
#include "input/dm_input.h"

dm_engine_data* e_data = NULL;

bool dm_engine_create()
{
    e_data = (dm_engine_data*)dm_alloc(sizeof(dm_engine_data));
    
    dm_event_set_callback(dm_engine_on_event);

    if(!dm_platform_startup(e_data, 1280, 720, "CEngine", 100, 100))
    {
        DM_FATAL("Platform could not be initialized!");
        return false;
    }

    if (!dm_renderer_init(e_data->platform_data, (dm_color) { 0.2f, 0.5f, 0.8f, 1.0f }))
    {
        DM_FATAL("Renderer could not be initialized!");
        return false;
    }

    e_data->is_running = true;
    e_data->is_suspended = false;

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
        DM_INFO("Window close event received. Shutting down...");
        e_data->is_running = false;
        return true;
    } break;
    case DM_KEY_UP_EVENT:
    {
        DM_DEBUG("Key up event received");
    } break;
    case DM_KEY_DOWN_EVENT:
    {
        DM_DEBUG("Key down event received");
    } break;
    case DM_MOUSEBUTTON_UP_EVENT:
    {
        DM_DEBUG("Mousebutton up event received");
    } break;
    case DM_MOUSEBUTTON_DOWN_EVENT:
    {
        DM_DEBUG("Mousebutton down event received");
    } break;
    case DM_MOUSE_MOVED_EVENT:
    {} break;
    case DM_MOUSE_SCROLLED_EVENT:
    {} break;
    case DM_WINDOW_RESIZE_EVENT:
    {} break;
    }

    return false;
}