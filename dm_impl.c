#include "dm.h"
#include <time.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "lib/stb_image/stb_image.h"

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

typedef struct dm_window_t 
{
    dm_window_flag flags;

    dm_input_state current_input;
    dm_input_state previous_input;

    RGFW_window* window;
} dm_window; 

bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_context* context)
{
    context->window = dm_alloc(sizeof(dm_window));
    dm_window* window = context->window;

    RGFW_windowFlags window_flags = 0;
    if(flags & DM_WINDOW_CREATE_FLAG_CENTER)    window_flags |= RGFW_windowCenter;
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

uint32_t dm_get_window_width(dm_context* context)
{
    dm_window* window = context->window;
    return window->window->w;
}

uint32_t dm_get_window_height(dm_context* context)
{
    dm_window* window = context->window;
    return window->window->h;
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

#ifdef DM_PLATFORM_WIN32
bool dm_platform_win32_decode_hresult(HRESULT hr)
{
    if (hr == S_OK) return true;
    
    dm_log(DM_LOG_FATAL, "Hresult failed with:");
    
    switch (hr)
    {
        case E_ABORT: 
        dm_log(DM_LOG_ERROR, "Operation aborted");
        break;
        
        case E_ACCESSDENIED:
        dm_log(DM_LOG_ERROR, "General access denied error");
        break;
        
        case E_FAIL:
        dm_log(DM_LOG_ERROR, "Unspecified failure");
        break;
        
        case E_HANDLE:
        dm_log(DM_LOG_ERROR, "Handle that is not valid");
        break;
        
        case E_INVALIDARG:
        dm_log(DM_LOG_ERROR, "One or more arguments are not valid");
        break;
        
        case E_NOINTERFACE:
        dm_log(DM_LOG_ERROR, "No such interface supported");
        break;
        
        case E_NOTIMPL:
        dm_log(DM_LOG_ERROR, "Not implemented");
        break;
        
        case E_OUTOFMEMORY:
        dm_log(DM_LOG_ERROR, "Failed to allocate necessary memory");
        break;
        
        case E_POINTER:
        dm_log(DM_LOG_ERROR, "Pointer that is not valid");
        break;
        
        case E_UNEXPECTED:
        dm_log(DM_LOG_ERROR, "Unexpected failure");
        break;
        
        case 0x80070002:
        dm_log(DM_LOG_ERROR, "File not found");
        break;
        
        case 0x887A0005:
        dm_log(DM_LOG_ERROR, "DXGI Error: Device removed: Device hung");
        break;
        
        case 0x887A0006:
        dm_log(DM_LOG_ERROR, "GPU will not respond to more commands due to invalid command");
        break;

        case 0x8876086c:
        dm_log(DM_LOG_ERROR, "Invalid call");
        break;
        
        default:
        dm_log(DM_LOG_ERROR, "Unknown error: %u", hr);
        break;
    }
    
    return false;
}
#endif

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

// === input ===
bool dm_input_mouse_moved(dm_context* context)
{
    dm_window* window = context->window;

    bool move_x = window->current_input.mouse.x != window->previous_input.mouse.x;
    bool move_y = window->current_input.mouse.y != window->previous_input.mouse.y;
    
    return move_x || move_y;
}

bool dm_input_is_key_pressed(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.keyboard.keys[key];
}

bool dm_input_key_just_pressed(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.keyboard.keys[key]==1 && window->previous_input.keyboard.keys[key]==0);
}

bool dm_input_key_just_released(dm_key_code key, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.keyboard.keys[key]==0 && window->previous_input.keyboard.keys[key]==1);
}

bool dm_input_is_mouse_button_pressed(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.buttons[button];
}

bool dm_input_mouse_button_just_pressed(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.mouse.buttons[button]==1 && window->previous_input.mouse.buttons[button]==0);
}

bool dm_input_mouse_button_just_released(dm_mousebutton_code button, dm_context* context)
{
    dm_window* window = context->window;

    return (window->current_input.mouse.buttons[button]==0 && window->previous_input.mouse.buttons[button]==1);
}

uint16_t dm_input_get_mouse_pos_x(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.x;
}

uint16_t dm_input_get_mouse_pos_y(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.y;
}

void dm_input_get_mouse_pos(uint16_t* x, uint16_t* y, dm_context* context)
{
    *x = dm_input_get_mouse_pos_x(context);
    *y = dm_input_get_mouse_pos_y(context);
}

int dm_input_get_mouse_delta_x(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.x - window->previous_input.mouse.x;
}

int dm_input_get_mouse_delta_y(dm_context* context)
{
    dm_window* window = context->window;

    return window->current_input.mouse.y - window->previous_input.mouse.y;
}

void dm_input_get_mouse_delta(int* x, int* y, dm_context* context)
{
    *x = dm_input_get_mouse_delta_x(context);
    *y = dm_input_get_mouse_delta_y(context);
}

/*****************
* RENDERING IMPL *
******************/
#ifdef DM_DIRECTX12
#include <windows.h>

#define COBJMACROS
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#undef COBJMACROS

typedef struct dm_dx12_cpu_heap_handle_t
{
    D3D12_CPU_DESCRIPTOR_HANDLE begin;
    D3D12_CPU_DESCRIPTOR_HANDLE current;
} dm_dx12_cpu_heap_handle;

typedef struct dm_dx12_gpu_heap_handle_t
{
    D3D12_GPU_DESCRIPTOR_HANDLE begin;
    D3D12_GPU_DESCRIPTOR_HANDLE current;
} dm_dx12_gpu_heap_handle;

typedef struct dm_dx12_descriptor_heap_t
{
    ID3D12DescriptorHeap* heap;
    size_t                size, count;
    
    dm_dx12_cpu_heap_handle cpu_handle;
    dm_dx12_gpu_heap_handle gpu_handle;
} dm_dx12_descriptor_heap;

typedef struct dm_dx12_fence_t
{
    ID3D12Fence* fence;
    uint64_t     value;
} dm_dx12_fence;

typedef struct dm_dx12_raster_pipeline_t
{
    ID3D12PipelineState* state;

    D3D_PRIMITIVE_TOPOLOGY topology;
    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor;
} dm_dx12_raster_pipeline;

typedef struct dm_dx12_compute_pipeline_t
{
    ID3D12PipelineState* state;
} dm_dx12_compute_pipeline;

typedef uint16_t dm_dx12_resource_index;

