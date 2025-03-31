#ifndef DM_H
#define DM_H

/***********
DEFINES
*************/
#include "dm_defines.h"
#include "dm_app_defines.h"

/********
INCLUDES
**********/
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "dm_math.h"
#include "dm_intrinsics.h"

/*******
HASHING
*********/
typedef uint32_t dm_hash;
typedef uint64_t dm_hash64;

/******
RANDOM
********/
typedef struct mt19937
{
	uint32_t mt[624];
	uint32_t mti;
} mt19937;

typedef struct mt19937_64
{
	uint64_t mt[312];
	size_t   mti;
} mt19937_64;

/*****
INPUT
*******/
typedef enum dm_mousebutton_code
{
    DM_MOUSEBUTTON_L,
    DM_MOUSEBUTTON_R,
    DM_MOUSEBUTTON_M,
    DM_MOUSEBUTTON_DOUBLE,
    DM_MOUSEBUTTON_UNKNOWN
} dm_mousebutton_code;

#define MAKE_KEYCODE(NAME, CODE) DM_KEY_##NAME = CODE
typedef enum dm_key_code
{
	MAKE_KEYCODE(BACKSPACE, 0x08),
	MAKE_KEYCODE(TAB,       0x09),
	MAKE_KEYCODE(ENTER,     0x0D),
	MAKE_KEYCODE(SHIFT,     0x10),
	MAKE_KEYCODE(CTRL,      0x11),
	MAKE_KEYCODE(ESCAPE,    0x1B),
	MAKE_KEYCODE(SPACE,     0x20),
    
	// keys above arrows
	MAKE_KEYCODE(END,       0x23),
	MAKE_KEYCODE(HOME,      0x24),
	MAKE_KEYCODE(PRINT,     0x2A),
	MAKE_KEYCODE(INSERT,    0x2D),
	MAKE_KEYCODE(DELETE,    0x2E),
	MAKE_KEYCODE(NUMLCK,    0x90),
	MAKE_KEYCODE(SCRLLCK,   0x91),
	MAKE_KEYCODE(PAUSE,     0x13),
	MAKE_KEYCODE(PAGEUP,    0x21),
	MAKE_KEYCODE(PAGEDOWN,  0x22),
    
	MAKE_KEYCODE(LEFT,      0x25),
	MAKE_KEYCODE(UP,        0x26),
	MAKE_KEYCODE(RIGHT,     0x27),
	MAKE_KEYCODE(DOWN,      0x28),
    
	// numbers
	MAKE_KEYCODE(0,         0x30),
	MAKE_KEYCODE(1,         0x31),
	MAKE_KEYCODE(2,         0x32),
	MAKE_KEYCODE(3,         0x33),
	MAKE_KEYCODE(4,         0x34),
	MAKE_KEYCODE(5,         0x35),
	MAKE_KEYCODE(6,         0x36),
	MAKE_KEYCODE(7,         0x37),
	MAKE_KEYCODE(8,         0x38),
	MAKE_KEYCODE(9,         0x39),
    
	// letters
	MAKE_KEYCODE(A,         0x41),
	MAKE_KEYCODE(B,         0x42),
	MAKE_KEYCODE(C,         0x43),
	MAKE_KEYCODE(D,         0x44),
	MAKE_KEYCODE(E,         0x45),
	MAKE_KEYCODE(F,         0x46),
	MAKE_KEYCODE(G,         0x47),
	MAKE_KEYCODE(H,         0x48),
	MAKE_KEYCODE(I,         0x49),
	MAKE_KEYCODE(J,         0x4A),
	MAKE_KEYCODE(K,         0x4B),
	MAKE_KEYCODE(L,         0x4C),
	MAKE_KEYCODE(M,         0x4D),
	MAKE_KEYCODE(N,         0x4E),
	MAKE_KEYCODE(O,         0x4F),
	MAKE_KEYCODE(P,         0x50),
	MAKE_KEYCODE(Q,         0x51),
	MAKE_KEYCODE(R,         0x52),
	MAKE_KEYCODE(S,         0x53),
	MAKE_KEYCODE(T,         0x54),
	MAKE_KEYCODE(U,         0x55),
	MAKE_KEYCODE(V,         0x56),
	MAKE_KEYCODE(W,         0x57),
	MAKE_KEYCODE(X,         0x58),
	MAKE_KEYCODE(Y,         0x59),
	MAKE_KEYCODE(Z,         0x5A),
    
	// numpad
	MAKE_KEYCODE(NUMPAD_0,  0x60),
	MAKE_KEYCODE(NUMPAD_1,  0x61),
	MAKE_KEYCODE(NUMPAD_2,  0x62),
	MAKE_KEYCODE(NUMPAD_3,  0x63),
	MAKE_KEYCODE(NUMPAD_4,  0x64),
	MAKE_KEYCODE(NUMPAD_5,  0x65),
	MAKE_KEYCODE(NUMPAD_6,  0x66),
	MAKE_KEYCODE(NUMPAD_7,  0x67),
	MAKE_KEYCODE(NUMPAD_8,  0x68),
	MAKE_KEYCODE(NUMPAD_9,  0x69),
    
	MAKE_KEYCODE(MULTIPLY,  0x6A),
	MAKE_KEYCODE(ADD,       0x6B),
	MAKE_KEYCODE(SUBTRACT,  0x6C),
	MAKE_KEYCODE(DECIMAL,   0x6D),
	MAKE_KEYCODE(DIVIDE,    0x6E),
    
	// function
	MAKE_KEYCODE(F1,        0x70),
	MAKE_KEYCODE(F2,        0x71),
	MAKE_KEYCODE(F3,        0x72),
	MAKE_KEYCODE(F4,        0x73),
	MAKE_KEYCODE(F5,        0x74),
	MAKE_KEYCODE(F6,        0x75),
	MAKE_KEYCODE(F7,        0x76),
	MAKE_KEYCODE(F8,        0x77),
	MAKE_KEYCODE(F9,        0x78),
	MAKE_KEYCODE(F10,       0x79),
	MAKE_KEYCODE(F11,       0x7A),
	MAKE_KEYCODE(F12,       0x7B),
	MAKE_KEYCODE(F13,       0x7C),
	MAKE_KEYCODE(F14,       0x7D),
	MAKE_KEYCODE(F15,       0x7E),
	MAKE_KEYCODE(F16,       0x7F),
	MAKE_KEYCODE(F17,       0x80),
	MAKE_KEYCODE(F18,       0x81),
	MAKE_KEYCODE(F19,       0x82),
	MAKE_KEYCODE(F20,       0x83),
	MAKE_KEYCODE(F21,       0x84),
	MAKE_KEYCODE(F22,       0x85),
	MAKE_KEYCODE(F23,       0x86),
	MAKE_KEYCODE(F24,       0x87),
	
	// modifiers
	MAKE_KEYCODE(LSHIFT,    0xA0),
	MAKE_KEYCODE(RSHIFT,    0xA1),
	MAKE_KEYCODE(RCTRL,     0xA2),
	MAKE_KEYCODE(LCTRL,     0xA3),
	MAKE_KEYCODE(ALT,       0x12),
    MAKE_KEYCODE(LALT,      0xA4),
    MAKE_KEYCODE(RALT,      0xA5),
	MAKE_KEYCODE(CAPSLOCK,  0x14),
    MAKE_KEYCODE(LSUPER,    0x5B),
    MAKE_KEYCODE(RSUPER,    0x5C),
    
	// misc
	MAKE_KEYCODE(COMMA,     0xBC),
	MAKE_KEYCODE(PERIOD,    0xBE),
	MAKE_KEYCODE(PLUS,      0xBB),
	MAKE_KEYCODE(EQUAL,     0xBB),
	MAKE_KEYCODE(MINUS,     0xBD),
	MAKE_KEYCODE(COLON,     0xBA),
	MAKE_KEYCODE(TILDE,     0xC0),
    MAKE_KEYCODE(LBRACE,    0xDB),
	MAKE_KEYCODE(RBRACE,    0xDD),
	MAKE_KEYCODE(LSLASH,    0xDC),
	MAKE_KEYCODE(RSLASH,    0xBF),
	MAKE_KEYCODE(QUOTE,     0xDE)
} dm_key_code;

