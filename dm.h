#ifndef DM_H
#define DM_H

/********
INCLUDES
**********/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef  DM_PLATFORM_APPLE
#include <immintrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#else
#include "arm_neon.h"
#endif

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
#ifndef DM_OPENGL
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

#define DM_MATH_ANGLE_RAD_TOLERANCE 0.001f
#define DM_MATH_SQRT2               1.41421356237309f
#define DM_MATH_INV_SQRT2           0.70710678118654f
#define DM_MATH_SQRT3               1.73205080756887f
#define DM_MATH_INV_SQRT3           0.57735026918962f
#define DM_MATH_DEG_TO_RAD          0.0174533f
#define DM_MATH_RAD_TO_DEG          57.2958f

// math macros
#define DM_MAX(X, Y) (X > Y ? X : Y)
#define DM_MIN(X, Y) (X < Y ? X : Y)
#define DM_SIGN(X) (X < 0.0f ? -1.0f : 1.0f)
#define DM_CLAMP(X, MIN, MAX) DM_MIN(DM_MAX(X, MIN), MAX)
#define DM_BIT_SHIFT(X) (1 << X)

/**********
MATH TYPES
************/
typedef struct dm_vec2_t
{
    union
    {
        float v[2];
        struct
        {
            float x, y;
        };
    };
} dm_vec2;

typedef struct dm_vec3_t
{
    union
    {
        float v[3];
        float xy[2];
        struct
        {
            float x, y, z;
        };
    };
} dm_vec3;

typedef struct dm_vec4_t
{
    union
    {
        float v[4];
        float xyz[3];
        struct
        {
            float x, y, z, w;
        };
    };
} dm_vec4;

typedef struct dm_quat_t
{
    union
    {
        float v[4];
        float ijk[3];
        struct
        {
            float i,j,k,r;
        };
    };
} dm_quat;

typedef struct dm_mat2_t
{
	union
	{
		dm_vec2 rows[2];
		float m[2 * 2];
	};
} dm_mat2;

typedef struct dm_mat3_t
{
	union
	{
		dm_vec3 rows[3];
		float m[3 * 3];
        float mm[3][3];
	};
} dm_mat3;

typedef struct dm_mat4_t
{
	union
	{
		dm_vec4 rows[4];
		float m[4 * 4];
        float mm[4][4];
	};
} dm_mat4;

static const dm_vec3 dm_vec3_unit_x = { 1,0,0 };
static const dm_vec3 dm_vec3_unit_y = { 0,1,0 };
static const dm_vec3 dm_vec3_unit_z = { 0,0,1 };

/****
SIMD
******/
#ifdef DM_SIMD_256
typedef __m256  dm_mm_float;
typedef __m256i dm_mm_int;
#define DM_SIMD_N 8
#else
typedef __m128  dm_mm_float;
typedef __m128i dm_mm_int;
#define DM_SIMD_N 4
#endif

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
    dm_font fonts[DM_RENDERER_MAX_RESOURCE_COUNT];
    uint32_t font_count;
    
    dm_render_command_manager command_manager;
    
    uint32_t width, height;
#ifdef DM_OPENGL
    uint32_t uniform_bindings;
#endif
    
    void* internal_renderer;
} dm_renderer;

