#ifndef __DM_METAL_SHADER_H__
#define __DM_METAL_SHADER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"
#include <stdbool.h>

bool dm_metal_create_shader_library(dm_shader* shader, NSString* path, dm_metal_renderer* renderer);
void dm_metal_destroy_shader_library(dm_shader* shader);

#endif

#endif