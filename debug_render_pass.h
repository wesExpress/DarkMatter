#ifndef DEBUG_RENDER_PASS_H
#define DEBUG_RENDER_PASS_H

#include "dm.h"

bool debug_render_pass_init(dm_context* context);
void debug_render_pass_shutdown(dm_context* context);
bool debug_render_pass_render(dm_context* context);

// rendering funcs
void debug_render_line(float pos_0[3], float pos_1[3], float color[4], dm_context* context);
void debug_render_arrow(float pos_0[3], float pos_1[3], float color[4], dm_context* context);
void debug_render_bilboard(float pos[3], float width, float height, float color[4], dm_context* context);
void debug_render_aabb(float pos[3], float dim[3], float color[4], dm_context* context);
void debug_render_cube(float pos[3], float dim[3], float orientation[4], float color[4], dm_context* context);

#endif //DEBUG_RENDER_PASS_H