typedef struct dm_mouse_state
{
    bool     buttons[3];
    uint32_t x, y;
    float    scroll;
} dm_mouse_state;

typedef struct dm_keyboard_state
{
	bool keys[256];
} dm_keyboard_state;

typedef struct dm_input_state
{
	dm_keyboard_state keyboard;
	dm_mouse_state    mouse;
} dm_input_state;

/*****
EVENT
*******/
typedef enum dm_event_type_t
{
    DM_EVENT_KEY_DOWN,
    DM_EVENT_KEY_UP,
    DM_EVENT_MOUSEBUTTON_DOWN,
    DM_EVENT_MOUSEBUTTON_UP,
    DM_EVENT_MOUSE_MOVE,
    DM_EVENT_MOUSE_SCROLL,
    DM_EVENT_WINDOW_CLOSE,
    DM_EVENT_WINDOW_RESIZE,
    DM_EVENT_UNKNOWN
} dm_event_type;

typedef struct dm_event_t
{
    dm_event_type type;
    union
    {
        dm_key_code         key;
        dm_mousebutton_code button;
        uint32_t            coords[2];
        uint32_t            new_rect[2];
        float               delta;
    };
} dm_event;

#ifdef DM_MORE_EVENTS
typedef uint16_t dm_event_count;
#define DM_MAX_EVENTS_PER_FRAME UINT16_MAX
#else
typedef uint8_t dm_event_count;
#define DM_MAX_EVENTS_PER_FRAME UINT8_MAX
#endif
typedef struct dm_event_list_t
{
    dm_event       events[DM_MAX_EVENTS_PER_FRAME];
    dm_event_count num;
} dm_event_list;

