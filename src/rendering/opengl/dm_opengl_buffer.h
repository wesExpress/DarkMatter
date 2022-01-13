#ifndef __DM_OPENGL_BUFFER_H__
#define __DM_OPENGL_BUFFER_H__

#include "dm_defines.h"

#ifdef DM_OPENGL

#include "dm_opengl_renderer.h"

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data);
void dm_opengl_delete_buffer(dm_buffer* buffer);
void dm_opengl_bind_buffer(dm_buffer* buffer);

#endif

#endif