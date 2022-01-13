#ifndef __DM_DIRECTX_SWAP_CHAIN_H__
#define __DM_DIRECTX_SWAP_CHAIN_H__

#include "dm_defines.h"

#ifdef DM_DIRECTX

#include <stdbool.h>

#include "dm_directx_renderer.h"
#include "platform/dm_platform.h"

bool dm_directx_create_swapchain(dm_internal_pipeline* pipeline);
void dm_directx_destroy_swapchain(dm_internal_pipeline* pipeline);

#endif // directx check

#endif 