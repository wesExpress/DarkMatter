#ifndef __DM_OPENGL_OBJECT_SHADER_H__
#define __DM_OPENGL_OBJECT_SHADER_H__

#include "dm_defines.h"

#if DM_OPENGL
	
#include "../dm_opengl_renderer.h"
#include <stdbool.h>

bool dm_opengl_object_shader_create(dm_renderer_data* renderer_data);
void dm_opengl_object_shader_destroy(dm_shader* object_shader);

void dm_opengl_object_shader_use(dm_shader* object_shader);

#endif

#endif