typedef struct dm_dx12_resource_t
{
    dm_dx12_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_resource_index device[DM_MAX_FRAMES_IN_FLIGHT];

    uint32_t descriptor_index[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_resource;

typedef struct dm_dx12_vertex_buffer_t
{
    dm_dx12_resource resource;

    D3D12_VERTEX_BUFFER_VIEW views[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_vertex_buffer;

typedef struct dm_dx12_index_buffer_t
{
    dm_dx12_resource resource;

    DXGI_FORMAT             index_format;
    D3D12_INDEX_BUFFER_VIEW views[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_index_buffer;

typedef struct dm_dx12_constant_buffer_t
{
    dm_dx12_resource resource;

    size_t                      size, big_size;
    void*                       mapped_addresses[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_constant_buffer;

typedef struct dm_dx12_texture_t
{
    dm_dx12_resource resource;

    DXGI_FORMAT format;
    uint8_t     bytes_per_channel, n_channels;
} dm_dx12_texture;

typedef struct dm_dx12_storage_buffer_t
{
    dm_dx12_resource resource;

    size_t                      size;
} dm_dx12_storage_buffer;

#ifdef DM_HARDWARE_RAYTRACING
typedef struct dm_dx12_sbt_t
{
    dm_dx12_resource_index index[DM_MAX_FRAMES_IN_FLIGHT];
    size_t  stride, size;
    size_t  offset[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_sbt;

typedef struct dm_dx12_raytracing_pipeline_t
{
    ID3D12StateObject*   pso;
    dm_dx12_sbt          raygen_sbt, miss_sbt, hitgroup_sbt;
} dm_dx12_raytracing_pipeline;

typedef struct dm_dx12_as_t
{
    dm_dx12_resource_index result;
    dm_dx12_resource_index scratch;
    size_t  scratch_size;
} dm_dx12_as;

typedef struct dm_dx12_blas_t
{
    dm_dx12_as as[DM_MAX_FRAMES_IN_FLIGHT];
} dm_dx12_blas;

typedef struct dm_dx12_tlas_t
{
    dm_dx12_as as[DM_MAX_FRAMES_IN_FLIGHT];

    dm_resource_handle instance_buffer;
} dm_dx12_tlas;

#define DM_DX12_MAX_RESOURCES ((2 + 3 + (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS + DM_MAX_TEXTURES + DM_MAX_BLAS + DM_MAX_TLAS) * 2) * DM_MAX_FRAMES_IN_FLIGHT)
#else
#define DM_DX12_MAX_RESOURCES ((2 + 3 + (DM_MAX_VBS + DM_MAX_IBS + DM_MAX_CBS + DM_MAX_SBS + DM_MAX_TEXTURES) * 2) * DM_MAX_FRAMES_IN_FLIGHT)
#endif // DM_HARDWARE_RAYTRACING   

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

typedef struct dm_metal_texture_t
{
    dm_resource_index host[DM_MAX_FRAMES_IN_FLIGHT];
    dm_resource_index device[DM_MAX_FRAMES_IN_FLIGHT];
    MTLPixelFormat format;
} dm_metal_texture;

typedef struct dm_metal_index_buffer_t
{
    dm_metal_buffer buffer;
    MTLIndexType index_type;
} dm_metal_index_buffer;

typedef struct dm_metal_sampler_t
{
    id<MTLSamplerState> state;
} dm_metal_sampler;

typedef struct dm_metal_raster_pipeline_t
{
    id<MTLRenderPipelineState> pipeline_state;

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

    uint32_t vertex_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t fragment_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];

    id<MTLArgumentEncoder> vertex_encoder;
    id<MTLArgumentEncoder> fragment_encoder;
} dm_metal_raster_pipeline;

typedef struct dm_metal_heap_t
{
    id<MTLHeap> heap;
    size_t size;
} dm_metal_heap;
#elif defined(DM_VULKAN)
#endif

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
#ifdef DM_DIRECTX12
ID3D12Device5*   device;
    IDXGISwapChain4* swap_chain;

    ID3D12CommandQueue*         command_queue;
    ID3D12CommandAllocator*     command_allocator[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* command_list[DM_MAX_FRAMES_IN_FLIGHT];

    ID3D12CommandQueue*         compute_command_queue;
    ID3D12CommandAllocator*     compute_command_allocator[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12GraphicsCommandList7* compute_command_list[DM_MAX_FRAMES_IN_FLIGHT];

    dm_dx12_fence fences[DM_MAX_FRAMES_IN_FLIGHT];
    HANDLE        fence_event;

    dm_dx12_fence compute_fences[DM_MAX_FRAMES_IN_FLIGHT];
    HANDLE        compute_fence_event;

    uint32_t                render_targets[DM_MAX_FRAMES_IN_FLIGHT], depth_stencil_targets[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap rtv_heap, depth_stencil_heap;
    dm_dx12_descriptor_heap resource_heap[DM_MAX_FRAMES_IN_FLIGHT];
    dm_dx12_descriptor_heap sampler_heap[DM_MAX_FRAMES_IN_FLIGHT];
    ID3D12RootSignature*    bindless_root_signature;
    
    IDXGIFactory4* factory;
    IDXGIAdapter1* adapter;

    dm_dx12_raster_pipeline rast_pipelines[DM_MAX_RASTER_PIPES];
    uint32_t                rast_pipe_count;

    dm_dx12_compute_pipeline compute_pipelines[DM_MAX_COMPUTE_PIPES];
    uint32_t                 comp_pipe_count;

    dm_dx12_vertex_buffer vertex_buffers[DM_MAX_VBS];
    uint32_t              vb_count;

    dm_dx12_index_buffer  index_buffers[DM_MAX_IBS];
    uint32_t              ib_count;

    dm_dx12_constant_buffer constant_buffers[DM_MAX_CBS];
    uint32_t                cb_count;

    dm_dx12_texture textures[DM_MAX_TEXTURES];
    uint32_t        texture_count;

    dm_dx12_storage_buffer storage_buffers[DM_MAX_SBS];
    uint32_t               sb_count;


#ifdef DM_HARDWARE_RAYTRACING
    dm_dx12_raytracing_pipeline rt_pipelines[DM_MAX_RT_PIPES];
    uint32_t                    rt_pipe_count;

    dm_dx12_tlas tlas[DM_MAX_TLAS];
    uint32_t     tlas_count;

    dm_dx12_blas blas[DM_MAX_BLAS];
    uint32_t     blas_count;
#endif

    ID3D12Resource* resources[DM_DX12_MAX_RESOURCES];
    uint32_t        resource_count;

    dm_pipeline_type active_pipeline_type;

#ifdef DM_DEBUG
    ID3D12Debug* debug;
    IDXGIDebug1* dxgi_debug;
    IDXGIInfoQueue* info_queue;
#endif
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

    id<MTLDepthStencilState> depth_stencil_state;
    
    dispatch_semaphore_t frame_semaphore;

    dm_metal_heap resource_heap[DM_MAX_FRAMES_IN_FLIGHT];

    // regular buffers, 3 argument for raster pipes, times frames in flight
    id<MTLBuffer> buffers[(DM_MAX_BUFFERS + DM_MAX_RASTER_PIPES * 2 + 1) * DM_MAX_FRAMES_IN_FLIGHT];     
    uint32_t      buffer_count;

    // regular textures, 2 for renderpasses, depth target, time frames in flight
    id<MTLTexture> textures[(DM_MAX_TEXTURES + DM_MAX_RENDERPASS + 1) * DM_MAX_FRAMES_IN_FLIGHT];
    uint32_t       texture_count;

    dm_metal_renderpass renderpasses[DM_MAX_RENDERPASS];
    dm_metal_raster_pipeline raster_pipes[DM_MAX_RASTER_PIPES];
    dm_metal_buffer vertex_buffers[DM_MAX_VBS];
    dm_metal_index_buffer index_buffers[DM_MAX_IBS];
    dm_metal_buffer constant_buffers[DM_MAX_CBS];
    dm_metal_buffer storage_buffers[DM_MAX_SBS];
    dm_metal_texture metal_textures[10];
    dm_metal_sampler samplers[DM_MAX_SAMPLERS];

    uint32_t renderpass_count, raster_pipe_count, vb_count, ib_count, cb_count, sb_count, metal_texture_count, sampler_count;

    dm_pipeline_handle active_pipeline;
    dm_resource_handle active_index_buffer;
    uint32_t texture_argument_buffer[DM_MAX_FRAMES_IN_FLIGHT];
    id<MTLArgumentEncoder> texture_encoder;
#elif defined(DM_VULKAN)
#endif

    uint8_t current_frame;
    uint32_t width, height;

    dm_command_buffer render_commands;
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

#ifdef DM_DIRECTX12
#ifdef DM_DEBUG
void dm_dx12_get_debug_message(dm_renderer* renderer)
{
    const uint32_t num_messages = IDXGIInfoQueue_GetNumStoredMessages(renderer->info_queue, DXGI_DEBUG_ALL);
    for(uint32_t i=0; i<num_messages; i++)
    {
        size_t len = 0;
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, NULL, &len);
        DXGI_INFO_QUEUE_MESSAGE* buffer = dm_alloc(len);
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, buffer, &len);

        dm_log(DM_LOG_ERROR, "%s\n", buffer->pDescription);
        dm_free((void**)&buffer);
    }
}
#else
void dm_dx12_get_debug_message(dm_renderer* renderer) {}
#endif

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_wait_for_previous_frame(bool advance, dm_renderer* renderer)
{
    HRESULT hr;

    dm_dx12_fence* fence = &renderer->fences[renderer->current_frame];

    hr = ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value); 
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    if(advance) renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);

    fence = &renderer->fences[renderer->current_frame];

    if(ID3D12Fence_GetCompletedValue(fence->fence) < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

        WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
    }

    fence->value++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_descriptor_heap(uint32_t descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, dm_dx12_descriptor_heap* heap, dm_renderer* renderer)
{
    HRESULT hr;
    void* temp = NULL;

    D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
    desc.Type            = type;
    desc.NumDescriptors  = descriptor_count;
    desc.Flags          |= flags;

    hr = ID3D12Device5_CreateDescriptorHeap(renderer->device, &desc, &IID_ID3D12DescriptorHeap, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateDescriptorHeap failed"); return false; }
    heap->heap = temp;
    temp = NULL;

    heap->size = ID3D12Device5_GetDescriptorHandleIncrementSize(renderer->device, type);

    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap->heap, &heap->cpu_handle.begin);
    heap->cpu_handle.current = heap->cpu_handle.begin;

    if(flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap->heap, &heap->gpu_handle.begin);
        heap->gpu_handle.current = heap->gpu_handle.begin;
    }

    return true;
}
#endif

// === backend ===
bool dm_renderer_init(dm_context* context)
{
    context->renderer = dm_alloc(sizeof(dm_renderer));
    dm_renderer* renderer = context->renderer;

    dm_window* window = context->window;

    int w,h;
    RGFW_window_getSize(window->window, &w,&h);

    renderer->width = w;
    renderer->height = h;

#ifdef DM_DIRECTX12
#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing DirectX12 backend...");
#endif
     HRESULT hr;

    void* temp = NULL;

    // create device
#ifdef DM_DEBUG
    hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12GetDebugInterface failed"); return false; }
    renderer->debug = temp;
    temp = NULL;

    ID3D12Debug_EnableDebugLayer(renderer->debug);

    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "DXGIGetDebugInterface failed"); return false; }
    renderer->dxgi_debug = temp;
    temp = NULL;

    hr = IDXGIDebug1_QueryInterface(renderer->dxgi_debug, &IID_IDXGIInfoQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGIDebug1_QueryInterface failed"); return false; }
    renderer->info_queue = temp;
    temp = NULL;
#endif

    hr = CreateDXGIFactory1(&IID_IDXGIFactory4, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "CreateDXGIFactory1 failed"); return false; }
    renderer->factory = temp;
    temp = NULL;

    int adapter_index = 0;
    bool found = false;

    DXGI_ADAPTER_DESC1 adapter_desc = { 0 };
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_1;

    while(IDXGIFactory4_EnumAdapters1(renderer->factory, adapter_index, &renderer->adapter) != DXGI_ERROR_NOT_FOUND)
    {
        IDXGIAdapter1_GetDesc1(renderer->adapter, &adapter_desc);

        if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter_index++;
            continue;
        }

        hr = D3D12CreateDevice((IUnknown*)renderer->adapter, feature_level, &IID_ID3D12Device, NULL);
        if(SUCCEEDED(hr))
        {
            found = true;
            break;
        }

        adapter_index++;
    }

    if(!found) { dm_log(DM_LOG_FATAL, "No suitable DirectX12 adapter found"); return false; }

    hr = D3D12CreateDevice((IUnknown*)renderer->adapter, feature_level, &IID_ID3D12Device5, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12CreateDevice failed"); return false; }

    dm_log(DM_LOG_INFO, "Device: %ls", adapter_desc.Description);

    renderer->device = temp;
    temp = NULL;

    // command queue
    D3D12_COMMAND_QUEUE_DESC desc = { 0 };
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = ID3D12Device5_CreateCommandQueue(renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer->command_queue = temp;
    temp = NULL;

    hr = ID3D12Device5_CreateCommandQueue(renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer->compute_command_queue = temp;
    temp = NULL;

    // swap chain
    DXGI_SAMPLE_DESC sample_desc = { 0 };
    sample_desc.Count = 1;

    DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
    swap_desc.BufferCount  = DM_MAX_FRAMES_IN_FLIGHT;
    swap_desc.Width        = renderer->width;
    swap_desc.Height       = renderer->height;
    swap_desc.BufferUsage  = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.SampleDesc   = sample_desc;
    swap_desc.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;        

    HWND hwnd = GetActiveWindow();

    hr = IDXGIFactory4_CreateSwapChainForHwnd(renderer->factory, (IUnknown*)renderer->command_queue, hwnd, &swap_desc, NULL,NULL, (IDXGISwapChain1**)&renderer->swap_chain);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGIFactory4_CreateSwapChainForHwnd failed"); return false; }

    IDXGIFactory4_MakeWindowAssociation(renderer->factory, hwnd, DXGI_MWA_NO_ALT_ENTER); 

    renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);

    // command allocators and lists
    const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer->command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer->device, 0, type, renderer->command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer->command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer->command_list[i]);
    }

    // open first for initial recording
    ID3D12GraphicsCommandList7_Reset(renderer->command_list[0], renderer->command_allocator[0], NULL);

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer->compute_command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer->device, 0, type, renderer->compute_command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer->compute_command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer->compute_command_list[i]);
    }

    // rtv heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer->rtv_heap, renderer)) return false;

    // depth stencil heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer->depth_stencil_heap, renderer)) return false;

    // resource heap(s)
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer->resource_heap[i], renderer)) return false;
        if(!dm_dx12_create_descriptor_heap(DM_MAX_SAMPLERS, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer->sampler_heap[i], renderer)) return false;
    }

    // bindless root signature 
    D3D12_ROOT_PARAMETER1 root_params[1] = { 0 };
    root_params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].Constants.ShaderRegister = 0;
    root_params[0].Constants.RegisterSpace  = 0;
    root_params[0].Constants.Num32BitValues = 10;
    root_params[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = { 0 };
    root_sig_desc.Version                = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_sig_desc.Desc_1_1.Flags         = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
    root_sig_desc.Desc_1_1.NumParameters = 1;
    root_sig_desc.Desc_1_1.pParameters   = root_params;

    ID3DBlob* blob = NULL;
    hr = D3D12SerializeVersionedRootSignature(&root_sig_desc, &blob, NULL);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "D3D12SerializeVersionedRootSignature failed"); return false; }

    hr = ID3D12Device_CreateRootSignature(renderer->device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateRootSignature failed"); return false; }
    renderer->bindless_root_signature = temp;

    temp = NULL;
    ID3D10Blob_Release(blob);

    // render targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = IDXGISwapChain4_GetBuffer(renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer->resources[renderer->resource_count] = temp;
        temp = NULL;
        ID3D12Resource* rt = renderer->resources[renderer->resource_count];
        renderer->render_targets[i] = renderer->resource_count++;

        D3D12_RENDER_TARGET_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer->rtv_heap.cpu_handle.current;

        ID3D12Device5_CreateRenderTargetView(renderer->device, rt, &view_desc, *handle);

        handle->ptr += renderer->rtv_heap.size;
    }

    // depth stencil targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = renderer->width;
        resource_desc.Height           = renderer->height;
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        void* temp = NULL;
        hr = ID3D12Device5_CreateCommittedResource(renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed"); dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed"); return false; }
        renderer->resources[renderer->resource_count] = temp;
        renderer->depth_stencil_targets[i] = renderer->resource_count++;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer->depth_stencil_heap.cpu_handle.current;

        ID3D12Device5_CreateDepthStencilView(renderer->device, renderer->resources[renderer->depth_stencil_targets[i]], &view_desc, *handle);

        handle->ptr += renderer->depth_stencil_heap.size;
    }

    // fence stuff
    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer->fences[i].fence = temp;
        temp = NULL;
    }

    renderer->fences[0].value = 1;

    renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer->fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer->compute_fences[i].fence = temp;
        temp = NULL;
    }

    renderer->compute_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer->compute_fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }
