#ifndef __DM_ENGINE_ENTRY_H__
#define __DM_ENGINE_ENTRY_H__

#include "dm_app_config.h"
#include "dm_engine.h"
#include "dm_logger.h"
#include <stdbool.h>

extern bool dm_create_app(dm_application* application);

typedef enum dm_engine_status
{
    DM_UNSUPPORTED_PLATFORM = -1,
    DM_APPLICATION_CREATION_FAILURE = -2,
    DM_ENGINE_CREATION_FAILURE = -3,
    DM_ENGINE_RUN_FAILURE = -4,
    DM_ENGINE_RUN_SUCCESS = 0
} dm_engine_status;

int main()
{
#ifdef DM_PLATFORM_UNSUPPORTED
    DM_LOG_FATAL("Trying to compile on unsupprted platform!");
    return DM_UNSUPPORTED_PLATFORM;
#endif

    dm_application app;
    if (!dm_create_app(&app))
    {
        DM_LOG_FATAL("Could not create application!");
        return DM_APPLICATION_CREATION_FAILURE;
    }

    ///////////////////////

    if (!dm_engine_create(&app))
    {
        return DM_ENGINE_CREATION_FAILURE;
    }

    if (!dm_engine_run())
    {
        return DM_ENGINE_RUN_FAILURE;
    }

    dm_engine_shutdown();

    return DM_ENGINE_RUN_SUCCESS;
}

#endif