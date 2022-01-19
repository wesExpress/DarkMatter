#include "dm_opengl_texture.h"

#ifdef DM_OPENGL

#include "core/dm_mem.h"
#include "dm_opengl_renderer.h"
#include <stb_image/stb_image.h>

bool dm_opengl_create_texture(dm_texture* texture)
{
	texture->internal_texture = (dm_internal_texture*)dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = (dm_internal_texture*)texture->internal_texture;

	unsigned char* data = stbi_load(texture->path, &texture->width, &texture->height, &texture->n_channels, 0);
	if (!data)
	{
		DM_LOG_FATAL("Could not load texture: %s", texture->path); 
		return false;
	}

	glGenTextures(1, &internal_texture->id);
	glCheckErrorReturn();

	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load data in
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->width, texture->height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glCheckErrorReturn();
	glGenerateMipmap(GL_TEXTURE_2D);
	glCheckErrorReturn();

	stbi_image_free(data);

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

	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();

	return true;
}

#endif