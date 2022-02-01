#ifndef __DM_OPENGL_TEXTURE_H__
#define __DM_OPENGL_TEXTURE_H__

#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include "rendering/dm_render_types.h"
#include "dm_opengl_renderer.h"
#include <stdbool.h>

bool dm_opengl_create_texture(dm_image* texture, int texture_slot, GLuint shader);
void dm_opengl_destroy_texture(dm_image* texture);
bool dm_opengl_bind_texture(dm_image* texture);

#endif

#endif