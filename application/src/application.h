#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <DarkMatter/DarkMatter.h>

bool dm_application_init(dm_application* app);
void dm_application_shutdown(dm_application* app);
bool dm_application_update(dm_application* app, float delta_time);
bool dm_application_render(dm_application* app, float delta_time);
bool dm_application_resize(dm_application* app, uint32_t width, uint32_t height);

#endif