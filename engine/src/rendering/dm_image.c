#include "dm_image.h"
#include "structures/dm_map.h"
#include "core/dm_logger.h"
#include <stdio.h>
#include <stb_image/stb_image.h>
#include "core/dm_mem.h"

dm_map* image_map = NULL;

extern bool dm_create_texture_impl(dm_image* image);
extern void dm_destroy_texture_impl(dm_image* image);

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

bool dm_load_image(const char* path, const char* name, bool flip, dm_texture_format format, dm_texture_format internal_format)
{
    dm_image image = {0};
    
    DM_LOG_DEBUG("Loading image: %s", path);
    
    stbi_set_flip_vertically_on_load(flip);
    
    int width, height;
    int num_channels = 0;
#if defined DM_DIRECTX || defined DM_METAL
    num_channels = STBI_rgb_alpha;
#endif
    
    unsigned char* data = stbi_load(path, &width, &height, &num_channels, num_channels);
    
    if(!data)
    {
        DM_LOG_FATAL("Failed to load image: %s", path);
        return false;
    }
    image.data = data;
    
    image.desc.width = width;
    image.desc.height = height;
    image.desc.n_channels = num_channels;
    image.desc.flip = flip;
    image.desc.format = format;
    image.desc.internal_format = internal_format;
    
    if(!dm_create_texture_impl(&image)) return false;
    
    dm_map_insert(image_map, (void*)name, &image);
    
    return true;
}

dm_image* dm_image_get(const char* name)
{
	return (dm_image*)dm_map_get(image_map, (void*)name);
}