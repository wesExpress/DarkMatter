#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <stdbool.h>
#include "dm_event.h"
/*
* create engine data ptr
* run platform startup
*/
bool dm_engine_create();

/*
* run paltform shutdown
* cleanup engine data ptr memory
*/
void dm_engine_shutdown();
	 
/*
* main engine loop
* will return true if window closes properly
* other cases not handled currently
*/
bool dm_engine_run();

/*
* event callback function
*/
bool dm_app_on_event(dm_event_type type, void* data);

#endif