/******************
DARKMATTER CONTEXT
********************/
typedef struct dm_context_t
{
    dm_platform_data platform_data;
    dm_input_state   input_states[2];
    dm_renderer      renderer;
    bool             is_running;
    double           start, end, delta;
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

// casting
dm_mm_int   dm_mm_cast_float_to_int(dm_mm_float mm);
dm_mm_float dm_mm_cast_int_to_float(dm_mm_int mm);

// float
dm_mm_float dm_mm_load_ps(float* d);
dm_mm_float dm_mm_set1_ps(float d);
void        dm_mm_store_ps(float* d, dm_mm_float mm);

dm_mm_float dm_mm_add_ps(dm_mm_float left, dm_mm_float right);
dm_mm_float dm_mm_sub_ps(dm_mm_float left, dm_mm_float right);
dm_mm_float dm_mm_mul_ps(dm_mm_float left, dm_mm_float right);
dm_mm_float dm_mm_div_ps(dm_mm_float left, dm_mm_float right);
dm_mm_float dm_mm_sqrt_ps(dm_mm_float mm);
dm_mm_float dm_mm_hadd_ps(dm_mm_float left, dm_mm_float right);
dm_mm_float dm_mm_fmadd_ps(dm_mm_float a, dm_mm_float b, dm_mm_float c);

float       dm_mm_extract_float(dm_mm_float mm);
float       dm_mm_sum_elements(dm_mm_float mm);

// int
dm_mm_int   dm_mm_load_i(int* d);
dm_mm_int   dm_mm_set1_i(int d);
void        dm_mm_store_i(int* i, dm_mm_int mm);

dm_mm_int   dm_mm_add_i(dm_mm_int left, dm_mm_int right);
dm_mm_int   dm_mm_sub_i(dm_mm_int left, dm_mm_int right);
dm_mm_int   dm_mm_mul_i(dm_mm_int left, dm_mm_int right);
dm_mm_int   dm_mm_div_i(dm_mm_int left, dm_mm_int right);
dm_mm_int   dm_mm_sqrt_i(dm_mm_int mm);
dm_mm_int   dm_mm_hadd_i(dm_mm_int left, dm_mm_int right);

// shifting
dm_mm_int   dm_mm_shiftl_1(dm_mm_int mm);
dm_mm_int   dm_mm_shiftr_1(dm_mm_int mm);

// memory
void* dm_alloc(size_t size);
void* dm_calloc(size_t count, size_t size);
void* dm_realloc(void* block, size_t size);
void  dm_free(void* block);
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
int      dm_input_get_mouse_scroll(dm_context* context);
int      dm_input_get_prev_mouse_x(dm_context* context);
int      dm_input_get_prev_mouse_y(dm_context* context);
void     dm_input_get_prev_mouse_pos(uint32_t* x, uint32_t* y, dm_context* context);
int      dm_input_get_mouse_scroll(dm_context* context);

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

// rendering
bool dm_renderer_create_static_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_dynamic_vertex_buffer(void* data, size_t data_size, size_t vertex_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_static_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_dynamic_index_buffer(void* data, size_t data_size, dm_render_handle* handle, dm_context* context);
#ifdef DM_OPENGL
bool dm_renderer_create_shader(const char* vertex_src, const char* pixel_src, dm_render_handle* vb_indices, uint32_t num_vb, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* handle, dm_context* context);
#else
bool dm_renderer_create_shader(const char* vertex_src, const char* pixel_src, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* handle, dm_context* context);
#endif
bool dm_renderer_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_texture_from_file(const char* path, uint32_t n_channels, bool flipped, const char* name, dm_render_handle* handle, dm_context* context);
bool dm_renderer_create_texture_from_data(uint32_t width, uint32_t height, uint32_t n_channels, void* data, const char* name, dm_render_handle* handle, dm_context* context);
bool dm_renderer_load_font(const char* path, dm_render_handle* handle, dm_context* context);

void dm_renderer_destroy_buffer(dm_render_handle handle, dm_context* context);
void dm_renderer_destroy_shader(dm_render_handle handle, dm_context* context);
void dm_renderer_destroy_uniform(dm_render_handle handle, dm_context* context);
void dm_renderer_destroy_pipeline(dm_render_handle handle, dm_context* context);
void dm_renderer_destroy_texture(dm_render_handle handle, dm_context* context);
void dm_renderer_destroy_font(dm_render_handle handle, dm_context* context);

#define DM_MAKE_VERTEX_ATTRIB(NAME, STRUCT, MEMBER, CLASS, DATA_T, COUNT, INDEX, NORMALIZED) { .name=NAME, .data_t=DATA_T, .attrib_class=CLASS, .stride=sizeof(STRUCT), .offset=offsetof(STRUCT, MEMBER), .count=COUNT, .index=INDEX, .normalized=NORMALIZED}

// render commands
void dm_render_command_clear(float r, float g, float b, float a, dm_context* context);
void dm_render_command_set_viewport(uint32_t width, uint32_t height, dm_context* context);
void dm_render_command_set_default_viewport(dm_context* context);
void dm_render_command_set_primitive_topology(dm_primitive_topology topology, dm_context* context);
void dm_render_command_bind_shader(dm_render_handle handle, dm_context* context);
void dm_render_command_bind_buffer(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_bind_uniform(dm_render_handle uniform_handle, uint32_t slot, dm_uniform_stage stage, uint32_t offset, dm_render_handle shader_handle, dm_context* context);
void dm_render_command_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_context* context);
void dm_render_command_update_uniform(dm_render_handle uniform_handle, void* data, size_t data_size, dm_render_handle shader_handle, dm_context* context);
void dm_render_command_bind_texture(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_context* context);
void dm_render_command_bind_default_framebuffer(dm_context* context);
//void dm_render_command_bind_framebuffer(dm_render_handle handle, dm_context* context);
//void dm_render_command_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_context* context);
void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_context* context);
void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_context* context);
void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_context* context);
void dm_render_command_toggle_wireframe(bool wireframe, dm_context* context);

