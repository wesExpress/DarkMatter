#include "dm_texture.h"
#include "structures/dm_map.h"
#include "core/dm_logger.h"
#include <stdio.h>
#include <stb_image/stb_image.h>

dm_map_t* texture_map = NULL;

void dm_texture_map_init()
{
	texture_map = dm_map_create(sizeof(dm_texture), DM_MAP_DEFAULT_SIZE);
}

void dm_texture_map_destroy()
{
	for(uint32_t i=0; i<texture_map->capacity; i++)
	{
		if(texture_map->items[i])
		{
			dm_texture* texture = (dm_texture*)texture_map->items[i]->value;
			stbi_image_free(texture->data);
		}
	}
	dm_map_destroy(texture_map);
}

bool dm_textures_load(dm_image_desc* image_descs, int num_descs)
{
	if(image_descs)
	{
		for(uint32_t i=0; i<num_descs; i++)
		{
			dm_texture texture = { 0 };
			texture.desc = image_descs[i];

			DM_LOG_INFO("Loading texture: %s", texture.desc.path);

			stbi_set_flip_vertically_on_load(texture.desc.flip);
#ifdef DM_OPENGL
			int num_channels = 0;
#elif defined DM_DIRECTX
			int num_channels = STBI_rgb_alpha;
#endif
			unsigned char* data = stbi_load(texture.desc.path, &texture.desc.width, &texture.desc.height, &texture.desc.n_channels, num_channels);
			if(!data)
			{
				DM_LOG_FATAL("Failed to load image: %s", texture.desc.path);
				return false;
			}
			texture.data = data;
			dm_map_insert(texture_map, texture.desc.path, &texture);
		}
	}

	return true;
}

dm_texture* dm_texture_get(const char* name)
{
	return (dm_texture*)dm_map_get(texture_map, name);
}