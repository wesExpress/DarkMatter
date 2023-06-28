#ifndef DM_IMPL_H
#define DM_IMPL_H

#include <string.h>
#include <math.h>
#include <time.h>

#include "mt19937/mt19937.h"
#include "mt19937/mt19937_64.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

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
    return (context->input_states[0].mouse.scroll != context->input_states[1].mouse.scroll);
}

uint32_t dm_input_get_mouse_x(dm_context* context)
{
	return context->input_states[0].mouse.x;
}

uint32_t dm_input_get_mouse_y(dm_context* context)
{
	return context->input_states[0].mouse.y;
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

int dm_input_get_mouse_delta_x(dm_context* context)
{
	return context->input_states[0].mouse.x - context->input_states[1].mouse.x;
}

int dm_input_get_mouse_delta_y(dm_context* context)
{
	return context->input_states[0].mouse.y - context->input_states[1].mouse.y;
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
	return context->input_states[0].mouse.scroll;
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
#ifdef DM_PLATFORM_WIN32
#include "dm_platform_win32.h"
#endif

/*******
LOGGING
*********/
void __dm_log_output(log_level level, const char* message, ...)
{
	static const char* log_tag[6] = { "DM_TRCE", "DM_DEBG", "DM_INFO", "DM_WARN", "DM_ERRR", "DM_FATL" };
    
    char msg_fmt[5000];
	memset(msg_fmt, 0, sizeof(msg_fmt));
    
	// ar_ptr lets us move through any variable number of arguments. 
	// start it one argument to the left of the va_args, here that is message.
	// then simply shove each of those arguments into message, which should be 
	// formatted appropriately beforehand
	va_list ar_ptr;
	va_start(ar_ptr, message);
	vsnprintf(msg_fmt, 5000, message, ar_ptr);
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
#ifdef DM_DIRECTX
#include "dm_renderer_dx11.h"
#elif defined(DM_OPENGL)
#include "dm_renderer_opengl.h"
#endif

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

void dm_renderer_destroy_buffer(dm_render_handle handle, dm_context* context)
{
    dm_renderer_backend_destroy_buffer(handle, &context->renderer);
}

#ifdef DM_OPENGL
bool dm_renderer_create_shader(const char* vertex_src, const char* pixel_src, dm_render_handle* vb_indices, uint32_t num_vb, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* handle, dm_context* context)
#else
bool dm_renderer_create_shader(const char* vertex_src, const char* pixel_src, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* handle, dm_context* context)
#endif
{
#ifdef DM_OPENGL
    if(dm_renderer_backend_create_shader(vertex_src, pixel_src, vb_indices, num_vb, attrib_descs, num_attribs, handle, &context->renderer)) return true;
#else
    if(dm_renderer_backend_create_shader(vertex_src, pixel_src, attrib_descs, num_attribs, handle, &context->renderer)) return true;
#endif
    
    DM_LOG_FATAL("Creating shader failed"); 
    return false; 
}

void dm_renderer_destroy_shader(dm_render_handle handle, dm_context* context)
{
    dm_renderer_backend_destroy_shader(handle, &context->renderer);
}

bool dm_renderer_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_uniform(size, stage, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Creating uniform failed");
    return false;
}

void dm_renderer_destroy_uniform(dm_render_handle handle, dm_context* context)
{
    dm_renderer_backend_destroy_uniform(handle, &context->renderer);
}

bool dm_renderer_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_pipeline(desc, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Creating pipeline failed");
    return false;
}

void dm_renderer_destroy_pipeline(dm_render_handle handle, dm_context* context)
{
    dm_renderer_backend_destroy_pipeline(handle, &context->renderer);
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

void dm_renderer_destroy_texture(dm_render_handle handle, dm_context* context)
{
    dm_renderer_backend_destroy_texture(handle, &context->renderer);
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
    
    unsigned char* alpha_bitmap = dm_alloc(w * h);;
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
    c->params.data = dm_alloc(command->params.size);
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

void dm_render_command_bind_uniform(dm_render_handle uniform_handle, uint32_t slot, dm_uniform_stage stage, uint32_t offset, dm_render_handle shader_handle, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_BIND_UNIFORM;
    
    DM_BYTE_POOL_INSERT(command.params, uniform_handle);
    DM_BYTE_POOL_INSERT(command.params, slot);
    DM_BYTE_POOL_INSERT(command.params, stage);
    DM_BYTE_POOL_INSERT(command.params, offset);
    DM_BYTE_POOL_INSERT(command.params, shader_handle);
    
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

void dm_render_command_update_uniform(dm_render_handle uniform_handle, void* data, size_t data_size, dm_render_handle shader_handle, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_UPDATE_UNIFORM;
    
    DM_BYTE_POOL_INSERT(command.params, uniform_handle);
    __dm_byte_pool_insert(&command.params, data, data_size);
    DM_BYTE_POOL_INSERT(command.params, data_size);
    DM_BYTE_POOL_INSERT(command.params, shader_handle);
    
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
            
#if 0
            case DM_RENDER_COMMAND_BEGIN_RENDER_PASS:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, pass_index);
                
                dm_renderpass* renderpass = DM_RENDERER_GET_RESOURCE(DM_RENDER_RESOURCE_RENDERPASS, pass_index); 
                if(!renderpass) { DM_LOG_FATAL("Trying to begin invalid renderpass"); return false; }
                if(!dm_render_command_begin_renderpass_impl(renderpass->internal_index)) { DM_LOG_FATAL("Begin renderpass failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_END_RENDER_PASS:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, pass_index);
                
                dm_renderpass* renderpass = DM_RENDERER_GET_RESOURCE(DM_RENDER_RESOURCE_RENDERPASS, pass_index); 
                if(!renderpass) { DM_LOG_FATAL("Trying to end invalid renderpass"); return false; }
                if(!dm_render_command_end_renderpass_impl(renderpass->internal_index)) { DM_LOG_FATAL("End renderpass failed"); return false; }
            } break;
#endif
            
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
                DM_BYTE_POOL_POP(command.params, dm_render_handle, shader_handle);
                DM_BYTE_POOL_POP(command.params, uint32_t, offset);
                DM_BYTE_POOL_POP(command.params, dm_uniform_stage, stage);
                DM_BYTE_POOL_POP(command.params, uint32_t, slot);
                DM_BYTE_POOL_POP(command.params, dm_render_handle, uniform_handle);
                
                if(!dm_render_command_backend_bind_uniform(uniform_handle, stage, slot, offset, &context->renderer)) { DM_LOG_FATAL("Bind uniform failed"); return false; }
            } break;
            case DM_RENDER_COMMAND_UPDATE_UNIFORM:
            {
                DM_BYTE_POOL_POP(command.params, dm_render_handle, shader_handle);
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
    
    return true;
}

/*********
FRAMEWORK
***********/
dm_context* dm_init(uint32_t window_x_pos, uint32_t window_y_pos, uint32_t window_w, uint32_t window_h, const char* window_title)
{
    dm_context* context = dm_alloc(sizeof(dm_context));
    
    if(!dm_platform_init(window_x_pos, window_y_pos, window_w, window_h, window_title, &context->platform_data))
    {
        dm_free(context);
        return NULL;
    }
    
    if(!dm_renderer_init(context))
    {
        dm_free(context);
        return NULL;
    }
    
    context->delta = 1.0f / DM_DEFAULT_MAX_FPS;
    context->is_running = true;
    
    return context;
}

void dm_shutdown(dm_context* context)
{
    dm_renderer_shutdown(context);
    dm_platform_shutdown(&context->platform_data);
    
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
                context->is_running = false;
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
                //dm_renderer_resize_window(e.new_rect[0], e.new_rect[1]);
            } break;
            
            case DM_EVENT_UNKNOWN:
            {
                DM_LOG_ERROR("Unknown event! Shouldn't be here...");
            } break;
        }
    }
}

bool dm_update_begin(dm_context* context)
{
    context->start = dm_platform_get_time(&context->platform_data);
    
    if(!dm_platform_pump_events(&context->platform_data)) 
    {
        context->is_running = false;
        return false;
    }
    
    dm_poll_events(context);
    
    return true;
}

bool dm_update_end(dm_context* context)
{
    context->platform_data.event_list.num = 0;
    
    context->end = dm_platform_get_time(&context->platform_data);
    context->delta = context->end - context->start;
    
    return true;
}

bool dm_renderer_begin_frame(dm_context* context)
{
    if(!dm_renderer_backend_begin_frame(&context->renderer))
    {
        DM_LOG_FATAL("Begin frame failed");
        return false;
    }
    
    dm_render_command_clear(0,0,0,1, context);
    dm_render_command_set_default_viewport(context);
    
    return true;
}

bool dm_renderer_end_frame(bool vsync, dm_context* context)
{
    if(!dm_renderer_submit_commands(context)) 
    { 
        context->is_running = false;
        DM_LOG_FATAL("Submiting render commands failed"); 
        return false; 
    }
    
    context->renderer.command_manager.command_count = 0;
    
    if(!dm_renderer_backend_end_frame(vsync, context)) 
    {
        context->is_running = false;
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
        
        fread(buffer, *size, 1, fp);
        fclose(fp);
    }
    return buffer;
}

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

#endif //DM_IMPL_H
