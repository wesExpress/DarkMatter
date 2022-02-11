#ifndef __DM_DIRECTX_SHADER_H__
#define __DM_DIRECTX_SHADER_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include <stdbool.h>

bool dm_directx_create_shader(dm_shader* shader, dm_vertex_layout layout, dm_directx_renderer* renderer);
void dm_directx_delete_shader(dm_shader* shader);

#endif

#endif