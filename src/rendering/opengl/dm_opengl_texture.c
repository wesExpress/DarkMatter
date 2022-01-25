#include "dm_opengl_texture.h"

#ifdef DM_OPENGL

#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "dm_opengl_renderer.h"
#include "dm_opengl_shader.h"
#include "dm_opengl_enum_conversion.h"

bool dm_opengl_create_texture(dm_texture* texture, int texture_slot, GLuint shader)
{
	texture->internal_texture = (dm_internal_texture*)dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = (dm_internal_texture*)texture->internal_texture;

	GLenum format = dm_texture_format_to_opengl_format(texture->desc.format);
	if (format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
	GLenum internal_format = dm_texture_format_to_opengl_format(texture->desc.internal_format);
	if (internal_format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
	GLenum min_filter = dm_texture_filter_to_opengl_filter(texture->desc.min_filter);
	if (min_filter == DM_TEXTURE_FILTER_UNKNOWN) return false;
	GLenum mag_filter = dm_texture_filter_to_opengl_filter(texture->desc.mag_filter);
	if (mag_filter == DM_TEXTURE_FILTER_UNKNOWN) return false;
	GLenum s_wrap = dm_edge_to_opengl_edge(texture->desc.s_wrap);
	if (s_wrap == DM_TEXTURE_EDGE_UNKNOWN) return false;
	GLenum t_wrap = dm_edge_to_opengl_edge(texture->desc.t_wrap);
	if (t_wrap == DM_TEXTURE_EDGE_UNKNOWN) return false;

	glGenTextures(1, &internal_texture->id);
	glCheckErrorReturn();

	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s_wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t_wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	// load data in
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture->desc.width, texture->desc.height, 0, format, GL_UNSIGNED_BYTE, texture->data);
	glCheckErrorReturn();
	glGenerateMipmap(GL_TEXTURE_2D);
	glCheckErrorReturn();

	internal_texture->slot = texture_slot;
	if(!dm_opengl_find_uniform_loc(shader, texture->desc.name, &internal_texture->location)) return false;

	return true;
}

void dm_opengl_destroy_texture(dm_texture* texture)
{
	dm_internal_texture* internal_texture = (dm_internal_texture*)texture->internal_texture;
	glDeleteTextures(1, &internal_texture->id);
	glCheckError();
	dm_free(texture->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

bool dm_opengl_bind_texture(dm_texture* texture)
{
	dm_internal_texture* internal_texture = (dm_internal_texture*)texture->internal_texture;

	glActiveTexture(GL_TEXTURE0 + internal_texture->slot);
	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	glUniform1i(internal_texture->location, internal_texture->slot);
	glCheckErrorReturn();

	return true;
}

#endif