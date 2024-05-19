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
#define DM_PLATFORM_APPLE
#define DM_METAL
#define DM_INLINE __attribute__((always_inline)) inline

#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define DM_PLATFORM_WIN32
#define DM_INLINE __forceinline

#elif __linux__ || __gnu_linux__
#define DM_PLATFORM_LINUX
#define DM_INLINE __always_inline

#else
#define DM_PLATFORM_GLFW
#define DM_INLINE
#endif

/*********
ALIGNMENT
***********/
#ifdef DM_PLATFORM_WIN32
#define DM_ALIGN(X) __declspec(align(X))
#else
#define DM_ALIGN(X) __attribute((aligned(X)))
#endif

/*****************************
SIMD INTRINSICS DETERMINATION
*******************************/
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#define DM_SIMD_X86
#elif defined(__aarch64__)
#define DM_SIMD_ARM
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
#include <arm_neon.h>
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
#define DM_MAX(X, Y)          (X > Y ? X : Y)
#define DM_MIN(X, Y)          (X < Y ? X : Y)
#define DM_SIGN(X)            ((0 < X) - (X < 0))
#define DM_SIGNF(X)           (float)((0 < X) - (X < 0))
#define DM_CLAMP(X, MIN, MAX) DM_MIN(DM_MAX(X, MIN), MAX)
#define DM_BIT_SHIFT(X)       (1 << X)

/**********
MATH TYPES
************/
typedef float dm_vec2[2];
typedef float dm_vec3[3];
typedef DM_ALIGN(16) float dm_vec4[4];
typedef DM_ALIGN(16) float dm_quat[4];

typedef float dm_mat2[2][2];
typedef float dm_mat3[3][3];
#ifdef __AVX__
typedef DM_ALIGN(32) float dm_mat4[4][4];
#else
typedef DM_ALIGN(16) float dm_mat4[4][4];
#endif

#define N2 2
#define N3 3
#define N4 4
#define M2 N2 * N2
#define M3 N3 * N3
#define M4 N4 * N4

#define DM_VEC2_SIZE sizeof(float) * N2
#define DM_VEC3_SIZE sizeof(float) * N3
#define DM_VEC4_SIZE sizeof(float) * N4
#define DM_QUAT_SIZE DM_VEC4_SIZE
#define DM_MAT2_SIZE sizeof(float) * N2
#define DM_MAT3_SIZE sizeof(float) * M3
#define DM_MAT4_SIZE sizeof(float) * M4

#define DM_VEC2_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_VEC2_SIZE)
#define DM_VEC3_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_VEC3_SIZE)
#define DM_VEC4_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_VEC4_SIZE)
#define DM_QUAT_COPY(DEST, SRC) DM_VEC4_COPY(DEST, SRC)
#define DM_MAT2_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_MAT2_SIZE)
#define DM_MAT3_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_MAT3_SIZE)
#define DM_MAT4_COPY(DEST, SRC) dm_memcpy(DEST, SRC, DM_MAT4_SIZE)

#define DM_ALIGN_BYTES(SIZE, ALIGNMENT) ((SIZE + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/****
SIMD
******/
#ifdef DM_SIMD_X86
typedef __m256  dm_simd256_float;
typedef __m256i dm_simd256_int;

typedef __m128  dm_simd_float;
typedef __m128i dm_simd_int;

#define DM_SIMD256_FLOAT_N 8
#elif defined(DM_SIMD_ARM)
// neon does not support 256bit registers
typedef float32x4_t dm_simd_float;

typedef int32x4_t   dm_simd_int;
#endif

#define DM_SIMD_FLOAT_N    4

/**********
INTRINSICS
************/
#include "dm_intrinsics.h"
#ifdef DM_SIMD_X86
#include "dm_intrinsics256.h"
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

/**********
STRUCTURES
************/
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
// render enums
typedef enum dm_buffer_type_t
{
	DM_BUFFER_TYPE_VERTEX,
	DM_BUFFER_TYPE_INDEX,
    DM_BUFFER_TYPE_COMPUTE,
    DM_BUFFER_TYPE_UNKNOWN
} dm_buffer_type;

