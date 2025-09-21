#ifndef __DM_H__
#define __DM_H__

#include "dm_defines.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include "lib/cglm/include/cglm/cglm.h"

/**********
 * MEMORY * 
 **********/
void* dm_alloc(size_t size);
void  dm_free(void** ptr);
void* dm_realloc(void* ptr, size_t size);
void  dm_memcpy(void* dst, void* src, size_t size);
void* dm_memset(void* dst, int value, size_t size);
void* dm_memzero(void* dst, size_t size);

/*********
* WINDOW *
**********/
typedef enum dm_window_create_flag_t
{
    DM_WINDOW_CREATE_FLAG_NONE      = 0,
    DM_WINDOW_CREATE_FLAG_CENTER    = 1,
    DM_WINDOW_CREATE_FLAG_NO_RESIZE = 2
} dm_window_create_flag;

typedef struct dm_window_t dm_window; 
bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_window* window);
void dm_window_shutdown(dm_window* window);
bool dm_window_should_close(dm_window window);
bool dm_window_resized(dm_window window);
bool dm_window_poll_events(dm_window* window);

uint32_t dm_window_get_width(dm_window window);
uint32_t dm_window_get_height(dm_window window);

typedef struct dm_timer_t
{
    double start, end;
} dm_timer;

double dm_get_time();
void   dm_timer_start(dm_timer* timer);
double dm_timer_elapsed(dm_timer* timer);
double dm_timer_elapsed_ms(dm_timer* timer);

/***********
 * LOGGING *
 ***********/
typedef enum dm_log_level_t
{
    DM_LOG_TRACE,
    DM_LOG_DEBUG,
    DM_LOG_INFO,
    DM_LOG_WARN,
    DM_LOG_ERROR,
    DM_LOG_FATAL
} dm_log_level;

void dm_log(dm_log_level log_level, const char* message, ...);

/********
* INPUT *
*********/
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
	MAKE_KEYCODE(QUOTE,     0xDE),

    MAKE_KEYCODE(NONE, 0xFF)
} dm_key_code;

typedef enum dm_mousebutton_code_t
{
    DM_MOUSEBUTTON_L,
    DM_MOUSEBUTTON_R,
    DM_MOUSEBUTTON_M,
    DM_MOUSEBUTTON_DOUBLE,
    DM_MOUSEBUTTON_UNKNOWN
} dm_mousebutton_code;

bool dm_input_is_key_pressed(dm_key_code key, dm_window window);
bool dm_input_key_just_pressed(dm_key_code key, dm_window window);
bool dm_input_key_just_released(dm_key_code key, dm_window window);

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_window window);
bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_window window);
bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_window window);

bool     dm_input_mouse_moved(dm_window window);
uint16_t dm_input_get_mouse_pos_x(dm_window window);
uint16_t dm_input_get_mouse_pos_y(dm_window window);
void     dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_window window);
int      dm_input_get_mouse_delta_x(dm_window window);
int      dm_input_get_mouse_delta_y(dm_window window);
void     dm_input_get_mouse_delta(int* x, int* y, dm_window window);

/*************
 * RENDERING *
 *************/
#ifndef DM_MAX_FRAMES_IN_FLIGHT
#define DM_MAX_FRAMES_IN_FLIGHT 3
#endif

typedef struct dm_renderer_t dm_renderer;
bool dm_renderer_init(dm_window window, dm_renderer* renderer);
bool dm_renderer_finish_init(dm_renderer* renderer);
void dm_renderer_shutdown(dm_renderer* renderer);
bool dm_renderer_resize(dm_window window, dm_renderer* renderer);
bool dm_renderer_begin_frame(dm_renderer* renderer);
bool dm_renderer_end_frame(bool vsync, dm_renderer* renderer);
bool dm_renderer_submit_render_commands(dm_renderer* renderer);

// === handles ===
typedef uint16_t dm_renderpass_handle;

// in metal, bindless needs the uint64_t gpu address
// DX12 and Vulkan just need a simple integer index
#ifdef DM_MORE_DESCRIPTORS
typedef uint32_t dm_resource_index;
#else
typedef uint16_t dm_resource_index;
#endif

// === resources ===
typedef enum dm_renderpass_type_t
{
    DM_RENDERPASS_TYPE_INVALID,
    DM_RENDERPASS_TYPE_DEFAULT,
    DM_RENDERPASS_TYPE_CUSTOM
} dm_renderpass_type;

typedef enum dm_renderpass_flag_t
{
    DM_RENDERPASS_FLAG_NONE  = 0,
    DM_RENDERPASS_FLAG_COLOR = 1,
    DM_RENDERPASS_FLAG_DEPTH = 2
} dm_renderpass_flag;

typedef enum dm_pipeline_type_t
{
    DM_PIPELINE_TYPE_INVALID,
    DM_PIPELINE_TYPE_RASTER,
    DM_PIPELINE_TYPE_COMPUTE,
#ifdef DM_HARDWARE_RAYTRACING
    DM_PIPELINE_TYPE_RT
#endif
} dm_pipeline_type;

typedef struct dm_pipeline_handle_t
{
    dm_pipeline_type type;
    uint16_t index;
} dm_pipeline_handle;

typedef enum dm_resource_type_t
{
    DM_RESOURCE_TYPE_INALID,
    DM_RESOURCE_TYPE_VERTEX_BUFFER,
    DM_RESOURCE_TYPE_INDEX_BUFFER,
    DM_RESOURCE_TYPE_CONSTANT_BUFFER,
    DM_RESOURCE_TYPE_STORAGE_BUFFER,
    DM_RESOURCE_TYPE_TEXTURE,
    DM_RESOURCE_TYPE_SAMPLER,
#ifdef DM_HARDWARE_RAYTRACING
    DM_RESOURCE_TYPE_BLAS,
    DM_RESOURCE_TYPE_TLAS,
#endif
} dm_resource_type;

typedef struct dm_resource_handle_t
{
    dm_resource_type type;
#ifdef DM_METAL
    uint64_t gpu_address;
#else
    uint32_t descriptor_index;
#endif
    dm_resource_index index;
} dm_resource_handle;

