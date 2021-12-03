#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "dm_defines.h"
#include "dm_engine_types.h"

/*
* Sets up the platform specific data.
*
* @param e_data -> dm_engine_data struct (ptr)
*/
bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title);
void dm_platform_shutdown(dm_engine_data* e_data);
	 
bool dm_platform_pump_messages(dm_engine_data* e_data);

void dm_platform_write(const char* message, uint8_t color);
void dm_platform_write_error(const char* message, uint8_t color);

#endif