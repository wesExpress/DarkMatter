#ifndef __DM_DIRECTX_SHADER_H__
#define __DM_DIRECTX_SHADER_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include <stdbool.h>

bool dm_directx_create_shader(dm_shader* shader, dm_vertex_layout layout, dm_internal_renderer* renderer, dm_render_pipeline* pipeline);
void dm_directx_delete_shader(dm_shader* shader, dm_internal_pipeline* pipeline);

#endif

#endif