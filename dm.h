#ifndef __DM_H__
#define __DM_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef NDEBUG
#define DM_DEBUG
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define DM_KILABYTE 1024
#define DM_MEGABYTE (DM_KILABYTE * 1024)
#define DM_GIGABYTE (DM_MEGABYTE * 1024)

#include "clog/clog.h"

#ifdef DM_DEBUG
#define LOG_DEBUG(...) DBG(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif
#define LOG_TRACE(...) TRC(__VA_ARGS__)
#define LOG_INFO(...)  INF(__VA_ARGS__)
#define LOG_WARN(...)  WRN(__VA_ARGS__)
#define LOG_ERROR(...) ERR(__VA_ARGS__)
#define LOG_FATAL(...) FTL(__VA_ARGS__)

//#define DM_RAY_TRACE

/********************
 * RENDERING HANDLES
 *********************/
typedef enum dm_pipeline_type_t
{
    DM_PIPELINE_TYPE_INVALID,
    DM_PIPELINE_TYPE_RASTER,
    DM_PIPELINE_TYPE_COMPUTE,
#ifdef DM_RAY_TRACE
    DM_PIPELINE_TYPE_RAY_TRACE
#endif
} dm_pipeline_type;

typedef enum dm_resource_type_t
{
    DM_RESOURCE_TYPE_INVALID,
    DM_RESOURCE_TYPE_RENDER_TARGET,
    DM_RESOURCE_TYPE_RESOURCE_DESCRIPTOR_HEAP,
    DM_RESOURCE_TYPE_SAMPLER_DESCRIPTOR_HEAP,
    DM_RESOURCE_TYPE_BUFFER,
    DM_RESOURCE_TYPE_TEXTURE2D_SAMPLED,
    DM_RESOURCE_TYPE_TEXTURE2D_STORAGE,
    DM_RESOURCE_TYPE_TEXTURE2D_COMBINED_SAMPLER,
    DM_RESOURCE_TYPE_SAMPLER,
#ifdef DM_RAY_TRACE
    DM_RESOURCE_TYPE_ACCELERATION_STRUCTURE,
#endif
} dm_resource_type;

typedef struct dm_handle_t
{
    union
    {
        dm_pipeline_type p_type : 8;
        dm_resource_type r_type : 8;
    };

    u32 index : 24;
    u32 heap_index;
} dm_handle;

/********************
 * RENDERING DEFINES
 *********************/ 
#define DM_FRAMES_IN_FLIGHT 3

#define DM_MAX_PIPES 10
#define DM_MAX_RASTER_PIPES    DM_MAX_PIPES
#define DM_MAX_COMPUTE_PIPES   DM_MAX_PIPES
#ifdef DM_RAY_TRACE
#define DM_MAX_RAY_TRACE_PIPES DM_MAX_PIPES
#define DM_MAX_PIPELINES (DM_MAX_RASTER_PIPES + DM_MAX_COMPUTE_PIPES + DM_MAX_RAYTRACE_PIPES)
#else
#define DM_MAX_PIPELINES (DM_MAX_RASTER_PIPES + DM_MAX_COMPUTE_PIPES)
#endif

#define DM_MAX_DESCRIPTOR_HEAPS (DM_MAX_PIPES * 3)

#define DM_MAX_TEXTURES 100
#define DM_MAX_BUFFERS  100
#define DM_MAX_SAMPLERS 10
#ifdef DM_RAY_TRACE
#define DM_MAX_ACCELS   10
#endif

/**************
 * RASTER PIPE
 ***************/
typedef enum dm_raster_shader_stage_t
{
    DM_RASTER_SHADER_STAGE_VERTEX,
    DM_RASTER_SHADER_STAGE_FRAGMENT,
    DM_RASTER_SHADER_STAGE_MAX
} dm_raster_shader_stage;

typedef struct dm_raster_shader_t
{
    char path[512];
    char entry[512];
} dm_raster_shader;

typedef enum dm_blend_op_t
{
    DM_BLEND_OP_INVALID,
    DM_BLEND_OP_ADD,
    DM_BLEND_OP_SUBTRACT,
    DM_BLEND_OP_MIN,
    DM_BLEND_OP_MAX
} dm_blend_op;

