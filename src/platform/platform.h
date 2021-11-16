#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "defines.h"
#include "engine.h"

bool platform_startup(engine_data* e_data);
void platform_shutdown(engine_data* e_data);

bool platform_pump_messages(engine_data* e_data);

#endif