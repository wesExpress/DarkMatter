#ifndef __DM_IMAGE_H__
#define __DM_IMAGE_H__

#include "dm_render_types.h"
#include "core/dm_defines.h"
#include <stdbool.h>

void dm_image_map_init();
void dm_image_map_destroy();

/*
load an image from a given path
*/
DM_API bool dm_load_image(const char* path, const char* name, bool flip, dm_texture_format format, dm_texture_format internal_format);

dm_image* dm_image_get(const char* name);

#endif