#elif defined(DM_METAL)
#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing Metal backend...");
#endif
    renderer->device = MTLCreateSystemDefaultDevice();
    if(!renderer->device) { dm_log(DM_LOG_FATAL, "Could not create Metal device"); return false; }

    // check feature support
    if(renderer->device.argumentBuffersSupport!=MTLArgumentBuffersTier2) { dm_log(DM_LOG_FATAL, "ArgumentBuffersTier2 not supported"); return false; }
#ifdef DM_RAYTRACING
    if(!renderer->device.supportsRaytracing) { DM_LOG_FATAL("Device does not support ray tracing"); return false; }
#endif // DM_RAYTRACING

    // swapchain
    renderer->swapchain        = [CAMetalLayer layer];
    renderer->swapchain.device = renderer->device;
    renderer->swapchain.opaque = YES;

    renderer->command_queue = [renderer->device newCommandQueue];
    if(!renderer->command_queue) { dm_log(DM_LOG_FATAL, "newCommandQueue failed"); return false; }

    // must set the content view's layer to our metal layer
    MTKView* view = (MTKView*)RGFW_window_getView_OSX(window->window);
    [view setWantsLayer: YES];
    [view setLayer:renderer->swapchain];
    
    CGSize drawable_size = CGSizeMake(w,h);
    renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture 
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->textures[renderer->texture_count] = [renderer->device newTextureWithDescriptor:descriptor];

        renderer->depth_target[i] = renderer->texture_count++;
    }

    [descriptor release];
    
    MTLDepthStencilDescriptor* depth_descriptor = [MTLDepthStencilDescriptor new];
    depth_descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    depth_descriptor.depthWriteEnabled = YES;
    renderer->depth_stencil_state = [renderer->device newDepthStencilStateWithDescriptor:depth_descriptor];

    [depth_descriptor release];

    // command queue 
    renderer->command_queue = [renderer->device newCommandQueue];

    // synchronization 
    renderer->frame_semaphore = dispatch_semaphore_create(DM_MAX_FRAMES_IN_FLIGHT);

    
