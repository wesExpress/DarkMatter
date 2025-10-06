#include "dm.h"
#include <time.h>

#ifdef DM_PLATFORM_APPLE
#include <mach/mach_time.h>
#elif defined(DM_PLATFORM_WIN32)
#include <windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image/stb_image.h"

void* dm_alloc(size_t size)
{
    void* temp = malloc(size);
    return dm_memzero(temp, size);
}

void dm_free(void** ptr)
{
    free(*ptr);
    *ptr = NULL;
}

void* dm_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void dm_memcpy(void* dest, const void* src, size_t size)
{
    memcpy(dest, src, size);
}

void* dm_memset(void* dst, int value, size_t size)
{
    return memset(dst, value, size);
}

void* dm_memzero(void* dst, size_t size)
{
    return dm_memset(dst, 0, size);
}

double dm_get_time()
{
#ifdef DM_PLATFORM_WIN32
    LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

    return (double)time.QuadPart / (double)frequency.QuadPart;
#elif defined(DM_PLATFORM_LINUX)
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec*0.000000001;
#elif defined(DM_PLATFORM_APPLE)
    mach_timebase_info_data_t clock_timebase;
    mach_timebase_info(&clock_timebase);
    
    uint64_t mach_absolute = mach_absolute_time();
    
    uint64_t nanos = (double)(mach_absolute * (uint64_t)clock_timebase.numer) / (double)clock_timebase.denom;
    
    return nanos / 1.0e9;
#endif
}

void dm_timer_start(dm_timer* timer)
{
    timer->start = dm_get_time();
}

double dm_timer_elapsed(dm_timer* timer)
{
    return dm_get_time() - timer->start;
}

double dm_timer_elapsed_ms(dm_timer* timer)
{
    return dm_timer_elapsed(timer) * 1000;
}

void dm_platform_write(const char* message, uint8_t color)
{
#ifdef DM_PLATFORM_WIN32
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	static uint8_t levels[6] = { 8, 1, 2, 6, 4, 64 };

	SetConsoleTextAttribute(console_handle, levels[color]);
	size_t len = strlen(message);
	LPDWORD number_written = 0;

	WriteConsoleA(console_handle, message, (DWORD)len, number_written, 0);

	// resets to white
	SetConsoleTextAttribute(console_handle, 7);
#elif defined(DM_PLATFORM_LINUX) || defined(DM_PLATFORM_APPLE)
    static char* levels[6] = {
        "1;30",   // white
        "1;34",   // blue
        "1;32",   // green
        "1;33",   // yellow
        "1;31",   // red
        "0;41"    // highlighted red
    };

    char out[5000];
    sprintf(
        out,
        "\033[%sm%s \033[0m",
        levels[color], message
    );

    printf("%s", out);
    #endif
}

void dm_log(dm_log_level level, const char* message, ...)
{
	static const char* log_tag[6] = { "DM_TRCE", "DM_DEBG", "DM_INFO", "DM_WARN", "DM_ERRR", "DM_FATL" };
    
#define DM_MSG_LEN 4980
    char msg_fmt[DM_MSG_LEN];
	memset(msg_fmt, 0, sizeof(msg_fmt));
    
	// ar_ptr lets us move through any variable number of arguments. 
	// start it one argument to the left of the va_args, here that is message.
	// then simply shove each of those arguments into message, which should be 
	// formatted appropriately beforehand
	va_list ar_ptr;
	va_start(ar_ptr, message);
	vsnprintf(msg_fmt, DM_MSG_LEN, message, ar_ptr);
	va_end(ar_ptr);
    
	// add log tag and time code of message
	char out[5000];
    
	time_t now;
	time(&now);
	struct tm* local = localtime(&now);
    
    sprintf(out, 
            "[%s] (%02d:%02d:%02d): %s\n",
            log_tag[level],
            local->tm_hour, local->tm_min, local->tm_sec,
            
            msg_fmt);
    
	dm_platform_write(out, level);
}

/*****************
* RENDERING IMPL *
******************/
typedef struct dm_command_param_t
{
    union
    {
        uint8_t              u8;
        uint16_t             u16;
        uint32_t             u32;
        uint64_t             u64;
        size_t               s;
        bool                 b;
        int                  i;
        float                f;
        double               d;
        void*                v;
        dm_resource_handle   rh;
        dm_renderpass_handle rph;
        dm_pipeline_handle   ph;
    };
} dm_command_param;

