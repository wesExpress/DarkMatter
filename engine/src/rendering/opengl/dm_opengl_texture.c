#include "dm_opengl_texture.h"

#ifdef DM_OPENGL

#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "dm_opengl_renderer.h"
#include "dm_opengl_shader.h"
#include "dm_opengl_enum_conversion.h"

bool dm_opengl_create_texture(dm_image* image, int texture_slot, GLuint shader)
{
	image->internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = image->internal_texture;

	GLenum format = dm_texture_format_to_opengl_format(image->desc.format);
	if (format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
	GLenum internal_format = dm_texture_format_to_opengl_format(image->desc.internal_format);
	if (internal_format == DM_TEXTURE_FORMAT_UNKNOWN) return false;

	glGenTextures(1, &internal_texture->id);
	glCheckErrorReturn();

	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	// load data in
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image->desc.width, image->desc.height, 0, format, GL_UNSIGNED_BYTE, image->data);
	glCheckErrorReturn();
	glGenerateMipmap(GL_TEXTURE_2D);
	glCheckErrorReturn();

	internal_texture->slot = texture_slot;
	if(!dm_opengl_find_uniform_loc(shader, image->desc.name, &internal_texture->location)) return false;

	return true;
}

void dm_opengl_destroy_texture(dm_image* image)
{
	dm_internal_texture* internal_texture = image->internal_texture;
	glDeleteTextures(1, &internal_texture->id);
	glCheckError();
	dm_free(image->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

bool dm_opengl_bind_texture(dm_image* image)
{
	dm_internal_texture* internal_texture = image->internal_texture;

	glActiveTexture(GL_TEXTURE0 + internal_texture->slot);
	glCheckErrorReturn();
	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	glUniform1i(internal_texture->location, internal_texture->slot);
	glCheckErrorReturn();

	return true;
}

#endif