#ifndef APP_H
#define APP_H

#include "dm.h"

typedef struct basic_camera_t
{
    float fov, near_plane, far_plane;
    float move_speed, look_sens;
    
    uint32_t width, height;
    
    float pos[N3];
    float up[N3], forward[N3], right[N3];
    float proj[M4], view[M4], inv_view[M4], view_proj[M4];
} basic_camera;

#define MAX_ENTITIES 520
typedef struct application_data_t
{
    uint32_t     entity_count;
    dm_entity    entities[MAX_ENTITIES];
    basic_camera camera;
    
    void*        render_pass_data;
    void*        debug_render_pass_data;
} application_data;

bool app_init(dm_context* context);
void app_shutdown(dm_context* context);
bool app_update(dm_context* context);
bool app_render(dm_context* context);

#endif //APP_H