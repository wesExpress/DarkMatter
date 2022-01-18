#ifndef __DM_OPENGL_SHADER_H__
#define __DM_OPENGL_SHADER_H__

#include "dm_defines.h"

#ifdef DM_OPENGL

#include "dm_opengl_renderer.h"

bool dm_opengl_create_shader(dm_shader* shader);
void dm_opengl_delete_shader(dm_shader* shader);
void dm_opengl_bind_shader(dm_shader* shader);
bool dm_opengl_bind_uniform(dm_constant_buffer* cb);
bool dm_opengl_find_uniform_loc(GLuint program, const char* name, GLint* location);

#endif

#endif