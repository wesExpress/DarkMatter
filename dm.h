#ifndef DM_H
#define DM_H

/***********
APP DEFINES
*************/
#include "dm_app_defines.h"

/********************************************
DETERMINE PLATFORM AND RENDERING BACKEND

we default to directX on windows platforms, 
metal on OSX. But if DM_OPENGL is defined 
then we overwrite that for windows. On mac, 
we switch our platform to GLFW if OpenGL is 
desired as creating an OpenGL context is 
depreciated
**********************************************/
#ifdef __APPLE__
#ifdef DM_OPENGL
#define DM_PLATFORM_GLFW
#else
#define DM_PLATFORM_APPLE
#define DM_METAL
#endif
#define DM_INLINE __attribute__((always_inline)) inline

#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define DM_PLATFORM_WIN32
#if !defined(DM_OPENGL) && !defined(DM_VULKAN)
#define DM_DIRECTX
#endif
#define DM_INLINE __forceinline

#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_LINUX
#define DM_INLINE __always_inline
#else
#define DM_PLATFORM_GLFW
#define DM_INLINE
#endif

// opengl on mac is limited to 4.1
#ifdef DM_OPENGL
#define DM_OPENGL_MAJOR 4
#ifdef __APPLE__
#define DM_OPENGL_MINOR 1
#else
#define DM_OPENGL_MINOR 6
#endif
#endif

/********
INCLUDES
**********/
#ifdef DM_PLATFORM_LINUX
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#ifndef  DM_PLATFORM_APPLE
#include <immintrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#else
//#include "arm_neon.h"
#endif

/*****
TYPES
*******/
#ifndef __cplusplus
#define false 0
#define true  1
#ifndef __bool_true_false_are_defined
typedef _Bool bool;
#endif
#endif

/****
MATH
******/
// various constants
#define DM_MATH_PI                  3.14159265359826f
#define DM_MATH_INV_PI              0.31830988618f
#define DM_MATH_2PI                 6.2831853071795f
#define DM_MATH_INV_2PI             0.159154943091f
#define DM_MATH_4PI                 12.566370614359173f
#define DM_MATH_INV_4PI             0.0795774715459f
#define DM_MATH_INV_12              0.0833333f

#define DM_MATH_ANGLE_RAD_TOLERANCE 0.001f
#define DM_MATH_SQRT2               1.41421356237309f
#define DM_MATH_INV_SQRT2           0.70710678118654f
#define DM_MATH_SQRT3               1.73205080756887f
#define DM_MATH_INV_SQRT3           0.57735026918962f
#define DM_MATH_DEG_TO_RAD          0.0174533f
#define DM_MATH_RAD_TO_DEG          57.2958f

#define DM_MATH_1024_INV            0.0009765625f

// math macros
#define DM_MAX(X, Y) (X > Y ? X : Y)
#define DM_MIN(X, Y) (X < Y ? X : Y)
#define DM_SIGN(X) ((0 < X) - (X < 0))
#define DM_SIGNF(X) (float)((0 < X) - (X < 0))
#define DM_CLAMP(X, MIN, MAX) DM_MIN(DM_MAX(X, MIN), MAX)
#define DM_BIT_SHIFT(X) (1 << X)

/****
SIMD
******/
// TODO: not apple supported yet
#ifndef DM_PLATFORM_APPLE
#ifdef DM_SIMD_256
typedef __m256  dm_mm_float;
typedef __m256i dm_mm_int;
#define DM_SIMD_N 8
#else
typedef __m128  dm_mm_float;
typedef __m128i dm_mm_int;
#define DM_SIMD_N 4
#endif
#endif

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
	size_t mti;
} mt19937_64;

/*****
INPUT
*******/
typedef enum dm_mousebutton_code
{
    DM_MOUSEBUTTON_L,
    DM_MOUSEBUTTON_R,
    DM_MOUSEBUTTON_M,
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
	MAKE_KEYCODE(CAPSLOCK,  0x14),
    
	// misc
	MAKE_KEYCODE(COMMA,     0xBC),
	MAKE_KEYCODE(PERIOD,    0xBE),
	MAKE_KEYCODE(PLUS,      0xBB),
	MAKE_KEYCODE(EQUAL,     0xBB),
	MAKE_KEYCODE(MINUS,     0xBD),
	MAKE_KEYCODE(COLON,     0xBA),
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
    int      scroll;
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
        uint32_t            delta;
    };
} dm_event;

