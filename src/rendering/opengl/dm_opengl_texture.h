#ifndef __DM_OPENGL_TEXTURE_H__
#define __DM_OPENGL_TEXTURE_H__

#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include "rendering/dm_render_types.h"
#include <stdbool.h>

bool dm_opengl_create_texture(dm_texture* texture);
void dm_opengl_destroy_texture(dm_texture* texture);
bool dm_opengl_bind_texture(dm_texture* texture, int texture_slot);

#endif

#endif