typedef enum dm_buffer_usage_t
{
    DM_BUFFER_USAGE_DEFAULT,
    DM_BUFFER_USAGE_STATIC,
    DM_BUFFER_USAGE_DYNAMIC,
    DM_BUFFER_USAGE_UNKNOWN
} dm_buffer_usage;

typedef enum dm_buffer_cpu_access_t
{
    DM_BUFFER_CPU_WRITE,
    DM_BUFFER_CPU_READ
} dm_buffer_cpu_access;

typedef enum dm_compute_buffer_type_t
{
    DM_COMPUTE_BUFFER_TYPE_READ_ONLY,
    DM_COMPUTE_BUFFER_TYPE_WRITE_ONLY,
    DM_COMPUTE_BUFFER_TYPE_READ_WRITE,
    DM_COMPUTE_BUFFER_TYPE_UNKNOWN
} dm_compute_buffer_type;

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
    DM_VERTEX_DATA_T_UBYTE_NORM,
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
    DM_CULL_NONE,
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

typedef uint32_t         dm_render_handle;
typedef dm_render_handle dm_compute_handle;

#define DM_RENDER_HANDLE_INVALID UINT_MAX
#define DM_BUFFER_INVALID        DM_RENDER_HANDLE_INVALID
#define DM_TEXTURE_INVALID       DM_RENDER_HANDLE_INVALID
#define DM_SHADER_INVALID        DM_RENDER_HANDLE_INVALID
#define DM_PIPELINE_INVALID      DM_RENDER_HANDLE_INVALID

typedef enum dm_vertex_buffer_type_t
{
    DM_VERTEX_BUFFER_TYPE_VERTEX,
    DM_VERTEX_BUFFER_TYPE_INSTANCE,
    DM_VERTEX_BUFFER_TYPE_UNKNOWN
} dm_vertex_buffer_type;

typedef struct dm_vertex_buffer_desc_t
{
    dm_vertex_buffer_type type;
    size_t size, stride, count;
    const void* data;
} dm_vertex_buffer_desc;

typedef enum dm_index_buffer_data_t
{
    DM_INDEX_BUFFER_DATA_UINT16,
    DM_INDEX_BUFFER_DATA_UINT32,
    DM_INDEX_BUFFER_UNKNOWN
} dm_index_buffer_data;

typedef struct dm_index_buffer_desc_t
{
    dm_index_buffer_data data_type;
    size_t size, count;
    const void* data;
} dm_index_buffer_desc;

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

typedef enum dm_pipeline_flag_t
{
    DM_PIPELINE_FLAG_BLEND     = 1 << 0,
    DM_PIPELINE_FLAG_DEPTH     = 1 << 1,
    DM_PIPELINE_FLAG_STENCIL   = 1 << 2,
    DM_PIPELINE_FLAG_SCISSOR   = 1 << 3,
    DM_PIPELINE_FLAG_WIREFRAME = 1 << 4
} dm_pipeline_flag;

#define DM_PIPELINE_MAX_CBS 5
typedef struct dm_pipeline_desc_t
{
    dm_cull_mode          cull_mode;
    dm_winding_order      winding_order;
    
    dm_blend_equation     blend_eq;
    dm_blend_func         blend_src_f, blend_dest_f;
    dm_blend_equation     blend_alpha_eq;
    dm_blend_func         blend_src_alpha_f, blend_dest_alpha_f;
    
    dm_filter             sampler_filter;
    dm_texture_mode       u_mode, v_mode, w_mode;
    dm_pipeline_flag      flags;
    dm_comparison         blend_comp, depth_comp, stencil_comp, sampler_comp;
    
    dm_primitive_topology primitive_topology;
    float                 min_lod, max_lod;
    
    dm_vertex_attrib_desc* attribs;
    uint32_t               attrib_count;
    
    dm_render_handle cbs[DM_PIPELINE_MAX_CBS];
    size_t           cb_sizes[DM_PIPELINE_MAX_CBS];
    uint32_t         cb_count;
    
    void*  vertex_shader_data;
    size_t vertex_shader_size;
    
    void*  pixel_shader_data;
    size_t pixel_shader_size;
} dm_pipeline_desc;

