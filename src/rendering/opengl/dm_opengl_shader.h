#ifndef __DM_OPENGL_SHADER_H__
#define __DM_OPENGL_SHADER_H__

#include "dm_defines.h"

#if DM_OPENGL

#include "dm_opengl_renderer.h"

bool dm_opengl_find_uniform_location(GLint shader, const char* name, int* location);
bool dm_opengl_update_uniform(int location, dm_opengl_uniform uniform_t, void* data);

bool dm_opengl_create_shader_module(const char* name, const char* type, GLenum type_enum, int index, dm_opengl_shader_stage* stages);

#endif

#endif