#define DM_MAX_EVENTS_PER_FRAME 1000
typedef struct dm_event_list_t
{
    dm_event events[DM_MAX_EVENTS_PER_FRAME];
    size_t   num;
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
STRUCTURES
************/
// slot array
typedef uint32_t dm_slot_index;

typedef struct dm_slot_array_t
{
    size_t    capacity, count, elem_size;
    void*     data;
    uint16_t* status;
} dm_slot_array;

// map
typedef struct dm_map dm_map;

// byte pool
// basically a growing void ptr. useful for cacheing func params
typedef struct dm_byte_pool_t
{
    size_t size;
    void* data;
} dm_byte_pool;

/*********
RENDERING
***********/
#define DM_DEFAULT_MAX_FPS 60.0f

// render enums
typedef enum dm_buffer_type
{
	DM_BUFFER_TYPE_VERTEX,
	DM_BUFFER_TYPE_INDEX,
    DM_BUFFER_TYPE_UNKNOWN
} dm_buffer_type;

typedef enum dm_buffer_usage
{
    DM_BUFFER_USAGE_DEFAULT,
    DM_BUFFER_USAGE_STATIC,
    DM_BUFFER_USAGE_DYNAMIC,
    DM_BUFFER_USAGE_UNKNOWN
} dm_buffer_usage;

typedef enum dm_buffer_cpu_access
{
    DM_BUFFER_CPU_WRITE,
    DM_BUFFER_CPU_READ
} dm_buffer_cpu_access;

typedef enum dm_shader_type_t
{
    DM_SHADER_TYPE_VERTEX,
    DM_SHADER_TYPE_PIXEL,
    DM_SHADER_TYPE_UNKNOWN
} dm_shader_type;

typedef enum dm_vertex_data_t
{
    DM_VERTEX_DATA_T_BYTE,
    DM_VERTEX_DATA_T_UBYTE,
    DM_VERTEX_DATA_T_SHORT,
    DM_VERTEX_DATA_T_USHORT,
    DM_VERTEX_DATA_T_INT,
    DM_VERTEX_DATA_T_UINT,
    DM_VERTEX_DATA_T_FLOAT,
    DM_VERTEX_DATA_T_DOUBLE,
    DM_VERTEX_DATA_T_MATRIX_INT,
    DM_VERTEX_DATA_T_MATRIX_FLOAT,
    DM_VERTEX_DATA_T_UNKNOWN
} dm_vertex_data_t;

typedef enum dm_vertex_attrib_class_t
{
    DM_VERTEX_ATTRIB_CLASS_VERTEX,
    DM_VERTEX_ATTRIB_CLASS_INSTANCE,
    DM_VERTEX_ATTRIB_CLASS_UNKNOWN
} dm_vertex_attrib_class;

typedef enum dm_comparison_t
{
    DM_COMPARISON_ALWAYS,
    DM_COMPARISON_NEVER,
    DM_COMPARISON_EQUAL,
    DM_COMPARISON_NOTEQUAL,
    DM_COMPARISON_LESS,
    DM_COMPARISON_LEQUAL,
    DM_COMPARISON_GREATER,
    DM_COMPARISON_GEQUAL,
    DM_COMPARISON_UNKNOWN
} dm_comparison;

typedef enum dm_cull_mode_t
{
    DM_CULL_FRONT,
    DM_CULL_BACK,
    DM_CULL_FRONT_BACK,
    DM_CULL_UNKNOWN
} dm_cull_mode;

typedef enum dm_winding_order_t
{
    DM_WINDING_CLOCK,
    DM_WINDING_COUNTER_CLOCK,
    DM_WINDING_UNKNOWN
} dm_winding_order;

typedef enum dm_blend_equation_t
{
    DM_BLEND_EQUATION_ADD,
    DM_BLEND_EQUATION_SUBTRACT,
    DM_BLEND_EQUATION_REVERSE_SUBTRACT,
    DM_BLEND_EQUATION_MIN,
    DM_BLEND_EQUATION_MAX,
    DM_BLEND_EQUATION_UNKNOWN
} dm_blend_equation;

typedef enum dm_blend_func_t
{
    DM_BLEND_FUNC_ZERO,
    DM_BLEND_FUNC_ONE,
    DM_BLEND_FUNC_SRC_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR,
    DM_BLEND_FUNC_DST_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_DST_COLOR,
    DM_BLEND_FUNC_SRC_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
    DM_BLEND_FUNC_DST_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA,
    DM_BLEND_FUNC_CONST_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR,
    DM_BLEND_FUNC_CONST_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA,
    DM_BLEND_FUNC_UNKNOWN
} dm_blend_func;

typedef enum dm_primitive_topology_t
{
    DM_TOPOLOGY_POINT_LIST,
    DM_TOPOLOGY_LINE_LIST,
    DM_TOPOLOGY_LINE_STRIP,
    DM_TOPOLOGY_TRIANGLE_LIST,
    DM_TOPOLOGY_TRIANGLE_STRIP,
    DM_TOPOLOGY_UNKNOWN
} dm_primitive_topology;

typedef enum dm_filter_t
{
    DM_FILTER_NEAREST,
    DM_FILTER_LINEAR,
    DM_FILTER_NEAREST_MIPMAP_NEAREST,
    DM_FILTER_LIENAR_MIPMAP_NEAREST,
    DM_FILTER_NEAREST_MIPMAP_LINEAR,
    DM_FILTER_LINEAR_MIPMAP_LINEAR,
    DM_FILTER_UNKNOWN
} dm_filter;

typedef enum dm_texture_mode_t
{
    DM_TEXTURE_MODE_WRAP,
    DM_TEXTURE_MODE_EDGE,
    DM_TEXTURE_MODE_BORDER,
    DM_TEXTURE_MODE_MIRROR_REPEAT,
    DM_TEXTURE_MODE_MIRROR_EDGE,
    DM_TEXTURE_MODE_UNKNOWN
} dm_texture_mode;

typedef enum dm_uniform_stage_t
{
    DM_UNIFORM_STAGE_VERTEX,
    DM_UNIFORM_STAGE_PIXEL,
    DM_UNIFORM_STAGE_BOTH,
    DM_UNIFORM_STAGE_UNKNOWN
} dm_uniform_stage;

typedef dm_slot_index dm_render_handle;

typedef struct dm_buffer_desc_t
{
    dm_buffer_type type;
    dm_buffer_usage usage;
    dm_buffer_cpu_access cpu_access;
    size_t elem_size;
    size_t buffer_size;
} dm_buffer_desc;

typedef struct dm_uniform_t
{
    size_t           size;
    dm_uniform_stage stage;
    dm_render_handle internal_index;
    char             name[512];
} dm_uniform;

typedef struct dm_vertex_attrib_desc_t
{
    const char*            name;
    dm_vertex_data_t       data_t;
    dm_vertex_attrib_class attrib_class;
    bool                   normalized;
    size_t                 stride;
    size_t                 offset;
    size_t                 count;
    size_t                 index;
} dm_vertex_attrib_desc;

typedef struct dm_pipeline_desc_t
{
    dm_cull_mode          cull_mode;
    dm_winding_order      winding_order;
    dm_blend_equation     blend_eq;
    dm_blend_func         blend_src_f, blend_dest_f;
    dm_filter             sampler_filter;
    dm_texture_mode       u_mode, v_mode, w_mode;
    dm_comparison         blend_comp, depth_comp, stencil_comp, sampler_comp;
    dm_primitive_topology primitive_topology;
    bool                  blend, depth, stencil, wireframe;
    float                 min_lod, max_lod;
} dm_pipeline_desc;

typedef struct dm_shader_desc_t
{
    char vertex[512];
    char pixel[512];
    char master[512];
    
    dm_render_handle vb[2];
    uint32_t         vb_count;
} dm_shader_desc;

typedef enum dm_load_operation_t
{
    DM_LOAD_OPERATION_LOAD,
    DM_LOAD_OPERATION_CLEAR,
    DM_LOAD_OPERATION_DONT_CARE,
    DM_LOAD_OPERATION_UNKNOWN
} dm_load_operation;

typedef enum dm_store_operation_t
{
    DM_STORE_OPERATION_STORE,
    DM_STORE_OPERATION_DONT_CARE,
    DM_STORE_OPERATION_UNKNOWN
} dm_store_operation;

typedef enum dm_renderpass_flag_t
{
    DM_RENDERPASS_FLAG_COLOR   = 1 << 0,
    DM_RENDERPASS_FLAG_DEPTH   = 1 << 1,
    DM_RENDERPASS_FLAG_STENCIL = 1 << 2,
    DM_RENDERPASS_FLAG_UNKNOWN = 1 << 3
} dm_renderpass_flag;

typedef struct dm_renderpass_desc_t
{
    dm_load_operation  color_load_op;
    dm_store_operation color_store_op;
    
    dm_load_operation  color_stencil_load_op;
    dm_store_operation color_stencil_store_op;
    
    dm_load_operation  depth_load_op;
    dm_store_operation depth_store_op;
    
    dm_load_operation  depth_stencil_load_op;
    dm_store_operation depth_stencil_store_op;
    
    dm_renderpass_flag flags;
} dm_renderpass_desc;

typedef struct dm_mesh_t
{
    uint32_t         vertex_count, index_count;
    uint32_t         vertex_offset, index_offset;
    dm_render_handle texture_index;
    float*           positions;
    float*           normals;
    float*           tex_coords;
    uint32_t*        indices;
} dm_mesh;

typedef struct dm_bakedchar
{
    uint16_t x0, x1, y0, y1;
    float x_off, y_off;
    float advance;
} dm_bakedchar;

typedef struct dm_font_t
{
    dm_bakedchar glyphs[96];
    dm_render_handle texture_index;
} dm_font;

typedef enum dm_render_command_type_t
{
    DM_RENDER_COMMAND_SET_TOPOLOGY,
    DM_RENDER_COMMAND_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_END_RENDER_PASS,
    DM_RENDER_COMMAND_SET_VIEWPORT,
    DM_RENDER_COMMAND_CLEAR,
    DM_RENDER_COMMAND_BIND_SHADER,
    DM_RENDER_COMMAND_BIND_PIPELINE,
    DM_RENDER_COMMAND_BIND_BUFFER,
    DM_RENDER_COMMAND_BIND_UNIFORM,
    DM_RENDER_COMMAND_UPDATE_BUFFER,
    DM_RENDER_COMMAND_UPDATE_UNIFORM,
    DM_RENDER_COMMAND_BIND_TEXTURE,
    DM_RENDER_COMMAND_UNBIND_TEXTURE,
    DM_RENDER_COMMAND_UPDATE_TEXTURE,
    DM_RENDER_COMMAND_BIND_DEFAULT_FRAMEBUFFER,
    DM_RENDER_COMMAND_BIND_FRAMEBUFFER,
    DM_RENDER_COMMAND_BIND_FRAMEBUFFER_TEXTURE,
    DM_RENDER_COMMAND_DRAW_ARRAYS,
    DM_RENDER_COMMAND_DRAW_INDEXED,
    DM_RENDER_COMMAND_DRAW_INSTANCED,
    DM_RENDER_COMMAND_TOGGLE_WIREFRAME,
    DM_RENDER_COMMAND_UNKNOWN
} dm_render_command_type;

typedef struct dm_render_command_t
{
    dm_render_command_type type;
    dm_byte_pool           params;
} dm_render_command;

#define DM_MAX_RENDER_COMMANDS 100
typedef struct dm_render_command_manager_t
{
    dm_render_command commands[DM_MAX_RENDER_COMMANDS];
    uint32_t          command_count;
} dm_render_command_manager;

#define DM_RENDERER_MAX_RESOURCE_COUNT 100
typedef struct dm_renderer_t
{
    dm_font  fonts[DM_RENDERER_MAX_RESOURCE_COUNT];
    uint32_t font_count;
    
    dm_render_command_manager command_manager;
    
    uint32_t width, height;
#ifdef DM_OPENGL
    uint32_t uniform_bindings;
#endif
    
    bool vsync;
    
    void* internal_renderer;
} dm_renderer;

/***
ECS
*****/
#ifndef DM_ECS_MAX_ENTITIES
#define DM_ECS_MAX_ENTITIES 1024
#endif

#define DM_ECS_COMPONENT_BLOCK_SIZE     512
#define DM_ECS_INV_COMPONENT_BLOCK_SIZE 0.001953125f   // TODO: this is inverse 512, so if above changes, this should too

#ifdef DM_ECS_MORE_COMPONENTS
typedef uint64_t dm_ecs_id;
#define DM_ECS_MAX 64
#define DM_ECS_INVALID_ID ULONG_MAX
#else
typedef uint32_t dm_ecs_id;
#define DM_ECS_MAX 32
#define DM_ECS_INVALID_ID UINT_MAX
#endif

#ifdef DM_ECS_MORE_COMPONENT_MEMBERS
#define DM_ECS_MAX_COMPONENT_MEMBER_NUM 64
#else
#define DM_ECS_MAX_COMPONENT_MEMBER_NUM 32
#endif

typedef uint32_t dm_entity;
#define DM_ECS_INVALID_ENTITY UINT_MAX

typedef enum dm_ecs_system_timing_t
{
    DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN,
    DM_ECS_SYSTEM_TIMING_UPDATE_END,
    DM_ECS_SYSTEM_TIMING_RENDER_BEGIN,
    DM_ECS_SYSTEM_TIMING_RENDER_END,
    DM_ECS_SYSTEM_TIMING_UNKNOWN
} dm_ecs_system_timing;

typedef struct dm_ecs_system_manager_t
{
    uint32_t  component_count;
    dm_ecs_id component_mask;
    dm_ecs_id component_ids[DM_ECS_MAX];
    
    uint32_t entity_indices[DM_ECS_MAX_ENTITIES][DM_ECS_MAX];
    uint32_t entity_count;
    
    bool (*run_func)(void*,void*);
    void (*shutdown_func)(void*,void*);
    
    void* system_data;
} dm_ecs_system_manager;

typedef struct dm_ecs_component_manager_t
{
    size_t   size;
    uint32_t entity_count;
    void*    data;
} dm_ecs_component_manager;

typedef struct dm_ecs_manager_t
{
    // entities; indexed via hashing
    dm_entity entities_ordered[DM_ECS_MAX_ENTITIES];
    dm_entity entities[DM_ECS_MAX_ENTITIES];
    uint32_t  entity_count;
    
    // precomputed entity hash data
    uint32_t   entity_component_indices[DM_ECS_MAX_ENTITIES][DM_ECS_MAX];
    dm_ecs_id  entity_component_masks[DM_ECS_MAX_ENTITIES];
    
    // components
    dm_ecs_component_manager components[DM_ECS_MAX];
    uint32_t                 num_registered_components;
    
    // systems
    dm_ecs_system_manager systems[DM_ECS_SYSTEM_TIMING_UNKNOWN][DM_ECS_MAX];
    uint32_t              num_registered_systems[DM_ECS_SYSTEM_TIMING_UNKNOWN];
} dm_ecs_manager;

/*******
PHYSICS
*********/
#define DM_PHYSICS_MAX_GJK_ITER      64
#define DM_PHYSICS_EPA_MAX_FACES     64
#define DM_PHYSICS_EPA_MAX_ITER      64

#ifndef DM_DEBUG
#define DM_PHYSICS_FIXED_DT          0.00416f // 1 / 240
#define DM_PHYSICS_FIXED_DT_INV      240
#else
#define DM_PHYSICS_FIXED_DT          0.00833f // 1 / 120
#define DM_PHYSICS_FIXED_DT_INV      120
#endif

typedef struct dm_plane
{
    float normal[3];
    float distance;
} dm_plane;

typedef struct dm_simplex
{
    float    points[4][3];
    uint32_t size;
} dm_simplex;

typedef struct dm_contact_constraint
{
    float  jacobian[4][3];
    float  delta_v[4][3];
    float  b;
    double lambda, warm_lambda, impulse_sum, impulse_min, impulse_max;
} dm_contact_constraint;

typedef struct dm_contact_point
{
    dm_contact_constraint normal;
    dm_contact_constraint friction_a, friction_b;
    
    float global_pos[2][3];
    float local_pos[2][3];
    float penetration;
} dm_contact_point;

typedef struct dm_contact_data_t
{
    float mass, inv_mass;
    float i_body_inv_00, i_body_inv_11, i_body_inv_22;
    float v_damp, w_damp;
    
    float *vel_x, *vel_y, *vel_z;
    float *w_x, *w_y, *w_z;
} dm_contact_data;

#define DM_PHYSICS_CLIP_MAX_PTS 15
typedef struct dm_contact_manifold
{
    float     normal[3];
    float     tangent_a[3]; 
    float     tangent_b[3];
    float     orientation_a[4];
    float     orientation_b[4];
    
    dm_contact_data contact_data[2];
    
    dm_contact_point points[DM_PHYSICS_CLIP_MAX_PTS];
    uint32_t         point_count;
    
    uint32_t  entity_a, entity_b;
} dm_contact_manifold;

// supported collision shapes
typedef enum dm_collision_shape_t
{
    DM_COLLISION_SHAPE_SPHERE,
    DM_COLLISION_SHAPE_BOX,
    DM_COLLISION_SHAPE_UNKNOWN
} dm_collision_shape;

/******************
DARKMATTER CONTEXT
********************/
typedef struct dm_context_init_packet_t
{
    uint32_t window_x, window_y;
    uint32_t window_width, window_height;
    char     window_title[512];
    char     asset_folder[512];
} dm_context_init_packet;

typedef struct dm_context_t
{
    dm_platform_data   platform_data;
    dm_input_state     input_states[2];
    dm_renderer        renderer;
    dm_ecs_manager     ecs_manager;
    
    double             start, end, delta;
    mt19937            random;
    mt19937_64         random_64;
    uint32_t           flags;
    
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
void  dm_free(void* block);
void* dm_memset(void* dest, int value, size_t size);
void* dm_memzero(void* block, size_t size);
void* dm_memcpy(void* dest, const void* src, size_t size);
void  dm_memmove(void* dest, const void* src, size_t size);

// math
int   dm_abs(int x);
float dm_fabs(float x);
float dm_sqrtf(float x);
float dm_math_angle_xy(float x, float y);
float dm_sin(float angle);
float dm_cos(float angle);
float dm_tan(float angle);
float dm_sind(float angle);
float dm_cosd(float angle);
float dm_asin(float value);
float dm_acos(float value);
float dm_atan(float x, float y);
float dm_smoothstep(float edge0, float edge1, float x);
float dm_smootherstep(float edge0, float edge1, float x);
float dm_exp(float x);
float dm_powf(float x, float y);
float dm_roundf(float x);
int   dm_round(float x);
int   dm_ceil(float x);
int   dm_floor(float x);
float dm_logf(float x);
float dm_log2f(float x);
bool  dm_isnan(float x);

#include "dm_math.h"

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
int      dm_input_get_mouse_scroll(dm_context* context);
int      dm_input_get_prev_mouse_x(dm_context* context);
int      dm_input_get_prev_mouse_y(dm_context* context);
void     dm_input_get_prev_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context);
int      dm_input_get_prev_mouse_scroll(dm_context* context);

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

// events for platform
void dm_add_window_close_event(dm_event_list* event_list);
void dm_add_window_resize_event(uint32_t new_width, uint32_t new_height, dm_event_list* event_list);
void dm_add_mousebutton_down_event(dm_mousebutton_code button, dm_event_list* event_list);
void dm_add_mousebutton_up_event(dm_mousebutton_code button, dm_event_list* event_list);
void dm_add_mouse_move_event(uint32_t mouse_x, uint32_t mouse_y, dm_event_list* event_list);
void dm_add_mouse_scroll_event(uint32_t delta, dm_event_list* event_list);
void dm_add_key_down_event(dm_key_code key, dm_event_list* event_list);
void dm_add_key_up_event(dm_key_code key, dm_event_list* event_list);

// random
int dm_random_int(dm_context* context);
int dm_random_int_range(int start, int end, dm_context* context);
uint32_t dm_random_uint32(dm_context* context);
uint32_t dm_random_uint32_range(uint32_t start, uint32_t end, dm_context* context);
uint64_t dm_random_uint64(dm_context* context);
uint64_t dm_random_uint64_range(uint64_t start, uint64_t end, dm_context* context);
float dm_random_float(dm_context* context);
float dm_random_float_range(float start, float end,dm_context* context);
double dm_random_double(dm_context* context);
double dm_random_double_range(double start, double end, dm_context* context);

// rendering
bool dm_renderer_create_static_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_dynamic_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_static_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_dynamic_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_context* context);
bool dm_renderer_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_texture_from_file(const char* path, uint32_t n_channels, bool flipped, const char* name, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_texture_from_data(uint32_t width, uint32_t height, uint32_t n_channels, void* data, const char* name, dm_render_handle* handle, dm_context* context);
bool dm_renderer_load_font(const char* path, dm_render_handle* handle, dm_context* context);

#define DM_MAKE_VERTEX_ATTRIB(NAME, STRUCT, MEMBER, CLASS, DATA_T, COUNT, INDEX, NORMALIZED) { .name=NAME, .data_t=DATA_T, .attrib_class=CLASS, .stride=sizeof(STRUCT), .offset=offsetof(STRUCT, MEMBER), .count=COUNT, .index=INDEX, .normalized=NORMALIZED}

dm_pipeline_desc dm_renderer_default_pipeline();

// render commands
void dm_render_command_clear(float r, float g, float b, float a, dm_context* context);
void dm_render_command_set_viewport(uint32_t width, uint32_t height, dm_context* context);
void dm_render_command_set_default_viewport(dm_context* context);
void dm_render_command_set_primitive_topology(dm_primitive_topology topology, dm_context* context);
void dm_render_command_bind_shader(dm_render_handle handle, dm_context* context);
void dm_render_command_bind_pipeline(dm_render_handle handle, dm_context* context);
void dm_render_command_bind_buffer(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_bind_uniform(dm_render_handle uniform_handle, uint32_t slot, dm_uniform_stage stage, uint32_t offset, dm_context* context);
void dm_render_command_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_context* context);
void dm_render_command_update_uniform(dm_render_handle uniform_handle, void* data, size_t data_size, dm_context* context);
void dm_render_command_bind_texture(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_context* context);
void dm_render_command_bind_default_framebuffer(dm_context* context);
//void dm_render_command_bind_framebuffer(dm_render_handle handle, dm_context* context);
//void dm_render_command_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_context* context);
void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_context* context);
void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_context* context);
void dm_render_command_toggle_wireframe(bool wireframe, dm_context* context);