typedef enum dm_render_command_type_t
{
    DM_RENDER_COMMAND_TYPE_INVALID,
    DM_RENDER_COMMAND_TYPE_BEGIN_FRAME,
    DM_RENDER_COMMAND_TYPE_END_FRAME,
    DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE,
    DM_RENDER_COMMAND_TYPE_END_UPDATE,
    DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_END_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE,
    DM_RENDER_COMMAND_TYPE_SET_VIEWPORT,
    DM_RENDER_COMMAND_TYPE_SET_SCISSOR,
    DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES,
    DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED,
#ifdef DM_HARDWARE_RAYTRACING
    DM_RENDER_COMMAND_TYPE_BIND_RAYTRACING_PIPELINE,
    DM_RENDER_COMMAND_TYPE_DISPATCH_RAYS,
    DM_RENDER_COMMAND_TYPE_UPDATE_TLAS
#endif
} dm_render_command_type;

typedef enum dm_compute_command_type_t
{
    DM_COMPUTE_COMMAND_TYPE_INVALID,
    DM_COMPUTE_COMMAND_TYPE_BEGIN_RECORDING,
    DM_COMPUTE_COMMAND_TYPE_END_RECORDING,
    DM_COMPUTE_COMMAND_BIND_COMPUTE_PIPELINE,
    DM_COMPUTE_COMMAND_SUBMIT_RESOURCES,
    DM_COMPUTE_COMMAND_DISPATCH
} dm_compute_command_type;

typedef enum dm_command_buffer_type_t
{
    DM_COMMAND_BUFFER_TYPE_INVALID,
    DM_COMMAND_BUFFER_TYPE_RENDER,
    DM_COMMAND_BUFFER_TYPE_COMPUTE
} dm_command_buffer_type;

#define DM_COMMAND_MAX_PARAMS 10
typedef struct dm_command_t
{
    union
    {
        dm_render_command_type  r_type;
        dm_compute_command_type c_type;
    };
    dm_command_param params[DM_COMMAND_MAX_PARAMS];
} dm_command;

#define DM_COMMAND_BUFFER_MAX_COMMANDS 100
typedef struct dm_command_buffer_t
{
    dm_command_buffer_type type;
    dm_command commands[DM_COMMAND_BUFFER_MAX_COMMANDS];
    uint16_t   command_count;
} dm_command_buffer;

typedef enum dm_renderer_flag_t
{
    DM_RENDERER_FLAG_NONE = 0,
    DM_RENDERER_FLAG_VSYNC_ON = 1,
} dm_renderer_flag;

typedef struct dm_renderer_t 
{
    uint32_t width, height;

    dm_command_buffer render_commands;

    void* backend;
} dm_renderer;

// === render commands ===
void dm_render_command_submit(dm_command command, dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    if(renderer->render_commands.command_count >= DM_COMMAND_BUFFER_MAX_COMMANDS)
    {
        dm_log(DM_LOG_ERROR, "Too many render commands");
        return;
    }

    renderer->render_commands.commands[renderer->render_commands.command_count++] = command;
}

