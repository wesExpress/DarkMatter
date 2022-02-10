#ifndef __DM_DIRECTX_BUFFER_H__
#define __DM_DIRECTX_BUFFER_H__

#include "core/dm_defines.h"

#ifdef DM_DIRECTX

#include "structures/dm_list.h"
#include "dm_directx_renderer.h"
#include <stdbool.h>

bool dm_directx_create_buffer(dm_buffer* buffer, void* data, dm_directx_renderer* renderer);
void dm_directx_delete_buffer(dm_buffer* buffer);
void dm_directx_bind_buffer(dm_buffer* buffer, uint32_t offset, dm_directx_renderer* renderer);
void dm_directx_bind_vertex_buffers(dm_list* buffers, dm_directx_renderer* renderer);

#endif

#endif