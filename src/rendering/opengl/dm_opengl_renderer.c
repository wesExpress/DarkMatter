#include "dm_opengl_renderer.h"

#ifdef DM_OPENGL

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
#include "dm_opengl_enum_conversion.h"
#include "dm_opengl_shader.h"
#include "dm_opengl_buffer.h"
#include <stdio.h>
#include <stdlib.h>

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    if (!dm_platform_init_opengl())
    {
        return false;
    }

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
    dm_camera_update_view_proj(&renderer_data->camera);

    dm_platform_swap_buffers();

    return true;
}

void dm_renderer_draw_arrays_impl(int first, size_t count)
{
    glDrawArrays(GL_TRIANGLES, (GLint)first, (GLsizei)count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    glDrawElements(internal_pipe->primitive, pipeline->render_packet.count, GL_UNSIGNED_INT, (void*)(uintptr_t)pipeline->render_packet.offset);
    glCheckError();
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    pipeline->interal_pipeline = (dm_internal_pipeline*)dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    glGenVertexArrays(1, &internal_pipe->vao);
    glCheckError();

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
        internal_pipe->depth_func = dm_depth_eq_to_opengl_func(pipeline->depth_desc.equation);
        if (internal_pipe->depth_func == DM_DEPTH_EQUATION_UNKNOWN) return false;
    }

    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        internal_pipe->stencil_func = dm_stencil_eq_to_opengl_func(pipeline->stencil_desc.equation);
        if (internal_pipe->stencil_func == DM_STENCIL_EQUATION_UNKNOWN) return false;
    }

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
    dm_internal_pipeline* interanl_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
    glDeleteVertexArrays(1, &interanl_pipe->vao);
    glCheckError();

    dm_opengl_delete_buffer(pipeline->render_packet.vertex_buffer);
    dm_opengl_delete_buffer(pipeline->render_packet.index_buffer);
    dm_opengl_delete_shader(pipeline->raster_desc.shader);

    dm_free(pipeline->interal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(dm_buffer_desc vb_desc, void* vb_data, dm_buffer_desc ib_desc, void* ib_data, dm_shader_desc vs_desc, dm_shader_desc ps_desc, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    glBindVertexArray(internal_pipe->vao);
    glCheckError();

    /*
    // shader
    */
    dm_shader* shader = pipeline->raster_desc.shader;
    shader->vertex_desc = vs_desc;
    shader->pixel_desc = ps_desc;

    if (!dm_opengl_create_shader(shader)) return false;
    pipeline->raster_desc.shader = shader;

    /*
    // buffers
    */
    dm_buffer* vertex_buffer = pipeline->render_packet.vertex_buffer;
    dm_buffer* index_buffer = pipeline->render_packet.index_buffer;

    vertex_buffer->desc = vb_desc;
    index_buffer->desc = ib_desc;

    if (!dm_opengl_create_buffer(vertex_buffer, vb_data)) return false;
    if (!dm_opengl_create_buffer(index_buffer, ib_data)) return false;

    /*
    // vertex attributes/layout
    */
    dm_opengl_bind_buffer(vertex_buffer);
    dm_opengl_bind_buffer(index_buffer);

    for (int i = 0; i < v_layout.num; i++)
    {
        dm_vertex_attrib_desc attrib_desc = v_layout.attributes[i];

        GLenum data_t = dm_vertex_data_t_to_opengl(attrib_desc.data_t);
        if (data_t == DM_VERTEX_DATA_T_UNKNOWN) return false;

        glVertexAttribPointer(i, attrib_desc.size, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)attrib_desc.offset);
        glCheckError();
        glEnableVertexAttribArray(i);
        glCheckError();
    }

    glBindVertexArray(0);

    pipeline->render_packet.vertex_buffer = vertex_buffer;
    pipeline->render_packet.index_buffer = index_buffer;

    return true;
}

// render pass stuff

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline)
{
    
}

void dm_renderer_end_rederpass_impl()
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_STENCIL_TEST);
    //glDisable(GL_BLEND);
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    // blending
    if (pipeline->blend_desc.is_enabled)
    {
        glEnable(GL_BLEND);
        glBlendEquation(internal_pipe->blend_func);
        glCheckError();
        glBlendFunc(internal_pipe->blend_src, internal_pipe->blend_dest);
        glCheckError();
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
        glCheckError();
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
    //glEnable(GL_CULL_FACE);
    //glCullFace(internal_pipe->cull);
    //glCheckError();

    //glFrontFace(internal_pipe->winding);
   // glCheckError();

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
    glCheckError();

    dm_opengl_bind_buffer(pipeline->render_packet.vertex_buffer);
    dm_opengl_bind_buffer(pipeline->render_packet.index_buffer);

    return true;
}

void dm_renderer_set_viewport_impl(dm_viewport* viewport)
{
    glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
    glCheckError();
}

void dm_renderer_clear_impl(dm_render_pipeline* pipeline, dm_color* clear_color)
{
    glClearColor(clear_color->x, clear_color->y, clear_color->z, clear_color->w);
    glCheckError();
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
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