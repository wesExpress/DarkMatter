#ifndef __DM_OPENGL_SHADER_H__
#define __DM_OPENGL_SHADER_H__

#include "dm_defines.h"

#if DM_OPENGL

#include "dm_opengl_renderer.h"

GLuint dm_opengl_compile_shader(dm_shader_desc desc);
bool dm_opengl_validate_shader(GLuint shader);
bool dm_opengl_validate_program(GLuint program);

bool dm_opengl_find_uniform_location(GLint shader, const char* name, int* location);
bool dm_opengl_update_uniform(int location, dm_opengl_uniform uniform_t, void* data);

#endif

#endif