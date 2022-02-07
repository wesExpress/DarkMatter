#include "dm_opengl_renderer.h"

#ifdef DM_OPENGL

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"
#include "platform/dm_platform.h"
#include "dm_opengl_enum_conversion.h"
#include "dm_opengl_shader.h"
#include "dm_opengl_buffer.h"
#include "dm_opengl_texture.h"
#include "rendering/dm_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing OpenGL render backend...");

    if (!dm_platform_init_opengl()) return false;

    DM_LOG_INFO("OpenGL Info:");
    DM_LOG_INFO("       Vendor  : %s", glGetString(GL_VENDOR));
    DM_LOG_INFO("       Renderer: %s", glGetString(GL_RENDERER));
    DM_LOG_INFO("       Version : %s", glGetString(GL_VERSION));
    
#ifndef __APPLE__
#if DM_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif
#endif

    glViewport(0, 0, renderer_data->width, renderer_data->height);

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_platform_shutdown_opengl();
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{

}

bool dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    dm_platform_swap_buffers();

    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    pipeline->internal_pipeline = dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

    glGenVertexArrays(1, &internal_pipe->vao);
    glCheckErrorReturn();

    if (pipeline->blend_desc.is_enabled)
    {
        internal_pipe->blend_func = dm_blend_eq_to_opengl_func(pipeline->blend_desc.equation);
        if (internal_pipe->blend_func == DM_BLEND_EQUATION_UNKNOWN) return false;

        internal_pipe->blend_src = dm_blend_func_to_opengl_func(pipeline->blend_desc.src);
        if (internal_pipe->blend_src == DM_BLEND_FUNC_UNKNOWN) return false;
        internal_pipe->blend_dest = dm_blend_func_to_opengl_func(pipeline->blend_desc.dest);
        if (internal_pipe->blend_dest == DM_BLEND_FUNC_UNKNOWN) return false;
    }

    if (pipeline->depth_desc.is_enabled)
    {
        internal_pipe->depth_func = dm_comp_to_opengl_comp(pipeline->depth_desc.comparison);
        if (internal_pipe->depth_func == DM_COMPARISON_UNKNOWN) return false;
    }

    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        internal_pipe->stencil_func = dm_comp_to_opengl_comp(pipeline->stencil_desc.comparison);
        if (internal_pipe->stencil_func == DM_COMPARISON_UNKNOWN) return false;
    }

    // sampler
    GLenum min_filter = dm_filter_to_opengl_filter(pipeline->sampler_desc.filter);
    if (min_filter == DM_FILTER_UNKNOWN) return false;
    GLenum mag_filter = dm_filter_to_opengl_filter(pipeline->sampler_desc.filter);
    if (mag_filter == DM_FILTER_UNKNOWN) return false;
    GLenum s_wrap = dm_texture_mode_to_opengl_mode(pipeline->sampler_desc.u);
    if (s_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    GLenum t_wrap = dm_texture_mode_to_opengl_mode(pipeline->sampler_desc.v);
    if (t_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glCheckErrorReturn();

    // Rasterizer info
    // face culling
    internal_pipe->cull = dm_cull_to_opengl_cull(pipeline->raster_desc.cull_mode);
    if (internal_pipe->cull == DM_CULL_UNKNOWN) return false;

    internal_pipe->winding = dm_wind_top_opengl_wind(pipeline->raster_desc.winding_order);
    if (internal_pipe->winding == DM_WINDING_UNKNOWN) return false;

    // primitive
    internal_pipe->primitive = dm_topology_to_opengl_primitive(pipeline->raster_desc.primitive_topology);
    if (internal_pipe->primitive == DM_TOPOLOGY_UNKNOWN) return false;

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* interanl_pipe = pipeline->internal_pipeline;
    glDeleteVertexArrays(1, &interanl_pipe->vao);
    glCheckError();

    dm_internal_constant_buffer* internal_cb = pipeline->view_proj->internal_buffer;
    dm_free(internal_cb->data, sizeof(dm_mat4), DM_MEM_RENDERER_BUFFER);
    
    dm_opengl_delete_buffer(pipeline->vertex_buffer);
    dm_opengl_delete_buffer(pipeline->index_buffer);
    dm_opengl_delete_buffer(pipeline->inst_buffer);
    dm_free(pipeline->view_proj->internal_buffer, sizeof(dm_internal_constant_buffer), DM_MEM_RENDERER_BUFFER);
    dm_opengl_delete_shader(pipeline->raster_desc.shader);

    // textures
    for (uint32_t i = 0; i < pipeline->render_packet.image_paths->count; i++)
    {
        dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        dm_image* image = dm_image_get(key->string);
        dm_opengl_destroy_texture(image);
    }

    dm_free(pipeline->internal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, void* mvp_data, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

    glBindVertexArray(internal_pipe->vao);
    glCheckErrorReturn();

    /*
    // shader
    */
    if (!dm_opengl_create_shader(pipeline->raster_desc.shader)) return false;

    /*
    // buffers
    */
    if (!dm_opengl_create_buffer(pipeline->vertex_buffer, vb_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->index_buffer, ib_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->inst_buffer, NULL)) return false;

    /*
    // vertex attributes/layout
    */
    uint32_t count = 0;
    dm_opengl_bind_buffer(pipeline->index_buffer);

    for (int i = 0; i < v_layout.num; i++)
    {
        dm_vertex_attrib_desc attrib_desc = v_layout.attributes[i];

        switch(attrib_desc.attrib_class)
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
        if((attrib_desc.data_t==DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t==DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            if(attrib_desc.data_t==DM_VERTEX_DATA_T_MATRIX_INT) data_t = GL_INT;
            else if(attrib_desc.data_t==DM_VERTEX_DATA_T_MATRIX_FLOAT) data_t = GL_FLOAT;
            else 
            {
                DM_LOG_FATAL("Unknown vertex data type!");
                return false;
            }

            for(uint32_t j=0; j<attrib_desc.count; j++)
            {
                size_t new_offset;;
                if(data_t==GL_INT) new_offset = j * (attrib_desc.offset + sizeof(int) * attrib_desc.count);
                else if(data_t==GL_FLOAT) new_offset = j * (attrib_desc.offset + sizeof(float) * attrib_desc.count);

                glVertexAttribPointer(count, attrib_desc.count, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)new_offset);
                glCheckErrorReturn();
                glEnableVertexAttribArray(count);
                glCheckErrorReturn();
                if(attrib_desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
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
            count++;
        }
    }

    glBindVertexArray(0);

    // constant buffers
    dm_internal_shader* internal_shader = pipeline->raster_desc.shader->internal_shader;
    pipeline->view_proj->internal_buffer = dm_alloc(sizeof(dm_internal_constant_buffer), DM_MEM_RENDERER_BUFFER);
    dm_internal_constant_buffer* internal_cb = pipeline->view_proj->internal_buffer;
    if (!dm_opengl_find_uniform_loc(internal_shader->id, pipeline->view_proj->desc.name, &internal_cb->location)) return false;
    internal_cb->data = dm_alloc(sizeof(dm_mat4), DM_MEM_RENDERER_BUFFER);
    dm_memcpy(internal_cb->data, mvp_data, sizeof(dm_mat4));

    // textures

    for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
    {
        dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        dm_image* image = dm_image_get(key->string);
        if (!dm_opengl_create_texture(image, i, internal_shader->id)) return false;
    }

    return true;
}

// render pass stuff

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline)
{
    
}

void dm_renderer_end_rederpass_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if(pipeline->depth_desc.is_enabled) glDisable(GL_DEPTH_TEST);
    //glDisable(GL_STENCIL_TEST);
    //glDisable(GL_BLEND);
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->internal_pipeline;

    // blending
    if (pipeline->blend_desc.is_enabled)
    {
        glEnable(GL_BLEND);
        glBlendEquation(internal_pipe->blend_func);
        glCheckErrorReturn();
        glBlendFunc(internal_pipe->blend_src, internal_pipe->blend_dest);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_BLEND);
    }

    // depth testing
    if (pipeline->depth_desc.is_enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(internal_pipe->depth_func);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    // stencil testing
    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }

    // face culling
    glEnable(GL_CULL_FACE);
    glCullFace(internal_pipe->cull);
    glCheckErrorReturn();

    //glFrontFace(internal_pipe->winding);
    //glCheckErrorReturn();

    // wireframe
    if (pipeline->wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // shader
    dm_opengl_bind_shader(pipeline->raster_desc.shader);

    // vao
    glBindVertexArray(internal_pipe->vao);
    glCheckErrorReturn();

    // view proj
    dm_opengl_bind_uniform(pipeline->view_proj);

    // TODO: need to change this eventually, this won't work with multiple textures per draw call
    // textures
    for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
    {
        dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        dm_image* image = dm_image_get(key->string);

        if (!dm_opengl_bind_texture(image)) return false;
    }

    return true;
}

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t size)
{
    switch(buffer->desc.type)
    {
    case DM_BUFFER_TYPE_VERTEX:
    {
        dm_internal_buffer* internal_buffer = buffer->internal_buffer;
        glBindBuffer(internal_buffer->type, internal_buffer->id);
        glBufferSubData(internal_buffer->type, 0, size, data);
    } break;
    case DM_BUFFER_TYPE_CONSTANT:
    {
        dm_internal_constant_buffer* internal_cb = buffer->internal_buffer;
        dm_memcpy(internal_cb->data, data, size);
    } break;
    default:
        DM_LOG_ERROR("Haven't implemented this buffer update type!");
        return false;
    }

    return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    switch (buffer->desc.type)
    {
    case DM_BUFFER_TYPE_VERTEX: 
        dm_opengl_bind_buffer(buffer);
        break;
    case DM_BUFFER_TYPE_CONSTANT: return dm_opengl_bind_uniform(buffer);
    default: 
        DM_LOG_ERROR("Haven't implemented this bind buffer type yet!");
        return false;
    }

    return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline)
{
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glCheckError();
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
    glClearColor(clear_color->x, clear_color->y, clear_color->z, clear_color->w);
    glCheckError();

    glClear(GL_COLOR_BUFFER_BIT);
    if(pipeline->depth_desc.is_enabled) glClear(GL_DEPTH_BUFFER_BIT);
    glCheckError();
}

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count)
{
    glDrawArrays(GL_TRIANGLES, (GLint)first, (GLsizei)count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

    //glDrawElements(internal_pipe->primitive, pipeline->render_packet.count, GL_UNSIGNED_INT, (void*)(uintptr_t)pipeline->render_packet.offset);
    glCheckError();
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

    glDrawElementsInstanced(internal_pipe->primitive, num_indices, GL_UNSIGNED_INT, 0, num_insts);
    glCheckError();
}

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char* error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        DM_LOG_ERROR("%s | %s (%d)", error, file, line);
    }
    return errorCode;
}

#endif