#elif defined(DM_VULKAN)
#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing Vulkan renderer...");
#endif
#endif

    return true;
}

#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_copy_buffer_to_heap(dm_metal_buffer* buffer, id<MTLBlitCommandEncoder> blit_encoder, dm_renderer* renderer)
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

bool dm_finish_init(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
    HRESULT hr;

    ID3D12CommandQueue* command_queue = renderer->command_queue;

    ID3D12CommandList*  command_lists[] = { (ID3D12CommandList*)renderer->command_list[0] };

    ID3D12GraphicsCommandList7_Close(renderer->command_list[0]);
    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    hr =ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[0].fence, renderer->fences[0].value);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    hr = ID3D12Fence_SetEventOnCompletion(renderer->fences[0].fence, renderer->fences[0].value, renderer->fence_event);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

    WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);

    renderer->fences[0].value++;
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
    for(uint32_t i=0; i<renderer->vb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->vertex_buffers[i], blit_encoder, renderer)) return false;
    }
    for(uint32_t i=0; i<renderer->ib_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->index_buffers[i].buffer, blit_encoder, renderer)) return false;
    }
    for(uint32_t i=0; i<renderer->cb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->constant_buffers[i], blit_encoder, renderer)) return false;
    }
    for(uint32_t i=0; i<renderer->sb_count; i++)
    {
        if(!dm_metal_copy_buffer_to_heap(&renderer->storage_buffers[i], blit_encoder, renderer)) return false;
    }

    for(uint32_t i=0; i<renderer->metal_texture_count; i++)
    {
        dm_metal_texture t = renderer->metal_textures[i];

        for(uint8_t j=0; j<DM_MAX_FRAMES_IN_FLIGHT; j++)
        {
            MTLTextureDescriptor* desc = [MTLTextureDescriptor new];

            id<MTLTexture> host_texture = renderer->textures[t.host[j]];

            desc.width  = host_texture.width;
            desc.height = host_texture.height;
            desc.pixelFormat = t.format;
            desc.storageMode = renderer->resource_heap[j].heap.storageMode;

            id<MTLTexture> heap_texture = [renderer->resource_heap[j].heap newTextureWithDescriptor:desc];
            if(!heap_texture) { dm_log(DM_LOG_FATAL, "newTextureWithDescriptor failed"); return false; }

            MTLRegion region = MTLRegionMake2D(0,0, host_texture.width,host_texture.height);

            [blit_encoder copyFromTexture:host_texture sourceSlice:0 sourceLevel:0 sourceOrigin:region.origin sourceSize:region.size toTexture:heap_texture destinationSlice:0 destinationLevel:0 destinationOrigin:region.origin];

            [desc release];

            renderer->textures[renderer->texture_count] = heap_texture;
            renderer->metal_textures[i].device[j] = renderer->texture_count++;
        }
    }

    // argument encoder
    MTLArgumentDescriptor* descriptors[DM_MAX_TEXTURES + DM_MAX_SAMPLERS];

    uint32_t index = 0;
    for(uint32_t i=0; i<DM_MAX_TEXTURES; i++)
    {
        descriptors[index] = [MTLArgumentDescriptor new];
        descriptors[index].dataType    = MTLDataTypeTexture;
        descriptors[index].textureType = MTLTextureType2D;
        descriptors[index].index       = index;
        index++;
    }
    for(uint32_t i=0; i<DM_MAX_SAMPLERS; i++)
    {
        descriptors[index] = [MTLArgumentDescriptor new];
        descriptors[index].dataType = MTLDataTypeSampler;
        descriptors[index].index    = index;
        index++;
    }

    NSArray* argument_array = [NSArray arrayWithObjects:descriptors count:(DM_MAX_TEXTURES + DM_MAX_SAMPLERS)];
    renderer->texture_encoder = [renderer->device newArgumentEncoderWithArguments:argument_array];

    size_t size = renderer->texture_encoder.encodedLength;
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        id<MTLBuffer> buffer = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
        renderer->buffers[renderer->buffer_count] = buffer;
        renderer->texture_argument_buffer[i] = renderer->buffer_count++;
    }

    for(uint32_t i=0; i<DM_MAX_SAMPLERS+DM_MAX_TEXTURES; i++)
    {
        [descriptors[i] release];
    }

    [blit_encoder endEncoding];
    [command_buffer commit];
#elif defined(DM_VULKAN)
#endif

    return true;
}

