#ifndef __DM_DIRECTX_BUFFER_H__
#define __DM_DIRECTX_BUFFER_H__

#include "dm_defines.h"

#ifdef DM_DIRECTX

#include "dm_directx_renderer.h"
#include <stdbool.h>

bool dm_directx_create_buffer(dm_buffer* buffer, void* data, dm_internal_pipeline* pipeline);
void dm_directx_delete_buffer(dm_buffer* buffer, dm_internal_pipeline* pipeline);
void dm_directx_bind_buffer(dm_buffer* buffer, dm_internal_pipeline* pipeline);

#endif

#endif