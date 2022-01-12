#include "dm_opengl_renderer.h"

#if DM_OPENGL

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

    glBufferData(internal_buffer->type, buffer->desc.data_size, data, internal_buffer->usage);
    glCheckError();

    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    if (buffer->desc.type == DM_BUFFER_TYPE_VERTEX)
    {
        if (!internal_pipe->vao_init)
        {
            glBindVertexArray(internal_pipe->vao);
            glCheckError();

            for (int i = 0; i < pipeline->vertex_layout.num; i++)
            {
                dm_vertex_attrib_desc attrib_desc = pipeline->vertex_layout.attributes[i];

                glEnableVertexAttribArray(i);

                switch (attrib_desc.attrib)
                {
                case DM_VERTEX_ATTRIB_POS: glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)attrib_desc.offset);
                case DM_VERTEX_ATTRIB_NORM: glVertexAttribPointer(i, 3, GL_INT, GL_TRUE, 3 * sizeof(int), (void*)attrib_desc.offset);
                case DM_VERTEX_ATTRIB_COLOR: glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, GL_FALSE, 4 * sizeof(char), (void*)attrib_desc.offset);
                case DM_VERTEX_ATTRIB_TEX_COORD: glVertexAttribPointer(i, 2, GL_SHORT, GL_TRUE, 2 * sizeof(short), (void*)attrib_desc.offset);
                default:
                    DM_LOG_FATAL("Unknown vertex attribute!");
                    return false;
                }
                glCheckError();
            }

            internal_pipe->vao_init = true;
        }
    }

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