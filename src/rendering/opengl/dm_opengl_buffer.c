#include "dm_opengl_renderer.h"

#if DM_OPENGL

#include "dm_opengl_buffer.h"
#include "dm_opengl_enum_conversion.h"

bool dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data, dm_render_pipeline* pipeline)
{
    buffer->internal_buffer = (dm_internal_buffer*)dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    internal_buffer->type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    if (internal_buffer->type == DM_BUFFER_TYPE_UNKNOWN) return false;
    internal_buffer->usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    if (internal_buffer->usage == DM_BUFFER_USAGE_UNKNOWN) return false;

    glGenBuffers(1, &internal_buffer->id);
    glCheckError();

    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();

    glBufferData(internal_buffer->type, buffer->desc.size, data, internal_buffer->usage);
    glCheckError();

    return true;
}

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data)
{
    buffer->internal_buffer = (dm_internal_buffer*)dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    internal_buffer->type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    if (internal_buffer->type == DM_BUFFER_TYPE_UNKNOWN) return false;
    internal_buffer->usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    if (internal_buffer->usage == DM_BUFFER_USAGE_UNKNOWN) return false;

    glGenBuffers(1, &internal_buffer->id);
    glCheckError();

    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();

    glBufferData(internal_buffer->type, buffer->desc.size, data, internal_buffer->usage);
    glCheckError();

    return true;
}

void dm_renderer_delete_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glDeleteBuffers(1, &internal_buffer->id);
    glCheckError();
    dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

void dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();
}

#endif