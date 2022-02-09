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
#include "dm_opengl_render_pass.h"

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

    glViewport(renderer_data->viewport.x, renderer_data->viewport.y, renderer_data->viewport.width, renderer_data->viewport.height);

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_platform_shutdown_opengl();
}

bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_STENCIL_TEST);
    //glDisable(GL_BLEND);

    dm_platform_swap_buffers();

    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    pipeline->internal_pipeline = dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

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

    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* interanl_pipe = pipeline->internal_pipeline;
    
    dm_opengl_delete_buffer(pipeline->vertex_buffer);
    dm_opengl_delete_buffer(pipeline->index_buffer);
    dm_opengl_delete_buffer(pipeline->inst_buffer);

    // textures
    for (uint32_t i = 0; i < pipeline->render_packet.image_paths->count; i++)
    {
        dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
        dm_image* image = dm_image_get(key->string);
        dm_opengl_destroy_texture(image);
    }

    dm_free(pipeline->internal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;

    /*
    // buffers
    */
    if (!dm_opengl_create_buffer(pipeline->vertex_buffer, vb_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->index_buffer, ib_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->inst_buffer, NULL)) return false;

    // textures
    //dm_internal_shader* internal_shader = pipeline->raster_desc.shader->internal_shader;
    //for(uint32_t i=0; i<pipeline->render_packet.image_paths->count; i++)
    //{
    //    dm_string* key = dm_list_at(pipeline->render_packet.image_paths, i);
    //    dm_image* image = dm_image_get(key->string);
    //    if (!dm_opengl_create_texture(image, i, internal_shader->id)) return false;
    //}

    return true;
}

// render pass stuff

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    return dm_opengl_create_render_pass(render_pass, v_layout, pipeline);
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
    dm_opengl_destroy_render_pass(render_pass);
}

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
    return dm_opengl_begin_render_pass(render_pass);
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
    
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
    default: 
        DM_LOG_ERROR("Haven't implemented this bind buffer type yet!");
        return false;
    }

    return true;
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
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

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    glDrawArrays(internal_pass->primitive, start, count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    glDrawElements(internal_pass->primitive, num_indices, GL_UNSIGNED_INT, index_offset);
    glCheckError();
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;

    glDrawElementsInstanced(internal_pass->primitive, num_indices, GL_UNSIGNED_INT, 0, num_insts);
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