typedef struct dm_shader_desc_t
{
    char vertex[512];
    char pixel[512];
    char master[512];
    
    dm_render_handle vb[2];
    uint32_t         vb_count;
} dm_shader_desc;

typedef struct dm_compute_shader_desc_t
{
    char     path[512];
#ifdef DM_METAL
    char     function[512];
#endif
} dm_compute_shader_desc;

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

typedef struct dm_renderpass_desc_t
{
    float top, bottom;
    float left, right;
    
    float depth_min, depth_max;
} dm_renderpass_desc;

#ifdef DM_RAYTRACING
// raytracing acceleration structure
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

typedef struct dm_blas_desc_t
{
    dm_blas_geometry_type geom_type;
    dm_blas_geometry_flag geom_flag;
    
    dm_render_handle vertex_buffer;
    dm_render_handle index_buffer;
    
    uint32_t         vertex_count, index_count;
    size_t           vertex_size, index_size;
    size_t           vertex_offset, index_offset;
} dm_blas_desc;

#define DM_MAX_BLAS 100
typedef struct dm_tlas_desc_t
{
    dm_mat4*  instance_transforms;
    uint32_t* instance_meshes;
    uint32_t  instance_count;
    uint32_t  max_instance_count;
} dm_tlas_desc;

typedef struct dm_acceleration_structure_desc_t
{
    dm_blas_desc blas_descs[DM_MAX_BLAS];
    uint32_t     blas_count;
    
    dm_tlas_desc tlas_desc;
    
    uint8_t hit_group_count;
} dm_acceleration_structure_desc;

// raytracing pipeline
#define DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS      3 // primary, shadow, bounce
typedef enum dm_raytracing_pipeline_hit_group_geometry_type_t
{
    DM_RAYTRACING_PIPELINE_HIT_GROUP_GEOMETRY_TYPE_TRIANGLES,
    DM_RAYTRACING_PIPELINE_HIT_GROUP_GEOMETRY_TYPE_PROCEDURAL,
    DM_RAYTRACING_PIPELINE_HIT_GROUP_GEOMETRY_TYPE_UNKNOWN,
} dm_raytracing_pipeline_hit_group_geometry_type;

typedef enum dm_raytracing_pipeline_hit_group_flag_t
{
    DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_ANY_HIT       = 1 << 0,
    DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_CLOSEST_HIT   = 1 << 1,
    DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_INTERSECTION  = 1 << 2,
    DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_UNKNOWN       = 1 << 3
} dm_raytracing_pipeline_hit_group_flag;

typedef enum dm_raytracing_pipeline_shader_param_type_t
{
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_VERTEX_BUFFER,
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_INDEX_BUFFER,
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_CONSTANT_BUFFER,
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_ACCELERATION_STRUCTURE,
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_TEXTURE,
    DM_RAYTRACING_PIPELINE_SHADER_PARAM_TYPE_VERTEX_UNKNOWN
} dm_raytracing_pipeline_shader_param_type;

#define DM_RAYTRACING_PIPELINE_SHADER_MAX_PARAMS 10
typedef struct dm_rt_pipeline_shader_params_t
{
    dm_raytracing_pipeline_shader_param_type types[DM_RAYTRACING_PIPELINE_SHADER_MAX_PARAMS];
    uint32_t                                 registers[DM_RAYTRACING_PIPELINE_SHADER_MAX_PARAMS];
    uint32_t                                 count;
} dm_rt_pipeline_shader_params;

typedef struct dm_raytracing_pipeline_hit_group_desc_t
{
    dm_raytracing_pipeline_hit_group_geometry_type geom_type;
    dm_raytracing_pipeline_hit_group_flag          flags;
    
    dm_rt_pipeline_shader_params params;
    
    char name[512];
    char any_hit[512];
    char closest_hit[512];
    char intersection[512];
} dm_raytracing_pipeline_hit_group_desc;