// ecs
dm_ecs_id dm_ecs_register_component(size_t component_block_size, dm_context* context);
dm_ecs_id dm_ecs_register_system(dm_ecs_id* component_ids, uint32_t component_count, dm_ecs_system_timing timing, bool (*run_func)(void*,void*), void (*shutdown_func)(void*,void*), dm_context* context);

dm_entity dm_ecs_create_entity(dm_context* context);

void* dm_ecs_get_component_block(dm_ecs_id component_id, dm_context* context);
void  dm_ecs_get_component_count(dm_ecs_id component_id, uint32_t* index, dm_context* context);
void  dm_ecs_entity_add_component(dm_entity entity, dm_ecs_id component_id, dm_context* context);

// physics
bool dm_physics_gjk(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const dm_collision_shape shapes[2], float supports[2][3], dm_simplex* simplex);
void dm_physics_epa(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const dm_collision_shape shapes[2], float penetration[3], float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3], float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3], uint32_t* polytope_count, dm_simplex* simplex);
bool dm_physics_collide_entities(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold);
void dm_physics_constraint_lambda(dm_contact_constraint* constraint, dm_contact_manifold* manifold);
void dm_physics_constraint_apply(dm_contact_constraint* constraint, dm_contact_manifold* manifold);
void dm_physics_apply_constraints(dm_contact_manifold* manifold);

// framework funcs
dm_context* dm_init(dm_context_init_packet init_packet);
void        dm_shutdown(dm_context* context);

void        dm_start(dm_context* context);
void        dm_end(dm_context* context);

bool        dm_update_begin(dm_context* context);
bool        dm_update_end(dm_context* context);
bool        dm_renderer_begin_frame(dm_context* context);
bool        dm_renderer_end_frame(dm_context* context);
bool        dm_context_is_running(dm_context* context);

void* dm_read_bytes(const char* path, const char* mode, size_t* size);

uint32_t __dm_get_screen_width(dm_context* context);
uint32_t __dm_get_screen_height(dm_context* context);

#define DM_SCREEN_WIDTH(CONTEXT)  __dm_get_screen_width(context)
#define DM_SCREEN_HEIGHT(CONTEXT) __dm_get_screen_height(context)

#define DM_ARRAY_LEN(ARRAY) sizeof(ARRAY) / sizeof(ARRAY[0])

void dm_grow_dyn_array(void** array, uint32_t count, uint32_t* capacity, size_t elem_size, float load_factor, uint32_t resize_factor);

void dm_qsort(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b));
#ifdef DM_LINUX
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b, void* c), void* d);
#else
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(void* c, const void* a, const void* b), void* d);
#endif

// timer
void   dm_timer_start(dm_timer* timer, dm_context* context);
void   dm_timer_restart(dm_timer* timer, dm_context* context);
double dm_timer_elapsed(dm_timer* timer, dm_context* context);
double dm_timer_elapsed_ms(dm_timer* timer, dm_context* context);

