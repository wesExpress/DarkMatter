#ifndef __DM_OPENGL_RENDERER_H__
#define __DM_OPENGL_RENDERER_H__

#include "dm_defines.h"

#if DM_OPENGL
#include "rendering/dm_renderer.h"

#include <glad/glad.h>

typedef struct dm_internal_buffer
{
	GLuint id;
	GLenum type, usage, vao;
} dm_internal_buffer;

typedef struct dm_internal_shader
{
	GLuint id;
} dm_internal_shader;

#endif

#endif