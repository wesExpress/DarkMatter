#include "dm_opengl_object_shader.h"

#if DM_OPENGL

#include "dm_logger.h"
#include "dm_mem.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

bool dm_opengl_object_shader_create(dm_renderer_data* renderer_data)
{
	dm_shader_desc v_desc = { 0 };
	v_desc.path = "shaders/glsl/object_vertex.glsl";
	v_desc.type = DM_SHADER_TYPE_VERTEX;

	dm_shader_desc p_desc = { 0 };
	p_desc.path = "shaders/glsl/object_pixel.glsl";
	p_desc.type = DM_SHADER_TYPE_PIXEL;
	
	renderer_data->object_shader = (dm_shader*)dm_alloc(sizeof(dm_shader));

	renderer_data->object_shader->vertex_desc = v_desc;
	renderer_data->object_shader->pixel_desc = p_desc;

	if (!dm_opengl_create_shader(&renderer_data->object_shader))
	{
		DM_LOG_ERROR("Failed to create object shader!");
		return false;
	}

	// pipeline generation
	dm_viewport viewport = { 0 };
	viewport.y = renderer_data->height;
	viewport.width = renderer_data->width;
	viewport.height = renderer_data->height;
	viewport.max_depth = 1.0f;

	// attributes


	return true;
}

void dm_opengl_object_shader_destroy(dm_shader* object_shader)
{
	dm_internal_shader* internal_shader = (dm_internal_shader*)object_shader->internal_shader;

	glDeleteProgram(internal_shader->id);
	glCheckError();
	dm_free(object_shader->internal_shader);
	dm_free(object_shader);
}

void dm_opengl_object_shader_use(dm_shader* object_shader)
{
	dm_internal_shader* internal_shader = (dm_internal_shader*)object_shader->internal_shader;

	glUseProgram(internal_shader->id);
	glCheckError();
}

#endif