#define DM_MAX_RENDERPASS 10
#define DM_MAX_RASTER_PIPES 10
#define DM_MAX_COMPUTE_PIPES 10
#define DM_MAX_TEXTURES (DM_MAX_RENDERPASS * 2 + 10) 
#define DM_MAX_VBS 10
#define DM_MAX_IBS 10
#define DM_MAX_CBS 10
#define DM_MAX_SBS 10
#ifdef DM_HARDWARE_RAYTRACING
#define DM_MAX_RT_PIPES 10
#define DM_MAX_RT_BLAS  DM_MAX_VBS
#define DM_MAX_RT_TLAS  10
#define DM_MAX_BUFFERS (DM_MAX_VBS * 2 + DM_MAX_IBS * 2 + DM_MAX_CBS * 2 + DM_MAX_SBS * 2 + DM_MAX_RT_BLAS + DM_MAX_RT_TLAS) 
#else
#define DM_MAX_BUFFERS (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS) * 2 // 2 for host and device
#endif

typedef struct dm_renderpass_desc_t
{
    dm_renderpass_flag flags;
    dm_renderpass_type type;
} dm_renderpass_desc;

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
    DM_INPUT_TOPOLOGY_LINE_LIST,
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

typedef struct dm_raster_pipeline_desc_t
{
    dm_raster_input_assembler_desc input_assembler;
    dm_rasterizer_desc             rasterizer;
    dm_depth_stencil_desc          depth_stencil;

    dm_viewport viewport;
    dm_scissor  scissor;
} dm_raster_pipeline_desc;

typedef struct dm_vertex_buffer_desc_t
{
    size_t size, stride;
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
    size_t size;
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
    uint32_t           width, height, n_channels;
    dm_texture_format  format;
    const void*        data;
    dm_resource_handle sampler;
} dm_texture_desc;

typedef enum dm_sampler_address_mode_t
{
    DM_SAMPLER_ADDRESS_MODE_BORDER,
    DM_SAMPLER_ADDRESS_MODE_WRAP,
    DM_SAMPLER_ADDRESS_MODE_UNKNOWN
} dm_sampler_address_mode;

typedef struct dm_sampler_desc_t
{
    dm_sampler_address_mode address_u, address_v, address_w;
} dm_sampler_desc;

bool dm_renderer_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
bool dm_renderer_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_renderer* renderer);

// === render commands ===
void dm_render_command_begin_update(dm_renderer* renderer);
void dm_render_command_end_update(dm_renderer* renderer);
void dm_render_command_begin_render_pass(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_renderer* renderer);
void dm_render_command_end_render_pass(dm_renderpass_handle handle, dm_renderer* renderer);
void dm_render_command_bind_raster_pipeline(dm_pipeline_handle handle, dm_renderer* renderer);
void dm_render_command_set_constants(uint32_t count, size_t offset, void* data, dm_renderer* renderer);
void dm_render_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_renderer* renderer);
void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, size_t offset, dm_renderer* renderer);
void dm_render_command_bind_index_buffer(dm_resource_handle handle, size_t offset, dm_renderer* renderer);
void dm_render_command_update_vertex_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer);
void dm_render_command_update_index_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer);
void dm_render_command_update_constant_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer);
void dm_render_command_update_storage_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer);
void dm_render_command_update_texture(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_renderer* renderer);
void dm_render_command_draw_instanced(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_renderer* renderer);
void dm_render_command_draw_instanced_indexed(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_renderer* renderer);

// === compute commands ===
void dm_compute_command_begin_recording(dm_renderer* renderer);
void dm_compute_command_end_recording(dm_renderer* renderer);
void dm_compute_command_bind_compute_pipeline(dm_pipeline_handle handle, dm_renderer* renderer);
void dm_compute_command_set_constants(uint32_t count, size_t offset, void* data, dm_renderer* renderer);
void dm_compute_command_dispatch(uint16_t x, uint16_t y, uint16_t z, dm_renderer* renderer);

/******************
 * VARIOUS MACROS *
 ******************/
#define DM_MAX(A, B) (A > B ? A : B)
#define DM_MIN(A, B) (A < B ? A : B)

/*****************
* IMPLEMENTATION *
******************/
#define DM_IMPLEMENTATION
#ifdef DM_IMPLEMENTATION

#define RGFW_IMPLEMENTATION
#include "lib/RGFW/RGFW.h"

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

void dm_memcpy(void* dest, void* src, size_t size)
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

// === input ===
typedef struct dm_keyboard_t
{
    bool keys[256];
} dm_keyboard;

typedef struct dm_mouse_t
{
    bool buttons[3];
    uint16_t x,y;
    float scroll;
} dm_mouse;

typedef struct dm_input_state_t
{
    dm_keyboard keyboard;
    dm_mouse    mouse;
} dm_input_state;

// === window ===
typedef enum dm_window_flag_t
{
    DM_WINDOW_FLAG_NONE   = 0,
    DM_WINDOW_FLAG_CLOSE  = 1,
    DM_WINDOW_FLAG_RESIZE = 2,
} dm_window_flag;

struct dm_window_t 
{
    RGFW_window* window;
    dm_window_flag flags;

    dm_input_state current_input;
    dm_input_state previous_input;
};

bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_window* window)
{
    RGFW_windowFlags window_flags = 0;
    if(flags & DM_WINDOW_CREATE_FLAG_CENTER) window_flags |= RGFW_windowCenter;
    if(flags & DM_WINDOW_CREATE_FLAG_NO_RESIZE) window_flags |= RGFW_windowNoResize;

    window->window = RGFW_createWindow(title, x,y,width,height, window_flags);
    if(!window->window) { return false; }

    return true;
}

void dm_window_shutdown(dm_window* window)
{
    RGFW_window_close(window->window);
}

bool dm_window_should_close(dm_window window)
{
    return window.flags & DM_WINDOW_FLAG_CLOSE;
}

bool dm_window_resized(dm_window window)
{
    return window.flags & DM_WINDOW_FLAG_RESIZE;
}

uint32_t dm_window_get_width(dm_window window)
{
    return window.window->w;
}

uint32_t dm_window_get_height(dm_window window)
{
    return window.window->h;
}

