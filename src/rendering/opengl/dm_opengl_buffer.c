#include "dm_opengl_buffer.h"

#ifdef DM_OPENGL

#include "dm_opengl_enum_conversion.h"
#include "core/dm_mem.h"

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data)
{
    buffer->internal_buffer = (dm_internal_buffer*)dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    internal_buffer->type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    if (internal_buffer->type == DM_BUFFER_TYPE_UNKNOWN) return false;
    internal_buffer->usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    if (internal_buffer->usage == DM_BUFFER_USAGE_UNKNOWN) return false;

    glGenBuffers(1, &internal_buffer->id);
    glCheckErrorReturn();

    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckErrorReturn();

    glBufferData(internal_buffer->type, buffer->desc.buffer_size, data, internal_buffer->usage);
    glCheckErrorReturn();

    return true;
}

void dm_opengl_delete_buffer(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glDeleteBuffers(1, &internal_buffer->id);
    glCheckError();
    dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

void dm_opengl_bind_buffer(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();
}

#endif