#include "dm_image.h"
#include "structures/dm_map.h"
#include "core/dm_logger.h"
#include <stdio.h>
#include <stb_image/stb_image.h>

dm_map* image_map = NULL;

void dm_image_map_init()
{
	image_map = dm_map_create(sizeof(dm_image), 0);
}

void dm_image_map_destroy()
{
	for(uint32_t i=0; i<image_map->capacity; i++)
	{
		if(image_map->items[i])
		{
			dm_image* image = (dm_image*)image_map->items[i]->value;
			stbi_image_free(image->data);
		}
	}
	dm_map_destroy(image_map);
}

bool dm_images_load(dm_image_desc* image_descs, int num_descs)
{
	if(image_descs)
	{
		for(uint32_t i=0; i<num_descs; i++)
		{
			dm_image image = { 0 };
			image.desc = image_descs[i];

			DM_LOG_DEBUG("Loading image: %s", image.desc.path);

			stbi_set_flip_vertically_on_load(image.desc.flip);

			int num_channels;
#if defined DM_DIRECTX || defined DM_METAL
			num_channels = STBI_rgb_alpha;
#else
			num_channels = 0;
#endif
			unsigned char* data = stbi_load(image.desc.path, &image.desc.width, &image.desc.height, &image.desc.n_channels, num_channels);
			if(!data)
			{
				DM_LOG_FATAL("Failed to load image: %s", image.desc.path);
				return false;
			}
			image.data = data;
			dm_map_insert(image_map, image.desc.path, &image);
		}
	}

	return true;
}

dm_image* dm_image_get(const char* name)
{
	return (dm_image*)dm_map_get(image_map, name);
}