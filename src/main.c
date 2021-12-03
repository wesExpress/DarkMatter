#include "dm_engine.h"

int main()
{
    if(!dm_engine_create())
    {
        return -1;
    }

    if(!dm_engine_run())
    {
        return -2;
    }

    dm_engine_shutdown();

    return 0;
}