double dm_get_time()
{
#ifdef DM_PLATFORM_WINDOWS
    LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

    return time.QuadPart / frequency.QuadPart;
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

#ifndef DM_DEBUG
DM_INLINE
#endif
dm_key_code dm_convert_key_code(RGFW_key key)
{
    switch(key)
    {
        case RGFW_F1:  return DM_KEY_F1;
        case RGFW_F2:  return DM_KEY_F2;
        case RGFW_F3:  return DM_KEY_F3;
        case RGFW_F4:  return DM_KEY_F4;
        case RGFW_F5:  return DM_KEY_F5;
        case RGFW_F6:  return DM_KEY_F6;
        case RGFW_F7:  return DM_KEY_F7;
        case RGFW_F8:  return DM_KEY_F8;
        case RGFW_F9:  return DM_KEY_F9;
        case RGFW_F10: return DM_KEY_F10;
        case RGFW_F11: return DM_KEY_F11;
        case RGFW_F12: return DM_KEY_F12;
        case RGFW_F13: return DM_KEY_F13;
        case RGFW_F14: return DM_KEY_F14;
        case RGFW_F15: return DM_KEY_F15;
        case RGFW_F16: return DM_KEY_F16;

        case RGFW_a: return DM_KEY_A;
        case RGFW_b: return DM_KEY_B;
        case RGFW_c: return DM_KEY_C;
        case RGFW_d: return DM_KEY_D;
        case RGFW_e: return DM_KEY_E;
        case RGFW_f: return DM_KEY_F;
        case RGFW_h: return DM_KEY_H;
        case RGFW_i: return DM_KEY_I;
        case RGFW_j: return DM_KEY_J;
        case RGFW_k: return DM_KEY_K;
        case RGFW_l: return DM_KEY_L;
        case RGFW_m: return DM_KEY_M;
        case RGFW_n: return DM_KEY_N;
        case RGFW_o: return DM_KEY_O;
        case RGFW_p: return DM_KEY_P;
        case RGFW_q: return DM_KEY_Q;
        case RGFW_r: return DM_KEY_R;
        case RGFW_s: return DM_KEY_S;
        case RGFW_t: return DM_KEY_T;
        case RGFW_u: return DM_KEY_U;
        case RGFW_v: return DM_KEY_V;
        case RGFW_w: return DM_KEY_W;
        case RGFW_x: return DM_KEY_X;
        case RGFW_y: return DM_KEY_Y;
        case RGFW_z: return DM_KEY_Z;

        case RGFW_left:  return DM_KEY_LEFT;
        case RGFW_right: return DM_KEY_RIGHT;
        case RGFW_down:  return DM_KEY_DOWN;
        case RGFW_up:    return DM_KEY_UP;

        case RGFW_home:        return DM_KEY_HOME;
        case RGFW_printScreen: return DM_KEY_PRINT;
        case RGFW_pageUp:      return DM_KEY_PAGEUP;
        case RGFW_pageDown:    return DM_KEY_PAGEDOWN;
        case RGFW_numLock:     return DM_KEY_NUMLCK;

        case RGFW_tab:      return DM_KEY_TAB;
        case RGFW_capsLock: return DM_KEY_CAPSLOCK;
        case RGFW_escape:   return DM_KEY_ESCAPE;

        case RGFW_1: return DM_KEY_1;
        case RGFW_2: return DM_KEY_2;
        case RGFW_3: return DM_KEY_3;
        case RGFW_4: return DM_KEY_4;
        case RGFW_5: return DM_KEY_5;
        case RGFW_6: return DM_KEY_6;
        case RGFW_7: return DM_KEY_7;
        case RGFW_8: return DM_KEY_8;
        case RGFW_9: return DM_KEY_9;
        case RGFW_0: return DM_KEY_0;

        case RGFW_minus:  return DM_KEY_MINUS;
        case RGFW_kpPlus: return DM_KEY_PLUS;

        case RGFW_controlR:
        case RGFW_controlL: return DM_KEY_CTRL;
        case RGFW_altR:     return DM_KEY_RALT;
        case RGFW_altL:     return DM_KEY_LALT;
        case RGFW_shiftL:   return DM_KEY_LSHIFT;
        case RGFW_shiftR:   return DM_KEY_RSHIFT;
        case RGFW_space:    return DM_KEY_SPACE;
        case RGFW_enter:    return DM_KEY_ENTER;

        case RGFW_bracket:   return DM_KEY_LBRACE;
        case RGFW_comma:     return DM_KEY_COMMA;
        case RGFW_period:    return DM_KEY_PERIOD;
        case RGFW_backSpace: return DM_KEY_BACKSPACE;
        case RGFW_semicolon: return DM_KEY_COLON;

        default: return DM_KEY_NONE;
    }
}

dm_mousebutton_code dm_convert_mousebutton(RGFW_mouseButton button)
{
    switch(button)
    {
        case RGFW_mouseLeft:   return DM_MOUSEBUTTON_L;
        case RGFW_mouseRight:  return DM_MOUSEBUTTON_R;
        case RGFW_mouseMiddle: return DM_MOUSEBUTTON_M;

        default: return DM_MOUSEBUTTON_UNKNOWN;
    }   
}

bool dm_window_poll_events(dm_window* window)
{
    window->flags = DM_WINDOW_FLAG_NONE;

    window->previous_input = window->current_input;

    RGFW_waitForEvent(RGFW_eventWaitNext);
    RGFW_event event;
    while (RGFW_window_checkEvent(window->window, &event)) 
    {
        dm_key_code key;
        dm_mousebutton_code button;
        switch(event.type)
        {
            case RGFW_quit:
            window->flags |= DM_WINDOW_FLAG_CLOSE;
            break;

            case RGFW_keyPressed:
            case RGFW_keyReleased:
            key = dm_convert_key_code(event.key.value);
            if(key==DM_KEY_NONE) continue;

            window->current_input.keyboard.keys[key] = event.type==RGFW_keyPressed ? 1 : 0;
            break;

            case RGFW_mouseButtonPressed:
            case RGFW_mouseButtonReleased:
            button = dm_convert_mousebutton(event.button.value);
            if(button==DM_MOUSEBUTTON_UNKNOWN) continue;

            window->current_input.mouse.buttons[button] = event.type==RGFW_mouseButtonPressed ? 1 : 0;
            break;

            case RGFW_mousePosChanged:
            window->current_input.mouse.x = event.mouse.x;
            window->current_input.mouse.y = event.mouse.y;
            break;

            case RGFW_windowResized:
            window->flags |= DM_WINDOW_FLAG_RESIZE;
            break;

            default:
            break;
        }
    }

    return true;
} 

void dm_platform_write(const char* message, uint8_t color)
{
    #ifdef DM_PLATFORM_WINDOWS
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

// === input ===
bool dm_input_mouse_moved(dm_window window)
{
    bool move_x = window.current_input.mouse.x != window.previous_input.mouse.x;
    bool move_y = window.current_input.mouse.y != window.previous_input.mouse.y;
    
    return move_x || move_y;
}

bool dm_input_is_key_pressed(dm_key_code key, dm_window window)
{
    return window.current_input.keyboard.keys[key];
}

bool dm_input_key_just_pressed(dm_key_code key, dm_window window)
{
    return (window.current_input.keyboard.keys[key]==1 && window.previous_input.keyboard.keys[key]==0);
}

bool dm_input_key_just_released(dm_key_code key, dm_window window)
{
    return (window.current_input.keyboard.keys[key]==0 && window.previous_input.keyboard.keys[key]==1);
}

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_window window)
{
    return window.current_input.mouse.buttons[button];
}

bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_window window)
{
    return (window.current_input.mouse.buttons[button]==1 && window.previous_input.mouse.buttons[button]==0);
}

bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_window window)
{
    return (window.current_input.mouse.buttons[button]==0 && window.previous_input.mouse.buttons[button]==1);
}

uint16_t dm_input_get_mouse_pos_x(dm_window window)
{
    return window.current_input.mouse.x;
}

uint16_t dm_input_get_mouse_pos_y(dm_window window)
{
    return window.current_input.mouse.y;
}

void dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_window window)
{
    *x = dm_input_get_mouse_pos_x(window);
    *y = dm_input_get_mouse_pos_y(window);
}

int dm_input_get_mouse_delta_x(dm_window window)
{
    return window.current_input.mouse.x - window.previous_input.mouse.x;
}

int dm_input_get_mouse_delta_y(dm_window window)
{
    return window.current_input.mouse.y - window.previous_input.mouse.y;
}

void dm_input_get_mouse_delta(int* x, int* y, dm_window window)
{
    *x = dm_input_get_mouse_delta_x(window);
    *y = dm_input_get_mouse_delta_y(window);
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
    DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE,
    DM_RENDER_COMMAND_TYPE_END_UPDATE,
    DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_END_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE,
    DM_RENDER_COMMAND_TYPE_SET_CONSTANTS,
    DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES,
    DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED
} dm_render_command_type;

typedef enum dm_compute_command_type_t
{
    DM_COMPUTE_COMMAND_TYPE_INVALID,
    DM_COMPUTE_COMMAND_TYPE_BEGIN_RECORDING,
    DM_COMPUTE_COMMAND_TYPE_END_RECORDING,
    DM_COMPUTE_COMMAND_BIND_COMPUTE_PIPELINE,
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

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

typedef struct dm_metal_renderpass_t
{
    dm_resource_index render_target[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index depth_target[DM_MAX_FRAMES_IN_FLIGHT];
} dm_metal_renderpass;

typedef struct dm_metal_buffer_t
{
    dm_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index device[DM_MAX_FRAMES_IN_FLIGHT]; 
} dm_metal_buffer;

typedef struct dm_metal_index_buffer_t
{
    dm_metal_buffer buffer;
    MTLIndexType index_type;
} dm_metal_index_buffer;

typedef struct dm_metal_raster_pipeline_t
{
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLDepthStencilState>   depth_stencil_state;
    id<MTLSamplerState>        sampler_state;

    id<MTLLibrary>  vertex_library;
    id<MTLLibrary>  fragment_library;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;

    uint32_t uniform_offset;
    
    MTLCullMode cull_mode;
    MTLWinding  winding;

    MTLPrimitiveType primitive_type;
    MTLTriangleFillMode fill_mode;

    MTLViewport    viewport;
    MTLScissorRect scissor;

    dm_metal_buffer vertex_argument_buffer;
    dm_metal_buffer fragment_argument_buffer;
    id<MTLArgumentEncoder> vertex_encoder;
    id<MTLArgumentEncoder> fragment_encoder;
} dm_metal_raster_pipeline;

typedef struct dm_metal_heap_t
{
    id<MTLHeap> heap;
    size_t size;
    // TODO: not sure if I like this
    dm_resource_handle* resources[DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS + DM_MAX_TEXTURES];
    uint32_t resource_count;
} dm_metal_heap;
#endif

struct dm_renderer_t
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLDevice> device;

    id<MTLCommandQueue>          command_queue;
    
    id<MTLCommandBuffer>         render_command_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLRenderCommandEncoder>  render_command_encoder[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLBlitCommandEncoder>    render_blit_encoder[DM_MAX_FRAMES_IN_FLIGHT];

    id<MTLCommandBuffer>         compute_command_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLComputeCommandEncoder> compute_command_encoder[DM_MAX_FRAMES_IN_FLIGHT];

    CAMetalLayer* swapchain;
    id<CAMetalDrawable> render_target;
    uint32_t depth_target[DM_MAX_FRAMES_IN_FLIGHT];
    
    dispatch_semaphore_t frame_semaphore;
    dispatch_semaphore_t argument_buffer_semaphore;

    dm_metal_heap resource_heap[DM_MAX_FRAMES_IN_FLIGHT];
    dm_metal_heap sampler_heap[DM_MAX_FRAMES_IN_FLIGHT];

    //id<MTLBuffer> argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    //dm_metal_buffer argument_buffer;
    id<MTLBuffer> buffers[(DM_MAX_BUFFERS + DM_MAX_RASTER_PIPES * 2 * 2)* DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t      buffer_count;

    id<MTLTexture> textures[DM_MAX_TEXTURES * DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t       texture_count;

    dm_metal_renderpass renderpasses[DM_MAX_RENDERPASS];
    dm_metal_raster_pipeline raster_pipes[DM_MAX_RASTER_PIPES];
    dm_metal_buffer vertex_buffers[DM_MAX_VBS];
    dm_metal_index_buffer index_buffers[DM_MAX_IBS];
    dm_metal_buffer constant_buffers[DM_MAX_CBS];
    dm_metal_buffer storage_buffers[DM_MAX_SBS];

    uint32_t renderpass_count, raster_pipe_count, vb_count, ib_count, cb_count, sb_count;

    dm_pipeline_handle active_pipeline;
    dm_resource_handle active_index_buffer;

#endif

    uint8_t current_frame;

    dm_command_buffer render_commands;
};

// === render commands ===
void dm_render_command_submit(dm_command command, dm_command_buffer* command_buffer)
{
    if(command_buffer->command_count >= DM_COMMAND_BUFFER_MAX_COMMANDS)
    {
        dm_log(DM_LOG_ERROR, "Too many commands");
        return;
    }

    command_buffer->commands[command_buffer->command_count++] = command;
}

void dm_render_command_begin_update(dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_end_update(dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_UPDATE;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_begin_render_pass(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS;

    command.params[0].rph = handle;
    command.params[1].f = r;
    command.params[2].f = g;
    command.params[3].f = b;
    command.params[4].f = a;
    command.params[5].f = depth;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_end_render_pass(dm_renderpass_handle handle, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_RENDER_PASS;

    command.params[0].rph = handle;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_bind_raster_pipeline(dm_pipeline_handle handle, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE;

    command.params[0].ph = handle;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_set_constants(uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_SET_CONSTANTS;

    command.params[0].u32 = count;
    command.params[1].s   = offset;
    command.params[2].v   = data;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_renderer* renderer)
{
    dm_command command = { 0 };
    
    command.r_type = DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES;

    command.params[0].v   = (void*)handles;
    command.params[1].u16 = count;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, size_t offset, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].u8 = slot;
    command.params[2].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_bind_index_buffer(dm_resource_handle handle, size_t offset, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_update_vertex_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_update_index_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_update_constant_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_update_storage_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_update_texture(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE;

    command.params[0].rh  = handle;
    command.params[1].u16 = width;
    command.params[2].u16 = height;
    command.params[3].v   = data;
    command.params[4].s   = size;
    command.params[5].s   = offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_draw_instanced(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = vertex_count;
    command.params[3].s   = vertex_offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

void dm_render_command_draw_instanced_indexed(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_renderer* renderer)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = index_count;
    command.params[3].s   = index_offset;
    command.params[4].s   = vertex_offset;

    dm_render_command_submit(command, &renderer->render_commands);
}

// === backend ===
bool dm_renderer_init(dm_window window, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_log(DM_LOG_DEBUG, "Initializing Metal backend...");

    renderer->device = MTLCreateSystemDefaultDevice();
    if(!renderer->device)
    {
        dm_log(DM_LOG_FATAL, "Could not create Metal device");
        return false;
    }

    // check feature support
    if(renderer->device.argumentBuffersSupport!=MTLArgumentBuffersTier2)
    {
        dm_log(DM_LOG_FATAL, "ArgumentBuffersTier2 not supported");
        return false;
    }
#ifdef DM_RAYTRACING
    if(!renderer->device.supportsRaytracing)
    {
        DM_LOG_FATAL("Device does not support ray tracing");
        return false;
    }
#endif // DM_RAYTRACING

    // swapchain
    renderer->swapchain        = [CAMetalLayer layer];
    renderer->swapchain.device = renderer->device;
    renderer->swapchain.opaque = YES;

    renderer->command_queue = [renderer->device newCommandQueue];
    if(!renderer->command_queue)
    {
        dm_log(DM_LOG_FATAL, "newCommandQueue failed");
        return false;
    }

    // must set the content view's layer to our metal layer
    NSView* view = (NSView*)RGFW_window_getView_OSX(window.window);
    [view setWantsLayer: YES];
    [view setLayer:renderer->swapchain];
    
    NSSize layer_size = view.layer.frame.size;
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    NSSize drawable_size = NSMakeSize(layer_size.width * scale, layer_size.height * scale);
    renderer->swapchain.contentsScale = scale;
    renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture 
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:window.window->w height:window.window->h mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->textures[renderer->texture_count] = [renderer->device newTextureWithDescriptor:descriptor];

        renderer->depth_target[i] = renderer->texture_count++;
    }

    [descriptor release];

    // command queue 
    renderer->command_queue = [renderer->device newCommandQueue];

    // synchronization 
    renderer->frame_semaphore = dispatch_semaphore_create(DM_MAX_FRAMES_IN_FLIGHT);
    renderer->argument_buffer_semaphore = dispatch_semaphore_create(1);

#endif

    return true;
}

#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_copy_buffer_to_heap(dm_metal_buffer* buffer, dm_renderer* renderer, id<MTLBlitCommandEncoder> blit_encoder)
{
    id<MTLBuffer> host_buffer;
    id<MTLBuffer> device_buffer;
    size_t size;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        host_buffer = renderer->buffers[buffer->host[i]];
        size = host_buffer.length;

        device_buffer = [renderer->resource_heap[i].heap newBufferWithLength:size options:MTLResourceStorageModePrivate];
        if(!device_buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }
        [blit_encoder copyFromBuffer:host_buffer sourceOffset:0 toBuffer:device_buffer destinationOffset:0 size:size];

        renderer->buffers[renderer->buffer_count] = device_buffer;
        buffer->device[i] = renderer->buffer_count++;
    }

    return true;
}
#endif

bool dm_renderer_finish_init(dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLCommandBuffer> command_buffer = [renderer->command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = command_buffer.blitCommandEncoder;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        MTLHeapDescriptor* descriptor = [MTLHeapDescriptor new];
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.size        = renderer->resource_heap[i].size;
#ifdef DM_DEBUG 
        dm_log(DM_LOG_DEBUG, "Resource heap (frame %u) size: %u bytes", i, descriptor.size); 
#endif  

        renderer->resource_heap[i].heap = [renderer->device newHeapWithDescriptor:descriptor];

        [descriptor release];
    }

    // move resources to the heap
    for(uint32_t i=0; i<renderer->raster_pipe_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->raster_pipes[i].vertex_argument_buffer, renderer, blit_encoder)) return false;
        if(!dm_metal_copy_buffer_to_heap(&renderer->raster_pipes[i].fragment_argument_buffer, renderer, blit_encoder)) return false;
    }

    for(uint32_t i=0; i<renderer->vb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->vertex_buffers[i], renderer, blit_encoder)) return false;
    }
    for(uint32_t i=0; i<renderer->ib_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->index_buffers[i].buffer, renderer, blit_encoder)) return false;
    }
    for(uint32_t i=0; i<renderer->cb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->constant_buffers[i], renderer, blit_encoder)) return false;
    }

    // update gpu resource id
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        for(uint32_t j=0; j<renderer->resource_heap[i].resource_count; j++)
        {
            dm_resource_handle* handle = renderer->resource_heap[i].resources[j];

            switch(handle->type)
            {
                case DM_RESOURCE_TYPE_VERTEX_BUFFER:
                handle->gpu_address = renderer->buffers[renderer->vertex_buffers[handle->index].device[i]].gpuAddress;
                break;
                case DM_RESOURCE_TYPE_INDEX_BUFFER:
                handle->gpu_address = renderer->buffers[renderer->index_buffers[handle->index].buffer.device[i]].gpuAddress;
                break;
                case DM_RESOURCE_TYPE_CONSTANT_BUFFER:
                handle->gpu_address = renderer->buffers[renderer->constant_buffers[handle->index].device[i]].gpuAddress;
                break;

                default:
                dm_log(DM_LOG_FATAL, "Unknown/unsupported resource. Should not be here.");
                return false;
            }
        }
    }

    [blit_encoder endEncoding];
    [command_buffer commit];
#endif

    return true;
}