typedef struct dm_raytracing_pipeline_desc_t
{
    char raygen[512];
    char miss[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS][512];
    uint32_t miss_count;
    
    dm_rt_pipeline_shader_params global_params;
    dm_rt_pipeline_shader_params raygen_params;
    
    dm_rt_pipeline_shader_params miss_params[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS];
    
    dm_raytracing_pipeline_hit_group_desc hit_groups[DM_RAYTRACING_PIPELINE_MAX_HIT_GROUPS];
    uint32_t                              hit_group_count;
    
    uint32_t max_instance_count;
    
    void*  shader_data;
    size_t shader_data_size;
    
    size_t   payload_size;
    uint32_t max_recursion;
} dm_raytracing_pipeline_desc;
#endif

// model loading
typedef enum dm_mesh_vertex_attrib_t
{
    DM_MESH_VERTEX_ATTRIB_POSITION,
    DM_MESH_VERTEX_ATTRIB_NORMAL,  
    DM_MESH_VERTEX_ATTRIB_TEXCOORD,
    DM_MESH_VERTEX_ATTRIB_UNKNOWN
} dm_mesh_vertex_attrib;

typedef enum dm_mesh_index_type_t
{
    DM_MESH_INDEX_TYPE_UINT16,
    DM_MESH_INDEX_TYPE_UINT32,
    DM_MESH_INDEX_TYPE_UNKNOWN
} dm_mesh_index_type;

// font
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

