#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "dm_event.h"
#include "dm_engine_types.h"
#include <stdbool.h>

/*
* create engine data ptr
* run platform startup
*/
DM_API bool dm_engine_create(dm_application* app);

/*
* run paltform shutdown
* cleanup engine data ptr memory
*/
DM_API void dm_engine_shutdown();

/*
* main engine loop
* will return true if window closes properly
* other cases not handled currently
*/
DM_API bool dm_engine_run();

/*
* event callback function
*/
DM_API bool dm_engine_on_event(dm_event_type type, void* data);

#endif