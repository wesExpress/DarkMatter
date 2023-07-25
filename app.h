#ifndef APP_H
#define APP_H

#include "dm.h"

typedef struct basic_camera_t
{
    float proj[M4];
    float pos[3];
} basic_camera;

#define MAX_ENTITIES 256
typedef struct application_data_t
{
    dm_entity    entities[MAX_ENTITIES];
    basic_camera camera;
    void*        render_pass_data;
} application_data;

bool app_init(dm_context* context);
void app_shutdown(dm_context* context);
bool app_update(dm_context* context);
bool app_render(dm_context* context);

#endif //APP_H