typedef enum dm_blend_factor_t
{
    DM_BLEND_FACTOR_INVALID,
    DM_BLEND_FACTOR_ZERO,
    DM_BLEND_FACTOR_ONE,
    DM_BLEND_FACTOR_SRC_ALPHA,
    DM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
} dm_blend_factor;

typedef struct dm_raster_pipe_desc_t
{
    dm_raster_shader shaders[DM_RASTER_SHADER_STAGE_MAX];

    dm_blend_op color_blend_op, alpha_blend_op;
    dm_blend_factor color_src_factor, color_dst_factor;
    dm_blend_factor alpha_src_factor, alpha_dst_factor;
} dm_raster_pipe_desc;

/****************
 * RENDER TARGET
 *****************/
typedef enum dm_render_target_flag_t
{
    DM_RENDER_TARGET_FLAG_COLOR = 1,
    DM_RENDER_TARGET_FLAG_DEPTH = 2
} dm_render_target_flag;

typedef enum dm_renderattachment_load_op_t
{
    DM_RENDER_ATTACHMENT_LOAD_OP_INVALID,
    DM_RENDER_ATTACHMENT_LOAD_OP_LOAD,
    DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR,
    DM_RENDER_ATTACHMENT_LOAD_OP_DONT_CARE
} dm_render_attachment_load_op;

typedef enum dm_render_attachment_store_op_t
{
    DM_RENDER_ATTACHMENT_STORE_OP_INVALID,
    DM_RENDER_ATTACHMENT_STORE_OP_STORE,
    DM_RENDER_ATTACHMENT_STORE_OP_DONT_CARE
} dm_render_attachment_store_op;

typedef struct dm_render_attachment_desc_t
{
    dm_render_attachment_load_op  load_op;
    dm_render_attachment_store_op store_op;

    dm_handle handle; // ignored if swapchain
} dm_render_attachment_desc;

typedef enum dm_render_target_type_t
{
    DM_RENDER_TARGET_TYPE_INVALID,
    DM_RENDER_TARGET_TYPE_SWAPCHAIN,
    DM_RENDER_TARGET_TYPE_CUSTOM
} dm_render_target_type;

typedef struct dm_render_target_desc_t
{
    dm_render_attachment_desc color_attachment;
    dm_render_attachment_desc depth_attachment;

    dm_render_target_flag flags; // ignored if swapchain
    dm_render_target_type type;
} dm_render_target_desc;

/******************
 * DESCRIPTOR HEAP
 *******************/
typedef struct dm_resource_descriptor_heap_desc_t
{
    u32 buffer_count, texture_count; 
} dm_resource_descriptor_heap_desc;

typedef struct dm_sampler_descriptor_heap_desc_t
{
    u32 sampler_count;
} dm_sampler_descriptor_heap_desc;

/************
 * TEXTURE2D
 *************/
typedef enum dm_texture2d_type_t
{
    DM_TEXTURE2D_TYPE_INVALID,
    DM_TEXTURE2D_TYPE_SAMPLED,
    DM_TEXTURE2D_TYPE_STORAGE,
    DM_TEXTURE2D_TYPE_COMBINED_SAMPLER
} dm_texture2d_type;

typedef enum dm_texture2d_format_t
{
    DM_TEXTURE2D_FORMAT_INVALID,
} dm_texture2d_format;

typedef struct dm_texture2d_desc_t
{
    u32 width, height;

    void* data;
    size_t size;

    dm_texture2d_type   type;
    dm_texture2d_format format;
} dm_texture2d_desc;

/*********
 * BUFFER
 **********/
typedef enum dm_buffer_type_t
{
    DM_BUFFER_TYPE_INVALID,
    DM_BUFFER_TYPE_VERTEX,
    DM_BUFFER_TYPE_INDEX,
    DM_BUFFER_TYPE_STORAGE
} dm_buffer_type;

typedef enum dm_buffer_reside_t
{
    DM_BUFFER_RESIDE_INVALID,
    DM_BUFFER_RESIDE_CPU,
    DM_BUFFER_RESIDE_GPU
} dm_buffer_reside;