void dm_renderer_shutdown(dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
    HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->current_frame = i;
        ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[i].fence, renderer->fences[i].value);
        if(!dm_dx12_wait_for_previous_frame(false, renderer)) { dm_log(DM_LOG_ERROR, "Waiting for previous frame failed"); continue; }
    }

    for(uint32_t i=0; i<renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_MAX_FRAMES_IN_FLIGHT; j++)
        {
            ID3D12Resource_Unmap(renderer->resources[renderer->constant_buffers[i].resource.device[j]], 0,0);
            renderer->constant_buffers[i].mapped_addresses[j] = NULL;
        }
    }

    for(uint32_t i=0; i<renderer->resource_count; i++)
    {
        ID3D12Resource_Release(renderer->resources[i]);
    }

    for(uint32_t i=0; i<renderer->rast_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rast_pipelines[i].state);
    }

#ifdef DM_HARDWARE_RAYTRACING
    for(uint32_t i=0; i<renderer->rt_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rt_pipelines[i].pso);
    }
#endif

    for(uint32_t i=0; i<renderer->comp_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->compute_pipelines[i].state);
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12GraphicsCommandList7_Release(renderer->compute_command_list[i]);
        ID3D12CommandAllocator_Release(renderer->compute_command_allocator[i]);
        ID3D12Fence_Release(renderer->compute_fences[i].fence);

        ID3D12GraphicsCommandList7_Release(renderer->command_list[i]);
        ID3D12CommandAllocator_Release(renderer->command_allocator[i]);
        ID3D12Fence_Release(renderer->fences[i].fence);

        ID3D12DescriptorHeap_Release(renderer->resource_heap[i].heap);
        ID3D12DescriptorHeap_Release(renderer->sampler_heap[i].heap);
    }

    CloseHandle(renderer->compute_fence_event);
    CloseHandle(renderer->fence_event);

    ID3D12RootSignature_Release(renderer->bindless_root_signature);
    ID3D12DescriptorHeap_Release(renderer->rtv_heap.heap);
    ID3D12DescriptorHeap_Release(renderer->depth_stencil_heap.heap);
    IDXGISwapChain4_Release(renderer->swap_chain);
    ID3D12CommandQueue_Release(renderer->compute_command_queue);
    ID3D12CommandQueue_Release(renderer->command_queue);
    ID3D12Device5_Release(renderer->device);

#ifdef DM_DEBUG
    IDXGIDebug1* dbg = NULL;
    void* temp = NULL;
    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_ERROR, "ID3D12Debug_QueryInterface failed"); ID3D12Debug_Release(renderer->debug); }
    else
    {
        dbg = temp;
        temp = NULL;
        ID3D12Debug_Release(renderer->debug);
        IDXGIDebug1_ReportLiveObjects(dbg, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        IDXGIDebug1_Release(dbg);
    }
#endif // DM_DEBUG
#elif defined(DM_METAL)
    for(uint32_t i=0; i<renderer->raster_pipe_count; i++)
    {
        [renderer->raster_pipes[i].fragment_encoder release];
        [renderer->raster_pipes[i].vertex_encoder release];

        [renderer->raster_pipes[i].fragment_func release];
        [renderer->raster_pipes[i].vertex_func release];

        [renderer->raster_pipes[i].pipeline_state release];
    }

    for(uint32_t i=0; i<renderer->sampler_count; i++)
    {
        [renderer->samplers[i].state release];
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

    [renderer->texture_encoder release];
    [renderer->depth_stencil_state release];
    [renderer->swapchain release];
    [renderer->command_queue release];
    [renderer->device release];
#elif defined(DM_VULKAN)
#endif
}

bool dm_renderer_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    renderer->width  = width;
    renderer->height = height;

#ifdef DM_DIRECTX12
     HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_dx12_fence* fence = &renderer->fences[i];
        size_t fence_value = fence->value++;
        ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value);
        if(ID3D12Fence_GetCompletedValue(fence->fence) < fence_value)
        {
            ID3D12Fence_SetEventOnCompletion(fence->fence, fence_value, renderer->fence_event);
            WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
        }
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(renderer->resources[renderer->render_targets[i]]);
        ID3D12Resource_Release(renderer->resources[renderer->depth_stencil_targets[i]]);
    }

    hr = IDXGISwapChain4_ResizeBuffers(renderer->swap_chain, DM_MAX_FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_ResizeBuffers failed"); return false; }

    void* temp = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle = renderer->rtv_heap.cpu_handle.begin;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_target_handle  = renderer->depth_stencil_heap.cpu_handle.begin;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* render_target = renderer->resources[renderer->render_targets[i]];

        hr = IDXGISwapChain4_GetBuffer(renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer->resources[renderer->render_targets[i]] = temp;
        temp = NULL;

        D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        ID3D12Device_CreateRenderTargetView(renderer->device, renderer->resources[renderer->render_targets[i]], &desc, render_target_handle);
        render_target_handle.ptr += renderer->rtv_heap.size;

        // depth stencil
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        hr = ID3D12Device5_CreateCommittedResource(renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed");
            dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed");
            return false;
        }
        renderer->resources[renderer->depth_stencil_targets[i]] = temp;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        ID3D12Device5_CreateDepthStencilView(renderer->device, renderer->resources[renderer->depth_stencil_targets[i]], &view_desc, depth_target_handle);
        depth_target_handle.ptr += renderer->depth_stencil_heap.size;
    }

    renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);
#elif defined(DM_METAL)
    //CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    //renderer->swapchain.contentsScale = scale;
    //CGSize drawable_size;
    //drawable_size.width  = renderer->width;
    //drawable_size.height = renderer->height;
    renderer->swapchain.drawableSize = CGSizeMake(width, height);
    
    // depth texture
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        [renderer->textures[renderer->depth_target[i]] release];
        renderer->textures[renderer->depth_target[i]] = [renderer->device newTextureWithDescriptor:descriptor];
    }
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_begin_frame(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
    HRESULT hr;

    const uint8_t current_frame = renderer->current_frame;

    ID3D12CommandAllocator*     command_allocator = renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    hr = ID3D12CommandAllocator_Reset(command_allocator);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandAllocator_Reset failed"); return false; }

    hr = ID3D12GraphicsCommandList7_Reset(command_list, command_allocator, NULL);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12GraphicsCommandList7_Reset failed"); return false; }

    ID3D12DescriptorHeap* heaps[] = { renderer->resource_heap[current_frame].heap, renderer->sampler_heap[current_frame].heap };
    ID3D12GraphicsCommandList7_SetDescriptorHeaps(command_list, DM_COUNTOF(heaps), heaps);
    ID3D12GraphicsCommandList7_SetGraphicsRootSignature(command_list, renderer->bindless_root_signature);
    ID3D12GraphicsCommandList7_SetComputeRootSignature(command_list, renderer->bindless_root_signature);

    renderer->active_pipeline_type = DM_PIPELINE_TYPE_INVALID;
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

#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_end_frame(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
    HRESULT hr;

    ID3D12CommandQueue*         command_queue = renderer->command_queue;
    ID3D12CommandAllocator*     command_allocator = renderer->command_allocator[current_frame];
    ID3D12GraphicsCommandList7* command_list = renderer->command_list[current_frame];

    hr = ID3D12GraphicsCommandList7_Close(command_list);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12GraphicsCommandList7_Close failed"); return false; }

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)command_list };

    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    UINT present_flag = 0;
    present_flag = DXGI_PRESENT_ALLOW_TEARING;

    bool vsync = context->flags & DM_CONTEXT_FLAG_VSYNC_ON ? 1 : 0;
    hr = IDXGISwapChain4_Present(renderer->swap_chain, vsync, present_flag);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_Present failed"); return false; }

    if(!dm_dx12_wait_for_previous_frame(true, renderer)) { dm_log(DM_LOG_FATAL, "Waiting for previous frame failed"); return false; }
#elif defined(DM_METAL)
    // present
    renderer->swapchain.displaySyncEnabled = context->flags & DM_RENDERER_FLAG_VSYNC_ON ? YES : NO;
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
#elif defined(DM_VULKAN)
#endif

    return true;
}