/*****************
INLINED FUNCTIONS
*******************/
DM_INLINE
dm_hash64 dm_hash_fnv1a(const char* key)
{
    dm_hash64 hash = 14695981039346656037UL;
	for (int i = 0; key[i]; i++)
	{
		hash ^= key[i];
		hash *= 1099511628211;
	}
    
	return hash;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
DM_INLINE
dm_hash dm_hash_32bit(uint32_t key)
{
    dm_hash hash = ((key >> 16) ^ key)   * 0x119de1f3;
    hash         = ((hash >> 16) ^ hash) * 0x119de1f3;
    hash         = (hash >> 16) ^ hash;
    
    return hash;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
DM_INLINE
dm_hash64 dm_hash_64bit(uint64_t key)
{
	dm_hash64 hash = (key ^ (key >> 30))   * UINT64_C(0xbf58476d1ce4e5b9);
	hash           = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
	hash           = hash ^ (hash >> 31);
    
	return hash;
}

// http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
DM_INLINE
dm_hash dm_hash_int_pair(int x, int y)
{
    if(y < x) return ( y << 16 ) + x;
    
    return (x << 16) + y;
}

// https://stackoverflow.com/questions/682438/hash-function-providing-unique-uint-from-an-integer-coordinate-pair
DM_INLINE
dm_hash64 dm_hash_uint_pair(uint32_t x, uint32_t y)
{
    if(y < x) return ( (uint64_t)y << 32 ) ^ x;
    
    return ((uint64_t)x << 32) ^ y;
}

// alternative to modulo: https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
DM_INLINE
uint32_t dm_hash_reduce(uint32_t x, uint32_t n)
{
    return ((uint64_t)x * (uint64_t)n) >> 32;
}

DM_INLINE
uint32_t dm_ecs_entity_get_index(dm_entity entity, dm_context* context)
{
    //const uint32_t index = dm_hash_32bit(entity) % context->ecs_manager.entity_capacity;
    const uint32_t index = entity % DM_ECS_MAX_ENTITIES;
    dm_entity* entities = context->ecs_manager.entities;
    
    if(entities[index]==entity) return index;
    
    uint32_t runner = index + 1;
    if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    
    while(runner != index)
    {
        if(entities[runner]==entity) return runner;
        
        runner++;
        if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    }
    
    DM_LOG_FATAL("Could not find entity index, should not be here...");
    return DM_ECS_INVALID_ENTITY;
}

DM_INLINE
uint32_t dm_ecs_entity_get_component_index(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==DM_ECS_INVALID_ENTITY) return DM_ECS_INVALID_ENTITY;
    
    return context->ecs_manager.entity_component_indices[entity_index][component_id];
}

DM_INLINE
bool dm_ecs_entity_has_component(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==DM_ECS_INVALID_ENTITY) return false;
    
    return context->ecs_manager.entity_component_masks[entity_index] & component_id;
}

DM_INLINE
bool dm_ecs_entity_has_component_multiple(dm_entity entity, dm_ecs_id component_mask, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==DM_ECS_INVALID_ENTITY) return false;
    
    dm_ecs_id entity_mask = context->ecs_manager.entity_component_masks[entity_index];
    
    // NAND entity mask with opposite of component mask
    dm_ecs_id result = ~(entity_mask & ~component_mask);
    // XOR with opposite of entity mask
    result ^= ~entity_mask;
    // success only if this equals the component mask
    return (result == component_mask);
}

/**********
INTRINSICS
************/
#include "dm_intrinsics.h"

/**************
IMPLEMENTATION
****************/
#ifdef DM_IMPLEMENTATION
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>

#include "mt19937/mt19937.h"
#include "mt19937/mt19937_64.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

// TODO: REMOVE
#include "rendering/imgui_render_pass.h"

/*********
BYTE POOL
***********/
void __dm_byte_pool_insert(dm_byte_pool* byte_pool, void* data, size_t data_size)
{
    if(!byte_pool->data) byte_pool->data = dm_alloc(data_size);
    else                 byte_pool->data = dm_realloc(byte_pool->data, byte_pool->size + data_size);
    
    void* dest = (char*)byte_pool->data + byte_pool->size;
    dm_memcpy(dest, data, data_size);
    byte_pool->size += data_size;
}

#define DM_BYTE_POOL_INSERT(BYTE_POOL, DATA) __dm_byte_pool_insert(&BYTE_POOL, &DATA, sizeof(DATA))

void* __dm_byte_pool_pop(dm_byte_pool* byte_pool, size_t data_size)
{
    if(!byte_pool->size)
    {
        DM_LOG_ERROR("Trying to pop from an empty byte pool");
        return NULL;
    }
    
    void* result = (char*)byte_pool->data + byte_pool->size - data_size;
    byte_pool->size -= data_size;
    return result;
}
#define DM_BYTE_POOL_POP(BYTE_POOL, T, NAME) T NAME = *(T*)__dm_byte_pool_pop(&BYTE_POOL, sizeof(T))

/****
MATH
******/
int dm_abs(int x)
{
    return abs(x);
}

float dm_sqrtf(float x)
{
    return sqrtf(x);
}

float dm_fabs(float x)
{
    return fabsf(x);
}

float dm_math_angle_xy(float x, float y)
{
	float theta = 0.0f;
    
	if (x >= 0.0f)
	{
		theta = dm_atan(x, y);
		if (theta < 0.0f) theta += 2.0f * DM_MATH_PI;
	}
	else theta = dm_atan(x, y) + DM_MATH_PI;
    
	return theta;
}

float dm_sin(float angle)
{
	return sinf(angle);
}

float dm_cos(float angle)
{
	return cosf(angle);
}

float dm_tan(float angle)
{
	return tanf(angle);
}

float dm_sind(float angle)
{
    return dm_sin(angle * DM_MATH_DEG_TO_RAD);
}

float dm_cosd(float angle)
{
    return dm_cos(angle * DM_MATH_DEG_TO_RAD);
}

float dm_asin(float value)
{
    return asinf(value);
}

float dm_acos(float value)
{
    return acosf(value);
}

float dm_atan(float x, float y)
{
    return atan2f(y, x);
}

float dm_smoothstep(float edge0, float edge1, float x)
{
    float h = (x - edge0) / (edge1 - edge0);
    h = DM_CLAMP(h, 0, 1);
    return h * h * (3 - 2 * h);
}

float dm_smootherstep(float edge0, float edge1, float x)
{
    float h = (x - edge0) / (edge1 - edge0);
    h = DM_CLAMP(h, 0, 1);
    return h * h * h * (h * (h * 6 - 15) + 10);
}

float dm_exp(float x)
{
    return expf(x);
}

float dm_powf(float x, float y)
{
    return powf(x, y);
}

float dm_roundf(float x)
{
    return roundf(x);
}

int dm_round(float x)
{
    return (int)round(x);
}

int dm_ceil(float x)
{
    return (int)ceil(x);
}

int dm_floor(float x)
{
    return (int)floor(x);
}

float dm_logf(float x)
{
    return logf(x);
}

float dm_log2f(float x)
{
    return log2f(x);
}

bool dm_isnan(float x)
{
    return isnan(x);
}

/******
MEMORY
********/
void* dm_alloc(size_t size)
{
    void* temp = malloc(size);
    if(!temp) return NULL;
    dm_memzero(temp, size);
    return temp;
}

void* dm_calloc(size_t count, size_t size)
{
    void* temp = calloc(count, size);
    if(!temp) return NULL;
    return temp;
}

void* dm_realloc(void* block, size_t size)
{
    void* temp = realloc(block, size);
    if(temp) block = temp;
    return block;
}

void dm_free(void* block)
{
    free(block);
    block = NULL;
}

void* dm_memset(void* dest, int value, size_t size)
{
    return memset(dest, value, size);
}

void* dm_memzero(void* block, size_t size)
{
    return dm_memset(block, 0, size);
}

void* dm_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest, src, size);
}

void dm_memmove(void* dest, const void* src, size_t size)
{
    memmove(dest, src, size);
}

/***********
RANDOM IMPL
*************/
int dm_random_int(dm_context* context)
{
    return dm_random_uint32(context) - INT_MAX;
}

int dm_random_int_range(int start, int end, dm_context* context)
{
    int old_range = UINT_MAX;
    int new_range = end - start;
    
    return ((dm_random_int(context) + INT_MAX) * new_range) / old_range + start;
}

uint32_t dm_random_uint32(dm_context* context)
{
    return genrand_int32(&context->random);
}

uint32_t dm_random_uint32_range(uint32_t start, uint32_t end, dm_context* context)
{
    uint32_t old_range = UINT_MAX;
    uint32_t new_range = end - start;
    
    return (dm_random_uint32(context) * new_range) / old_range + start;
}

uint64_t dm_random_uint64(dm_context* context)
{
    return genrand64_uint64(&context->random_64);
}

uint64_t dm_random_uint64_range(uint64_t start, uint64_t end, dm_context* context)
{
    uint64_t old_range = ULLONG_MAX;
    uint64_t new_range = end - start;
    
    return (dm_random_uint64(context) - new_range) / old_range + start;
}

float dm_random_float(dm_context* context)
{
    return genrand_float32_full(&context->random);
}

float dm_random_float_range(float start, float end, dm_context* context)
{
    float range = end - start;
    
    return dm_random_float(context) * range + start;
}

double dm_random_double(dm_context* context)
{
    return genrand64_real1(&context->random_64);
}

double dm_random_double_range(double start, double end, dm_context* context)
{
    double range = end - start;
    
    return dm_random_double(context) * range + start;
}

/*****
INPUT
*******/
void dm_input_clear_keyboard(dm_context* context)
{
	dm_memzero(context->input_states[0].keyboard.keys, sizeof(bool) * 256);
}

void dm_input_reset_mouse_x(dm_context* context)
{
	context->input_states[0].mouse.x = 0;
}

void dm_input_reset_mouse_y(dm_context* context)
{
	context->input_states[0].mouse.y = 0;
}

void dm_input_reset_mouse_pos(dm_context* context)
{
	dm_input_reset_mouse_x(context);
	dm_input_reset_mouse_y(context);
}

void dm_input_reset_mouse_scroll(dm_context* context)
{
	context->input_states[0].mouse.scroll = 0;
}

void dm_input_clear_mousebuttons(dm_context* context)
{
	dm_memzero(context->input_states[0].mouse.buttons, sizeof(bool) * 3);
}

