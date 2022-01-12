#ifndef __DM_OPENGL_BUFFER_H__
#define __DM_OPENGL_BUFFER_H__

#include "dm_opengl_renderer.h"

#ifdef DM_OPENGL

bool dm_renderer_create_buffer_impl(dm_buffer* buffer, void* data, dm_render_pipeline* pipeline);
void dm_renderer_delete_buffer_impl(dm_buffer* buffer);
void dm_renderer_bind_buffer_impl(dm_buffer* buffer);

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data);

#endif

#endif