typedef struct dm_buffer_desc_t
{
    size_t size, stride;
    dm_buffer_type type;
    dm_buffer_reside reside;
    void* data; // must be long-lasting so it does not decay before creating buffer
} dm_buffer_desc;

/**********
 * SAMPLER
 ***********/
typedef struct dm_sampler_desc_t
{
    int d;
} dm_sampler_desc;

/**********
 * CONTEXT
 ***********/
// arena
typedef struct dm_arena_t
{
    size_t size, capacity;
    void* start;
    void* current;
} dm_arena;

// window
typedef struct dm_window_t
{
    u16 width, height;

    size_t offset;
} dm_window;

// renderer
typedef struct dm_renderer_t
{
    u16 width, height;

    size_t offset;
} dm_renderer;

typedef enum dm_context_flag_t
{
    DM_CONTEXT_FLAG_IS_RUNNING     = 1,
    DM_CONTEXT_FLAG_RENDERER_VSYNC = 2,
    DM_CONTEXT_FLAG_WINDOW_RESIZED = 4,
} dm_context_flag;

typedef struct dm_context_t
{
    dm_window window;
    dm_renderer renderer;

    dm_context_flag flags;

    dm_arena arena;
} dm_context;

// functions
void dm_arena_create(dm_arena *arena, size_t size);
void dm_arena_detroy(dm_arena *arena);
void* dm_arena_alloc(dm_arena *arena, size_t size, size_t *offset);
void* dm_arena_get_ptr(dm_arena arena, size_t offset);

bool dm_init(dm_context *context, u16 width, u16 height, const char *title, dm_context_flag flags);
void dm_shutdown(dm_context *context);
bool dm_is_running(dm_context context);
void dm_update(dm_context *context);
bool dm_begin_render(dm_context *context);
bool dm_end_render(dm_context *context);

void* dm_read_bytes(const char *path, size_t *size);

// resources
bool dm_renderer_create_raster_pipeline(dm_context *context, dm_raster_pipe_desc desc, dm_handle *handle);

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_handle *handle);
bool dm_renderer_create_resource_descriptor_heap(dm_context *context, dm_resource_descriptor_heap_desc desc, dm_handle *handle);
bool dm_renderer_create_sampler_descriptor_heap(dm_context *context, dm_sampler_descriptor_heap_desc desc, dm_handle *handle);
bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_handle *handle);
bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_handle *handle);
bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_handle *handle);

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_handle heap, dm_handle *resources[], u32 count);
bool dm_renderer_upload_samplers_to_heap(dm_context *context, dm_handle heap, dm_handle *samplers[], u32 count);
u64 dm_renderer_get_buffer_address(dm_context *context, dm_handle handle);

bool dm_renderer_create_compute_pipeline(dm_context *context, dm_handle *handle);

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_handle handle, float r, float g, float b, float a, float d);
void dm_render_command_end_rendering(dm_context *context, dm_handle handle);
void dm_render_command_bind_resource_descriptor_heap(dm_context *context, dm_handle handle);
void dm_render_command_bind_sampler_descriptor_heap(dm_context *context, dm_handle handle);
void dm_render_command_bind_pipeline(dm_context *context, dm_handle handle);
void dm_render_command_bind_index_buffer(dm_context *context, dm_handle handle, size_t offset);
void dm_render_command_push_constants(dm_context *context, dm_handle handle);
void dm_render_command_push_data(dm_context *context, void *data, size_t size);
void dm_render_command_draw(dm_context *context, u32 index_count, u32 instance_count);

void dm_render_command_update_buffer(dm_context *context, dm_handle handle, void *data, size_t size);
void dm_render_command_copy_buffer(dm_context *context, dm_handle src, dm_handle dst);

void dm_render_command_update_texture(dm_context *context, dm_handle handle, void* data, size_t size);
void dm_render_command_copy_texture(dm_context *context, dm_handle src, dm_handle dst);

// compute commands
void dm_compute_command_push_data(dm_context *context, void *data, size_t size);
void dm_compute_command_bind_pipeline(dm_context *context, dm_handle handle);
void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z);

// macros
#define DM_ALIGN(VALUE, ALIGNMENT) ((VALUE + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#endif // __DM_H__
