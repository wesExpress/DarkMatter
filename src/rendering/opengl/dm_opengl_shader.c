#include "dm_opengl_shader.h"

#if DM_OPENGL

#include "dm_logger.h"
#include "dm_mem.h"
#include "platform/dm_filesystem.h"

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