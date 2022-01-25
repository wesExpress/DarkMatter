#ifndef __DM_DIRECTX_TEXTURE_H__
#define __DM_DIRECTX_TEXTURE_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include "rendering/dm_render_types.h"
#include <stdbool.h>

bool dm_directx_create_texture(dm_texture* texture, dm_internal_renderer* renderer);
void dm_directx_destroy_texture(dm_texture* texture);
void dm_directx_bind_texture(dm_texture* texture, uint32_t slot, dm_internal_renderer* renderer, dm_internal_pipeline* pipeline);

#endif

#endif