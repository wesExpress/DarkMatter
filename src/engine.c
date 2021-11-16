#include "engine.h"
#include "platform/platform.h"

static engine_data* e_data;

bool engine_create()
{
    e_data = (engine_data*)malloc(sizeof(engine_data));
    if(!platform_startup(e_data))
    {
        return false;
    }

    return true;
}

void engine_shutdown()
{
    platform_shutdown(e_data);
    free(e_data);
}

bool engine_run()
{
    while(platform_pump_messages(e_data))
    {

    }

    return false;
}