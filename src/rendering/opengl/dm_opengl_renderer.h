#ifndef __DM_OPENGL_RENDERER_H__
#define __DM_OPENGL_RENDERER_H__

#include "dm_defines.h"

#if DM_OPENGL
#include "rendering/dm_renderer.h"

#include <glad/glad.h>

typedef struct dm_internal_buffer
{
	GLuint id, vao;
	GLenum type, usage, data_type;
} dm_internal_buffer;

typedef struct dm_internal_shader
{
	GLuint id;
} dm_internal_shader;

typedef struct dm_internal_pipeline
{
	GLuint vao;
	bool vao_init;
} dm_internal_pipeline;

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

bool dm_opengl_create_vertex_buffer(dm_buffer* buffer, void* data, int num_v_attribs, dm_vertex_attrib* v_attribs);
bool dm_opengl_create_elem_buffer(dm_buffer* buffer, void* data);

GLenum glCheckError_(const char *file, int line);
#if DM_DEBUG
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#else
#define glCheckError()
#endif

#endif

#endif