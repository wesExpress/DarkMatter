#include "dm_engine.h"
#include "dm_logger.h"

int main()
{
#ifdef DM_PLATFORM_UNSUPPORTED
    DM_LOG_FATAL("Trying to compile on unsupprted platform!");
    return false;
#endif

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