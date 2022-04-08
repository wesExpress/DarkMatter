#include "dm_engine_api.h"
#include "platform/dm_platform.h"

float dm_get_time()
{
    return dm_platform_get_time();
}