bool dm_renderer_begin_frame(dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dispatch_semaphore_wait(renderer->frame_semaphore, DISPATCH_TIME_FOREVER);

    // new render target
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    CGSize size = renderer->swapchain.bounds.size;
    renderer->swapchain.contentsScale = scale;
    renderer->swapchain.drawableSize = size;
    
    renderer->render_target = [renderer->swapchain nextDrawable];
    if(!renderer->render_target) { dm_log(DM_LOG_FATAL, "nextDrawable failed"); return false; }

    // command buffer
    MTLCommandBufferDescriptor* desc = [MTLCommandBufferDescriptor new];
    desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    renderer->render_command_buffer[renderer->current_frame] = [renderer->command_queue commandBufferWithDescriptor:desc];
    [desc release];

    // pipeline
    renderer->active_pipeline.type = DM_PIPELINE_TYPE_INVALID;
#endif

    return true;
}

bool dm_renderer_end_frame(bool vsync, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    // present
    renderer->swapchain.displaySyncEnabled = vsync;
    [renderer->render_command_buffer[current_frame] presentDrawable:renderer->render_target];

    // sync
    __block dispatch_semaphore_t block_semaphore = renderer->frame_semaphore;
    [renderer->render_command_buffer[current_frame] addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(block_semaphore);
    }];

    // final commit
    [renderer->render_command_buffer[current_frame] commit];
    [renderer->render_command_buffer[current_frame] waitUntilCompleted];

    // update frame
    renderer->current_frame = (renderer->current_frame + 1) % DM_MAX_FRAMES_IN_FLIGHT;
