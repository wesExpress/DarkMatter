#ifndef __DM_METAL_BUFFER_H__
#define __DM_METAL_BUFFER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"

bool dm_metal_create_buffer(dm_buffer* buffer, void* data, dm_metal_renderer* metal_renderer);
void dm_metal_destroy_buffer(dm_buffer* buffer);

#endif

#endif