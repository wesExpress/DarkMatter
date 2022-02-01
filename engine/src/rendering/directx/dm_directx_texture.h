#ifndef __DM_DIRECTX_TEXTURE_H__
#define __DM_DIRECTX_TEXTURE_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include "rendering/dm_render_types.h"
#include <stdbool.h>

bool dm_directx_create_texture(dm_image* image, dm_internal_renderer* renderer);
void dm_directx_destroy_texture(dm_image* image);
void dm_directx_bind_texture(dm_image* image, uint32_t slot, dm_internal_renderer* renderer);

#endif

#endif