#endif

    return true;
}

void dm_renderer_shutdown(dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    for(uint32_t i=0; i<renderer->raster_pipe_count; i++)
    {
        [renderer->raster_pipes[i].fragment_encoder release];
        [renderer->raster_pipes[i].vertex_encoder release];

        [renderer->raster_pipes[i].fragment_func release];
        [renderer->raster_pipes[i].vertex_func release];

        [renderer->raster_pipes[i].pipeline_state release];
    }

    for(uint32_t i=0; i<renderer->buffer_count; i++)
    {
        [renderer->buffers[i] release];
    }
    for(uint32_t i=0; i<renderer->texture_count; i++)
    {
        [renderer->textures[i] release];
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        [renderer->compute_command_encoder[i] release];
        [renderer->resource_heap[i].heap release];
    }

    [renderer->swapchain release];
    [renderer->command_queue release];
    [renderer->device release];
#endif
}

bool dm_renderer_resize(dm_window window, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    renderer->swapchain.contentsScale = scale;
    CGSize drawable_size;
    drawable_size.width  = window.window->w;
    drawable_size.height = window.window->h;
    renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        [renderer->textures[renderer->depth_target[i]] release];
        renderer->textures[renderer->depth_target[i]] = [renderer->device newTextureWithDescriptor:descriptor];
    }
#endif

    return true;
}

/*************
 * RESOURCES *
 *************/
bool dm_renderer_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_metal_renderpass renderpass = { 0 };

    switch(desc.type)
    {
        case DM_RENDERPASS_TYPE_DEFAULT:
        break;

        case DM_RENDERPASS_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom renderpasses not supported yet");
        default:
        return false;
    }

    dm_memcpy(renderer->renderpasses + renderer->renderpass_count, &renderpass, sizeof(renderpass));

    *handle = renderer->renderpass_count++;
#endif

    return true;
}
#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_shader(const char* path, id<MTLLibrary>* library, id<MTLDevice> device)
{
    NSString* file = [NSString stringWithUTF8String:path];

    NSURL* library_url = [NSURL URLWithString:file];
    NSError* library_error = NULL;

    *library = [device newLibraryWithURL:library_url error:&library_error];
    if(!*library)
    {
        dm_log(DM_LOG_FATAL, "Creating Metal shader library failed");
        dm_log(DM_LOG_ERROR, "%s", [library_error.localizedDescription UTF8String]);
        [file release];
        [library_url release];

        return false;
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_shader_function(const char* shader_function, id<MTLFunction>* function, id<MTLLibrary> library, id<MTLDevice> device)
{
    NSString* func_name = [[NSString alloc] initWithUTF8String:shader_function];

    *function = [library newFunctionWithName:func_name];

    if(!*function) { dm_log(DM_LOG_FATAL, "Failed to create Metal function"); return false; }

    [func_name release];
    return true;
}
#endif // DM_METAL

bool dm_renderer_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_metal_raster_pipeline pipeline = { 0 };

    // shaders 
    if(!dm_metal_create_shader(desc.rasterizer.vertex_shader_desc.path, &pipeline.vertex_library, renderer->device)) return false;
    if(!dm_metal_create_shader_function("vertex_main", &pipeline.vertex_func, pipeline.vertex_library, renderer->device)) return false;

    if(!dm_metal_create_shader(desc.rasterizer.pixel_shader_desc.path, &pipeline.fragment_library, renderer->device)) return false;
    if(!dm_metal_create_shader_function("fragment_main", &pipeline.fragment_func, pipeline.fragment_library, renderer->device)) return false;

    // pipeline state
    MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
    pipe_desc.rasterSampleCount = 1;

    pipe_desc.vertexFunction   = pipeline.vertex_func;
    pipe_desc.fragmentFunction = pipeline.fragment_func;

    pipe_desc.colorAttachments[0].pixelFormat                 = MTLPixelFormatBGRA8Unorm;
    pipe_desc.colorAttachments[0].blendingEnabled             = YES;
    pipe_desc.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
    pipe_desc.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    pipe_desc.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    pipe_desc.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;
    pipe_desc.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
    pipe_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    NSError* error = NULL;
    pipeline.pipeline_state = [renderer->device newRenderPipelineStateWithDescriptor:pipe_desc error:&error];
    if(!pipeline.pipeline_state)
    {
        dm_log(DM_LOG_FATAL, "Creating Metal pipeline state failed");
        dm_log(DM_LOG_ERROR, "%s", [error.localizedDescription UTF8String]);
        [pipeline.vertex_library release];
        [pipeline.fragment_library release];
        [pipeline.vertex_func release];
        [pipeline.fragment_func release];

        return false;
    }

    // misc
    switch(desc.rasterizer.cull_mode)
    {
        case DM_RASTERIZER_CULL_MODE_BACK:
        pipeline.cull_mode = MTLCullModeBack;
        break;

        case DM_RASTERIZER_CULL_MODE_FRONT:
        pipeline.cull_mode = MTLCullModeFront;
        break;

        case DM_RASTERIZER_CULL_MODE_NONE:
        pipeline.cull_mode = MTLCullModeNone;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported cull mode");
        return false;
    }

    switch(desc.rasterizer.front_face)
    {
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pipeline.winding = MTLWindingCounterClockwise;
        break;

        case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
        pipeline.winding = MTLWindingClockwise;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported winding");
        return false;
    }

    switch(desc.input_assembler.topology)
    {
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.primitive_type = MTLPrimitiveTypeTriangle;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.primitive_type = MTLPrimitiveTypeLine;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown/unsupported topology");
        return false;
    }

    pipeline.fill_mode = desc.rasterizer.polygon_fill==DM_RASTERIZER_POLYGON_FILL_WIREFRAME ? MTLTriangleFillModeLines : MTLTriangleFillModeFill;

    pipeline.vertex_encoder   = [pipeline.vertex_func newArgumentEncoderWithBufferIndex:0];
    if(!pipeline.vertex_encoder) { dm_log(DM_LOG_FATAL, "newArgumentEncoderWithBufferIndex failed"); return false; }
    pipeline.fragment_encoder = [pipeline.fragment_func newArgumentEncoderWithBufferIndex:0];
    if(!pipeline.fragment_encoder) { dm_log(DM_LOG_FATAL, "newArgumentEncoderWithBufferIndex failed"); return false; }

    // argument buffer
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        size_t size = pipeline.vertex_encoder.encodedLength;
        id<MTLBuffer> buffer = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }

        renderer->buffers[renderer->buffer_count] = buffer;
        pipeline.vertex_argument_buffer.host[i] = renderer->buffer_count++;

        size = pipeline.fragment_encoder.encodedLength;
        buffer = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }

        renderer->buffers[renderer->buffer_count] = buffer;
        pipeline.fragment_argument_buffer.host[i] = renderer->buffer_count++;
    }

    //
    renderer->raster_pipes[renderer->raster_pipe_count] = pipeline;
    handle->index = renderer->raster_pipe_count++;
