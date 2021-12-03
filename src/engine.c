#include "engine.h"
#include "platform/platform.h"
#include "mem.h"

engine_data* e_data = NULL;

/*
* create engine data ptr
* run platform startup
*/
bool engine_create()
{
    e_data = (engine_data*)mem_alloc(sizeof(engine_data));
    
    if(!platform_startup(e_data, 1280, 720, "CEngine"))
    {
        return false;
    }

    return true;
}

/*
* run paltform shutdown
* cleanup engine data ptr memory
*/
void engine_shutdown()
{
    platform_shutdown(e_data);
    free(e_data);
}

/*
* main engine loop
* will return true if window closes properly
* other cases not handled currently
*/
bool engine_run()
{
    while(platform_pump_messages(e_data))
    {
        
    }

    return true;
}