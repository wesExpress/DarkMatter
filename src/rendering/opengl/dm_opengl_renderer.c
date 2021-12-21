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

GLuint dm_opengl_compile_shader(dm_shader_desc desc);
bool dm_opengl_validate_shader(GLuint shader);
bool dm_opengl_validate_program(GLuint program);

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

#if DM_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, renderer_data->width, renderer_data->height);

    return true;
}

void dm_renderer_shutdown_impl()
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

    // testing TODO: remove

    //float vertices[] =
    //{
    //    -0.5f, -0.5f, 0.0f,
    //    0.5f, -0.5f, 0.0f,
    //    0.0f, 0.5f, 0.0f
    //};
    //
    //GLuint vbo, vao;
    //glGenBuffers(1, &vbo);
    //glGenVertexArrays(1, &vao);
    //glBindVertexArray(vao);
    //glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(0);
    //
    //const char* vertexShaderSource = "#version 330 core\n"
    //    "layout (location = 0) in vec3 aPos;\n"
    //    "void main()\n"
    //    "{\n"
    //    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    //    "}\0";
    //const char* fragmentShaderSource = "#version 330 core\n"
    //    "out vec4 FragColor;"
    //
    //    "void main()"
    //    "{"
    //        "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);"
    //    "} \0";
    //
    //GLuint vertex_shader;
    //vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    //glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    //glCompileShader(vertex_shader);
    //
    //int  success;
    //char infoLog[512];
    //glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    //
    //if (!success)
    //{
    //    glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
    //    DM_LOG_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);
    //}
    //
    //unsigned int fragmentShader;
    //fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    //glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    //glCompileShader(fragmentShader);
    //
    //glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    //
    //if (!success)
    //{
    //    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    //    DM_LOG_ERROR("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s", infoLog);
    //}
    //
    //GLuint shader_program;
    //shader_program = glCreateProgram();
    //
    //glAttachShader(shader_program, vertex_shader);
    //glAttachShader(shader_program, fragmentShader);
    //glLinkProgram(shader_program);
    //
    //glUseProgram(shader_program);
    //glDeleteShader(vertex_shader);
    //glDeleteShader(fragmentShader);
    //
    //glBindVertexArray(vao);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    //
    //glDeleteVertexArrays(1, &vao);
    //glDeleteBuffers(1, &vbo);
    //glDeleteProgram(shader_program);
}

void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    dm_camera_update_view_proj(&renderer_data->camera);

    dm_platform_swap_buffers();
}

void dm_renderer_draw_arrays_impl(int first, size_t count)
{
    glDrawArrays(GL_TRIANGLES, (GLint)first, (GLsizei)count);
}

void dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data)
{
    buffer->internal_buffer = (dm_internal_buffer*)dm_alloc(sizeof(dm_internal_buffer));
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;

    glGenBuffers(1, &internal_buffer->id);

    internal_buffer->type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    DM_ASSERT(internal_buffer->type != DM_BUFFER_TYPE_UNKNOWN);
    internal_buffer->usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    DM_ASSERT(internal_buffer->usage != DM_BUFFER_USAGE_UNKNOWN);
    internal_buffer->data_type = dm_data_to_opengl_data(buffer->desc.data_type);
    DM_ASSERT(internal_buffer->data_type != DM_BUFFER_DATA_UNKNOWN);
    
    switch (internal_buffer->type)
    {
    case GL_ARRAY_BUFFER:
    {
        glGenVertexArrays(1, &internal_buffer->vao);
        glBindVertexArray(internal_buffer->vao);
        glBindBuffer(
            internal_buffer->data_type,
            internal_buffer->id);
        glBufferData(
            internal_buffer->data_type, 
            buffer->desc.data_size, 
            data, 
            internal_buffer->usage);
        glVertexAttribPointer(
            0, 
            buffer->desc.num_v_elements, 
            internal_buffer->data_type, 
            GL_FALSE,
            buffer->desc.num_v_elements * buffer->desc.elem_size, 
            (void*)0);
        glEnableVertexAttribArray(0);
    }   break;
    case GL_ELEMENT_ARRAY_BUFFER:
        glBindBuffer(
            internal_buffer->data_type, 
            internal_buffer->id);
        glBufferData(
            internal_buffer->data_type, 
            buffer->desc.data_size, 
            data, 
            internal_buffer->usage);
        break;
    }
}

void dm_renderer_delete_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;

    if (internal_buffer->type == DM_BUFFER_TYPE_VERTEX) glDeleteVertexArrays(1, &internal_buffer->vao);
    glDeleteBuffers(1, &internal_buffer->id);
    dm_free(buffer->internal_buffer);
}

void dm_renderer_bind_buffer_impl(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = (dm_internal_buffer*)buffer->internal_buffer;

    if (internal_buffer->type == GL_VERTEX_ARRAY) glBindVertexArray(internal_buffer->vao);
    //glBindBuffer(internal_buffer->type, internal_buffer->id);
}

void dm_renderer_create_shader_impl(dm_shader* shader, dm_vertex_layout_type vertex_layout)
{
    shader->internal_shader = (dm_internal_shader*)dm_alloc(sizeof(dm_internal_shader));
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    GLuint vertex_shader = dm_opengl_compile_shader(shader->vertex_desc);
    GLuint frag_shader = dm_opengl_compile_shader(shader->pixel_desc);

    DM_LOG_DEBUG("Linking shader...");
    internal_shader->id = glCreateProgram();
    glAttachShader(internal_shader->id, vertex_shader);
    glAttachShader(internal_shader->id, frag_shader);
    glLinkProgram(internal_shader->id);

    DM_ASSERT(dm_opengl_validate_program(internal_shader->id));

    glDetachShader(internal_shader->id, vertex_shader);
    glDetachShader(internal_shader->id, frag_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);
}

void dm_renderer_delete_shader_impl(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glDeleteProgram(internal_shader->id);
    dm_free(shader->internal_shader);
}

void dm_renderer_bind_shader_impl(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glUseProgram(internal_shader->id);
}

GLuint dm_opengl_compile_shader(dm_shader_desc desc)
{
    GLuint shader = glCreateShader(desc.type);

    DM_LOG_DEBUG("Compiling shader: %s", desc.path);

    FILE* file = fopen(desc.path, "r");
    DM_ASSERT_MSG(file, "Could not fopen file: %s", desc.path);

    char* b = 0;
    long length;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    b = dm_alloc(length);
    DM_ASSERT(b);

    fread(b, 1, length, file);
    fclose(file);

    DM_ASSERT(b);

    glShaderSource(shader, 1, &b, NULL);
    glCompileShader(shader);

    DM_ASSERT(dm_opengl_validate_shader(shader));

    dm_free(b);

    return shader;
}

bool dm_opengl_validate_shader(GLuint shader)
{
    GLint result;
    int length;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0)
    {
        char message[512];
        glGetShaderInfoLog(shader, length, NULL, message);
        DM_LOG_FATAL("%s", message);

        glDeleteShader(shader);

        return false;
    }

    return true;
}

bool dm_opengl_validate_program(GLuint program)
{
    GLint result;
    int length;

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    if (length > 0)
    {
        char message[512];
        glGetProgramInfoLog(program, length, NULL, message);
        DM_LOG_FATAL("%s", message);

        glDeleteProgram(program);

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

#endif