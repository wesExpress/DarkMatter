#include "dm_opengl_renderer.h"

#if DM_OPENGL

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
#include "dm_opengl_enum_conversion.h"
#include <stdio.h>
#include <stdlib.h>

GLuint dm_opengl_compile_shader(dm_shader_desc desc);
bool dm_opengl_validate_shader(GLuint shader);
bool dm_opengl_validate_program(GLuint program);

void dm_renderer_bind_buffer_impl(dm_buffer* buffer);

bool dm_renderer_init_impl(dm_renderer_data* renderer_data)
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, renderer_data->width, renderer_data->height);
    
    // built-in shaders
    

    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_platform_shutdown_opengl();
}

bool dm_renderer_resize_impl(int new_width, int new_height)
{
    glViewport(0, 0, new_width, new_height);

    return true;
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{
    glClearColor(
        renderer_data->clear_color.x,
        renderer_data->clear_color.y,
        renderer_data->clear_color.z,
        renderer_data->clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    dm_camera_update_view_proj(&renderer_data->camera);

    dm_platform_swap_buffers();
}

void dm_renderer_draw_arrays_impl(int first, size_t count)
{
    glDrawArrays(GL_TRIANGLES, (GLint)first, (GLsizei)count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(int num, int offset)
{
    glDrawElements(GL_TRIANGLES, num, GL_UNSIGNED_INT, 0);
    glCheckError();
}

void dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    pipeline->interal_pipeline = (dm_internal_pipeline*)dm_alloc(sizeof(dm_internal_pipeline));
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

    glGenVertexArrays(1, &internal_pipe->vao);
    glCheckError();
    internal_pipe->vao_init = false;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* interanl_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
    glDeleteVertexArrays(1, &interanl_pipe->vao);
    interanl_pipe->vao_init = false;

    dm_free(pipeline->interal_pipeline);
}

// render pass stuff

void dm_renderer_begin_renderpass_impl()
{

}

void dm_renderer_end_rederpass_impl()
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    // blending
    if (pipeline->blend_desc.is_enabled)
    {
        GLenum func = dm_blend_eq_to_opengl_func(pipeline->blend_desc.equation);
        if (func == DM_BLEND_EQUATION_UNKNOWN) return false;

        GLenum src = dm_blend_func_to_opengl_func(pipeline->blend_desc.src);
        if (src == DM_BLEND_FUNC_UNKNOWN) return false;
        GLenum dest = dm_blend_func_to_opengl_func(pipeline->blend_desc.dest);
        if (dest == DM_BLEND_FUNC_UNKNOWN) return false;

        glEnable(GL_BLEND);
        glBlendEquation(func);
        glCheckError();
        glBlendFunc(src, dest);
        glCheckError();
    }
    else
    {
        glDisable(GL_BLEND);
    }

    // depth testing
    if (pipeline->depth_desc.is_enabled)
    {
        GLenum func = dm_depth_eq_to_opengl_func(pipeline->depth_desc.equation);
        if (func == DM_DEPTH_EQUATION_UNKNOWN) return false;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(func);
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
        GLenum func = dm_stencil_eq_to_opengl_func(pipeline->stencil_desc.equation);
        if (func == DM_STENCIL_EQUATION_UNKNOWN) return false;

        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }

    // face culling
    GLenum cull = dm_cull_to_opengl_cull(pipeline->raster_desc.cull_mode);
    if (cull == DM_CULL_UNKNOWN) return false;
    glEnable(GL_CULL_FACE);
    glCullFace(cull);

    GLenum winding = dm_wind_top_opengl_wind(pipeline->raster_desc.winding_order);
    if (winding == DM_WINDING_UNKNOWN) return false;
    glFrontFace(winding);
    glCheckError();

    // shader
    dm_shader* shader = dm_renderer_get_shader(pipeline->raster_desc.shader);
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;
    if (!internal_shader) return false;
    glUseProgram(internal_shader->id);
    glCheckError();

    // vao
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
    if (!internal_pipe->vao)
    {
        DM_LOG_FATAL("Vertex Array Object for pipeline is invalid!");
        return false;
    }
    glBindVertexArray(internal_pipe->vao);
    glCheckError();

    return false;
}

void dm_renderer_bind_vertex_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(GL_ARRAY_BUFFER, internal_buffer->id);
    glCheckError();
}

void dm_renderer_bind_index_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, internal_buffer->id);
    glCheckError();
}

void dm_renderer_set_viewport_impl(dm_viewport* viewport)
{
    glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
    glCheckError();
}

void dm_renderer_clear_impl(dm_color* clear_color)
{
    glClearColor(
        clear_color->x,
        clear_color->y,
        clear_color->z,
        clear_color->w
    );
    glCheckError();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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