void dm_input_set_key_pressed(dm_key_code key, dm_context* context)
{
	context->input_states[0].keyboard.keys[key] = 1;
}

void dm_input_set_key_released(dm_key_code key, dm_context* context)
{
	context->input_states[0].keyboard.keys[key] = 0;
}

void dm_input_set_mousebutton_pressed(dm_mousebutton_code button, dm_context* context)
{
	context->input_states[0].mouse.buttons[button] = 1;
}

void dm_input_set_mousebutton_released(dm_mousebutton_code button, dm_context* context)
{
	context->input_states[0].mouse.buttons[button] = 0;
}

void dm_input_set_mouse_x(int x, dm_context* context)
{
	context->input_states[0].mouse.x = x;
}

void dm_input_set_mouse_y(int y, dm_context* context)
{
	context->input_states[0].mouse.y = y;
}

void dm_input_set_mouse_scroll(int delta, dm_context* context)
{
	context->input_states[0].mouse.scroll += delta;
}

void dm_input_update_state(dm_context* context)
{
	dm_memcpy(&context->input_states[1], &context->input_states[0], sizeof(dm_input_state));
}

bool dm_input_is_key_pressed(dm_key_code key, dm_context* context)
{
	return context->input_states[0].keyboard.keys[key];
}

bool dm_input_is_mousebutton_pressed(dm_mousebutton_code button, dm_context* context)
{
	return context->input_states[0].mouse.buttons[button];
}

bool dm_input_key_just_pressed(dm_key_code key, dm_context* context)
{
	return ((context->input_states[0].keyboard.keys[key] == 1) && (context->input_states[1].keyboard.keys[key] == 0));
}

bool dm_input_key_just_released(dm_key_code key, dm_context* context)
{
	return ((context->input_states[0].keyboard.keys[key] == 0) && (context->input_states[1].keyboard.keys[key] == 1));
}

bool dm_input_mousebutton_just_pressed(dm_mousebutton_code button, dm_context* context)
{
	return ((context->input_states[0].mouse.buttons[button] == 1) && (context->input_states[1].mouse.buttons[button] == 0));
}

bool dm_input_mousebutton_just_released(dm_mousebutton_code button, dm_context* context)
{
	return ((context->input_states[0].mouse.buttons[button] == 0) && (context->input_states[1].mouse.buttons[button] == 1));
}

bool dm_input_mouse_has_moved(dm_context* context)
{
	return ((context->input_states[0].mouse.x != context->input_states[1].mouse.x) || (context->input_states[0].mouse.y != context->input_states[1].mouse.y));
}

bool dm_input_mouse_has_scrolled(dm_context* context)
{
    return (context->input_states[0].mouse.scroll != context->input_states[1].mouse.scroll && context->input_states[0].mouse.scroll != 0);
}

uint32_t dm_input_get_mouse_x(dm_context* context)
{
	return context->input_states[0].mouse.x;
}

uint32_t dm_input_get_mouse_y(dm_context* context)
{
	return context->input_states[0].mouse.y;
}

int dm_input_get_mouse_delta_x(dm_context* context)
{
	return context->input_states[0].mouse.x - context->input_states[1].mouse.x;
}

int dm_input_get_mouse_delta_y(dm_context* context)
{
	return context->input_states[0].mouse.y - context->input_states[1].mouse.y;
}

void dm_input_get_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context)
{
	*x = context->input_states[0].mouse.x;
	*y = context->input_states[0].mouse.y;
}

void dm_input_get_mouse_delta(int* x, int* y, dm_context* context)
{
	*x = dm_input_get_mouse_delta_x(context);
	*y = dm_input_get_mouse_delta_y(context);
}

int dm_input_get_prev_mouse_x(dm_context* context)
{
	return context->input_states[1].mouse.x;
}

int dm_input_get_prev_mouse_y(dm_context* context)
{
	return context->input_states[1].mouse.y;
}

void dm_input_get_prev_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context)
{
	*x = context->input_states[1].mouse.x;
	*y = context->input_states[1].mouse.y;
}

int dm_input_get_mouse_scroll(dm_context* context)
{
	return context->input_states[0].mouse.scroll;
}

int dm_input_get_prev_mouse_scroll(dm_context* context)
{
	return context->input_states[1].mouse.scroll;
}

/*****
EVENT
*******/
void dm_add_window_close_event(dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_WINDOW_CLOSE;
}

void dm_add_window_resize_event(uint32_t new_width, uint32_t new_height, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_WINDOW_RESIZE;
    e->new_rect[0] = new_width;
    e->new_rect[1] = new_height;
}

void dm_add_mousebutton_down_event(dm_mousebutton_code button, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSEBUTTON_DOWN;
    e->button = button;
}

void dm_add_mousebutton_up_event(dm_mousebutton_code button, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSEBUTTON_UP;
    e->button = button;
}

void dm_add_mouse_move_event(uint32_t mouse_x, uint32_t mouse_y, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSE_MOVE;
    e->coords[0] = mouse_x;
    e->coords[1] = mouse_y;
}

void dm_add_mouse_scroll_event(uint32_t delta, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSE_SCROLL;
    e->delta = delta;
}

void dm_add_key_down_event(dm_key_code key, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_KEY_DOWN;
    e->key = key;
}

void dm_add_key_up_event(dm_key_code key, dm_event_list* event_list)
{
    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_KEY_UP;
    e->key = key;
}

/********
PLATFORM
**********/
extern bool   dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_platform_data* platform_data);
extern void   dm_platform_shutdown(dm_platform_data* platform_data);
extern double dm_platform_get_time(dm_platform_data* platform_data);
extern void   dm_platform_write(const char* message, uint8_t color);
extern bool   dm_platform_pump_events(dm_platform_data* platform_data);

/*******
LOGGING
*********/
void __dm_log_output(log_level level, const char* message, ...)
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

/*********
RENDERING
***********/
extern bool dm_renderer_backend_init(dm_context* context);
extern void dm_renderer_backend_shutdown(dm_context* context);
extern bool dm_renderer_backend_begin_frame(dm_renderer* renderer);
extern bool dm_renderer_backend_end_frame(dm_context* context);
extern void dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer);

extern bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer);

extern void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer);
extern void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer);
extern bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer);
extern bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer);
extern bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer);
extern bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer);
extern void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer);
extern void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer);
extern void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer);
extern void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer);

// renderer
bool dm_renderer_init(dm_context* context)
{
    if(!dm_renderer_backend_init(context)) { DM_LOG_FATAL("Could not initialize renderer backend"); return false; }
    
    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_renderer_backend_shutdown(context);
}

// buffer
bool dm_renderer_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_buffer(desc, data, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not create buffer");
    return false;
}

