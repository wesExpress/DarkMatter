#ifndef __DM_H__
#define __DM_H__

#include "dm_defines.h"

typedef uint16_t dm_renderpass_handle;

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

typedef uint32_t dm_resource_index;
typedef uint32_t dm_viewport_index;
typedef uint32_t dm_scissor_index;

typedef struct dm_resource_handle_t
{
    dm_resource_type type;
    dm_resource_index index;
} dm_resource_handle;
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

// === context ===
typedef struct dm_window_t   dm_window;
typedef struct dm_renderer_t dm_renderer;

typedef enum dm_context_flag_t
{
    DM_CONTEXT_FLAG_NONE,
    DM_CONTEXT_FLAG_IS_RUNNING,
    DM_CONTEXT_FLAG_VSYNC_ON,
} dm_context_flag;

typedef struct dm_context_t
{
    dm_context_flag flags;

    dm_command_buffer render_command_buffer;
    dm_command_buffer compute_command_buffer;

    dm_window*   window;
    dm_renderer* renderer;
} dm_context;

typedef enum dm_window_create_flag_t
{
    DM_WINDOW_CREATE_FLAG_NONE      = 0,
    DM_WINDOW_CREATE_FLAG_CENTER    = 1,
    DM_WINDOW_CREATE_FLAG_NO_RESIZE = 2
} dm_window_create_flag;

dm_context* dm_init(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags);
void dm_shutdown(dm_context* context);
bool dm_update(dm_context* context);

/********
 * MATH *
 ********/
#ifndef DM_DIRECTX12
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include "lib/cglm/include/cglm/cglm.h"

#define DM_PI      3.1415926535
#define DM_2PI    (2.f * DM_PI)
#define DM_INV_PI  0.31830989f
#define DM_INV_2PI 0.15915494f
#define DM_INV_180 0.0055555f
#define DM_DEG_TO_RAD(X) (X * DM_PI * DM_INV_180)
#define DM_RAD_TO_DEG(X) (X * 180.f * DM_INV_PI)

/**********
 * MEMORY * 
 **********/
void* dm_alloc(size_t size);
void  dm_free(void** ptr);
void* dm_realloc(void* ptr, size_t size);
void  dm_memcpy(void* dst, const void* src, size_t size);
void* dm_memset(void* dst, int value, size_t size);
void* dm_memzero(void* dst, size_t size);

/*********
 * TIMER *
 ********/
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
#ifdef DM_DEBUG
    DM_LOG_TRACE,
    DM_LOG_DEBUG,
#endif
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

bool dm_input_is_key_pressed(dm_key_code key, dm_context* context);
bool dm_input_key_just_pressed(dm_key_code key, dm_context* context);
bool dm_input_key_just_released(dm_key_code key, dm_context* context);

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_context* context);
bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_context* context);
bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_context* context);

bool     dm_input_mouse_moved(dm_context* context);
uint16_t dm_input_get_mouse_pos_x(dm_context* context);
uint16_t dm_input_get_mouse_pos_y(dm_context* context);
void     dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_context* context);
int      dm_input_get_mouse_delta_x(dm_context* context);
int      dm_input_get_mouse_delta_y(dm_context* context);
void     dm_input_get_mouse_delta(int* x, int* y, dm_context* context);

/*********
* WINDOW *
**********/
uint32_t dm_get_window_width(dm_context* context);
uint32_t dm_get_window_height(dm_context* context);;

/*************
 * RENDERING *
 *************/
#ifndef DM_MAX_FRAMES_IN_FLIGHT
#define DM_MAX_FRAMES_IN_FLIGHT 3
#endif

bool dm_finish_init(dm_context* context);
bool dm_submit_render_commands(dm_context* context);

// === handles ===