/*************
 * RESOURCES *
 *************/
bool dm_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_context* context)
{
    dm_renderer* renderer =  context->renderer;

#ifdef DM_DIRECTX12 
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

    renderer->renderpasses[renderer->renderpass_count] = renderpass;
    *handle = renderer->renderpass_count++;
#elif defined(DM_VULKAN)
#endif

    return true;
}

#ifdef DM_DIRECTX12
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_load_shader_data(const char* path, ID3D10Blob** blob)
{
    HRESULT hr;
    wchar_t ws[512];
    swprintf(ws, 512, L"%hs", path);
    hr = D3DReadFileToBlob(ws, blob);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        dm_log(DM_LOG_FATAL, "D3DReadFileToBlob failed");
        dm_log(DM_LOG_ERROR, "Could not load shader: %s", path);
        return false;
    }

    return true;
}
#elif defined(DM_METAL)
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

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_metal_create_argument_buffer(uint32_t* index, id<MTLArgumentEncoder> encoder, dm_renderer* renderer)
{
    size_t size = encoder.encodedLength;
    id<MTLBuffer> buffer = [renderer->device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
    if(!buffer) { dm_log(DM_LOG_FATAL, "newBufferWithLength failed"); return false; }

    renderer->buffers[renderer->buffer_count] = buffer;
    *index = renderer->buffer_count++;

    return true;
}
#endif // DM_METAL

bool dm_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    void* temp = NULL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // === rasterizer ===
    switch(desc.rasterizer.cull_mode)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown cull mode. Assuming D3D12_CULL_MODE_BACK");
        case DM_RASTERIZER_CULL_MODE_BACK:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        break;

        case DM_RASTERIZER_CULL_MODE_FRONT:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        break;

        case DM_RASTERIZER_CULL_MODE_NONE:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        break;
    }

    switch(desc.rasterizer.polygon_fill)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown polygon fill mode. Assuming D3D12_FILL_MODE_SOLID");
        case DM_RASTERIZER_POLYGON_FILL_FILL:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        break;

        case DM_RASTERIZER_POLYGON_FILL_WIREFRAME:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    }

    switch(desc.rasterizer.front_face)
    {
        case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        break;

        default:
        dm_log(DM_LOG_ERROR, "Unknown front face. Assuming counter clockwise");
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = TRUE;
        break;
    }

    // TODO: needs to be configurable
    // === blending ===
    D3D12_RENDER_TARGET_BLEND_DESC blend_desc = { 0 };
    
    blend_desc.BlendEnable           = TRUE;
    blend_desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOp               = D3D12_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha         = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.LogicOpEnable         = FALSE;
    blend_desc.LogicOp               = D3D12_LOGIC_OP_NOOP;

    // TODO: needs to be configurable
    // === depth/stencil ===
    D3D12_DEPTH_STENCIL_DESC depth_desc = { 0 };

    depth_desc.DepthEnable      = desc.depth_stencil.depth ? TRUE : FALSE;
    depth_desc.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
    depth_desc.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_desc.StencilEnable    = FALSE;
    depth_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_desc.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;

    // === shaders ===
    ID3DBlob* vs = NULL;
    ID3DBlob* ps = NULL;
    
    const char* vertex_path = desc.rasterizer.vertex_shader_desc.path;
    const char* pixel_path  = desc.rasterizer.pixel_shader_desc.path;

    if(!dm_dx12_load_shader_data(vertex_path, &vs)) { dm_log(DM_LOG_ERROR, "Could not load vertex shader"); return false; }
    if(!dm_dx12_load_shader_data(pixel_path,  &ps)) { dm_log(DM_LOG_ERROR, "Could not load pixel shader"); return false; }
    
    pso_desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vs);
    pso_desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(vs);

    pso_desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(ps);
    pso_desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(ps);

    // === input assembler ===
    D3D12_INPUT_ELEMENT_DESC input_element_descs[DM_RENDER_MAX_INPUT_ELEMENTS] = { 0 };
    uint8_t element_index = 0;

    for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
    {
        uint8_t matrix_count = desc.input_assembler.input_elements[i].format == DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4 ? 4 : 1;

        for(uint8_t j=0; j<matrix_count; j++)
        {
            input_element_descs[element_index].SemanticName      = desc.input_assembler.input_elements[i].name; 
            input_element_descs[element_index].SemanticIndex     = j;
            input_element_descs[element_index].InputSlot         = desc.input_assembler.input_elements[i].slot;
            input_element_descs[element_index].AlignedByteOffset = i==0 ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;

            switch(desc.input_assembler.input_elements[i].format)
            {
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32_FLOAT;
                break;

                default:
                dm_log(DM_LOG_ERROR, "Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;

                case DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4:
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            }

            switch(desc.input_assembler.input_elements[i].class)
            {
                default:
                dm_log(DM_LOG_ERROR, "Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
                case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
                input_element_descs[element_index].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                break;

                case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
                input_element_descs[element_index].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                input_element_descs[element_index].InstanceDataStepRate = 1;
                input_element_descs[element_index].InputSlot            = 1;
                break;
            }

            element_index++;
        }
    }

    pso_desc.InputLayout.pInputElementDescs = input_element_descs;
    pso_desc.InputLayout.NumElements        = element_index;

    switch(desc.input_assembler.topology)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknow primitive topology. Assuming D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST");
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    }

    // === viewport and scissor ===
    switch(desc.viewport.type)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown viewport type. Assumin DM_VIEWPORT_TYPE_DEFAULT");
        case DM_VIEWPORT_TYPE_DEFAULT:
        pipeline.viewport.Width    = (float)renderer->width;
        pipeline.viewport.Height   = (float)renderer->height;
        pipeline.viewport.MaxDepth = 1.f;
        break;

        case DM_VIEWPORT_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom viewport for dx12 pipeline not supported yet");
        return false;
    }

    switch(desc.scissor.type)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown scissor type. Assuming DM_SCISSOR_TYPE_DEFAULT");
        case DM_SCISSOR_TYPE_DEFAULT:
        pipeline.scissor.right  = (float)renderer->width;
        pipeline.scissor.bottom = (float)renderer->height;
        break;

        case DM_SCISSOR_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom scissor for dx12 not supported yet");
        return false;
    }
    
    // === pipeline state ===
    pso_desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count           = 1;
    pso_desc.SampleMask                 = 0xffffffff;
    pso_desc.NumRenderTargets           = 1;
    pso_desc.BlendState.RenderTarget[0] = blend_desc;
    pso_desc.DepthStencilState          = depth_desc;
    pso_desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
    pso_desc.pRootSignature             = renderer->bindless_root_signature;

    hr = ID3D12Device5_CreateGraphicsPipelineState(renderer->device, &pso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        dm_log(DM_LOG_FATAL, "ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(renderer);
        return false;
    }
    pipeline.state = temp;
    temp = NULL;
    
    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);

    // 
    renderer->rast_pipelines[renderer->rast_pipe_count] = pipeline;
    handle->index = renderer->rast_pipe_count++;