/*******
LOGGING
*********/
typedef enum log_level
{
	DM_LOG_LEVEL_TRACE,
	DM_LOG_LEVEL_DEBUG,
	DM_LOG_LEVEL_INFO,
	DM_LOG_LEVEL_WARN,
	DM_LOG_LEVEL_ERROR,
	DM_LOG_LEVEL_FATAL
} log_level;

/********
PLATFORM
**********/
typedef struct dm_window_data_t
{
    uint32_t width, height;
    char     title[512];
} dm_window_data;

typedef struct dm_platform_data_t
{
    dm_window_data window_data;
    dm_event_list  event_list;
    char           asset_path[512];
    void*          internal_data;
} dm_platform_data;

/**********
THREADPOOL
************/
typedef struct dm_thread_task_t
{
    void* (*func)(void*);
    void* args;
} dm_thread_task;

#ifndef DM_MAX_THREAD_COUNT
#define DM_MAX_THREAD_COUNT 32
#endif
#define DM_MAX_TASK_COUNT   1000

typedef struct dm_threadpool_t
{
    char tag[512];
    uint32_t thread_count;
    void* internal_pool;
} dm_threadpool;

/*********
RENDERING
***********/
#define DM_RENDERER_MAX_RESOURCE_COUNT 100
typedef enum dm_resource_type_t
{
    DM_RESOURCE_TYPE_UNKNOWN,
    DM_RESOURCE_TYPE_RASTER_PIPELINE,
    DM_RESOURCE_TYPE_COMPUTE_PIPELINE,
    DM_RESOURCE_TYPE_RAYTRACING_PIPELINE,
    DM_RESOURCE_TYPE_VERTEX_BUFFER,
    DM_RESOURCE_TYPE_INDEX_BUFFER,
    DM_RESOURCE_TYPE_CONSTANT_BUFFER,
    DM_RESOURCE_TYPE_STORAGE_BUFFER,
    DM_RESOURCE_TYPE_TEXTURE,
    DM_RESOURCE_TYPE_ACCELERATION_STRUCTURE
} dm_resource_type;

typedef struct dm_resource_handle_t
{
    dm_resource_type type;
    uint32_t         index;
} dm_resource_handle;

typedef enum dm_input_element_format_t
{
    DM_INPUT_ELEMENT_FORMAT_UNKNOWN,
    DM_INPUT_ELEMENT_FORMAT_FLOAT_2,
    DM_INPUT_ELEMENT_FORMAT_FLOAT_3,
    DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
    DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4
} dm_input_element_format;

typedef enum dm_input_element_class_t
{
    DM_INPUT_ELEMENT_CLASS_UNKNOWN,
    DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
    DM_INPUT_ELEMENT_CLASS_PER_INSTANCE,
} dm_input_element_class;

#define DM_RENDER_INPUT_ELEMENT_DESC_NAME_SIZE 512
typedef struct dm_input_element_desc_t
{
    char                    name[DM_RENDER_INPUT_ELEMENT_DESC_NAME_SIZE];
    uint8_t                 index;
    dm_input_element_format format; 
    uint8_t                 slot;
    uint32_t                offset;
    size_t                  stride;
    dm_input_element_class  class;
} dm_input_element_desc;

typedef enum dm_input_topology_t
{
    DM_INPUT_TOPOLOGY_UNKNOWN,
    DM_INPUT_TOPOLOGY_TRIANGLE_LIST,
} dm_input_topology;

typedef enum dm_rasterizer_polygon_fill_t
{
    DM_RASTERIZER_POLYGON_FILL_UNKNOWN,
    DM_RASTERIZER_POLYGON_FILL_FILL,
    DM_RASTERIZER_POLYGON_FILL_WIREFRAME,
} dm_rasterizer_polygon_fill;

typedef enum dm_rasterizer_cull_mode_t
{
    DM_RASTERIZER_CULL_MODE_UNKNOWN,
    DM_RASTERIZER_CULL_MODE_BACK,
    DM_RASTERIZER_CULL_MODE_FRONT,
    DM_RASTERIZER_CULL_MODE_NONE,
} dm_rasterizer_cull_mode;

