#include "dm_opengl_renderer.h"

#if DM_OPENGL

#include "dm_logger.h"
#include "platform/dm_platform.h"
#include "dm_assert.h"
#include "dm_mem.h"
#include <stdio.h>
#include <stdlib.h>

GLenum dm_buffer_to_opengl_buffer(dm_buffer_type dm_type);
GLenum dm_usage_to_opengl_draw(dm_buffer_usage dm_usage);
GLenum dm_data_to_opengl_data(dm_buffer_data_type dm_data);
GLenum dm_shader_to_opengl_shader(dm_shader_type dm_type);

GLenum dm_blend_eq_to_opengl_func(dm_blend_equation eq);
GLenum dm_blend_func_to_opengl_func(dm_blend_func func);
GLenum dm_depth_eq_to_opengl_func(dm_depth_equation eq);
GLenum dm_stencil_eq_to_opengl_func(dm_stencil_equation eq);
GLenum dm_cull_to_opengl_cull(dm_cull_mode cull);
GLenum dm_wind_top_opengl_wind(dm_winding_order winding);

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

bool dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data, dm_render_pipeline* pipeline)
{
    buffer->internal_buffer = (dm_internal_buffer*)dm_alloc(sizeof(dm_internal_buffer));
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

    if (internal_buffer->type == DM_BUFFER_TYPE_VERTEX) 
    {
        glDeleteVertexArrays(1, &internal_buffer->vao);
        glCheckError();
    }
    glDeleteBuffers(1, &internal_buffer->id);
    glCheckError();
    dm_free(buffer->internal_buffer);
}

void dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();
}

bool dm_renderer_create_shader_impl(dm_shader* shader)
{
    shader->internal_shader = (dm_internal_shader*)dm_alloc(sizeof(dm_internal_shader));
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    GLuint vertex_shader = dm_opengl_compile_shader(shader->vertex_desc);
    GLuint frag_shader = dm_opengl_compile_shader(shader->pixel_desc);

    DM_LOG_DEBUG("Linking shader...");
    internal_shader->id = glCreateProgram();
    glAttachShader(internal_shader->id, vertex_shader);
    glCheckError();
    glAttachShader(internal_shader->id, frag_shader);
    glCheckError();
    glLinkProgram(internal_shader->id);
    glCheckError();

    if(!dm_opengl_validate_program(internal_shader->id))
    {
        DM_LOG_FATAL("Failed to validate OpenGL shader!");
        return false;
    }

    glDetachShader(internal_shader->id, vertex_shader);
    glCheckError();
    glDetachShader(internal_shader->id, frag_shader);
    glCheckError();

    glDeleteShader(vertex_shader);
    glCheckError();
    glDeleteShader(frag_shader);
    glCheckError();

    return true;
}

void dm_renderer_delete_shader_impl(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glDeleteProgram(internal_shader->id);
    glCheckError();
    dm_free(shader->internal_shader);
}

void dm_renderer_bind_shader_impl(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glUseProgram(internal_shader->id);
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
        glBlendFunc(src, dest);
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

    // shader
    dm_shader* shader = dm_renderer_get_shader(pipeline->raster_desc.shader);
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;
    if (!internal_shader) return false;
    glUseProgram(internal_shader->id);

    // vao
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;
    if (!internal_pipe->vao)
    {
        DM_LOG_FATAL("Vertex Array Object for pipeline is invalid!");
        return false;
    }
    glBindVertexArray(internal_pipe->vao);

    return false;
}

void dm_renderer_bind_vertex_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(GL_ARRAY_BUFFER, internal_buffer->id);
}

void dm_renderer_bind_index_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, internal_buffer->id);
}

void dm_renderer_set_viewport_impl(dm_viewport* viewport)
{
    glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
}

