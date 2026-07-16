#include "dm.h"

// arena
void dm_arena_create(dm_arena *arena, size_t size)
{
#ifdef __AVX__
    size = DM_ALIGN(size, 32);
#else
    size = DM_ALIGN(size, 16);
#endif
    arena->capacity = size;
    arena->start = calloc(sizeof(u8), size);
    arena->current = arena->start;

#ifdef DM_DEBUG
    LOG_INFO("Arena size: %zu", arena->capacity);
#endif
}

void dm_arena_detroy(dm_arena *arena)
{
    free(arena->start);
    arena->start = NULL;
    arena->current = NULL;
}

void* dm_arena_alloc(dm_arena *arena, size_t size)
{
    if(arena->size + size > arena->capacity) 
    {
        LOG_ERROR("Trying to allocate beyond size of arena");
        return NULL;
    }

#ifdef __AVX__
    size = DM_ALIGN(size, 32);
#else
    size = DM_ALIGN(size, 16);
#endif

    arena->size += size;
    arena->current += size;

    return arena->current - size;;
}

extern bool dm_window_create(dm_context *context, u16 width, u16 height, const char *title);
extern void dm_window_destroy(dm_context *context);
extern void dm_window_poll_events(dm_context *context);
extern size_t dm_window_get_internal_size();

extern bool dm_renderer_init(dm_context* context);
extern void dm_renderer_shutdown(dm_context* context);
extern bool dm_renderer_begin_frame(dm_context* context);
extern bool dm_renderer_end_frame(dm_context* context);
extern bool dm_renderer_resize(dm_context *context, u16 width, u16 height);
extern size_t dm_renderer_get_internal_size();

// context
bool dm_init(dm_context* context, u16 width, u16 height, const char* title, dm_context_flag flags)
{
    size_t size = dm_window_get_internal_size();
    size += dm_renderer_get_internal_size();

    dm_arena_create(&context->arena, size);

    if(!dm_window_create(context, width, height, title)) return false;
    if(!dm_renderer_init(context))
    {
        dm_window_destroy(context);
        return false;
    }

    context->window.width = width;
    context->window.height = height;
    context->flags |= DM_CONTEXT_FLAG_IS_RUNNING;

    return true;
}

void dm_shutdown(dm_context* context)
{
    dm_renderer_shutdown(context);
    dm_window_destroy(context);

    dm_arena_detroy(&context->arena);
}

bool dm_is_running(dm_context *context)
{
    return context->flags & DM_CONTEXT_FLAG_IS_RUNNING;
}

bool dm_window_resized(dm_context *context)
{
    return context->flags & DM_CONTEXT_FLAG_WINDOW_RESIZED;
}

bool dm_update_begin(dm_context* context)
{
    dm_window_poll_events(context);

    if(context->flags & DM_CONTEXT_FLAG_WINDOW_RESIZED) 
        return dm_renderer_resize(context, context->window.width, context->window.height);

    return true;
}

void dm_update_end(dm_context *context)
{
    context->flags &= ~DM_CONTEXT_FLAG_WINDOW_RESIZED;
    context->flags &= ~DM_CONTEXT_FLAG_RENDERER_RESIZED;
}

bool dm_render_begin(dm_context* context)
{
    return dm_renderer_begin_frame(context);
}

bool dm_render_end(dm_context* context)
{
    return dm_renderer_end_frame(context);
}

void* dm_read_bytes(const char *path, size_t *size)
{
    FILE *fp;
    void* data = NULL;

    fp = fopen(path, "rb");
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        data = malloc(*size);
        
        size_t t = fread(data, *size, 1, fp);
        char d[512];
        memcpy(d, data, 512);
        if(t!=1) 
        {
            LOG_ERROR("Something bad happened with fread");
            return NULL;
        }
        
        fclose(fp);
    }
    else
    {
        LOG_ERROR("Could not open file: %s", path);
    }

    return data;
}
