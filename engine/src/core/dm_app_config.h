#ifndef __DM_APP_CONFIG_H__
#define __DM_APP_CONFIG_H__

#include "core/dm_defines.h"
#include "dm_engine_types.h"
#include <stdbool.h>

bool dm_application_init(dm_application* app);
bool dm_application_update(dm_application* app, float delta_time);
bool dm_application_render(dm_application* app, float delta_time);
void dm_application_shutdown(dm_application* app);
bool dm_application_resize(dm_application* app, uint32_t width, uint32_t height);

#endif