typedef enum dm_rasterizer_front_face_t
{
    DM_RASTERIZER_FRONT_FACE_UNKNOWN,
    DM_RASTERIZER_FRONT_FACE_CLOCKWISE,
    DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE,
} dm_rasterizer_front_face;

#define DM_RENDER_MAX_INPUT_ELEMENTS 20
typedef struct dm_raster_input_assembler_desc_t
{
    dm_input_element_desc input_elements[DM_RENDER_MAX_INPUT_ELEMENTS];
    uint8_t               input_element_count;

    dm_input_topology topology;
} dm_raster_input_assembler_desc;

typedef struct dm_shader_desc_t
{
    char path[512];
} dm_shader_desc;

typedef struct dm_rasterizer_desc_t
{
    dm_shader_desc vertex_shader_desc;
    dm_shader_desc pixel_shader_desc;

    dm_rasterizer_cull_mode    cull_mode;
    dm_rasterizer_polygon_fill polygon_fill;
    dm_rasterizer_front_face   front_face;
} dm_rasterizer_desc;

typedef enum dm_descriptor_group_flags_t
{
    DM_DESCRIPTOR_GROUP_FLAG_UNKNOWN        = 0x00,
    DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER  = 0x01,
    DM_DESCRIPTOR_GROUP_FLAG_PIXEL_SHADER   = 0x02,
    DM_DESCRIPTOR_GROUP_FLAG_COMPUTE_SHADER = 0x04
} dm_descriptor_group_flags;

typedef enum dm_descriptor_range_type_t
{
    DM_DESCRIPTOR_RANGE_TYPE_UNKNOWN,
    DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER,
    DM_DESCRIPTOR_RANGE_TYPE_TEXTURE,
    DM_DESCRIPTOR_RANGE_TYPE_READ_STORAGE_BUFFER,
    DM_DESCRIPTOR_RANGE_TYPE_WRITE_STORAGE_BUFFER,
    DM_DESCRIPTOR_RANGE_TYPE_ACCELERATION_STRUCTURE,
    DM_DESCRIPTOR_RANGE_TYPE_READ_TEXTURE,
    DM_DESCRIPTOR_RANGE_TYPE_WRITE_TEXTURE,
} dm_descriptor_range_type;

typedef enum dm_descriptor_type_t
{
    DM_DESCRIPTOR_TYPE_UNKNOWN,
    DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER,
    DM_DESCRIPTOR_TYPE_TEXTURE,
    DM_DESCRIPTOR_TYPE_READ_STORAGE_BUFFER,
    DM_DESCRIPTOR_TYPE_WRITE_STORAGE_BUFFER,
    DM_DESCRIPTOR_TYPE_READ_TEXTURE,
    DM_DESCRIPTOR_TYPE_WRITE_TEXTURE,
    DM_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE
} dm_descriptor_type;

#define DM_DESCRIPTOR_GROUP_MAX_DESCRIPTORS 10
typedef struct dm_descriptor_group_t
{
    dm_descriptor_group_flags flags;
    dm_descriptor_type        descriptors[DM_DESCRIPTOR_GROUP_MAX_DESCRIPTORS];
    uint8_t                   count;
} dm_descriptor_group;

typedef enum dm_viewport_type_t
{
    DM_VIEWPORT_TYPE_UNKNOWN,
    DM_VIEWPORT_TYPE_DEFAULT,
    DM_VIEWPORT_TYPE_CUSTOM
} dm_viewport_type;

typedef struct dm_viewport_t
{
    dm_viewport_type type;
    uint32_t left, right, top, bottom;
} dm_viewport;

typedef enum dm_scissor_type_t
{
    DM_SCISSOR_TYPE_UNKNOWN,
    DM_SCISSOR_TYPE_DEFAULT,
    DM_SCISSOR_TYPE_CUSTOM
} dm_scissor_type;

typedef struct dm_scissor_t
{
    dm_scissor_type type;
    uint32_t offset, extents;
} dm_scissor;

typedef struct dm_depth_stencil_desc_t
{
    bool depth, stencil;
} dm_depth_stencil_desc;

#define DM_MAX_DESCRIPTOR_GROUPS 2
typedef struct dm_raster_pipeline_desc_t
{
    dm_raster_input_assembler_desc input_assembler;
    dm_rasterizer_desc             rasterizer;
    dm_depth_stencil_desc          depth_stencil;

    dm_descriptor_group            descriptor_groups[DM_MAX_DESCRIPTOR_GROUPS];
    uint8_t                        descriptor_group_count;

    dm_viewport viewport;
    dm_scissor  scissor;

    bool sampler;
} dm_raster_pipeline_desc;