// === resources ===
#define DM_MAX_RENDERPASS 10
#define DM_MAX_RASTER_PIPES 10
#define DM_MAX_COMPUTE_PIPES 10
#define DM_MAX_TEXTURES 10 
#define DM_MAX_VBS 10
#define DM_MAX_IBS 10
#define DM_MAX_CBS 10
#define DM_MAX_SBS 10
#define DM_MAX_SAMPLERS 10
#define DM_MAX_VIEWPORTS 10
#define DM_MAX_SCISSORS 10
#ifdef DM_HARDWARE_RAYTRACING
#define DM_MAX_RT_PIPES 10
#define DM_MAX_RT_BLAS  DM_MAX_VBS
#define DM_MAX_RT_TLAS  10
#define DM_MAX_BUFFERS (DM_MAX_VBS * 2 + DM_MAX_IBS * 2 + DM_MAX_CBS * 2 + DM_MAX_SBS * 2 + DM_MAX_RT_BLAS + DM_MAX_RT_TLAS)
#else
#define DM_MAX_BUFFERS (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS) * 2 // 2 for host and device
#endif

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

typedef struct dm_viewport_t
{
    uint32_t left, right, top, bottom;
} dm_viewport;

typedef struct dm_scissor_t
{
    uint32_t left, right, top, bottom;
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
} dm_raster_pipeline_desc;

typedef struct dm_vertex_buffer_desc_t
{
    size_t size, stride;
    const void*  data;
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
    const void* data;
} dm_index_buffer_desc;

typedef struct dm_constant_buffer_desc_t
{
    size_t size;
    const void*  data;
} dm_constant_buffer_desc;

typedef struct dm_storage_buffer_desc_t
{
    size_t size, stride;
    const void*  data;
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
    dm_resource_handle sampler;
    const void*        data;
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

// resource creation
bool dm_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_context* context);
bool dm_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_context* context);
bool dm_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context);
bool dm_create_texture_from_file(const char* path, dm_resource_handle* handle, dm_context* context);
bool dm_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context);
void dm_create_viewport(dm_viewport viewport, dm_viewport_index* index, dm_context* context);
void dm_create_scissor(dm_scissor scissor, dm_scissor_index* index, dm_context* context);

uint32_t dm_get_resource_index(dm_resource_handle handle, dm_context* context);

// === render commands ===
void dm_render_command_begin_frame(dm_context* context);
void dm_render_command_end_frame(dm_context* context);
void dm_render_command_begin_update(dm_context* context);
void dm_render_command_end_update(dm_context* context);
void dm_render_command_begin_render_pass(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_context* context);
void dm_render_command_end_render_pass(dm_renderpass_handle handle, dm_context* context);
void dm_render_command_bind_raster_pipeline(dm_pipeline_handle handle, dm_context* context);
void dm_render_command_set_viewport(dm_viewport_index index, dm_context* context);
void dm_render_command_set_scissor(dm_scissor_index index, dm_context* context);
void dm_render_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_context* context);
void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, size_t offset, dm_context* context);
void dm_render_command_bind_index_buffer(dm_resource_handle handle, size_t offset, dm_context* context);
void dm_render_command_update_vertex_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context);
void dm_render_command_update_index_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context);
void dm_render_command_update_constant_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context);
void dm_render_command_update_storage_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context);
void dm_render_command_update_texture(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_context* context);
void dm_render_command_draw_instanced(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_context* context);
void dm_render_command_draw_instanced_indexed(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_context* context);

// === compute commands ===
void dm_compute_command_begin_recording(dm_context* context);
void dm_compute_command_end_recording(dm_context* context);
void dm_compute_command_bind_compute_pipeline(dm_pipeline_handle handle, dm_context* context);
void dm_compute_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_context* context);
void dm_compute_command_dispatch(uint16_t x, uint16_t y, uint16_t z, dm_context* context);

/******************
 * VARIOUS MACROS *
 ******************/
#define DM_MAX(A, B) (A > B ? A : B)
#define DM_MIN(A, B) (A < B ? A : B)
#define DM_COUNTOF(X) sizeof(X) / sizeof(X[0])

#endif // __DM_H__
