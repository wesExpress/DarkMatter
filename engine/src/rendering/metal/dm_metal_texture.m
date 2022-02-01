#include "dm_metal_texture.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

bool dm_metal_create_texture(dm_image* image, dm_metal_renderer* renderer)
{
    @autoreleasepool
    {
        image->internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
        dm_internal_texture* internal_texture = image->internal_texture;

        MTLTextureDescriptor* texture_desc = [[MTLTextureDescriptor alloc] init];

        texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_desc.width = image->desc.width;
        texture_desc.height = image->desc.height;

        internal_texture->texture = [renderer->device newTextureWithDescriptor:texture_desc];
        if(!internal_texture)
        {
            DM_LOG_FATAL("Could not create metal texture from image: %s", image->desc.path);
            return false;
        }

        MTLRegion region = MTLRegionMake2D(0, 0, image->desc.width, image->desc.height);

        NSUInteger bytes_per_row = 4 * image->desc.width;

        [internal_texture->texture replaceRegion: region
                                   mipmapLevel: 0
                                   withBytes: image->data
                                   bytesPerRow: bytes_per_row];
    }

    return true;
}

void dm_metal_destroy_texture(dm_image* image)
{
    dm_free(image->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

#endif