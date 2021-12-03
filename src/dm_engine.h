#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <stdbool.h>

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

#endif