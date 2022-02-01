#ifndef __DM_IMAGE_H__
#define __DM_IMAGE_H__

#include "dm_render_types.h"
#include <stdbool.h>

void dm_image_map_init();
void dm_image_map_destroy();
bool dm_images_load(dm_image_desc* image_descs, int num_descs);
dm_image* dm_image_get(const char* name);

#endif