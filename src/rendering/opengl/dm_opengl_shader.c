#include "dm_opengl_shader.h"

#ifdef DM_OPENGL

#include "dm_opengl_enum_conversion.h"
#include "dm_logger.h"
#include "dm_assert.h"
#include "dm_mem.h"

GLuint dm_opengl_compile_shader(dm_shader_desc desc);
bool dm_opengl_validate_shader(GLuint shader);
bool dm_opengl_validate_program(GLuint program);

bool dm_opengl_create_shader(dm_shader* shader)
{
    shader->internal_shader = (dm_internal_shader*)dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
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

    if (!dm_opengl_validate_program(internal_shader->id))
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

void dm_opengl_delete_shader(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glDeleteProgram(internal_shader->id);
    glCheckError();
    dm_free(shader->internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
}

void dm_opengl_bind_shader(dm_shader* shader)
{
    dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

    glUseProgram(internal_shader->id);
    glCheckError();
}

bool dm_opengl_bind_uniform(dm_constant_buffer* cb)
{
    dm_internal_constant_buffer* internal_buffer = (dm_internal_constant_buffer*)cb->internal_buffer;

    switch (cb->desc.data_t)
    {
    case DM_CONST_BUFFER_T_INT:
    {
        switch (cb->desc.count)
        {
        case 1:
        {
            int data = *(int*)cb->desc.data;
            glUniform1i(internal_buffer->location, data);
        } break;
        case 2:
        {
            dm_vec2 data = *(dm_vec2*)cb->desc.data;
            glUniform2i(internal_buffer->location, data.x, data.y);
        } break;
        case 3:
        {
            dm_vec3 data = *(dm_vec3*)cb->desc.data;
            glUniform3i(internal_buffer->location, data.x, data.y, data.z);
        } break;
        case 4:
        {
            dm_vec4 data = *(dm_vec4*)cb->desc.data;
            glUniform4i(internal_buffer->location, data.x, data.y, data.z, data.w);
        } break;
        default:
            DM_LOG_FATAL("Trying to upload to uniform with wrong size of ints");
            return false;
        }
    } break;

    case DM_CONST_BUFFER_T_BOOL:
    case DM_CONST_BUFFER_T_UINT:
    {
        switch (cb->desc.count)
        {
        case 1:
        {
            uint32_t data = *(uint32_t*)cb->desc.data;
            glUniform1ui(internal_buffer->location, data);
        } break;
        case 2:
        {
            dm_vec2 data = *(dm_vec2*)cb->desc.data;
            glUniform2ui(internal_buffer->location, data.x, data.y);
        } break;
        case 3:
        {
            dm_vec3 data = *(dm_vec3*)cb->desc.data;
            glUniform3ui(internal_buffer->location, data.x, data.y, data.z);
        } break;
        case 4:
        {
            dm_vec4 data = *(dm_vec4*)cb->desc.data;
            glUniform4ui(internal_buffer->location, data.x, data.y, data.z, data.w);
        } break;
        default:
            DM_LOG_FATAL("Trying to upload to uniform with wrong size of uints");
            return false;
        }
    } break;

    case DM_CONST_BUFFER_T_FLOAT:
    {
        switch (cb->desc.count)
        {
        case 1:
        {
            float data = *(float*)cb->desc.data;
            glUniform1f(internal_buffer->location, data);
        } break;
        case 2:
        {
            dm_vec2 data = *(dm_vec2*)cb->desc.data;
            glUniform2f(internal_buffer->location, data.x, data.y);
        } break;
        case 3:
        {
            dm_vec3* data = (dm_vec3*)cb->desc.data;
            glUniform3f(internal_buffer->location, data->x, data->y, data->z);
        } break;
        case 4:
        {
            dm_vec4 data = *(dm_vec4*)cb->desc.data;
            glUniform4f(internal_buffer->location, data.x, data.y, data.z, data.w);
        } break;
        default:
            DM_LOG_FATAL("Trying to upload to uniform with wrong size of floats");
            return false;
        }
    } break;

    case DM_CONST_BUFFER_T_MATRIX:
    {
        switch (cb->desc.count)
        {
        case 2:
        {
            dm_mat2 data = *(dm_mat2*)cb->desc.data;
            glUniformMatrix2fv(internal_buffer->location, 1, GL_FALSE, data.m);
        } break;
        case 3:
        {
            dm_mat3 data = *(dm_mat3*)cb->desc.data;
            glUniformMatrix3fv(internal_buffer->location, 1, GL_FALSE, data.m);
        } break;
        case 4:
        {
            dm_mat4 data = *(dm_mat4*)cb->desc.data;
            glUniformMatrix4fv(internal_buffer->location, 1, GL_FALSE, data.m);
        } break;
        default:
            DM_LOG_FATAL("Trying to upload to uniform with wrong size of matrices");
            return false;
        }
    } break;

    default:
        DM_LOG_FATAL("Unknown constant buffer data type!");
        return false;
    } 

    return true;
}

bool dm_opengl_find_uniform_loc(GLuint program, const char* name, GLint* location)
{
    *location = glGetUniformLocation(program, name);
    if (*location == 1)
    {
        DM_LOG_FATAL("Could not find uniform: '%s'", name);
        return false;
    }
    return true;
}

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
    char* string = dm_alloc(length + 1, DM_MEM_STRING);
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

    dm_free(string, length+1, DM_MEM_STRING);

    return shader;
}

bool dm_opengl_validate_shader(GLuint shader)
{
    GLint result = -1;
    int length = -1;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glCheckError();

    if (result != GL_TRUE)
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

    if (result == GL_FALSE)
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

#endif