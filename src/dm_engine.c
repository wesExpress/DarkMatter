#include "dm_engine.h"
#include "platform/dm_platform.h"
#include "dm_mem.h"

dm_engine_data* e_data = NULL;

bool dm_engine_create()
{
    e_data = (dm_engine_data*)dm_alloc(sizeof(dm_engine_data));
    
    if(!dm_platform_startup(e_data, 1280, 720, "CEngine", 100, 100))
    {
        return false;
    }

    return true;
}

void dm_engine_shutdown()
{
    dm_platform_shutdown(e_data);
    free(e_data);
}

bool dm_engine_run()
{
    while(dm_platform_pump_messages(e_data))
    {
        
    }

    return true;
}