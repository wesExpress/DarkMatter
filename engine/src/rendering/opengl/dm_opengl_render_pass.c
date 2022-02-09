#include "dm_opengl_render_pass.h"

#ifdef DM_OPENGL

#include "dm_opengl_shader.h"
#include "dm_opengl_buffer.h"
#include "dm_opengl_enum_conversion.h"

#include "core/dm_mem.h"
#include "core/dm_logger.h"

bool dm_opengl_create_render_pass(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    render_pass->internal_render_pass = dm_alloc(sizeof(dm_opengl_render_pass), DM_MEM_RENDER_PASS);
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    // face culling
    internal_pass->cull = dm_cull_to_opengl_cull(render_pass->raster_desc.cull_mode);
    if (internal_pass->cull == DM_CULL_UNKNOWN) return false;

    internal_pass->winding = dm_wind_top_opengl_wind(render_pass->raster_desc.winding_order);
    if (internal_pass->winding == DM_WINDING_UNKNOWN) return false;

    // primitive
    internal_pass->primitive = dm_topology_to_opengl_primitive(render_pass->raster_desc.primitive_topology);
    if (internal_pass->primitive == DM_TOPOLOGY_UNKNOWN) return false;

    // sampler
    internal_pass->min_filter = dm_filter_to_opengl_filter(render_pass->sampler_desc.filter);
    if (internal_pass->min_filter == DM_FILTER_UNKNOWN) return false;
    internal_pass->mag_filter = dm_filter_to_opengl_filter(render_pass->sampler_desc.filter);
    if (internal_pass->mag_filter == DM_FILTER_UNKNOWN) return false;
    internal_pass->s_wrap = dm_texture_mode_to_opengl_mode(render_pass->sampler_desc.u);
    if (internal_pass->s_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    internal_pass->t_wrap = dm_texture_mode_to_opengl_mode(render_pass->sampler_desc.v);
    if (internal_pass->t_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;

    // vertex array object
    glGenVertexArrays(1, &internal_pass->vao);
    glCheckErrorReturn();

    // shader
    if (!dm_opengl_create_shader(render_pass->shader)) return false;

    // vertex attributes
    uint32_t count = 0;
    glBindVertexArray(internal_pass->vao);

    dm_opengl_bind_buffer(pipeline->index_buffer);

    for (int i = 0; i < v_layout.num; i++)
    {
        dm_vertex_attrib_desc attrib_desc = v_layout.attributes[i];

        switch (attrib_desc.attrib_class)
        {
        case DM_VERTEX_ATTRIB_CLASS_VERTEX:
        {
            dm_opengl_bind_buffer(pipeline->vertex_buffer);
        } break;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE:
        {
            dm_opengl_bind_buffer(pipeline->inst_buffer);
        } break;
        }

        GLenum data_t;
        if ((attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) data_t = GL_INT;
            else if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT) data_t = GL_FLOAT;
            else
            {
                DM_LOG_FATAL("Unknown vertex data type!");
                return false;
            }

            for (uint32_t j = 0; j < attrib_desc.count; j++)
            {
                size_t new_offset;;
                if (data_t == GL_INT) new_offset = j * (attrib_desc.offset + sizeof(int) * attrib_desc.count);
                else if (data_t == GL_FLOAT) new_offset = j * (attrib_desc.offset + sizeof(float) * attrib_desc.count);

                glVertexAttribPointer(count, attrib_desc.count, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)new_offset);
                glCheckErrorReturn();
                glEnableVertexAttribArray(count);
                glCheckErrorReturn();
                if (attrib_desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
                {
                    glVertexAttribDivisor(count, 1);
                    glCheckErrorReturn();
                }
                count++;
            }
        }
        else
        {
            data_t = dm_vertex_data_t_to_opengl(attrib_desc.data_t);
            if (data_t == DM_VERTEX_DATA_T_UNKNOWN) return false;

            glVertexAttribPointer(count, attrib_desc.count, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)attrib_desc.offset);
            glCheckErrorReturn();
            glEnableVertexAttribArray(count);
            glCheckErrorReturn();
            if (attrib_desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
            {
                glVertexAttribDivisor(count, 1);
                glCheckErrorReturn();
            }
            count++;
        }
    }

    glBindVertexArray(0);

    // uniforms
    for (uint32_t i = 0; i < render_pass->uniforms->capacity; i++)
    {
        if (render_pass->uniforms->items[i])
        {
            dm_uniform* uniform = render_pass->uniforms->items[i]->value;

            if (!dm_opengl_create_uniform(uniform, render_pass->shader)) return false;
        }
    }

    return true;
}

void dm_opengl_destroy_render_pass(dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    glDeleteVertexArrays(1, &internal_pass->vao);
    glCheckError();

    dm_opengl_delete_shader(render_pass->shader);

    for (uint32_t i = 0; i < render_pass->uniforms->capacity; i++)
    {
        if (render_pass->uniforms->items[i])
        {
            dm_uniform* uniform = render_pass->uniforms->items[i]->value;
            dm_opengl_destroy_uniform(uniform);
        }
    }

    dm_free(render_pass->internal_render_pass, sizeof(dm_opengl_render_pass), DM_MEM_RENDER_PASS);
}

bool dm_opengl_begin_render_pass(dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    // vertex array
    glBindVertexArray(internal_pass->vao);
    glCheckErrorReturn();

    // sampler state
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, internal_pass->s_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, internal_pass->t_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, internal_pass->min_filter);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, internal_pass->mag_filter);
    glCheckErrorReturn();

    // face culling
    glEnable(GL_CULL_FACE);
    glCullFace(internal_pass->cull);
    glCheckErrorReturn();

    //glFrontFace(internal_pipe->winding);
    //glCheckErrorReturn();

    // wireframe
    if (render_pass->wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // shader
    dm_opengl_bind_shader(render_pass->shader);

    // uniforms
    for (uint32_t i = 0; i < render_pass->uniforms->capacity; i++)
    {
        if (render_pass->uniforms->items[i])
        {
            dm_uniform* uniform = render_pass->uniforms->items[i]->value;

            if (!dm_opengl_bind_uniform(uniform)) return false;
        }
    }

    return true;
}

#endif