#ifndef __DM_METAL_TEXTURE_H__
#define __DM_METAL_TEXTURE_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "rendering/dm_render_types.h"
#include "dm_metal_renderer.h"
#include <stdbool.h>

bool dm_metal_create_texture(dm_image* image, dm_metal_renderer* renderer);
void dm_metal_destroy_texture(dm_image* image);

#endif

#endif