#endif

    handle->type = DM_PIPELINE_TYPE_RASTER;

    return true;
}
#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_buffer(void* data, size_t size, dm_metal_buffer* buffer, dm_renderer* renderer) 
{
    id<MTLBuffer> host_buffer;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(data) 
        {
            host_buffer = [renderer->device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache]; 
            if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithBytes failed"); return false; }
        }
        else
        {
            host_buffer = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
            if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }
        }

        renderer->buffers[renderer->buffer_count] = host_buffer;
        buffer->host[i] = renderer->buffer_count++;

        // heap size
        size_t size = host_buffer.length; 
        MTLSizeAndAlign size_and_align = [renderer->device heapBufferSizeAndAlignWithLength:size options:MTLResourceStorageModePrivate];
        size_and_align.size += (size_and_align.size & (size_and_align.align - 1)) + size_and_align.align;
        renderer->resource_heap[i].size += size_and_align.size;
    }

    return true;
}
#endif  

bool dm_renderer_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    if(renderer->vb_count >= DM_MAX_VBS) { dm_log(DM_LOG_FATAL, "Trying to create too many vertex buffers. Increase DM_MAX_VBS"); return false; }

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, renderer)) return false;

    renderer->vertex_buffers[renderer->vb_count] = buffer;
    handle->index = renderer->vb_count++;
    
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->resource_heap[i].resources[renderer->resource_heap[i].resource_count++] = handle;
    }
#endif

    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    return true;
}

bool dm_renderer_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_metal_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_type = MTLIndexTypeUInt16;
        break;

        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_type = MTLIndexTypeUInt32;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported index type");
        return false;
    }
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer.buffer, renderer)) return false; 
    
    //
    renderer->index_buffers[renderer->ib_count] = buffer;
    handle->index = renderer->ib_count++;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->resource_heap[i].resources[renderer->resource_heap[i].resource_count++] = handle;
    }
#endif
    
    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    return true;
}

bool dm_renderer_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, renderer)) return false;

    renderer->constant_buffers[renderer->cb_count] = buffer;
    handle->index = renderer->cb_count++;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->resource_heap[i].resources[renderer->resource_heap[i].resource_count++] = handle;
    }
#endif

    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    return true;
}

bool dm_renderer_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    return true;
}

bool dm_renderer_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    return true;
}

bool dm_renderer_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    handle->type = DM_RESOURCE_TYPE_SAMPLER;

    return true;
}

/******************
* RENDER COMMANDS *
*******************/
bool dm_render_command_begin_update_backend(dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    renderer->render_blit_encoder[current_frame] = [renderer->render_command_buffer[current_frame] blitCommandEncoder];
#endif

    return true;
}

bool dm_render_command_end_update_backend(dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    [renderer->render_blit_encoder[current_frame] endEncoding];
#endif

    return true;
}

bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    const uint8_t current_frame = renderer->current_frame;

    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];

    pass_desc.colorAttachments[0].texture     = [renderer->render_target texture]; 
    pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass_desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
    pass_desc.colorAttachments[0].clearColor  = MTLClearColorMake(r,g,b,a);

    pass_desc.depthAttachment.texture     = renderer->textures[renderer->depth_target[current_frame]]; 
    pass_desc.depthAttachment.clearDepth  = depth;
    pass_desc.depthAttachment.storeAction = MTLStoreActionDontCare;
    pass_desc.depthAttachment.loadAction  = MTLLoadActionClear;

    dm_metal_renderpass* pass = &renderer->renderpasses[handle];

    id<MTLCommandBuffer> command_buffer = renderer->render_command_buffer[current_frame];
    id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if(!encoder) { dm_log(DM_LOG_FATAL, "renderCommandEncoderWithDescriptor failed"); return false; }

    renderer->render_command_encoder[current_frame] = encoder;

    // bind heap
    [encoder useHeap:renderer->resource_heap[current_frame].heap stages:MTLRenderStageVertex];
    [encoder useHeap:renderer->resource_heap[current_frame].heap stages:MTLRenderStageFragment];

    [pass_desc release];
#endif

    return true;
}

bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    [renderer->render_command_encoder[renderer->current_frame] endEncoding];
#endif

    return true;
}

bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];
    dm_metal_raster_pipeline* pipeline = &renderer->raster_pipes[handle.index];

    pipeline->viewport.width  = renderer->swapchain.drawableSize.width;
    pipeline->viewport.height = renderer->swapchain.drawableSize.height;
    pipeline->viewport.zfar   = 1.f;

    pipeline->scissor.x = 0;
    pipeline->scissor.y = 0;
    pipeline->scissor.width = renderer->swapchain.drawableSize.width;
    pipeline->scissor.height = renderer->swapchain.drawableSize.height;

    [encoder setRenderPipelineState:pipeline->pipeline_state];
    [encoder setFrontFacingWinding:pipeline->winding];
    [encoder setCullMode:pipeline->cull_mode];
    [encoder setViewport:pipeline->viewport];
    [encoder setScissorRect:pipeline->scissor];
    [encoder setTriangleFillMode:pipeline->fill_mode];

    renderer->active_pipeline = handle;