// commands
typedef enum dm_render_command_type_t
{
    DM_RENDER_COMMAND_BEGIN_RENDERPASS,
    DM_RENDER_COMMAND_END_RENDERPASS,
    
    DM_RENDER_COMMAND_BIND_PIPELINE,
    
    DM_RENDER_COMMAND_BIND_VERTEX_BUFFER,
    DM_RENDER_COMMAND_BIND_INDEX_BUFFER,
    DM_RENDER_COMMAND_BIND_TEXTURE,
    
    DM_RENDER_COMMAND_UPDATE_VERTEX_BUFFER,
    DM_RENDER_COMMAND_UPDATE_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_UPDATE_TEXTURE,
    
    DM_RENDER_COMMAND_DRAW_ARRAYS,
    DM_RENDER_COMMAND_DRAW_INDEXED,
    DM_RENDER_COMMAND_DRAW_INSTANCED,
    
    // misc
    DM_RENDER_COMMAND_SET_TOPOLOGY,
    DM_RENDER_COMMAND_TOGGLE_WIREFRAME,
    
#ifdef DM_RAYTRACING
    DM_RENDER_COMMAND_BIND_RAYTRACING_PIPELINE,
    
    DM_RENDER_COMMAND_UPDATE_ACCELERATION_STRUCTURE_INSTANCE,
    DM_RENDER_COMMAND_UPDATE_ACCELERATION_STRUCTURE_TLAS,
    
    DM_RENDER_COMMAND_DISPATCH_RAYS,
    
    DM_RENDER_COMMAND_COPY_TEXTURE_TO_SCREEN,
#endif
    
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

/*******
PHYSICS
*********/
#define DM_PHYSICS_MAX_GJK_ITER      64
#define DM_PHYSICS_EPA_MAX_FACES     64
#define DM_PHYSICS_EPA_MAX_ITER      64

#define DM_PHYSICS_FIXED_DT          0.00833f // 1 / 120
#define DM_PHYSICS_FIXED_DT_INV      120

// movement types
typedef enum dm_physics_movement_type_t
{
    DM_PHYSICS_MOVEMENT_TYPE_KINEMATIC,
    DM_PHYSICS_MOVEMENT_TYPE_STATIC,
    DM_PHYSICS_MOVEMENT_TYPE_UNKNOWN
} dm_physics_movement_type;

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
    
    float vel_x, vel_y, vel_z;
    float w_x, w_y, w_z;
    
    dm_physics_movement_type movement_type;
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
    
#ifdef DM_PHYSICS_DEBUG
    float    clipped_face[DM_PHYSICS_CLIP_MAX_PTS][3];
    uint32_t clipped_point_count;
    
    uint32_t face_count_a, face_count_b;
    float face_a[DM_PHYSICS_CLIP_MAX_PTS][3];
    float face_b[DM_PHYSICS_CLIP_MAX_PTS][3];
    
    uint32_t plane_count_a, plane_count_b;
    dm_plane planes_a[DM_PHYSICS_CLIP_MAX_PTS];
    dm_plane planes_b[DM_PHYSICS_CLIP_MAX_PTS];
#endif
    
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
    bool     vsync;
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
float dm_clamp(float x, float min, float max);

float dm_rad_to_deg(float radians);
float dm_deg_to_rad(float degrees);

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
bool dm_renderer_create_vertex_buffer(const dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_constant_buffer(const void* data, size_t data_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_context* context);

bool dm_renderer_resize_texutre(const void* data, uint32_t width, uint32_t height, dm_render_handle handle, dm_context* context);

#ifdef DM_RAYTRACING
bool dm_renderer_create_acceleration_structure(dm_acceleration_structure_desc desc, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_render_handle* handle, dm_context* context);

// shaders
uint32_t dm_renderer_raytracing_pipeline_add_miss(const char* name, dm_raytracing_pipeline_desc* desc);
uint32_t dm_renderer_raytracing_pipeline_add_triangles_hit_group(const char* name, dm_raytracing_pipeline_desc* desc);
uint32_t dm_renderer_raytracing_pipeline_add_procedural_hit_group(const char* name, dm_raytracing_pipeline_desc* desc);

void dm_renderer_raytracing_pipeline_hit_group_add_any_hit(const char* shader, uint32_t group_index, dm_raytracing_pipeline_desc* desc);
void dm_renderer_raytracing_pipeline_hit_group_add_closest_hit(const char* shader, uint32_t group_index, dm_raytracing_pipeline_desc* desc);
void dm_renderer_raytracing_pipeline_hit_group_add_intersection(const char* shader, uint32_t group_index, dm_raytracing_pipeline_desc* desc);

bool dm_renderer_rt_pipe_global_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_context* context);
bool dm_renderer_rt_pipe_raygen_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_context* context);
bool dm_renderer_rt_pipe_miss_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t miss_index, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_context* context);
bool dm_renderer_rt_pipe_hit_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t hit_group, uint32_t slot, uint32_t instance, dm_render_handle handle, dm_render_handle pipe_handle, dm_context* context);

#endif

// render commands
void dm_render_command_set_primitive_topology(dm_primitive_topology topology, dm_context* context);
void dm_render_command_bind_pipeline(dm_render_handle handle, dm_context* context);
void dm_render_command_bind_buffer(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_bind_texture(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_context* context);
void dm_render_command_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_context* context);
void dm_render_command_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_context* context);
void dm_render_command_begin_renderpass(dm_render_handle handle, dm_context* conext);
void dm_render_command_end_renderpass(dm_render_handle handle, dm_context* conext);
void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_context* context);
void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_context* context);
void dm_render_command_draw_instanced(uint32_t index_count, uint32_t vertex_count, uint32_t inst_count, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_context* context);
void dm_render_command_toggle_wireframe(bool wireframe, dm_context* context);

#ifdef DM_RAYTRACING
void dm_render_command_update_acceleration_structure_instance(dm_render_handle handle, uint32_t instance_id, void* data, size_t data_size,  dm_context* context);
void dm_render_command_update_acceleration_structure_tlas(dm_render_handle handle, dm_context* context);

void dm_render_command_bind_raytracing_pipeline(dm_render_handle handle, dm_context* context);
void dm_render_command_raytracing_pipeline_dispatch_rays(uint32_t width, uint32_t height, dm_render_handle handle, dm_context* context);
void dm_render_command_copy_texture_to_screen(dm_render_handle handle, dm_context* context);
#endif

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

#include "dm_math.h"

#endif //DM_H
