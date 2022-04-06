#include "dm_image.h"
#include "structures/dm_map.h"
#include "core/dm_logger.h"
#include <stdio.h>
#include <stb_image/stb_image.h>

dm_map* image_map = NULL;

bool dm_create_texture_impl(dm_image* image);
void dm_destroy_texture_impl(dm_image* image);

void dm_image_map_init()
{
	image_map = dm_map_create(DM_MAP_KEY_STRING, sizeof(dm_image), 0);
}

void dm_image_map_destroy()
{
    dm_for_map_item(image_map)
	{
        dm_image* image = item->value;
        dm_destroy_texture_impl(image);
        stbi_image_free(image->data);
	}
	dm_map_destroy(image_map);
}

bool dm_load_image(dm_image_desc desc)
{
    dm_image image = {0};
    image.desc = desc;
    
    DM_LOG_DEBUG("Loading image: %s", image.desc.path);
    
    stbi_set_flip_vertically_on_load(image.desc.flip);
    
    int num_channels = 0;
#if defined DM_DIRECTX || defined DM_METAL
    num_channels = STBI_rgb_alpha;
#endif
    
    unsigned char* data = stbi_load(image.desc.path, &image.desc.width, &image.desc.height, &image.desc.n_channels, num_channels);
    
    if(!data)
    {
        DM_LOG_FATAL("Failed to load image: %s", image.desc.path);
        return false;
    }
    image.data = data;
    
    if(!dm_create_texture_impl(&image)) return false;
    
    dm_map_insert(image_map, (void*)image.desc.path, &image);
    
    return true;
}

dm_image* dm_image_get(const char* name)
{
	return (dm_image*)dm_map_get(image_map, (void*)name);
}