void dm_render_command_begin_frame(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_FRAME;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_frame(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_FRAME;

    dm_render_command_submit(command, context);
}

void dm_render_command_begin_update(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_update(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_UPDATE;

    dm_render_command_submit(command, context);
}

void dm_render_command_begin_render_pass(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS;

    command.params[0].rph = handle;
    command.params[1].f = r;
    command.params[2].f = g;
    command.params[3].f = b;
    command.params[4].f = a;
    command.params[5].f = depth;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_render_pass(dm_renderpass_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_RENDER_PASS;

    command.params[0].rph = handle;

    dm_render_command_submit(command, context);
}

void dm_render_command_set_viewport(dm_viewport_index index, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_SET_VIEWPORT;

    command.params[0].u16 = index;

    dm_render_command_submit(command, context);
}

void dm_render_command_set_scissor(dm_scissor_index index, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_SET_SCISSOR;

    command.params[0].u16 = index;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_raster_pipeline(dm_pipeline_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE;

    command.params[0].ph = handle;

    dm_render_command_submit(command, context);
}

void dm_render_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_context* context)
{
    dm_command command = { 0 };
    
    command.r_type = DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES;

    command.params[0].v   = (void*)handles;
    command.params[1].u16 = count;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].u8 = slot;
    command.params[2].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_index_buffer(dm_resource_handle handle, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_vertex_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_index_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_constant_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_storage_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_texture(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE;

    command.params[0].rh  = handle;
    command.params[1].u16 = width;
    command.params[2].u16 = height;
    command.params[3].v   = data;
    command.params[4].s   = size;
    command.params[5].s   = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_draw_instanced(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = vertex_count;
    command.params[3].s   = vertex_offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_draw_instanced_indexed(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = index_count;
    command.params[3].s   = index_offset;
    command.params[4].s   = vertex_offset;

    dm_render_command_submit(command, context);
}

// === backend ===
extern void* dm_renderer_init_backend(dm_context* context);
extern bool dm_renderer_finish_init_backend(void* backend);
extern void dm_renderer_shutdown_backend(void* backend);
extern bool dm_renderer_resize_backend(uint32_t width, uint32_t height, void* backend);
extern uint32_t dm_get_resource_index_backend(dm_resource_handle handle, void* backend);

bool dm_renderer_init(dm_context* context)
{
    context->renderer = dm_alloc(sizeof(dm_renderer));
    dm_renderer* renderer = context->renderer;

    renderer->width = dm_get_window_width(context);
    renderer->height = dm_get_window_height(context);

    renderer->backend = dm_renderer_init_backend(context);

    return renderer->backend != NULL;
}

bool dm_finish_init(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    return dm_renderer_finish_init_backend(renderer->backend);
}

void dm_renderer_shutdown(dm_renderer* renderer)
{
    dm_renderer_shutdown_backend(renderer->backend);
}

bool dm_renderer_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    renderer->width  = width;
    renderer->height = height;

    return dm_renderer_resize_backend(width, height, renderer->backend);
}

uint32_t dm_get_resource_index(dm_resource_handle handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    return dm_get_resource_index_backend(handle, renderer->backend);
}

/*************
 * RESOURCES *
 *************/
extern bool dm_create_renderpass_backend(dm_renderpass_desc desc, dm_renderpass_handle* handle, void* backend);
extern bool dm_create_raster_pipeline_backend(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, void* backend);
extern bool dm_create_vertex_buffer_backend(dm_vertex_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_index_buffer_backend(dm_index_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_constant_buffer_backend(dm_constant_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_storage_buffer_backend(dm_storage_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_texture_backend(dm_texture_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_sampler_backend(dm_sampler_desc desc, dm_resource_handle* handle, void* backend);
extern void dm_create_viewport_backend(dm_viewport viewport, dm_viewport_index* index, void* backend);
extern void dm_create_scissor_backend(dm_scissor scissor, dm_scissor_index* index, void* backend);

bool dm_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_context* context)
{
    dm_renderer* renderer =  context->renderer;

    return dm_create_renderpass_backend(desc, handle, renderer->backend);
}

void dm_create_viewport(dm_viewport viewport, dm_viewport_index* index, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    dm_create_viewport_backend(viewport, index, renderer->backend);
}

void dm_create_scissor(dm_scissor scissor, dm_scissor_index* index, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    dm_create_scissor_backend(scissor, index, renderer->backend);
}

bool dm_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_PIPELINE_TYPE_RASTER;

    return dm_create_raster_pipeline_backend(desc, handle, renderer->backend);
}

bool dm_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    return dm_create_vertex_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    return dm_create_index_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    return dm_create_constant_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    return dm_create_storage_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    
    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    return dm_create_texture_backend(desc, handle, renderer->backend);
}

bool dm_create_texture_from_file(const char* path, dm_resource_handle* handle, dm_context* context)
{
    int x,y,n;
    void* data = stbi_load(path, &x,&y,&n,4);
    if(!data)
    {
        dm_log(DM_LOG_FATAL, "Could not load texture: %s", path);
        return false;
    }

    dm_texture_desc desc = { 
        .width=x,.height=y,.n_channels=4,
        .data=data,
        .format=DM_TEXTURE_FORMAT_BYTE_4_UNORM
    };

    bool result = dm_create_texture(desc, handle, context);

    stbi_image_free(data);
    return result;
}

bool dm_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_SAMPLER;

    return dm_create_sampler_backend(desc, handle, renderer->backend);
}

extern bool dm_render_command_begin_frame_backend(void* backend);
extern bool dm_render_command_end_frame_backend(bool vsync, void* backend);
extern bool dm_render_command_begin_update_backend(void* backend);
extern bool dm_render_command_end_update_backend(void* backend);
extern bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, void* backend);
extern bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, void* backend);
extern bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, void* backend);
extern bool dm_render_command_set_viewport_backend(dm_viewport_index index, void* backend);
extern bool dm_render_command_set_scissor_backend(dm_scissor_index index, void* backend);
extern bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, void* backend);
extern bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, void* backend);
extern bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, void* backend);
extern bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, void* backend);
extern bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, void* backend);

