#ifndef __DM_TEXTURE_H__
#define __DM_TEXTURE_H__

#include "dm_render_types.h"
#include <stdbool.h>

void dm_texture_map_init();
void dm_texture_map_destroy();
bool dm_textures_load(dm_image_desc* image_descs, int num_descs);
dm_texture* dm_texture_get(const char* name);

#endif