#elif defined(DM_METAL)
    dm_metal_raster_pipeline pipeline = { 0 };

    // shaders 
    if(!dm_metal_create_shader(desc.rasterizer.vertex_shader_desc.path, &pipeline.vertex_library, renderer->device))      return false;
    if(!dm_metal_create_shader_function("vertex_main", &pipeline.vertex_func, pipeline.vertex_library, renderer->device)) return false;

    if(!dm_metal_create_shader(desc.rasterizer.pixel_shader_desc.path, &pipeline.fragment_library, renderer->device))           return false;
    if(!dm_metal_create_shader_function("fragment_main", &pipeline.fragment_func, pipeline.fragment_library, renderer->device)) return false;

    // pipeline state
    MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
    pipe_desc.rasterSampleCount = 1;

    pipe_desc.vertexFunction   = pipeline.vertex_func;
    pipe_desc.fragmentFunction = pipeline.fragment_func;

    //pipe_desc.colorAttachments[0].pixelFormat                 = MTLPixelFormatRGBA8Unorm;
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
    pipeline.fragment_encoder = [pipeline.fragment_func newArgumentEncoderWithBufferIndex:1];
    if(!pipeline.fragment_encoder) { dm_log(DM_LOG_FATAL, "newArgumentEncoderWithBufferIndex failed"); return false; }

    // argument buffer
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_metal_create_argument_buffer(&pipeline.vertex_argument_buffer[i], pipeline.vertex_encoder, renderer)) return false;
        if(!dm_metal_create_argument_buffer(&pipeline.fragment_argument_buffer[i], pipeline.fragment_encoder, renderer)) return false;
    }

    //
    renderer->raster_pipes[renderer->raster_pipe_count] = pipeline;
    handle->index = renderer->raster_pipe_count++;
#elif defined(DM_VULKAN)
#endif

    handle->type = DM_PIPELINE_TYPE_RASTER;

    return true;
}

#ifdef DM_DIRECTX12
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_committed_resource(D3D12_HEAP_PROPERTIES properties, D3D12_RESOURCE_DESC desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, ID3D12Device5* device)
{
    void* temp = NULL;
    HRESULT hr = ID3D12Device5_CreateCommittedResource(device, &properties, flags, &desc, state, 0, &IID_ID3D12Resource, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommittedResource failed"); return false; }

    *resource = temp;
    temp = NULL;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_copy_memory(ID3D12Resource* resource, const void* data, size_t size)
{
    void* temp = NULL;

    D3D12_RANGE range = { 0 };
    range.End = size-1;
    
    ID3D12Resource_Map(resource, 0, &range, &temp);
    if(!temp) { dm_log(DM_LOG_FATAL, "ID3D12Resource_Map failed"); return false; }

    dm_memcpy(temp, data, size);
    ID3D12Resource_Unmap(resource, 0, &range);
    
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_buffer(const size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Resource** resource, dm_renderer* renderer)
{
    D3D12_RESOURCE_DESC desc = { 0 };
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width            = size;
    desc.Height           = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags            = flags;

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, renderer->device)) return false;  

    return true;
}
#elif defined(DM_METAL)
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

bool dm_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
    dm_dx12_vertex_buffer buffer = { 0 };

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "vertex_host_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "vertex_device_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;

        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].StrideInBytes  = desc.stride;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.NumElements         = desc.size / desc.stride;
        view_desc.Buffer.StructureByteStride = desc.stride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    } 

    //
    renderer->vertex_buffers[renderer->vb_count] = buffer;
    handle->index = renderer->vb_count++;
#elif defined(DM_METAL)
    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, renderer)) return false;

    renderer->vertex_buffers[renderer->vb_count] = buffer;
    handle->index = renderer->vb_count++;
#elif defined(DM_VULKAN)
#endif

    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    return true;
}

bool dm_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
    dm_dx12_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_format = DXGI_FORMAT_R16_UINT;
        break;

        default:
        dm_log(DM_LOG_ERROR, "Unknown index type. Assuming uint32");
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_format = DXGI_FORMAT_R32_UINT;
        break;
    }

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "index_host_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "index_device_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;
        
        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].Format         = buffer.index_format;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        switch(desc.index_type)
        {
            case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
            view_desc.Buffer.StructureByteStride = sizeof(uint16_t);
            break;

            case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
            view_desc.Buffer.StructureByteStride = sizeof(uint32_t);
            break;

            default:
            dm_log(DM_LOG_FATAL, "Should NOT be here");
            return false;
        }
        view_desc.Buffer.NumElements         = desc.size / view_desc.Buffer.StructureByteStride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    }

    //
    renderer->index_buffers[renderer->ib_count] = buffer;
    handle->index = renderer->ib_count++;
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
#elif defined(DM_VULKAN)
#endif
    
    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    return true;
}

bool dm_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, renderer)) return false;

    renderer->constant_buffers[renderer->cb_count] = buffer;
    handle->index = renderer->cb_count++;
#elif defined(DM_VULKAN)
#endif

    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    return true;
}

bool dm_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_buffer buffer = { 0 };
    
    if(!dm_metal_create_buffer(desc.data, desc.size, &buffer, renderer)) return false;

    renderer->storage_buffers[renderer->sb_count] = buffer;
    handle->index = renderer->sb_count++;
#elif defined(DM_VULKAN)
#endif

    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    return true;
}

bool dm_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_texture texture = { 0 };

    size_t bytes_per_pixel = 0;
    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        texture.format = MTLPixelFormatRGBA8Unorm;
        bytes_per_pixel = desc.n_channels * 1;
        break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported texture format for Metal");
        return false;
    }

    MTLTextureDescriptor* texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:texture.format width:desc.width height:desc.height mipmapped:NO];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        id<MTLTexture> t = [renderer->device newTextureWithDescriptor:texture_desc];
        
        MTLRegion region = MTLRegionMake2D(0,0, desc.width,desc.height);
        
        [t replaceRegion:region mipmapLevel:0 withBytes:desc.data bytesPerRow:(bytes_per_pixel * desc.width)];

        renderer->textures[renderer->texture_count] = t;
        texture.host[i] = renderer->texture_count++;

        //
        MTLSizeAndAlign size_and_align = [renderer->device heapTextureSizeAndAlignWithDescriptor:texture_desc];
        size_and_align.size += (size_and_align.size & (size_and_align.align - 1)) + size_and_align.align;
        renderer->resource_heap[i].size += size_and_align.size;
    }

    //
    renderer->metal_textures[renderer->metal_texture_count] = texture;
    handle->index = renderer->metal_texture_count++;
#elif defined(DM_VULKAN)
#endif

    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    return true;
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

#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif // DM_DEBUG
MTLSamplerAddressMode dm_convert_sampler_address(dm_sampler_address_mode mode)
{
    switch(mode)
    {
        default:
        case DM_SAMPLER_ADDRESS_MODE_WRAP:   return MTLSamplerAddressModeRepeat;
        case DM_SAMPLER_ADDRESS_MODE_BORDER: return MTLSamplerAddressModeClampToEdge;
    }
}
#endif // DM_METAL

bool dm_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_sampler sampler = { 0 };

    MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];

    sampler_desc.rAddressMode = dm_convert_sampler_address(desc.address_u);
    sampler_desc.sAddressMode = dm_convert_sampler_address(desc.address_v);
    sampler_desc.tAddressMode = dm_convert_sampler_address(desc.address_w);

    sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
    sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;

    sampler_desc.supportArgumentBuffers = YES;

    sampler.state = [renderer->device newSamplerStateWithDescriptor:sampler_desc];

    [sampler_desc release];
    
    //
    renderer->samplers[renderer->sampler_count] = sampler;
    handle->index = renderer->sampler_count++;