bool dm_submit_render_commands(dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    dm_command_buffer buffer = renderer->render_commands;
    void* backend = renderer->backend;

    for(uint32_t i=0; i<buffer.command_count; i++)
    {
        dm_command command = buffer.commands[i];
        dm_command_param* params = command.params;

        switch(command.r_type)
        {
            case DM_RENDER_COMMAND_TYPE_BEGIN_FRAME:
            if(dm_render_command_begin_frame_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin frame failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_FRAME:
            if(dm_render_command_end_frame_backend((context->flags & DM_CONTEXT_FLAG_VSYNC_ON), backend)) continue;
            dm_log(DM_LOG_FATAL, "End frame failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE:
            if(dm_render_command_begin_update_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin update failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_UPDATE:
            if(dm_render_command_end_update_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "End update failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS:
            if(dm_render_command_begin_render_pass_backend(params[0].rph,params[1].f,params[2].f,params[3].f,params[4].f,params[5].f, backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin render pass failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_RENDER_PASS:
            if(dm_render_command_end_render_pass_backend(params[0].rph, backend)) continue;
            dm_log(DM_LOG_FATAL, "End render pass failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE:
            if(dm_render_command_bind_raster_pipeline_backend(params[0].ph, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind raster pipeline failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_SET_VIEWPORT:
            dm_render_command_set_viewport_backend(params[0].u16, backend);
            continue;
            case DM_RENDER_COMMAND_TYPE_SET_SCISSOR:
            dm_render_command_set_scissor_backend(params[0].u16, backend);
            continue;

            case DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES:
            if(dm_render_command_submit_resources_backend(params[0].v,params[1].u16, backend)) continue;
            dm_log(DM_LOG_FATAL, "Submit resources failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER:
            if(dm_render_command_bind_vertex_buffer_backend(params[0].rh, params[1].u32, params[2].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER:
            if(dm_render_command_bind_index_buffer_backend(params[0].rh, params[1].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind index buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER:
            if(dm_render_command_update_vertex_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER:
            if(dm_render_command_update_index_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update index buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER:
            if(dm_render_command_update_constant_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update constant buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER:
            if(dm_render_command_update_storage_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update storage buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE:
            if(dm_render_command_update_texture_backend(params[0].rh, params[1].u16,params[2].u16,params[3].v,params[4].s,params[5].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update texture failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED:
            if(dm_render_command_draw_instanced_backend(params[0].u32,params[1].s,params[2].u32,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Draw instanced failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED:
            if(dm_render_command_draw_instanced_indexed_backend(params[0].u32,params[1].s,params[2].u32,params[3].s,params[4].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Draw instanced indexed failed");
            return false;

            default:
            dm_log(DM_LOG_ERROR, "Unknown render command, shouldn't be here...");
            break;
        }
    }

    renderer->render_commands.command_count = 0;
    return true;
}

/**********
* CONTEXT *
***********/
extern bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_context* context);
extern void dm_window_shutdown(void* window);
extern bool dm_window_poll_events(void* window);

dm_context* dm_init(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags)
{
    dm_context* context = dm_alloc(sizeof(dm_context));
    if(!context) { dm_log(DM_LOG_FATAL, "creating context failed"); return NULL; }

    if(!dm_window_create(x,y,width,height,title,flags, context)) return false;
    if(!dm_renderer_init(context)) return false;

    return context;
}

void dm_shutdown(dm_context* context)
{
    dm_renderer_shutdown(context->renderer);
    dm_window_shutdown(context->window);

    dm_free((void**)&context->renderer);
    dm_free((void**)&context->window);
    dm_free((void**)&context);
}

bool dm_update(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    if(!dm_window_poll_events(context->window)) return false;

    uint16_t width = dm_get_window_width(context);
    uint16_t height = dm_get_window_height(context);

    if(renderer->width != width || renderer->height != height)
    {
        if(!dm_renderer_resize(width, height, renderer)) return false;
    }

    return true;
}