typedef struct dm_vertex_buffer_desc_t
{
    size_t size, stride, element_size;
    void*  data;
} dm_vertex_buffer_desc;

typedef enum dm_index_buffer_index_type_t
{
    DM_INDEX_BUFFER_INDEX_TYPE_UNKNOWN,
    DM_INDEX_BUFFER_INDEX_TYPE_UINT16,
    DM_INDEX_BUFFER_INDEX_TYPE_UINT32
} dm_index_buffer_index_type;

typedef struct dm_index_buffer_desc_t
{
    size_t size, element_size;
    dm_index_buffer_index_type index_type;
    void* data;
} dm_index_buffer_desc;

typedef struct dm_constant_buffer_desc_t
{
    size_t size;
    void*  data;
} dm_constant_buffer_desc;

typedef struct dm_storage_buffer_desc_t
{
    size_t size, stride;
    void*  data;
    bool   write;
} dm_storage_buffer_desc;

typedef enum dm_texture_format_t
{
    DM_TEXTURE_FORMAT_UNKNOWN,
    DM_TEXTURE_FORMAT_BYTE_4_UINT,
    DM_TEXTURE_FORMAT_BYTE_4_UNORM,
    DM_TEXTURE_FORMAT_FLOAT_3,
    DM_TEXTURE_FORMAT_FLOAT_4,
} dm_texture_format;

typedef struct dm_texture_desc_t
{
    uint32_t          width, height, n_channels;
    dm_texture_format format;
    void*             data;
} dm_texture_desc;

// raytracing
typedef enum dm_blas_geometry_type_t
{
    DM_BLAS_GEOMETRY_TYPE_TRIANGLES,
    DM_BLAS_GEOMETRY_TYPE_UNKNOWN
} dm_blas_geometry_type;

typedef enum dm_blas_geometry_flag_t
{
    DM_BLAS_GEOMETRY_FLAG_OPAQUE,
    DM_BLAS_GEOMETRY_FLAG_UNKNOWN
} dm_blas_geometry_flag;

typedef enum dm_blas_vertex_type_t
{
    DM_BLAS_VERTEX_TYPE_FLOAT_3,
    DM_BLAS_VERTEX_TYPE_UNKNOWN
} dm_blas_vertex_type;

typedef struct dm_blas_desc_t
{
    dm_resource_handle vertex_buffer;
    dm_resource_handle index_buffer;

    dm_blas_vertex_type        vertex_type;
    dm_index_buffer_index_type index_type;

    size_t vertex_stride;

    uint32_t vertex_count, index_count;

    dm_blas_geometry_type geometry_type;
    dm_blas_geometry_flag flags;
} dm_blas_desc;

// this is the struct that both dx12 and vulkan use
// should enable optimized instancing for rt
typedef struct dm_raytracing_instance_t
{
    float    transform[3][4];
    uint32_t id : 24;
    uint32_t mask : 8;
    uint32_t sbt_offset : 24;
    uint32_t flags : 8;
    size_t   blas_address;
} dm_raytracing_instance;

#define DM_TLAS_MAX_BLAS 10
typedef struct dm_tlas_desc_t
{
    dm_blas_desc blas[DM_TLAS_MAX_BLAS];
    uint8_t      blas_count;

    dm_raytracing_instance* instances;
    uint32_t                instance_count;
} dm_tlas_desc;

typedef struct dm_acceleration_structure_desc_t
{
    dm_tlas_desc tlas;
} dm_acceleration_structure_desc;

typedef enum dm_rt_pipe_hit_group_stage_t
{
    DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST,
    DM_RT_PIPE_HIT_GROUP_STAGE_ANY,
    DM_RT_PIPE_HIT_GROUP_STAGE_INTERSECTION,
    DM_RT_PIPE_HIT_GROUP_STAGE_UNKNOWN
} dm_rt_pipe_hit_group_stage;

typedef enum dm_rt_pipe_hit_group_flag_t
{
    DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST       = 1,
    DM_RT_PIPE_HIT_GROUP_FLAG_ANY           = 2,
    DM_RT_PIPE_HIT_GROUP_FLAG_INTERSECTION  = 4
} dm_rt_pipe_hit_group_flag;

typedef enum dm_rt_pipe_hit_group_type_t
{
    DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES,
    DM_RT_PIPE_HIT_GROUP_TYPE_UNKONWN
} dm_rt_pipe_hit_group_type;