#elif defined(DM_VULKAN)
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
#elif defined(DM_METAL)
    renderer->render_blit_encoder[current_frame] = [renderer->render_command_buffer[current_frame] blitCommandEncoder];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_end_update_backend(dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    [renderer->render_blit_encoder[current_frame] endEncoding];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
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

    // depth state
    [encoder setDepthStencilState:renderer->depth_stencil_state];

    // argument buffer
    [renderer->texture_encoder setArgumentBuffer:renderer->buffers[renderer->texture_argument_buffer[current_frame]] offset:0];

    for(uint32_t i=0; i<renderer->metal_texture_count; i++)
    {
        [renderer->texture_encoder setTexture:renderer->textures[renderer->metal_textures[i].device[current_frame]] atIndex:i];
    }
    for(uint32_t i=0; i<renderer->sampler_count; i++)
    {
        [renderer->texture_encoder setSamplerState:renderer->samplers[i].state atIndex:(DM_MAX_TEXTURES + i)];
    }

    [encoder setFragmentBuffer:renderer->buffers[renderer->texture_argument_buffer[current_frame]] offset:0 atIndex:0];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    [renderer->render_command_encoder[renderer->current_frame] endEncoding];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];
    dm_metal_raster_pipeline* pipeline = &renderer->raster_pipes[handle.index];

    pipeline->viewport.width  = renderer->swapchain.drawableSize.width;
    pipeline->viewport.height = renderer->swapchain.drawableSize.height;
    pipeline->viewport.zfar   = 1.f;

    pipeline->scissor.x = 0;
    pipeline->scissor.y = 0;
    pipeline->scissor.width  = renderer->swapchain.drawableSize.width;
    pipeline->scissor.height = renderer->swapchain.drawableSize.height;

    [encoder setRenderPipelineState:pipeline->pipeline_state];
    [encoder setFrontFacingWinding:pipeline->winding];
    [encoder setCullMode:pipeline->cull_mode];
    [encoder setViewport:pipeline->viewport];
    [encoder setScissorRect:pipeline->scissor];
    [encoder setTriangleFillMode:pipeline->fill_mode];

    renderer->active_pipeline = handle;
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    if(renderer->active_pipeline.type==DM_PIPELINE_TYPE_INVALID) { dm_log(DM_LOG_FATAL, "No pipeline is set"); return false; }

    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    switch(renderer->active_pipeline.type)
    { 
        case DM_PIPELINE_TYPE_RASTER:
        {
            dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];

            id<MTLBuffer> vertex_argument_buffer   = renderer->buffers[pipeline.vertex_argument_buffer[current_frame]];
            id<MTLBuffer> fragment_argument_buffer = renderer->buffers[pipeline.fragment_argument_buffer[current_frame]];

            [pipeline.vertex_encoder setArgumentBuffer:vertex_argument_buffer offset:0];
            [pipeline.fragment_encoder setArgumentBuffer:fragment_argument_buffer offset:0];

            uint8_t index = 0;

            // set up user argument buffers
            for(uint16_t i=0; i<count; i++)
            {
                switch(handles[i].type)
                {
                    case DM_RESOURCE_TYPE_CONSTANT_BUFFER:
                    {
                        id<MTLBuffer> buffer = renderer->buffers[renderer->constant_buffers[handles[i].index].device[current_frame]];

                        // TODO: FIX!
                        [pipeline.vertex_encoder setBuffer:buffer offset:0 atIndex:index];
                        [pipeline.fragment_encoder setBuffer:buffer offset:0 atIndex:index++];
                    } break;
                    case DM_RESOURCE_TYPE_STORAGE_BUFFER:
                    {
                        id<MTLBuffer> buffer = renderer->buffers[renderer->storage_buffers[handles[i].index].device[current_frame]];

                        // TODO: FIX!
                        [pipeline.vertex_encoder setBuffer:buffer offset:0 atIndex:index];
                        [pipeline.fragment_encoder setBuffer:buffer offset:0 atIndex:index++];
                    } break;

                    case DM_RESOURCE_TYPE_TEXTURE:
                    {
                        id<MTLTexture> texture = renderer->textures[renderer->metal_textures[handles[i].index].device[current_frame]];

                        [pipeline.fragment_encoder setTexture:texture atIndex:index++];
                    } break;
                    case DM_RESOURCE_TYPE_SAMPLER:
                    {
                        id<MTLSamplerState> sampler = renderer->samplers[handles[i].index].state;

                        [pipeline.fragment_encoder setSamplerState:sampler atIndex:index++];
                    } break;

                    default:
                    dm_log(DM_LOG_FATAL, "Unknown/unsupported resource type");
                    return false;
                }
            }

            [encoder setVertexBuffer:vertex_argument_buffer offset:0 atIndex:0];
            [encoder setFragmentBuffer:fragment_argument_buffer offset:0 atIndex:1];
        } break;

        default:
        dm_log(DM_LOG_FATAL, "Unknown or unsupported pipeline type");
        return false;
    }
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    dm_metal_buffer buffer = renderer->vertex_buffers[handle.index];

    id<MTLBuffer> vertex_buffer = renderer->buffers[buffer.device[current_frame]];

    // always have an argument buffer being bound
    // vertex buffer must be second buffer
    // TODO: theoretically could be more than second buffer. should have an increasing slot
    [encoder setVertexBuffer:vertex_buffer offset:offset atIndex:(slot+1)];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    renderer->active_index_buffer = handle;
#elif defined(DM_VULKAN)
#endif

    return true;
}

#ifdef DM_METAL
#ifndef DM_DEBUG
DM_INLINE
#endif
void dm_metal_update_buffer(void* data, size_t size, size_t offset, dm_metal_buffer buffer, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

    id<MTLBlitCommandEncoder> blit_encoder = renderer->render_blit_encoder[current_frame];

    dm_memcpy(renderer->buffers[buffer.host[current_frame]].contents + offset, data, size);

    id<MTLBuffer> host_buffer   = renderer->buffers[buffer.host[current_frame]];
    id<MTLBuffer> device_buffer = renderer->buffers[buffer.device[current_frame]];

    [blit_encoder copyFromBuffer:host_buffer sourceOffset:offset toBuffer:device_buffer destinationOffset:offset size:size];
}
#endif // DM_METAL

bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_update_buffer(data, size, offset, renderer->vertex_buffers[handle.index], renderer);
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_update_buffer(data, size, offset, renderer->index_buffers[handle.index].buffer, renderer);
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_update_buffer(data, size, offset, renderer->constant_buffers[handle.index], renderer);
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    dm_metal_update_buffer(data, size, offset, renderer->storage_buffers[handle.index], renderer);
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_renderer* renderer)
{
#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];
    dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];

    [encoder drawPrimitives:pipeline.primitive_type vertexStart:vertex_offset vertexCount:vertex_count instanceCount:instance_count baseInstance:instance_offset];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_renderer* renderer)
{
    const uint8_t current_frame = renderer->current_frame;

#ifdef DM_DIRECTX12
#elif defined(DM_METAL)
    id<MTLRenderCommandEncoder> encoder = renderer->render_command_encoder[current_frame];

    dm_metal_raster_pipeline pipeline = renderer->raster_pipes[renderer->active_pipeline.index];
    dm_metal_index_buffer buffer = renderer->index_buffers[renderer->active_index_buffer.index];

    id<MTLBuffer> index_buffer = renderer->buffers[buffer.buffer.device[current_frame]];

    [encoder drawIndexedPrimitives:pipeline.primitive_type indexCount:index_count indexType:buffer.index_type indexBuffer:index_buffer indexBufferOffset:index_offset instanceCount:instance_count baseVertex:vertex_offset baseInstance:instance_offset];
#elif defined(DM_VULKAN)
#endif

    return true;
}

bool dm_submit_render_commands(dm_context* context)
{
    dm_renderer* renderer = context->renderer;
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

/**********
* CONTEXT *
***********/
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
    dm_window* window = context->window;
    dm_renderer* renderer = context->renderer;

    if(!dm_window_poll_events(window)) return false;

    int w,h;
    RGFW_window_getSize(window->window, &w, &h);

    if(renderer->width != w|| renderer->height != h)
    {
        if(!dm_renderer_resize(w, h, renderer)) return false;
    }

    return true;
}
