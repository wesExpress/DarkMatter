#include "dm_metal_buffer.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

bool dm_metal_create_buffer(dm_buffer* buffer, void* data, dm_metal_renderer* metal_renderer)
{
    @autoreleasepool
    {
        buffer->internal_buffer = dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
        dm_internal_buffer* internal_buffer = buffer->internal_buffer;

        internal_buffer->buffer = [metal_renderer->device newBufferWithBytes:data
                                                          length:buffer->desc.buffer_size
                                                          options:MTLResourceOptionCPUCacheModeDefault];
        if(!internal_buffer->buffer)
        {
            DM_LOG_FATAL("Could not create metal buffer!");
            return false;
        }
    }

    return true;
}

void dm_metal_destroy_buffer(dm_buffer* buffer)
{
    dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

#endif