typedef struct dm_raytracing_pipeline_hit_group_t
{
    char name[512];
    char path[512];

    char shaders[DM_RT_PIPE_HIT_GROUP_STAGE_UNKNOWN][512];
    dm_rt_pipe_hit_group_flag flags;

    dm_rt_pipe_hit_group_type type;

    dm_descriptor_group descriptor_groups[DM_MAX_DESCRIPTOR_GROUPS];
    uint8_t             descriptor_group_count;
} dm_raytracing_pipeline_hit_group;

#define DM_RT_PIPE_MAX_HIT_GROUPS 5
typedef struct dm_raytracing_pipeline_desc_t
{
    dm_raytracing_pipeline_hit_group hit_groups[DM_RT_PIPE_MAX_HIT_GROUPS];
    uint8_t                          hit_group_count;

    dm_descriptor_group global_descriptor_groups[DM_MAX_DESCRIPTOR_GROUPS];
    uint8_t             global_descriptor_group_count;

    dm_descriptor_group raygen_descriptor_groups[DM_MAX_DESCRIPTOR_GROUPS];
    uint8_t             raygen_descriptor_group_count;

    uint8_t max_depth;
    size_t  payload_size;
} dm_raytracing_pipeline_desc;

// compute
typedef struct dm_compute_pipeline_desc_t
{
    dm_descriptor_group descriptor_groups[DM_MAX_DESCRIPTOR_GROUPS];
    uint8_t             descriptor_group_count;

    dm_shader_desc shader;
} dm_compute_pipeline_desc;

// commands
typedef enum dm_render_command_type_t
{
    DM_RENDER_COMMAND_TYPE_UNKNOWN,
    DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_END_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE,
    DM_RENDER_COMMAND_TYPE_BIND_DESCRIPTOR_GROUP,
    DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_TEXTURE,
    DM_RENDER_COMMAND_TYPE_BIND_STORAGE_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_ACCELERATION_STRUCTURE,
    DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_ACCLERATION_STRUCTURE,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED
} dm_render_command_type;

typedef enum dm_compute_command_type_t
{
    DM_COMPUTE_COMMAND_TYPE_UNKNOWN,
    DM_COMPUTE_COMMAND_TYPE_BIND_COMPUTE_PIPELINE,
    DM_COMPUTE_COMMAND_TYPE_BIND_STORAGE_BUFFER,
    DM_COMPUTE_COMMAND_TYPE_BIND_CONSTANT_BUFFER,
    DM_COMPUTE_COMMAND_TYPE_BIND_TEXTURE,
    DM_COMPUTE_COMMAND_TYPE_DISPATCH,
} dm_compute_command_type;

typedef struct dm_command_param_t
{
    union
    {
        bool                bool_val;
        int                 int_val;
        uint8_t             u8_val;
        uint16_t            u16_val;
        uint32_t            u32_val;
        uint64_t            u64_val;
        size_t              size_t_val;
        float               float_val;
        dm_resource_handle  handle_val;
        void*               void_val;
    };
} dm_command_param;

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

typedef enum dm_command_manager_type_t
{
    DM_COMMAND_MANAGER_TYPE_RENDER,
    DM_COMMAND_MANAGER_TYPE_COMPUTE,
    DM_COMMAND_MANAGER_TYPE_UNKNOWN
} dm_command_manager_type;

typedef struct dm_command_manager_t
{
    uint32_t                capacity, count;
    dm_command_manager_type type;
    dm_command*             commands;
} dm_command_manager;

typedef struct dm_renderer_t
{
    dm_command_manager render_command_manager;
    dm_command_manager compute_command_manager;
    
    uint32_t width, height;
    bool vsync;
    
    void* internal_renderer;
} dm_renderer;

// font loading
typedef struct dm_font_baked_char
{
    uint16_t x0, x1, y0, y1;
    float    x_off, y_off;
    float    advance;
} dm_font_baked_char;

typedef struct dm_font_aligned_quad_t
{
    float x0,y0,s0,t0;
    float x1,y1,s1,t1;
} dm_font_aligned_quad;

typedef struct dm_font_t
{
    dm_font_baked_char glyphs[96];
    dm_resource_handle texture_handle;
} dm_font;

/******************
DARKMATTER CONTEXT
********************/
typedef struct dm_context_init_packet_t
{
    uint32_t window_x, window_y;
    uint32_t window_width, window_height;
    char     window_title[512];
    char     asset_folder[512];
    bool     vsync;
    size_t   app_data_size;
} dm_context_init_packet;

typedef struct dm_context_t
{
    dm_platform_data   platform_data;
    dm_input_state     input_states[2];
    dm_renderer        renderer;
    
    double             start, end, delta;
    mt19937            random;
    mt19937_64         random_64;
    uint32_t           flags;
    
    // application
    void*              app_data;
} dm_context;

