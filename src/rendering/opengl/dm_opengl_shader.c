#include "dm_opengl_shader.h"

#ifdef DM_OPENGL

#include "dm_opengl_enum_conversion.h"
#include "dm_logger.h"
#include "dm_assert.h"
#include "dm_mem.h"

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

bool dm_opengl_find_uniform_location(GLint shader, const char* name, int* location)
{
    *location = glGetUniformLocation(shader, name);
    glCheckError();

    return true;
}

bool dm_opengl_update_uniform(int location, dm_opengl_uniform uniform_t, void* data)
{
    switch(uniform_t)
    {
        case DM_OPENGL_UNI_INT:
        {
            int d = *(int*)data;
            glUniform1i(location, (GLint)d);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_INT2:
        {
            dm_vec2* d = (dm_vec2*)data;
            glUniform2i(location, (GLint)d->x, (GLint)d->y);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_INT3:
        {
            dm_vec3* d = (dm_vec3*)data;
            glUniform3i(location, (GLint)d->x, (GLint)d->y, (GLint)d->z);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_INT4:
        {
            dm_vec4* d = (dm_vec4*)data;
            glUniform4i(location, (GLint)d->x, (GLint)d->y, (GLint)d->z, (GLint)d->w);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_FLOAT:
        {
            float d = *(float*)data;
            glUniform1f(location, (GLfloat)d);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_FLOAT2:
        {
            dm_vec2* d = (dm_vec2*)data;
            glUniform2f(location, (GLfloat)d->x, (GLfloat)d->y);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_FLOAT3:
        {
            dm_vec3* d = (dm_vec3*)data;
            glUniform3f(location, (GLfloat)d->x, (GLfloat)d->y, (GLfloat)d->z);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_FLOAT4:
        {
            dm_vec4* d = (dm_vec4*)data;
            glUniform4f(location, (GLfloat)d->x, (GLfloat)d->y, (GLfloat)d->z, (GLfloat)d->w);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_MAT2:
        {
            dm_mat2* d = (dm_mat2*)data;
            glUniformMatrix2fv(location, 1, GL_FALSE, &d->m[0]);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_MAT3:
        {
            dm_mat3* d = (dm_mat3*)data;
            glUniformMatrix3fv(location, 1, GL_FALSE, &d->m[0]);
            glCheckError();
        } break;
        case DM_OPENGL_UNI_MAT4:
        {
            dm_mat4* d = (dm_mat4*)data;
            glUniformMatrix4fv(location, 1, GL_FALSE, &d->m[0]);
            glCheckError();
        } break;
    }

    return true;
}

#endif