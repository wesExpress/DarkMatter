#include "dm_engine.h"
#include "platform/dm_platform.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "rendering/dm_renderer.h"

dm_engine_data* e_data = NULL;

bool dm_engine_create()
{
    e_data = (dm_engine_data*)dm_alloc(sizeof(dm_engine_data));
    
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

    return true;
}

void dm_engine_shutdown()
{
    dm_renderer_shutdown();
    dm_platform_shutdown(e_data);
    free(e_data);
}

bool dm_engine_run()
{
    while(dm_platform_pump_messages(e_data))
    {
        dm_renderer_begin_scene();

        dm_renderer_end_scene();
    }

    return true;
}