#endif

    return true;
}

bool dm_render_command_set_constants_backend(uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    return true;
}

bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    if(renderer->active_pipeline.type==DM_PIPELINE_TYPE_INVALID) { dm_log(DM_LOG_FATAL, "No pipeline is set"); return false; }

    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    switch(renderer->active_pipeline.type)
    { 
        case DM_PIPELINE_TYPE_RASTER:
        {
            dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];

            id<MTLBuffer> vertex_argument_buffer   = renderer->buffers[pipeline.vertex_argument_buffer.host[current_frame]];
            id<MTLBuffer> fragment_argument_buffer = renderer->buffers[pipeline.fragment_argument_buffer.host[current_frame]];

            [pipeline.vertex_encoder setArgumentBuffer:vertex_argument_buffer offset:0];
            [pipeline.fragment_encoder setArgumentBuffer:fragment_argument_buffer offset:0];

            for(uint16_t i=0; i<count; i++)
            {
                switch(handles[i].type)
                {
                    case DM_RESOURCE_TYPE_CONSTANT_BUFFER:
                    {
                        id<MTLBuffer> buffer = renderer->buffers[renderer->constant_buffers[handles[i].index].device[current_frame]];

                        // TODO: FIX!
                        [pipeline.vertex_encoder setBuffer:buffer offset:0 atIndex:0];
                    } break;

                    default:
                    dm_log(DM_LOG_FATAL, "Unknown/unsupported resource type");
                    return false;
                }
            }

            [encoder setVertexBuffer:vertex_argument_buffer offset:0 atIndex:0];
            [encoder setFragmentBuffer:fragment_argument_buffer offset:0 atIndex:0];
        } break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported pipeline type");
        return false;
    }


    for(uint16_t i=0; i<count; i++)
    {
    }
#endif

    return true;
}

bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    dm_metal_buffer buffer = renderer->vertex_buffers[handle.index];

    id<MTLBuffer> vertex_buffer = renderer->buffers[buffer.device[current_frame]];

    // always have an argument buffer being bound
    // vertex buffer must be second buffer
    [encoder setVertexBuffer:vertex_buffer offset:offset atIndex:(slot+1)];
#endif

    return true;
}

bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    renderer->active_index_buffer = handle;
#endif

    return true;
}

bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    return true;
}

bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    return true;
}

bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLBlitCommandEncoder> blit_encoder = renderer->render_blit_encoder[current_frame];

    dm_metal_buffer buffer = renderer->constant_buffers[handle.index];

    dm_memcpy(renderer->buffers[buffer.host[current_frame]].contents, data, size);

    id<MTLBuffer> host_buffer   = renderer->buffers[buffer.host[current_frame]];
    id<MTLBuffer> device_buffer = renderer->buffers[buffer.device[current_frame]];

    [blit_encoder copyFromBuffer:host_buffer sourceOffset:0 toBuffer:device_buffer destinationOffset:0 size:size];
#endif

    return true;
}

bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    return true;
}

bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
#endif

    return true;
}

bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];
    dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];

    [encoder drawPrimitives:pipeline.primitive_type vertexStart:vertex_offset vertexCount:vertex_count instanceCount:instance_count baseInstance:instance_offset];
#endif

    return true;
}

bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_VULKAN)
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];
    dm_metal_index_buffer buffer = renderer->index_buffers[renderer->active_index_buffer.index];

    id<MTLBuffer> index_buffer = renderer->buffers[buffer.buffer.device[current_frame]];

    [encoder drawIndexedPrimitives:pipeline.primitive_type indexCount:index_count indexType:buffer.index_type indexBuffer:index_buffer indexBufferOffset:index_offset instanceCount:instance_count baseVertex:vertex_offset baseInstance:instance_offset];
#endif

    return true;
}

bool dm_renderer_submit_render_commands(dm_renderer* renderer)
{
    dm_command_buffer buffer = renderer->render_commands;

    for(uint32_t i=0; i<buffer.command_count; i++)
    {
        dm_command command = buffer.commands[i];
        dm_command_param* params = command.params;

        switch(command.r_type)
        {
            case DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE:
            if(dm_render_command_begin_update_backend(renderer)) continue;
            dm_log(DM_LOG_FATAL, "Begin update failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_UPDATE:
            if(dm_render_command_end_update_backend(renderer)) continue;
            dm_log(DM_LOG_FATAL, "End update failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS:
            if(dm_render_command_begin_render_pass_backend(params[0].rph,params[1].f,params[2].f,params[3].f,params[4].f,params[5].f, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Begin render pass failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_RENDER_PASS:
            if(dm_render_command_end_render_pass_backend(params[0].rph, renderer)) continue;
            dm_log(DM_LOG_FATAL, "End render pass failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE:
            if(dm_render_command_bind_raster_pipeline_backend(params[0].ph, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Bind raster pipeline failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_SET_CONSTANTS:
            if(dm_render_command_set_constants_backend(params[0].u32, params[1].s, params[2].v, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Set constants failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES:
            if(dm_render_command_submit_resources_backend(params[0].v,params[1].u16, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Submit resources failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER:
            if(dm_render_command_bind_vertex_buffer_backend(params[0].rh, params[1].u32, params[2].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Bind vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER:
            if(dm_render_command_bind_index_buffer_backend(params[0].rh, params[1].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Bind index buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER:
            if(dm_render_command_update_vertex_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Update vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER:
            if(dm_render_command_update_index_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Update index buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER:
            if(dm_render_command_update_constant_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Update constant buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER:
            if(dm_render_command_update_storage_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Update storage buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE:
            if(dm_render_command_update_texture_backend(params[0].rh, params[1].u16,params[2].u16,params[3].v,params[4].s,params[5].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Update texture failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED:
            if(dm_render_command_draw_instanced_backend(params[0].u32,params[1].s,params[2].u32,params[3].s, renderer)) continue;
            dm_log(DM_LOG_FATAL, "Draw instanced failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED:
            if(dm_render_command_draw_instanced_indexed_backend(params[0].u32,params[1].s,params[2].u32,params[3].s,params[4].s, renderer)) continue;
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

#endif // DM_IMPLEMENTATION

#endif // DM_H
