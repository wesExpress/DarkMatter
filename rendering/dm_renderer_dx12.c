#include "dm.h"

#ifdef DM_DIRECTX12

bool dm_renderer_backend_init(dm_context* context)
{
    return true;
}

bool dm_renderer_backend_finish_init(dm_context* context)
{
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    return true;
}

#endif