bool dm_renderer_create_static_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_VERTEX,
        .usage=DM_BUFFER_USAGE_STATIC,
        .cpu_access=DM_BUFFER_CPU_WRITE,
        .buffer_size=data_size,
        .elem_size=vertex_size
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_renderer_create_dynamic_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_VERTEX,
        .usage=DM_BUFFER_USAGE_DYNAMIC,
        .cpu_access=DM_BUFFER_CPU_READ,
        .buffer_size=data_size,
        .elem_size=vertex_size
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_renderer_create_static_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_INDEX,
        .usage=DM_BUFFER_USAGE_STATIC,
        .cpu_access=DM_BUFFER_CPU_WRITE,
        .buffer_size=data_size,
        .elem_size=sizeof(uint32_t)
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_renderer_create_dynamic_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_INDEX,
        .usage=DM_BUFFER_USAGE_DYNAMIC,
        .cpu_access=DM_BUFFER_CPU_READ,
        .buffer_size=data_size,
        .elem_size=sizeof(uint32_t)
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_renderer_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_context* context)
{
    if(dm_renderer_backend_create_shader_and_pipeline(shader_desc, pipe_desc, attrib_descs, attrib_count, shader_handle, pipe_handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Creating shader and pipeline failed");
    return false;
}

bool dm_renderer_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_uniform(size, stage, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Creating uniform failed");
    return false;
}

bool dm_renderer_create_texture_from_file(const char* path, uint32_t n_channels, bool flipped, const char* name, dm_render_handle* handle, dm_context* context)
{
    DM_LOG_DEBUG("Loading image: %s", path);
    
    int width, height;
    int num_channels = n_channels;
#if defined(DM_DIRECTX) || defined(DM_METAL)
    num_channels = 4;
#endif
    
    stbi_set_flip_vertically_on_load(flipped);
    unsigned char* data = stbi_load(path, &width, &height, &num_channels, num_channels);
    
    if(!data)
    {
        // TODO: Need to not crash, but have a default texture
        DM_LOG_FATAL("Failed to load image: %s", path);
        return false;
    }
    
    n_channels = num_channels;
    if(!dm_renderer_backend_create_texture(width, height, n_channels, data, name, handle, &context->renderer))
    {
        DM_LOG_FATAL("Failed to create texture from file: %s", path);
        stbi_image_free(data);
        return false;
    }
    
    stbi_image_free(data);
    
    return true;
}

bool dm_renderer_create_texture_from_data(uint32_t width, uint32_t height, uint32_t n_channels, void* data, const char* name, dm_render_handle* handle, dm_context* context)
{
    if(!dm_renderer_backend_create_texture(width, height, n_channels, data, name, handle, &context->renderer))
    {
        DM_LOG_FATAL("Failed to create texture from data");
        return false;
    }
    
    return true;
}

bool dm_renderer_load_font(const char* path, dm_render_handle* handle, dm_context* context)
{
    DM_LOG_DEBUG("Loading font: %s", path);
    
    size_t size = 0;
    void* buffer = dm_read_bytes(path, "rb", &size);
    
    if(!buffer) 
    {
        DM_LOG_FATAL("Font file '%s' not found");
        return false;
    }
    
    stbtt_fontinfo info;
    if(!stbtt_InitFont(&info, buffer, 0))
    {
        DM_LOG_FATAL("Could not initialize font: %s", path);
        return false;
    }
    
    uint32_t w = 512;
    uint32_t h = 512;
    uint32_t n_channels = 4;
    
    unsigned char* alpha_bitmap = dm_alloc(w * h);
    unsigned char* bitmap = dm_alloc(w * h * n_channels);
    dm_memzero(alpha_bitmap, w * h);
    dm_memzero(bitmap, w * h * n_channels);
    
    dm_font font = { 0 };
    stbtt_BakeFontBitmap(buffer, 0, 16, alpha_bitmap, 512, 512, 32, 96, (stbtt_bakedchar*)font.glyphs);
    
    // bitmaps are single alpha values, so make 4 channel texture
    for(uint32_t y=0; y<h; y++)
    {
        for(uint32_t x=0; x<w; x++)
        {
            uint32_t index = y * w + x;
            uint32_t bitmap_index = index * n_channels;
            unsigned char a = alpha_bitmap[index];
            
            bitmap[bitmap_index]     = 255;
            bitmap[bitmap_index + 1] = 255;
            bitmap[bitmap_index + 2] = 255;
            bitmap[bitmap_index + 3] = a;
        }
    }
    
    font.texture_index = -1;
    if(!dm_renderer_create_texture_from_data(w, h, n_channels, bitmap, "font_texture", &font.texture_index, context)) 
    { 
        DM_LOG_FATAL("Could not create texture for font: %s"); 
        return false; 
    }
    
    dm_free(alpha_bitmap);
    dm_free(bitmap);
    dm_free(buffer);
    
    dm_memcpy(context->renderer.fonts + context->renderer.font_count, &font, sizeof(dm_font));
    *handle = context->renderer.font_count++;
    
    return true;
}

void __dm_renderer_submit_render_command(dm_render_command* command, dm_render_command_manager* manager)
{
    dm_render_command* c = &manager->commands[manager->command_count++];
    c->type = command->type;
    c->params.size = command->params.size;
    if(!c->params.data) c->params.data = dm_alloc(command->params.size);
    else c->params.data = dm_realloc(c->params.data, command->params.size);
    dm_memcpy(c->params.data, command->params.data, command->params.size);
    
    dm_free(command->params.data);
}
#define DM_SUBMIT_RENDER_COMMAND(COMMAND) __dm_renderer_submit_render_command(&COMMAND, &context->renderer.command_manager)
#define DM_SUBMIT_RENDER_COMMAND_MANAGER(COMMAND, MANAGER) __dm_renderer_submit_render_command(&COMMAND, &MANAGER)
#define DM_TOO_MANY_COMMANDS context->renderer.command_manager.command_count > DM_MAX_RENDER_COMMANDS

void dm_render_command_clear(float r, float g, float b, float a, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_CLEAR;
    
    DM_BYTE_POOL_INSERT(command.params, r);
    DM_BYTE_POOL_INSERT(command.params, g);
    DM_BYTE_POOL_INSERT(command.params, b);
    DM_BYTE_POOL_INSERT(command.params, a);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_set_viewport(uint32_t width, uint32_t height, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_SET_VIEWPORT;
    
    DM_BYTE_POOL_INSERT(command.params, width);
    DM_BYTE_POOL_INSERT(command.params, height);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_set_default_viewport(dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_SET_VIEWPORT;
    
    DM_BYTE_POOL_INSERT(command.params, context->platform_data.window_data.width);
    DM_BYTE_POOL_INSERT(command.params, context->platform_data.window_data.height);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_set_primitive_topology(dm_primitive_topology topology, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_SET_TOPOLOGY;
    
    DM_BYTE_POOL_INSERT(command.params, topology);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

#if 0
void dm_render_command_begin_renderpass(dm_render_handle pass_index, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BEGIN_RENDER_PASS;
    
    DM_BYTE_POOL_INSERT(command.params, pass_index);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_end_renderpass(dm_render_handle pass_index, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_END_RENDER_PASS;
    
    DM_BYTE_POOL_INSERT(command.params, pass_index);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}
#endif

void dm_render_command_bind_shader(dm_render_handle handle, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_SHADER;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_pipeline(dm_render_handle handle, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_PIPELINE;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_buffer(dm_render_handle handle, uint32_t slot, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_BUFFER;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    DM_BYTE_POOL_INSERT(command.params, slot);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_uniform(dm_render_handle uniform_handle, uint32_t slot, dm_uniform_stage stage, uint32_t offset, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_UNIFORM;
    
    DM_BYTE_POOL_INSERT(command.params, uniform_handle);
    DM_BYTE_POOL_INSERT(command.params, slot);
    DM_BYTE_POOL_INSERT(command.params, stage);
    DM_BYTE_POOL_INSERT(command.params, offset);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_UPDATE_BUFFER;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    __dm_byte_pool_insert(&command.params, data, data_size);
    DM_BYTE_POOL_INSERT(command.params, data_size);
    DM_BYTE_POOL_INSERT(command.params, offset);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_update_uniform(dm_render_handle uniform_handle, void* data, size_t data_size, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_UPDATE_UNIFORM;
    
    DM_BYTE_POOL_INSERT(command.params, uniform_handle);
    __dm_byte_pool_insert(&command.params, data, data_size);
    DM_BYTE_POOL_INSERT(command.params, data_size);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_texture(dm_render_handle handle, uint32_t slot, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_TEXTURE;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    DM_BYTE_POOL_INSERT(command.params, slot);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_UPDATE_TEXTURE;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    DM_BYTE_POOL_INSERT(command.params, width);
    DM_BYTE_POOL_INSERT(command.params, height);
    __dm_byte_pool_insert(&command.params, data, data_size);
    DM_BYTE_POOL_INSERT(command.params, data_size);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_default_framebuffer(dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_DEFAULT_FRAMEBUFFER;
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_framebuffer(dm_render_handle handle, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_FRAMEBUFFER;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_FRAMEBUFFER_TEXTURE;
    
    DM_BYTE_POOL_INSERT(command.params, handle);
    DM_BYTE_POOL_INSERT(command.params, slot);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_DRAW_ARRAYS;
    
    DM_BYTE_POOL_INSERT(command.params, start);
    DM_BYTE_POOL_INSERT(command.params, count);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_DRAW_INDEXED;
    
    DM_BYTE_POOL_INSERT(command.params, num_indices);
    DM_BYTE_POOL_INSERT(command.params, index_offset);
    DM_BYTE_POOL_INSERT(command.params, vertex_offset);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_DRAW_INSTANCED;
    
    DM_BYTE_POOL_INSERT(command.params, num_indices);
    DM_BYTE_POOL_INSERT(command.params, num_insts);
    DM_BYTE_POOL_INSERT(command.params, index_offset);
    DM_BYTE_POOL_INSERT(command.params, vertex_offset);
    DM_BYTE_POOL_INSERT(command.params, inst_offset);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

void dm_render_command_toggle_wireframe(bool wireframe, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_TOGGLE_WIREFRAME;
    
    DM_BYTE_POOL_INSERT(command.params, wireframe);
    
    DM_SUBMIT_RENDER_COMMAND(command);
}

bool dm_renderer_submit_commands(dm_context* context)
{
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    
    for(uint32_t i=0; i<context->renderer.command_manager.command_count; i++)
    {
        dm_render_command command = context->renderer.command_manager.commands[i];
        
        switch(command.type)
        {
            case DM_RENDER_COMMAND_CLEAR:
            {
                DM_BYTE_POOL_POP(command.params, float, a);
                DM_BYTE_POOL_POP(command.params, float, b);
                DM_BYTE_POOL_POP(command.params, float, g);
                DM_BYTE_POOL_POP(command.params, float, r);
                
                dm_render_command_backend_clear(r, g, b, a, &context->renderer);
            } break;
            
            case DM_RENDER_COMMAND_SET_VIEWPORT:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, height);
                DM_BYTE_POOL_POP(command.params, uint32_t, width);
                
                dm_render_command_backend_set_viewport(width, height, &context->renderer);
            } break;
            
            case DM_RENDER_COMMAND_SET_TOPOLOGY:
            {
                DM_BYTE_POOL_POP(command.params, dm_primitive_topology, topology);
                
                if(!dm_render_command_backend_set_primitive_topology(topology, &context->renderer)) { DM_LOG_FATAL("Set topology failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_SHADER:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_bind_shader(handle, &context->renderer)) { DM_LOG_FATAL("Bind shader failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_PIPELINE:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_bind_pipeline(handle, &context->renderer)) { DM_LOG_FATAL("Bind pipeline failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_BUFFER:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, slot);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_bind_buffer(handle, slot, &context->renderer)) { DM_LOG_FATAL("Bind buffer failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_UPDATE_BUFFER:
            {
                DM_BYTE_POOL_POP(command.params, size_t, offset);
                DM_BYTE_POOL_POP(command.params, size_t, data_size);
                void* data = __dm_byte_pool_pop(&command.params, data_size);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_update_buffer(handle, data, data_size, offset, &context->renderer)) { DM_LOG_FATAL("Update buffer failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_UNIFORM:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, offset);
                DM_BYTE_POOL_POP(command.params, dm_uniform_stage, stage);
                DM_BYTE_POOL_POP(command.params, uint32_t, slot);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, uniform_handle);
                
                if(!dm_render_command_backend_bind_uniform(uniform_handle, stage, slot, offset, &context->renderer)) { DM_LOG_FATAL("Bind uniform failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_UPDATE_UNIFORM:
            {
                DM_BYTE_POOL_POP(command.params, size_t, data_size);
                void* data = __dm_byte_pool_pop(&command.params, data_size);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, uniform_handle);
                
                if(!dm_render_command_backend_update_uniform(uniform_handle, data, data_size, &context->renderer)) { DM_LOG_FATAL("Update uniform failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_TEXTURE:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, slot);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_bind_texture(handle, slot, &context->renderer)) { DM_LOG_FATAL("Bind texture failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_UPDATE_TEXTURE:
            {
                DM_BYTE_POOL_POP(command.params, size_t, data_size);
                void* data = __dm_byte_pool_pop(&command.params, data_size);
                DM_BYTE_POOL_POP(command.params, uint32_t, height);
                DM_BYTE_POOL_POP(command.params, uint32_t, width);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_update_texture(handle, width, height, data, data_size, &context->renderer)) { DM_LOG_FATAL("Update texture failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_BIND_DEFAULT_FRAMEBUFFER:
            {
                if(!dm_render_command_backend_bind_default_framebuffer(&context->renderer)) { DM_LOG_FATAL("Bind default framebuffer failed"); return false; } 
            } break;
            case DM_RENDER_COMMAND_BIND_FRAMEBUFFER:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                
                if(!dm_render_command_backend_bind_framebuffer(handle, &context->renderer)) { DM_LOG_FATAL("Bind framebuffer failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_BIND_FRAMEBUFFER_TEXTURE:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, handle);
                DM_BYTE_POOL_POP(command.params, uint32_t, slot);
                
                if(!dm_render_command_backend_bind_framebuffer_texture(handle, slot, &context->renderer)) { DM_LOG_FATAL("Bind framebuffer texture failed"); return false; }
            } break;
            
            case DM_RENDER_COMMAND_DRAW_ARRAYS:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, count);
                DM_BYTE_POOL_POP(command.params, uint32_t, start);
                
                dm_render_command_backend_draw_arrays(start, count, &context->renderer);
            } break;
            case DM_RENDER_COMMAND_DRAW_INDEXED:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, vertex_offset);
                DM_BYTE_POOL_POP(command.params, uint32_t, index_offset);
                DM_BYTE_POOL_POP(command.params, uint32_t, num_indices);
                
                dm_render_command_backend_draw_indexed(num_indices, index_offset, vertex_offset, &context->renderer);
            } break;
            case DM_RENDER_COMMAND_DRAW_INSTANCED:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, inst_offset);
                DM_BYTE_POOL_POP(command.params, uint32_t, vertex_offset);
                DM_BYTE_POOL_POP(command.params, uint32_t, index_offset);
                DM_BYTE_POOL_POP(command.params, uint32_t, num_insts);
                DM_BYTE_POOL_POP(command.params, uint32_t, num_indices);
                
                dm_render_command_backend_draw_instanced(num_indices, num_insts, index_offset, vertex_offset, inst_offset, &context->renderer);
            } break;
            
            case DM_RENDER_COMMAND_TOGGLE_WIREFRAME:
            {
                DM_BYTE_POOL_POP(command.params, bool, wireframe);
                
                dm_render_command_backend_toggle_wireframe(wireframe, &context->renderer);
            } break;
            
            default:
            DM_LOG_ERROR("Unknown render command! Shouldn't be here...");
            // TODO: do we kill here? Probably not, just ignore...
            //return false;
            break;
        }
    }
    
    imgui_draw_text_fmt(10,150, 0,1,1,1, context, "Rendering took %0.3lf ms", dm_timer_elapsed_ms(&t, context));
    return true;
}

dm_pipeline_desc dm_renderer_default_pipeline()
{
    dm_pipeline_desc pipeline_desc = { 0 };
    pipeline_desc.cull_mode = DM_CULL_BACK;
    pipeline_desc.winding_order = DM_WINDING_COUNTER_CLOCK;
    pipeline_desc.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;
    
    pipeline_desc.depth = true;
    pipeline_desc.depth_comp = DM_COMPARISON_LESS;
    
    pipeline_desc.blend = true;
    pipeline_desc.blend_eq = DM_BLEND_EQUATION_ADD;
    pipeline_desc.blend_src_f = DM_BLEND_FUNC_SRC_ALPHA;
    pipeline_desc.blend_dest_f = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
    
    return pipeline_desc;
}

/***
ECS
*****/
bool dm_ecs_init(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    dm_memset(ecs_manager->entities_ordered, DM_ECS_INVALID_ENTITY, sizeof(dm_entity) * DM_ECS_MAX_ENTITIES);
    dm_memset(ecs_manager->entities, DM_ECS_INVALID_ENTITY, sizeof(dm_entity) * DM_ECS_MAX_ENTITIES);
    
    size_t size = sizeof(uint32_t) * DM_ECS_MAX_ENTITIES * DM_ECS_MAX;
    dm_memset(ecs_manager->entity_component_indices, DM_ECS_INVALID_ID, size);
    dm_memzero(ecs_manager->entity_component_masks, sizeof(dm_ecs_id) * DM_ECS_MAX_ENTITIES);
    
    return true;
}

void dm_ecs_shutdown(dm_context* context)
{
    uint32_t i;
    
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    for(i=0; i<ecs_manager->num_registered_components; i++)
    {
        dm_free(ecs_manager->components[i].data);
    }
    
    dm_ecs_system_manager* system = NULL;
    for(i=0; i<DM_ECS_SYSTEM_TIMING_UNKNOWN; i++)
    {
        for(uint32_t j=0; j<ecs_manager->num_registered_systems[i]; j++)
        {
            system = &ecs_manager->systems[i][j];
            
            system->shutdown_func((void*)system,(void*)context);
            
            dm_free(system->system_data);
        }
    }
}

dm_ecs_id dm_ecs_register_component(size_t size, dm_context* context)
{
    if(context->ecs_manager.num_registered_components >= DM_ECS_MAX) return DM_ECS_INVALID_ID;
    
    dm_ecs_id id = context->ecs_manager.num_registered_components++;
    
    context->ecs_manager.components[id].size = size;
    context->ecs_manager.components[id].data = dm_alloc(size);
    
    return id;
}

dm_ecs_id dm_ecs_register_system(dm_ecs_id* component_ids, uint32_t component_count, dm_ecs_system_timing timing,  bool (*run_func)(void*,void*), void (*shutdown_func)(void*,void*), dm_context* context)
{
    if(context->ecs_manager.num_registered_systems[timing] >= DM_ECS_MAX) return DM_ECS_INVALID_ID;
    
    dm_ecs_id id = context->ecs_manager.num_registered_systems[timing]++;
    dm_ecs_system_manager* system = &context->ecs_manager.systems[timing][id];
    
    for(uint32_t i=0; i<component_count; i++)
    {
        system->component_mask   |= DM_BIT_SHIFT(component_ids[i]);
        system->component_ids[i]  = component_ids[i];
    }
    
    system->component_count = component_count;
    
    system->run_func      = run_func;
    system->shutdown_func = shutdown_func;
    
    system->entity_count = 0;
    
    dm_memset(system->entity_indices, DM_ECS_INVALID_ID, sizeof(uint32_t) * DM_ECS_MAX_ENTITIES * DM_ECS_MAX);
    
    return id;
}

void dm_ecs_entity_insert(dm_entity entity, dm_context* context)
{
    //const uint32_t index = dm_hash_32bit(entity) % context->ecs_manager.entity_capacity;
    const uint32_t index = entity % DM_ECS_MAX_ENTITIES;
    dm_entity* entities = context->ecs_manager.entities;
    
    if(entities[index]==DM_ECS_INVALID_ENTITY) 
    {
        entities[index] = entity;
        return;
    }
    
    uint32_t runner = index + 1;
    if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    
    while(runner != index)
    {
        if(entities[runner]==DM_ECS_INVALID_ENTITY) 
        {
            entities[runner] = entity;
            return;
        }
        
        runner++;
        if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    }
    
    DM_LOG_FATAL("Could not insert entity, should not be here...");
}

void dm_ecs_entity_insert_into_systems(dm_entity entity, dm_context* context)
{
    uint32_t  entity_index = dm_ecs_entity_get_index(entity, context);
    uint32_t  component_index;
    dm_ecs_id comp_id;
    
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    dm_ecs_system_manager* system = NULL;
    
    // for all system timings
    for(uint32_t t=0; t<DM_ECS_SYSTEM_TIMING_UNKNOWN; t++)
    {
        // for all systems in timing
        for(uint32_t s=0; s<ecs_manager->num_registered_systems[t]; s++)
        {
            system = &ecs_manager->systems[t][s];
            
            // early out if we don't have what this system needs
            if(!dm_ecs_entity_has_component_multiple(entity, system->component_mask, context)) continue;
            
            for(uint32_t c=0; c<system->component_count; c++)
            {
                comp_id = system->component_ids[c];
                
                component_index = ecs_manager->entity_component_indices[entity_index][comp_id];
                system->entity_indices[system->entity_count][comp_id] = component_index;
            }
            
            system->entity_count++;
        }
    }
}

bool dm_ecs_run_systems(dm_ecs_system_timing timing, dm_context* context)
{
    dm_ecs_system_manager* system = NULL;
    for(uint32_t i=0; i<context->ecs_manager.num_registered_systems[timing]; i++)
    {
        system = &context->ecs_manager.systems[timing][i];
        if(!system->run_func(system, context)) return false;
        
        // reset entity_count
        system->entity_count = 0;
    }
    
    return true;
}

dm_entity dm_ecs_create_entity(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    dm_entity entity;
    
    while(true)
    {
        entity = dm_random_uint32(context);
        if(entity!=DM_ECS_INVALID_ENTITY) break;
    }
    
    ecs_manager->entities_ordered[ecs_manager->entity_count++] = entity;
    dm_ecs_entity_insert(entity, context);
    
    return entity;
}

void* dm_ecs_get_component_block(dm_ecs_id component_id, dm_context* context)
{
    if(component_id==DM_ECS_INVALID_ID) return NULL;
    
    return context->ecs_manager.components[component_id].data;
}

void dm_ecs_get_component_count(dm_ecs_id component_id, uint32_t* index, dm_context* context)
{
    if(component_id==DM_ECS_INVALID_ID) return;
    
    *index = context->ecs_manager.components[component_id].entity_count;
}

void dm_ecs_entity_add_component(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    if(component_id==DM_ECS_INVALID_ID) return;
    
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    uint32_t c_index = context->ecs_manager.components[component_id].entity_count;
    
    context->ecs_manager.entity_component_indices[entity_index][component_id] = c_index;
    context->ecs_manager.entity_component_masks[entity_index] |= DM_BIT_SHIFT(component_id);
    context->ecs_manager.components[component_id].entity_count++;
}

/*********
FRAMEWORK
***********/
typedef enum dm_context_flags_t
{
    DM_CONTEXT_FLAG_IS_RUNNING,
    DM_CONTEXT_FLAG_UNKNOWN
} dm_context_flags;

dm_context* dm_init(dm_context_init_packet init_packet)
{
    dm_context* context = dm_alloc(sizeof(dm_context));
    
    context->platform_data.window_data.width = init_packet.window_width;
    context->platform_data.window_data.height = init_packet.window_height;
    strcpy(context->platform_data.window_data.title, init_packet.window_title);
    strcpy(context->platform_data.asset_path, init_packet.asset_folder);
    
    if(!dm_platform_init(init_packet.window_x, init_packet.window_y, &context->platform_data))
    {
        dm_free(context);
        return NULL;
    }
    
    if(!dm_renderer_init(context))
    {
        dm_platform_shutdown(&context->platform_data);
        dm_free(context);
        return NULL;
    }
    
    context->renderer.width = init_packet.window_width;
    context->renderer.height = init_packet.window_height;
    
    dm_ecs_init(context);
    
    // random init
    init_genrand(&context->random, (uint32_t)dm_platform_get_time(&context->platform_data));
    init_genrand64(&context->random_64, (uint64_t)dm_platform_get_time(&context->platform_data));
    
    context->delta = 1.0f / DM_DEFAULT_MAX_FPS;
    context->flags |= DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
    
    context->renderer.vsync = true;
    
    return context;
}

void dm_shutdown(dm_context* context)
{
    for(uint32_t i=0; i<DM_MAX_RENDER_COMMANDS; i++)
    {
        if(context->renderer.command_manager.commands[i].params.data) dm_free(context->renderer.command_manager.commands[i].params.data);
    }
    
    dm_renderer_shutdown(context);
    dm_platform_shutdown(&context->platform_data);
    dm_ecs_shutdown(context);
    
    dm_free(context);
}

void dm_poll_events(dm_context* context)
{
    for(uint32_t i=0; i<context->platform_data.event_list.num; i++)
    {
        dm_event e = context->platform_data.event_list.events[i];
        switch(e.type)
        {
            case DM_EVENT_WINDOW_CLOSE:
            {
                context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
                DM_LOG_WARN("Window close event received");
                return;
            } break;
            
            case DM_EVENT_KEY_DOWN:
            {
                if(e.key == DM_KEY_ESCAPE)
                {
                    dm_event* e2 = &context->platform_data.event_list.events[context->platform_data.event_list.num++];
                    e2->type = DM_EVENT_WINDOW_CLOSE;
                }
                
                if(!dm_input_is_key_pressed(e.key, context)) dm_input_set_key_pressed(e.key, context);
            } break;
            case DM_EVENT_KEY_UP:
            {
                if(dm_input_is_key_pressed(e.key, context)) dm_input_set_key_released(e.key, context);
            } break;
            
            case DM_EVENT_MOUSEBUTTON_DOWN:
            {
                if(!dm_input_is_mousebutton_pressed(e.button, context)) dm_input_set_mousebutton_pressed(e.button, context);
            } break;
            case DM_EVENT_MOUSEBUTTON_UP:
            {
                if(dm_input_is_mousebutton_pressed(e.button, context)) dm_input_set_mousebutton_released(e.button, context);
            } break;
            
            case DM_EVENT_MOUSE_MOVE:
            {
                dm_input_set_mouse_x(e.coords[0], context);
                dm_input_set_mouse_y(e.coords[1], context);
            } break;
            
            case DM_EVENT_MOUSE_SCROLL:
            {
                dm_input_set_mouse_scroll(e.delta, context);
            } break;
            
            case DM_EVENT_WINDOW_RESIZE:
            {
                context->platform_data.window_data.width  = e.new_rect[0];
                context->platform_data.window_data.height = e.new_rect[1];
                dm_renderer_backend_resize(e.new_rect[0], e.new_rect[1], &context->renderer);
            } break;
            
            case DM_EVENT_UNKNOWN:
            {
                DM_LOG_ERROR("Unknown event! Shouldn't be here...");
            } break;
        }
    }
}

void dm_start(dm_context* context)
{
    context->start = dm_platform_get_time(&context->platform_data);
}

void dm_end(dm_context* context)
{
    context->end = dm_platform_get_time(&context->platform_data);
    context->delta = context->end - context->start;
}

bool dm_update_begin(dm_context* context)
{
    // update input states
    context->input_states[1] = context->input_states[0];
    //dm_memzero(&context->input_states[0], sizeof(context->input_states[0]));
    context->input_states[0].mouse.scroll = 0;
    
    if(!dm_platform_pump_events(&context->platform_data)) 
    {
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        return false;
    }
    
    dm_poll_events(context);
    
    // reinsert entities
    for(uint32_t i=0; i<context->ecs_manager.entity_count; i++)
    {
        dm_ecs_entity_insert_into_systems(context->ecs_manager.entities_ordered[i], context);
    }
    
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN, context)) return false;
    
    return true;
}

bool dm_update_end(dm_context* context)
{
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_UPDATE_END, context)) return false;
    
    // cleaning
    context->platform_data.event_list.num = 0;
    
    return true;
}

bool dm_renderer_begin_frame(dm_context* context)
{
    if(!dm_renderer_backend_begin_frame(&context->renderer))
    {
        DM_LOG_FATAL("Begin frame failed");
        return false;
    }
    
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_RENDER_BEGIN, context)) return false;
    
    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_RENDER_END, context)) return false;
    
    // command submission
    if(!dm_renderer_submit_commands(context)) 
    { 
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        DM_LOG_FATAL("Submiting render commands failed"); 
        return false; 
    }
    
    context->renderer.command_manager.command_count = 0;
    
    if(!dm_renderer_backend_end_frame(context)) 
    {
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        DM_LOG_FATAL("End frame failed"); 
        return false; 
    }
    
    return true;
}

void* dm_read_bytes(const char* path, const char* mode, size_t* size)
{
    void* buffer = NULL;
    FILE* fp = fopen(path, mode);
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        buffer = dm_alloc(*size);
        
        size_t t = fread(buffer, *size, 1, fp);
        if(t!=1) 
        {
            DM_LOG_ERROR("Something bad happened with fread");
            return NULL;
        }
        
        fclose(fp);
    }
    return buffer;
}

bool dm_context_is_running(dm_context* context)
{
    return context->flags & DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
}

uint32_t __dm_get_screen_width(dm_context* context)
{
    return context->platform_data.window_data.width;
}

uint32_t __dm_get_screen_height(dm_context* context)
{
    return context->platform_data.window_data.height;
}

void dm_grow_dyn_array(void** array, uint32_t count, uint32_t* capacity, size_t elem_size, float load_factor, uint32_t resize_factor)
{
    if((float)count / (float)*capacity < load_factor) return;
    
    *capacity *= resize_factor;
    *array = dm_realloc(*array, *capacity * elem_size);
}

void dm_qsort(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b))
{
    qsort(array, count, elem_size, compar);
}

#ifdef DM_LINUX
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(const void* a, const void* b, void* c), void* d)
{
    qsort_r(array, count, elem_size, compar, d);
}
#else
void dm_qsrot_p(void* array, size_t count, size_t elem_size, int (compar)(void* c, const void* a, const void* b), void* d)
{
#ifdef DM_PLATFORM_WIN32
    qsort_s(array, count, elem_size, compar, d);
#elif defined(DM_PLATFORM_APPLE)
    qsort_r(array, count, elem_size, d, compar);
#endif
}
#endif

/*****
TIMER
*******/
void dm_timer_start(dm_timer* timer, dm_context* context)
{
    timer->start = dm_platform_get_time(&context->platform_data);
}

void dm_timer_restart(dm_timer* timer, dm_context* context)
{
    dm_timer_start(timer, context);
}

double dm_timer_elapsed(dm_timer* timer, dm_context* context)
{
    return (dm_platform_get_time(&context->platform_data) - timer->start);
}

double dm_timer_elapsed_ms(dm_timer* timer, dm_context* context)
{
    return dm_timer_elapsed(timer, context) * 1000;
}

/*********
MAIN FUNC
***********/
// application funcs
extern void dm_application_setup(dm_context_init_packet* init_packet);
extern bool dm_application_init(dm_context* context);
extern bool dm_application_update(dm_context* context);
extern bool dm_application_render(dm_context* context);
extern void dm_application_shutdown(dm_context* context);

typedef enum dm_exit_code_t
{
    DM_EXIT_CODE_SUCCESS,
    DM_EXIT_CODE_INIT_FAIL,
    DM_EXIT_CODE_UNKNOWN
} dm_exit_code;

#define DM_DEFAULT_SCREEN_X      100
#define DM_DEFAULT_SCREEN_Y      100
#define DM_DEFAULT_SCREEN_WIDTH  1280
#define DM_DEFAULT_SCREEN_HEIGHT 720
#define DM_DEFAULT_TITLE         "DarkMatter Application"
#define DM_DEFAULT_ASSETS_FOLDER "assets"
int main(int argc, char** argv)
{
    dm_context_init_packet init_packet = {
        DM_DEFAULT_SCREEN_X, DM_DEFAULT_SCREEN_Y,
        DM_DEFAULT_SCREEN_WIDTH, DM_DEFAULT_SCREEN_HEIGHT,
        DM_DEFAULT_TITLE,
        DM_DEFAULT_ASSETS_FOLDER
    };
    
    dm_application_setup(&init_packet);
    
    dm_context* context = dm_init(init_packet);
    if(!context) return DM_EXIT_CODE_INIT_FAIL;
    
    if(!dm_application_init(context)) 
    {
        dm_shutdown(context);
        getchar();
        return DM_EXIT_CODE_INIT_FAIL;
    }
    
    while(dm_context_is_running(context))
    {
        dm_start(context);
        
        // updating
        if(!dm_update_begin(context)) break;
        if(!dm_application_update(context)) break;
        if(!dm_update_end(context)) break;
        
        // rendering
        if(dm_renderer_begin_frame(context))
        {
            if(!dm_application_render(context)) break;
            if(!dm_renderer_end_frame(context)) break;
        }
        
        dm_end(context);
    }
    
    dm_application_shutdown(context);
    dm_shutdown(context);
    
    getchar();
    
    return DM_EXIT_CODE_SUCCESS;
}
#endif

#endif //DM_H
