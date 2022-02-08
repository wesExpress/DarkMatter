#ifndef __DM_OPENGL_RENDERER_H__
#define __DM_OPENGL_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include <glad/glad.h>

#include "rendering/dm_renderer.h"

typedef struct dm_internal_buffer
{
	GLuint id;
	GLenum type, usage, data_type;
} dm_internal_buffer;

typedef struct dm_internal_constant_buffer
{
	GLint location;
	void* data;
} dm_internal_constant_buffer;

typedef struct dm_internal_uniform
{
	GLint location;
} dm_internal_uniform;

typedef struct dm_internal_shader
{
	GLuint id;
} dm_internal_shader;

typedef struct dm_internal_pipeline
{
	GLuint vao;
	GLenum blend_src, blend_dest;
	GLenum blend_func, depth_func, stencil_func;
	GLenum cull, winding;
	GLenum primitive;
} dm_internal_pipeline;

typedef struct dm_internal_texture
{
	GLuint id, slot;
	GLint location;
} dm_internal_texture;

typedef enum dm_opengl_uniform
{
	DM_OPENGL_UNI_INT,
	DM_OPENGL_UNI_INT2,
	DM_OPENGL_UNI_INT3,
	DM_OPENGL_UNI_INT4,
	DM_OPENGL_UNI_FLOAT,
	DM_OPENGL_UNI_FLOAT2,
	DM_OPENGL_UNI_FLOAT3,
	DM_OPENGL_UNI_FLOAT4,
	DM_OPENGL_UNI_MAT2,
	DM_OPENGL_UNI_MAT3,
	DM_OPENGL_UNI_MAT4
} dm_opengl_uniform;

#if DM_DEBUG
GLenum glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#define glCheckErrorReturn() if(glCheckError()) return false
#else
#define glCheckError()
#define glCheckErrorReturn()
#endif

#endif

#endif