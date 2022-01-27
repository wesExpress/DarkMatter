#include "core/dm_engine.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_app_config.h"
#include "rendering/dm_renderer.h"
#include "platform/dm_platform.h"
#include "input/dm_input.h"

dm_engine_data* e_data = NULL;
static bool initialized = false;

static double start_time = 0.0;
static double end_time = 0.0;

bool dm_engine_create(dm_application* app)
{
    if (initialized)
    {
        DM_LOG_FATAL("Engine create called more than once!");
        return false;
    }

    e_data = dm_alloc(sizeof(dm_engine_data), DM_MEM_ENGINE);
    e_data->application = app;

    dm_event_set_callback(dm_engine_on_event);

    if(!dm_platform_startup(e_data, 
        app->engine_config.start_width, app->engine_config.start_height, 
        app->engine_config.name, 
        app->engine_config.start_x, app->engine_config.start_y))
    {
        DM_LOG_FATAL("Platform could not be initialized!");
        return false;
    }

    if (!dm_renderer_init(e_data->platform_data, e_data->application->engine_config.clear_color))
    {
        DM_LOG_FATAL("Renderer could not be initialized!");
        return false;
    }

    if (!e_data->application->dm_application_init(e_data->application))
    {
        DM_LOG_FATAL("Application could not be initialized!");
        return false;
    }

    if (!dm_renderer_init_object_data())
    {
        DM_LOG_FATAL("Could not initialize object data!");
        return false;
    }

    e_data->is_running = true;
    e_data->is_suspended = false;

    dm_mem_track();

    return true;
}

void dm_engine_shutdown()
{
    e_data->application->dm_application_shutdown(e_data->application);

    dm_free(e_data, sizeof(dm_engine_data), DM_MEM_ENGINE);

    dm_mem_track();
    dm_mem_all_freed();
}

bool dm_engine_run()
{
    while (e_data->is_running)
    {
        start_time = dm_platform_get_time();
        double delta_time = start_time - end_time;
        
        if (!dm_platform_pump_messages(e_data))
        {
            e_data->is_running = false;
        }

        if (!e_data->is_suspended)
        {
            if (!e_data->application->dm_application_update(e_data->application, delta_time))
            {
                DM_LOG_FATAL("Application update failed!");
                e_data->is_running = false;
                break;
            }

            if (!e_data->application->dm_application_render(e_data->application, delta_time))
            {
                DM_LOG_FATAL("Application rendering failed!");
                e_data->is_running = false;
                break;
            }

            if (!dm_renderer_begin_scene())
            {
                DM_LOG_FATAL("Something went wrong in begin scene...");
                e_data->is_running = false;
                break;
            }

            if (!dm_renderer_end_scene())
            {
                DM_LOG_FATAL("Something went wrong in end scene...");
                e_data->is_running = false;
                break;
            }



            end_time = start_time;
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
        dm_key_code key = (dm_key_code)(intptr_t)data;
        dm_input_set_key_released(key);

        DM_LOG_DEBUG("Key up event received: %c", key);
    } break;
    case DM_KEY_DOWN_EVENT:
    {
        dm_key_code key = (dm_key_code)(intptr_t)data;
        dm_input_set_key_pressed(key);

        // TODO: need to remove this eventaully
        if(key == DM_KEY_ESCAPE) dm_event_dispatch((dm_event){ DM_WINDOW_CLOSE_EVENT, NULL, NULL });

        DM_LOG_DEBUG("Key down event received: %c", key);
    } break;
    case DM_KEY_TYPE_EVENT:
    {
        // TODO: figure out what to put here
    } break;
    case DM_MOUSEBUTTON_UP_EVENT:
    {
        dm_mousebutton_code button = (dm_mousebutton_code)(intptr_t)data;
        dm_input_set_mousebutton_released(button);

        DM_LOG_DEBUG("Mousebutton up event received");
    } break;
    case DM_MOUSEBUTTON_DOWN_EVENT:
    {
        dm_mousebutton_code button = (dm_mousebutton_code)(intptr_t)data;
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
        int8_t scroll = (intptr_t)data;
        DM_LOG_DEBUG("Scroll: %d", scroll);
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