/*********
FRAMEWORK
***********/
typedef struct dm_timer_t
{
    double start, end;
} dm_timer;

/*****************
FUNC DECLARATIONS
*******************/
// memory
void* dm_alloc(size_t size);
void* dm_calloc(size_t count, size_t size);
void* dm_realloc(void* block, size_t size);
void  dm_free(void** block);
void* dm_memset(void* dest, int value, size_t size);
void* dm_memzero(void* block, size_t size);
void* dm_memcpy(void* dest, const void* src, size_t size);
void  dm_memmove(void* dest, const void* src, size_t size);

// general input
bool dm_input_is_key_pressed(dm_key_code key, dm_context* context);
bool dm_input_is_mousebutton_pressed(dm_mousebutton_code button, dm_context* context);
bool dm_input_key_just_pressed(dm_key_code key, dm_context* context);
bool dm_input_key_just_released(dm_key_code key, dm_context* context);
bool dm_input_mousebutton_just_pressed(dm_mousebutton_code button, dm_context* context);
bool dm_input_mousebutton_just_released(dm_mousebutton_code button, dm_context* context);
bool dm_input_mouse_has_moved(dm_context* context);
bool dm_input_mouse_has_scrolled(dm_context* context);

// getters for mouse
uint32_t dm_input_get_mouse_x(dm_context* context);
uint32_t dm_input_get_mouse_y(dm_context* context);
void     dm_input_get_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context);
void     dm_input_get_mouse_delta(int* x, int* y, dm_context* context);
int      dm_input_get_mouse_delta_x(dm_context* context);
int      dm_input_get_mouse_delta_y(dm_context* context);
float    dm_input_get_mouse_scroll(dm_context* context);
int      dm_input_get_prev_mouse_x(dm_context* context);
int      dm_input_get_prev_mouse_y(dm_context* context);
void     dm_input_get_prev_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context);
float    dm_input_get_prev_mouse_scroll(dm_context* context);

// clear/resetting functions
void dm_input_reset_mouse_x(dm_context* context);
void dm_input_reset_mouse_y(dm_context* context);
void dm_input_reset_mouse_pos(dm_context* context);
void dm_input_reset_mouse_scroll(dm_context* context);
void dm_input_clear_keyboard(dm_context* context);
void dm_input_clear_mousebuttons(dm_context* context);

// logging
void __dm_log_output(log_level level, const char* message, ...);

#define DM_LOG_TRACE(message, ...) __dm_log_output(DM_LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#define DM_LOG_DEBUG(message, ...) __dm_log_output(DM_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#define DM_LOG_INFO(message, ...)  __dm_log_output(DM_LOG_LEVEL_INFO,  message, ##__VA_ARGS__)
#define DM_LOG_WARN(message, ...)  __dm_log_output(DM_LOG_LEVEL_WARN,  message, ##__VA_ARGS__)
#define DM_LOG_ERROR(message, ...) __dm_log_output(DM_LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define DM_LOG_FATAL(message, ...) __dm_log_output(DM_LOG_LEVEL_FATAL, message, ##__VA_ARGS__)

// string
void dm_strcpy(char* dest, const char* src);

// events for platform
void dm_add_window_close_event(dm_event_list* event_list);
void dm_add_window_resize_event(uint32_t new_width, uint32_t new_height, dm_event_list* event_list);
void dm_add_mousebutton_down_event(dm_mousebutton_code button, dm_event_list* event_list);
void dm_add_mousebutton_up_event(dm_mousebutton_code button, dm_event_list* event_list);
void dm_add_mouse_move_event(uint32_t mouse_x, uint32_t mouse_y, dm_event_list* event_list);
void dm_add_mouse_scroll_event(float delta, dm_event_list* event_list);
void dm_add_key_down_event(dm_key_code key, dm_event_list* event_list);
void dm_add_key_up_event(dm_key_code key, dm_event_list* event_list);

// threads
uint32_t dm_get_available_processor_count(dm_context* context);

bool dm_threadpool_create(const char* tag, uint32_t num_threads, dm_threadpool* threadpool);
void dm_threadpool_destroy(dm_threadpool* threadpool);
void dm_threadpool_submit_task(dm_thread_task* task, dm_threadpool* threadpool);
void dm_threadpool_wait_for_completion(dm_threadpool* threadpool);

// random
int      dm_random_int(dm_context* context);
int      dm_random_int_range(int start, int end, dm_context* context);
uint32_t dm_random_uint32(dm_context* context);
uint32_t dm_random_uint32_range(uint32_t start, uint32_t end, dm_context* context);
uint64_t dm_random_uint64(dm_context* context);
uint64_t dm_random_uint64_range(uint64_t start, uint64_t end, dm_context* context);
float    dm_random_float(dm_context* context);
float    dm_random_float_range(float start, float end,dm_context* context);
double   dm_random_double(dm_context* context);
double   dm_random_double_range(double start, double end, dm_context* context);

