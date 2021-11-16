#include "engine.h"
#include "platform/platform.h"

static engine_data* e_data;

/*
* create engine data ptr
* run platform startup
*/
bool engine_create()
{
    e_data = (engine_data*)malloc(sizeof(engine_data));
    if(!platform_startup(e_data))
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