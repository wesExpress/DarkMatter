#ifndef __DM_OPENGL_RENDER_PASS_H__
#define __DM_OPENGL_RENDER_PASS_H__

#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include <stdbool.h>
#include "dm_opengl_renderer.h"

bool dm_opengl_create_render_pass(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline);
void dm_opengl_destroy_render_pass(dm_render_pass* render_pass);

bool dm_opengl_begin_render_pass(dm_render_pass* render_pass);

#endif

#endif