// structures
dm_slot_array* __dm_slot_array_create(size_t elem_size, size_t capacity);
void           dm_slot_array_destroy(dm_slot_array* slot_array);
void           dm_slot_array_insert(dm_slot_array* slot_array, void* data, dm_slot_index* index);
void           dm_slot_array_overwrite(dm_slot_array* slot_array, void* data, dm_slot_index index);
void           dm_slot_array_delete(dm_slot_array* slot_array, dm_slot_index index);
void*          dm_slot_array_get(dm_slot_array* slot_array, dm_slot_index index);

#define DM_SLOT_ARRAY_CREATE(T) __dm_slot_array_create(sizeof(T), DM_SLOT_ARRAY_DEFAULT_CAPACITY)

dm_map* __dm_map_create_key_str(size_t value_size);
dm_map* __dm_map_create_key_uint32(size_t value_size);
dm_map* __dm_map_create_key_uint64(size_t value_size);
dm_map* __dm_map_create_key_int_pair(size_t value_size);
dm_map* __dm_map_create_key_uint_pair(size_t value_size);

#define DM_MAP_CREATE_KEY_STR(VALUE_TYPE)       __dm_map_create_key_str(sizeof(VALUE_TYPE))
#define DM_MAP_CREATE_KEY_UINT32(VALUE_TYPE)    __dm_map_create_key_uint32(sizeof(VALUE_TYPE))
#define DM_MAP_CREATE_KEY_UINT64(VALUE_TYPE)    __dm_map_create_key_uint64(sizeof(VALUE_TYPE))
#define DM_MAP_CREATE_KEY_INT_PAIR(VALUE_TYPE)  __dm_map_create_key_int_pair(sizeof(VALUE_TYPE))
#define DM_MAP_CREATE_KEY_UINT_PAIR(VALUE_TYPE) __dm_map_create_key_uint_pair(sizeof(VALUE_TYPE))

void  dm_map_destroy(dm_map* map);

void  dm_map_insert(dm_map* map, void* key, void* value);
void* dm_map_get(dm_map* map, void* key);
void  dm_map_delete(dm_map* map, void* key);

bool  dm_map_has_key(dm_map* map, void* key);

void* dm_map_get_keys(dm_map* map, uint32_t* num_keys);
void* dm_map_get_values(dm_map* map, uint32_t* num_values);

// framework funcs
dm_context* dm_init(uint32_t window_x_pos, uint32_t windos_y_pos, uint32_t window_w, uint32_t window_h, const char* window_title);
void        dm_shutdown(dm_context* context);
bool        dm_update_begin(dm_context* context);
bool        dm_update_end(dm_context* context);
bool        dm_renderer_begin_frame(dm_context* context);
bool        dm_renderer_end_frame(bool vsync, dm_context* context);

void* dm_read_bytes(const char* path, const char* mode, size_t* size);

#define DM_ARRAY_LEN(ARRAY) sizeof(ARRAY) / sizeof(ARRAY[0])

// timer
void   dm_timer_start(dm_timer* timer, dm_context* context);
void   dm_timer_restart(dm_timer* timer, dm_context* context);
double dm_timer_elapsed(dm_timer* timer, dm_context* context);
double dm_timer_elapsed_ms(dm_timer* timer, dm_context* context);

/****
IMPL
******/
#ifdef DM_IMPL
#include "dm_impl.h"
#endif

#endif //DM_H