void dm_renderer_clear_impl(dm_color* clear_color)
{
    glClearColor(
        clear_color->x,
        clear_color->y,
        clear_color->z,
        clear_color->w
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// shader stuff

/*
uses dm_shader_desc to compile a glsl shader from file
*/
GLuint dm_opengl_compile_shader(dm_shader_desc desc)
{
    GLenum shader_type = dm_shader_to_opengl_shader(desc.type);
    GLuint shader = glCreateShader(shader_type);
    glCheckError();

    DM_LOG_DEBUG("Compiling shader: %s", desc.path);

    FILE* file = fopen(desc.path, "r");
    DM_ASSERT_MSG(file, "Could not fopen file: %s", desc.path);

    // determine size of memory to allocate to the buffer
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* string = dm_alloc(length+1);
    DM_ASSERT(string);
    
    // instead of setting string[length] to nul, we instead
    // find the last character that isn't garbage and set that
    // to nul. otherwise there could be garbage in between and
    // cause a seg fault
    //fread(string, length, 1, file);
    size_t read_count = fread(string, 1, length, file);
    string[read_count] = '\0';
    fclose(file);
    DM_ASSERT(string);
    
    const char* source = string;
    GLint l = (GLint)length;
    glShaderSource(shader, 1, &source, &l);
    glCheckError();
    glCompileShader(shader);
    glCheckError();

    DM_ASSERT(dm_opengl_validate_shader(shader));

    dm_free(string);

    return shader;
}

bool dm_opengl_validate_shader(GLuint shader)
{
    GLint result = -1;
    int length = -1;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glCheckError();
    
    if (result!=GL_TRUE)
    {
        GLchar message[512];
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        glCheckError();
        glGetShaderInfoLog(shader, sizeof(message), &length, message);
        glCheckError();
        DM_LOG_FATAL("%s", message);

        glDeleteShader(shader);
        glCheckError();

        return false;
    }

    return true;
}

bool dm_opengl_validate_program(GLuint program)
{
    GLint result = -1;
    int length = -1;

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glCheckError();

    if (result==GL_FALSE)
    {
        DM_LOG_ERROR("OpenGL Error: %d", glGetError());
        char message[512];
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        glCheckError();
        glGetProgramInfoLog(program, length, NULL, message);
        glCheckError();
        DM_LOG_FATAL("%s", message);

        glDeleteProgram(program);
        glCheckError();

        return false;
    }

    return true;
}

GLenum dm_buffer_to_opengl_buffer(dm_buffer_type dm_type)
{
    switch (dm_type)
    {
    case DM_BUFFER_TYPE_VERTEX: return GL_ARRAY_BUFFER;
    case DM_BUFFER_TYPE_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
    default:
        DM_LOG_FATAL("Unknown buffer type!");
        return DM_BUFFER_TYPE_UNKNOWN;
    }
}

GLenum dm_usage_to_opengl_draw(dm_buffer_usage dm_usage)
{
    switch (dm_usage)
    {
    case DM_BUFFER_USAGE_DEFAULT:
    case DM_BUFFER_USAGE_STATIC: return GL_STATIC_DRAW;
    case DM_BUFFER_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW;
    default:
        DM_LOG_FATAL("Unknown buffer usage type!");
        return DM_BUFFER_USAGE_UNKNOWN;
    }
}

GLenum dm_data_to_opengl_data(dm_buffer_data_type dm_data)
{
    switch (dm_data)
    {
    case DM_BUFFER_DATA_FLOAT: return GL_FLOAT;
    case DM_BUFFER_DATA_INT: return GL_INT;
    default:
        DM_LOG_FATAL("Unknown buffer data type!");
        return DM_BUFFER_DATA_UNKNOWN;
    }
}

GLenum dm_shader_to_opengl_shader(dm_shader_type dm_type)
{
    switch (dm_type)
    {
    case DM_SHADER_TYPE_VERTEX: return GL_VERTEX_SHADER;
    case DM_SHADER_TYPE_PIXEL: return GL_FRAGMENT_SHADER;
    default:
        DM_LOG_FATAL("Unknown shader type!");
        return DM_SHADER_TYPE_UNKNOWN;
    }
}

GLenum dm_blend_eq_to_opengl_func(dm_blend_equation eq)
{
    switch (eq)
    {
    case DM_BLEND_EQUATION_ADD: return GL_FUNC_ADD;
    case DM_BLEND_EQUATION_SUBTRACT: return GL_FUNC_SUBTRACT;
    case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
    case DM_BLEND_EQUATION_MIN: return GL_MIN;
    case DM_BLEND_EQUATION_MAX: return GL_MAX;
    default:
        DM_LOG_FATAL("Unknown blend function!");
        return DM_BLEND_EQUATION_UNKNOWN;
    }
}

GLenum dm_blend_func_to_opengl_func(dm_blend_func func)
{
    switch (func)
    {
    case DM_BLEND_FUNC_ZERO: return GL_ZERO;
    case DM_BLEND_FUNC_ONE: return GL_ONE;
    case DM_BLEND_FUNC_SRC_COLOR: return GL_SRC_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
    case DM_BLEND_FUNC_DST_COLOR: return GL_DST_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
    case DM_BLEND_FUNC_SRC_ALPHA: return GL_SRC_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    case DM_BLEND_FUNC_DST_ALPHA: return GL_DST_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
    case DM_BLEND_FUNC_CONST_COLOR: return GL_CONSTANT_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
    case DM_BLEND_FUNC_CONST_ALPHA: return GL_CONSTANT_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
    default:
        DM_LOG_FATAL("Unknown blend function!");
        return DM_BLEND_FUNC_UNKNOWN;
    }
}

GLenum dm_depth_eq_to_opengl_func(dm_depth_equation eq)
{
    switch (eq)
    {
    case DM_DEPTH_EQUATION_ALWAYS: return GL_ALWAYS;
    case DM_DEPTH_EQUATION_NEVER: return GL_NEVER;
    case DM_DEPTH_EQUATION_EQUAL: return GL_EQUAL;
    case DM_DEPTH_EQUATION_NOTEQUAL: return GL_NOTEQUAL;
    case DM_DEPTH_EQUATION_LESS: return GL_LESS;
    case DM_DEPTH_EQUATION_LEQUAL: return GL_LEQUAL;
    case DM_DEPTH_EQUATION_GREATER: return GL_GREATER;
    case DM_DEPTH_EQUATION_GEQUAL: return GL_GEQUAL;
    default:
        DM_LOG_FATAL("Unknown depth function!");
        return DM_DEPTH_EQUATION_UNKNOWN;
    }
}

GLenum dm_stencil_eq_to_opengl_func(dm_stencil_equation eq)
{
    switch (eq)
    {
    case DM_STENCIL_EQUATION_ALWAYS: return GL_ALWAYS;
    case DM_STENCIL_EQUATION_NEVER: return GL_NEVER;
    case DM_STENCIL_EQUATION_EQUAL: return GL_EQUAL;
    case DM_STENCIL_EQUATION_NOTEQUAL: return GL_NOTEQUAL;
    case DM_STENCIL_EQUATION_LESS: return GL_LESS;
    case DM_STENCIL_EQUATION_LEQUAL: return GL_LEQUAL;
    case DM_STENCIL_EQUATION_GREATER: return GL_GREATER;
    case DM_STENCIL_EQUATION_GEQUAL: return GL_GEQUAL;
    default:
        DM_LOG_FATAL("Unknown stencil function!");
        return DM_DEPTH_EQUATION_UNKNOWN;
    }
}

GLenum dm_cull_to_opengl_cull(dm_cull_mode cull)
{
    switch (cull)
    {
    case DM_CULL_FRONT: return GL_FRONT;
    case DM_CULL_BACK: return GL_BACK;
    case DM_CULL_FRONT_BACK: return GL_FRONT_AND_BACK;
    default:
        DM_LOG_FATAL("Unknown culling mode!");
        return DM_CULL_UNKNOWN;
    }
}

GLenum dm_wind_top_opengl_wind(dm_winding_order winding)
{
    switch (winding)
    {
    case DM_WINDING_CLOCK: return GL_CW;
    case DM_WINDING_COUNTER_CLOCK: return GL_CCW;
    default:
        DM_LOG_FATAL("Unkown winding order!");
        return DM_WINDING_UNKNOWN;
    }
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