float dm_random_float_normal(float mu, float sigma, dm_context* context);

// platform
void dm_platform_sleep(uint64_t ms, dm_context* context);

// rendering
bool dm_renderer_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_renderer_create_acceleration_structure(dm_acceleration_structure_desc desc, dm_resource_handle* handle, dm_context* context);

// don't really like this, but not really sure how to fix
bool dm_renderer_get_blas_gpu_address(dm_resource_handle acceleration_structure, uint8_t blas_index, size_t* address, dm_context* context);

void dm_render_command_begin_render_pass(float r, float g, float b, float a, dm_context* context);
void dm_render_command_end_render_pass(dm_context* context);

void dm_render_command_bind_raster_pipeline(dm_resource_handle handle, dm_context* context);
void dm_render_command_bind_raytracing_pipeline(dm_resource_handle handle, dm_context* context);
void dm_render_command_bind_descriptor_group(uint8_t group_index, uint8_t num_descriptors, uint32_t descriptor_buffer_index, dm_context* context);

void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, dm_context* context);
void dm_render_command_bind_index_buffer(dm_resource_handle handle, dm_context* context);
void dm_render_command_bind_constant_buffer(dm_resource_handle buffer, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_render_command_bind_texture(dm_resource_handle texture, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_render_command_bind_storage_buffer(dm_resource_handle buffer, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_render_command_bind_acceleration_structure(dm_resource_handle acceleration_structure, uint8_t binding, uint8_t descriptor_group, dm_context* context);

void dm_render_command_update_vertex_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context);
void dm_render_command_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context);
void dm_render_command_update_storage_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context);
void dm_render_command_update_acceleration_structure(void* instance_data, size_t size, uint32_t instance_count, dm_resource_handle handle, dm_context* context);

void dm_render_command_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_context* context);
void dm_render_command_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_context* context);

// font loading
bool dm_renderer_load_font(const char* path, int font_size, dm_font* font, dm_context* context);
dm_font_aligned_quad dm_font_get_aligned_quad(dm_font font, const char text, float* xf, float* yf);

// compute
bool dm_compute_create_compute_pipeline(dm_compute_pipeline_desc desc, dm_resource_handle* handle, dm_context* context);

bool dm_compute_command_begin_recording(dm_context* context);
bool dm_compute_command_end_recording(dm_context* context);
void dm_compute_command_bind_compute_pipeline(dm_resource_handle handle, dm_context* context);
void dm_compute_command_bind_storage_buffer(dm_resource_handle handle, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_compute_command_bind_constant_buffer(dm_resource_handle handle, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_compute_command_bind_texture(dm_resource_handle handle, uint8_t binding, uint8_t descriptor_group, dm_context* context);
void dm_compute_command_bind_descriptor_group(uint8_t group_index, uint8_t num_descriptors, uint32_t descriptor_buffer_index, dm_context* context);
void dm_compute_command_dispatch(const uint16_t x, const uint16_t y, const uint16_t z, dm_context* context);

// framework funcs
bool        dm_context_is_running(dm_context* context);

void dm_kill(dm_context* context);

void* dm_read_bytes(const char* path, const char* mode, size_t* size);

uint32_t __dm_get_screen_width(dm_context* context);
uint32_t __dm_get_screen_height(dm_context* context);

#define DM_SCREEN_WIDTH(CONTEXT)  __dm_get_screen_width(context)
#define DM_SCREEN_HEIGHT(CONTEXT) __dm_get_screen_height(context)

void dm_qsort(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b));
#ifdef DM_LINUX
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b, void* c), void* d);
#else
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(void* c, const void* a, const void* b), void* d);
#endif

void dm_assert(bool condition);
void dm_assert_msg(bool condition, const char* message);

// timer
void   dm_timer_start(dm_timer* timer, dm_context* context);
void   dm_timer_restart(dm_timer* timer, dm_context* context);
double dm_timer_elapsed(dm_timer* timer, dm_context* context);
double dm_timer_elapsed_ms(dm_timer* timer, dm_context* context);

// hash
dm_hash64 dm_hash_fnv1a(const char* key);
dm_hash   dm_hash_32bit(uint32_t key);
dm_hash64 dm_hash_64bit(uint64_t key);
dm_hash   dm_hash_int_pair(int x, int y);
dm_hash64 dm_hash_uint_pair(uint32_t x, uint32_t y);
uint32_t  dm_hash_reduce(uint32_t x, uint32_t n);

#endif //DM_H
