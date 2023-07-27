#include "dm.h"

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

/*******
HASHING
*********/
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

dm_hash dm_hash_32bit(uint32_t key)
{
    dm_hash hash = ((key >> 16) ^ key) * 0x119de1f3;
    hash= ((hash >> 16) ^ hash) * 0x119de1f3;
    hash= (hash >> 16) ^ hash;
    
    return hash;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
dm_hash64 dm_hash_64bit(uint64_t key)
{
	dm_hash64 hash = (key ^ (key >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
	hash = hash ^ (hash >> 31);
    
	return hash;
}

// http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
dm_hash dm_hash_int_pair(int x, int y)
{
    if(y < x) return ( y << 16 ) + x;
    
    return (x << 16) + y;
}

// https://stackoverflow.com/questions/682438/hash-function-providing-unique-uint-from-an-integer-coordinate-pair
dm_hash64 dm_hash_uint_pair(uint32_t x, uint32_t y)
{
    if(y < x) return ( (uint64_t)y << 32 ) ^ x;
    
    return ((uint64_t)x << 32) ^ y;
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
extern bool dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_platform_data* platform_data);
extern void dm_platform_shutdown(dm_platform_data* platform_data);
extern double dm_platform_get_time(dm_platform_data* platform_data);
extern void dm_platform_write(const char* message, uint8_t color);
extern bool dm_platform_pump_events(dm_platform_data* platform_data);

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
extern bool dm_renderer_backend_end_frame(bool vsync, dm_context* context);
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

/*******
PHYSICS
*********/
#include "dm_physics.h"

/***
ECS
*****/
#define DM_ECS_MANAGER_ENTITY_CAPACITY DM_ECS_COMPONENT_BLOCK_SIZE
#define DM_ECS_MANAGER_RESIZE_FACTOR   2
#define DM_ECS_MANAGER_LOAD_FACTOR     0.75f

bool dm_ecs_init(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    // entities
    ecs_manager->entity_capacity          = DM_ECS_MANAGER_ENTITY_CAPACITY;
    
    size_t block_size     = sizeof(dm_entity) * ecs_manager->entity_capacity;
    ecs_manager->entities = dm_alloc(block_size);
    dm_memset(ecs_manager->entities, DM_ECS_INVALID_ENTITY, block_size);
    
    block_size                            = sizeof(uint32_t) * DM_ECS_MAX * ecs_manager->entity_capacity;
    ecs_manager->entity_component_indices = dm_alloc(block_size);
    dm_memset(ecs_manager->entity_component_indices, DM_ECS_INVALID_ID, block_size);
    
    block_size                           *= DM_ECS_SYSTEM_TIMING_UNKNOWN;
    ecs_manager->entity_system_indices    = dm_alloc(block_size);
    dm_memset(ecs_manager->entity_system_indices, DM_ECS_INVALID_ID, block_size);
    
    block_size                            = sizeof(dm_ecs_id) * ecs_manager->entity_capacity;
    ecs_manager->entity_component_masks   = dm_alloc(block_size);
    dm_memzero(ecs_manager->entity_component_masks, block_size);
    
    // transform component
    ecs_manager->default_components.transform = dm_ecs_register_component(sizeof(dm_component_transform_block), context);
    if(ecs_manager->default_components.transform!=DM_ECS_INVALID_ID) return true;
    
    DM_LOG_FATAL("Could not register transform component"); 
    return false;
}

void dm_ecs_shutdown(dm_ecs_manager* ecs_manager)
{
    uint32_t i;
    
    for(i=0; i<ecs_manager->num_registered_components; i++)
    {
        dm_free(ecs_manager->components[i].data);
    }
    
    for(i=0; i<DM_ECS_SYSTEM_TIMING_UNKNOWN; i++)
    {
        for(uint32_t j=0; j<ecs_manager->num_registered_systems[i]; j++)
        {
            dm_free(ecs_manager->systems[i][j].data);
        }
    }
    
    dm_free(ecs_manager->entity_component_masks);
    dm_free(ecs_manager->entity_component_indices);
    dm_free(ecs_manager->entity_system_indices);
    dm_free(ecs_manager->entities);
}

dm_ecs_id dm_ecs_register_component(size_t component_block_size, dm_context* context)
{
    if(context->ecs_manager.num_registered_components >= DM_ECS_MAX) return DM_ECS_INVALID_ID;
    
    dm_ecs_id id = context->ecs_manager.num_registered_components++;
    
    context->ecs_manager.components[id].block_size   = component_block_size;
    context->ecs_manager.components[id].block_count  = 1;
    context->ecs_manager.components[id].entity_count = 0;
    
    context->ecs_manager.components[id].data = dm_alloc(component_block_size);
    
    return id;
}

dm_ecs_id dm_ecs_register_system(size_t* system_block_sizes, dm_ecs_id* component_ids, uint32_t component_count, dm_ecs_system_timing timing, bool (*insert_func)(dm_entity,dm_ecs_system_timing,dm_ecs_id,void*), bool (*run_func)(void*), dm_context* context)
{
    if(context->ecs_manager.num_registered_systems[timing] >= DM_ECS_MAX) return DM_ECS_INVALID_ID;
    
    dm_ecs_id id = context->ecs_manager.num_registered_systems[timing]++;
    
    for(uint32_t i=0; i<component_count; i++)
    {
        if(i>0 && component_ids[i] < component_ids[i-1]) 
        { 
            DM_LOG_ERROR("Re-run with component ids in increasing order"); 
            return DM_ECS_INVALID_ID; 
        }
        
        context->ecs_manager.systems[timing][id].block_size     += system_block_sizes[i];
        context->ecs_manager.systems[timing][id].component_mask |= DM_BIT_SHIFT(component_ids[i]);
    }
    
    context->ecs_manager.systems[timing][id].block_count = 1;
    context->ecs_manager.systems[timing][id].run_func    = run_func;
    context->ecs_manager.systems[timing][id].insert_func = insert_func;
    context->ecs_manager.systems[timing][id].data        = dm_alloc(context->ecs_manager.systems[timing][id].block_size);
    
    return id;
}

uint32_t dm_ecs_entity_get_index(dm_entity entity, dm_context* context)
{
    const uint32_t index = dm_hash_32bit(entity) % context->ecs_manager.entity_capacity;
    
    if(context->ecs_manager.entities[index]==entity) return index;
    
    uint32_t runner = index + 1;
    if(runner >= context->ecs_manager.entity_capacity) runner = 0;
    
    while(runner != index)
    {
        if(context->ecs_manager.entities[runner]==entity) return runner;
        
        runner++;
        if(runner >= context->ecs_manager.entity_capacity) runner = 0;
    }
    
    DM_LOG_FATAL("Could not find entity index, should not be here...");
    for(uint32_t i=0; i<context->ecs_manager.entity_capacity; i++)
    {
        if(context->ecs_manager.entities[i]!=entity) continue;
        DM_LOG_WARN("Entity found");
    }
    return UINT_MAX;
}

void dm_ecs_entity_insert(dm_entity entity, dm_context* context)
{
    const uint32_t index = dm_hash_32bit(entity) % context->ecs_manager.entity_capacity;
    
    if(context->ecs_manager.entities[index]==DM_ECS_INVALID_ENTITY) 
    {
        context->ecs_manager.entities[index] = entity;
        return;
    }
    
    uint32_t runner = index + 1;
    if(runner >= context->ecs_manager.entity_capacity) runner = 0;

    while(runner != index)
    {
        if(context->ecs_manager.entities[runner]==DM_ECS_INVALID_ENTITY) 
        {
            context->ecs_manager.entities[runner] = entity;
            return;
        }
        
        runner++;
        if(runner >= context->ecs_manager.entity_capacity) runner = 0;
    }
    
    DM_LOG_FATAL("Could not insert entity, should not be here...");
}

void dm_ecs_reinsert_entities(uint32_t old_capacity, dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;

    uint32_t cap = ecs_manager->entity_capacity;
    
    size_t e_size = sizeof(dm_entity) * cap;
    size_t c_size = sizeof(uint32_t) * DM_ECS_MAX * cap;
    size_t s_size = sizeof(uint32_t) * DM_ECS_MAX * DM_ECS_SYSTEM_TIMING_UNKNOWN * cap;
    size_t m_size = sizeof(dm_ecs_id) * cap;

    dm_entity* new_entities;
    uint32_t (*new_comp_inds)[DM_ECS_MAX];
    uint32_t (*new_sys_inds)[DM_ECS_SYSTEM_TIMING_UNKNOWN][DM_ECS_MAX];
    dm_ecs_id* new_masks;

    new_entities  = dm_alloc(e_size);
    new_comp_inds = dm_alloc(c_size);
    new_sys_inds  = dm_alloc(s_size);
    new_masks     = dm_alloc(m_size);

    dm_memset(new_entities, DM_ECS_INVALID_ENTITY, e_size);
    dm_memset(new_comp_inds, DM_ECS_INVALID_ID, c_size);
    dm_memset(new_sys_inds, DM_ECS_INVALID_ID, s_size);
    dm_memset(new_masks, DM_ECS_INVALID_ID, m_size);

    for(uint32_t i=0; i<old_capacity; i++)
    {
        dm_entity entity = ecs_manager->entities[i];
        if(entity==DM_ECS_INVALID_ENTITY) continue;

        uint32_t index = dm_hash_32bit(entity) % ecs_manager->entity_capacity;

        if(new_entities[index]==DM_ECS_INVALID_ENTITY)
        {
            new_entities[index] = entity;
            dm_memcpy(new_comp_inds[index], ecs_manager->entity_component_indices[i], sizeof(uint32_t) * DM_ECS_MAX);
            dm_memcpy(new_sys_inds[index], ecs_manager->entity_system_indices[i], sizeof(uint32_t) * DM_ECS_SYSTEM_TIMING_UNKNOWN * DM_ECS_MAX);
            new_masks[index] = ecs_manager->entity_component_masks[i];
        }
        // hash conflict, so iterate
        else
        {
            uint32_t runner = index + 1;
            if(runner >= ecs_manager->entity_capacity) runner = 0;

            while(runner != index)
            {
                if(new_entities[runner]==DM_ECS_INVALID_ENTITY)
                {
                    new_entities[runner] = entity;
                    dm_memcpy(new_comp_inds[runner], ecs_manager->entity_component_indices[i], sizeof(uint32_t) * DM_ECS_MAX);
                    dm_memcpy(new_sys_inds[runner], ecs_manager->entity_system_indices[i], sizeof(uint32_t) * DM_ECS_SYSTEM_TIMING_UNKNOWN * DM_ECS_MAX);
                    new_masks[runner] = ecs_manager->entity_component_masks[i];
                    
                    break;
                }
                else
                {
                    runner++;
                    if(runner >= ecs_manager->entity_capacity) runner = 0;
                }
            }
        }
    }

    dm_memcpy(ecs_manager->entities, new_entities, e_size);
    dm_memcpy(ecs_manager->entity_component_indices, new_comp_inds, c_size);
    dm_memcpy(ecs_manager->entity_system_indices, new_sys_inds, s_size);
    dm_memcpy(ecs_manager->entity_component_masks, new_masks, m_size);

    dm_free(new_entities);
    dm_free(new_comp_inds);
    dm_free(new_sys_inds);
    dm_free(new_masks);
}

bool dm_ecs_entity_insert_into_systems(dm_entity entity, dm_context* context)
{
    // for all system timings
    for(uint32_t t=0; t<DM_ECS_SYSTEM_TIMING_UNKNOWN; t++)
    {
        // for all systems in timing
        for(uint32_t s=0; s<context->ecs_manager.num_registered_systems[t]; s++)
        {
            // early out if we don't have what this system needs
            if(!dm_ecs_entity_has_component_multiple(entity, context->ecs_manager.systems[t][s].component_mask, context)) continue;
            
            if(!context->ecs_manager.systems[t][s].insert_func(entity,t,s,context)) return false;
        }
    }
    
    return true;
}

bool dm_ecs_run_systems(dm_ecs_system_timing timing, dm_context* context)
{
    for(uint32_t i=0; i<context->ecs_manager.num_registered_systems[timing]; i++)
    {
        if(!context->ecs_manager.systems[timing][i].run_func(context)) return false;
    }
    
    return true;
}

dm_entity dm_ecs_create_entity(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    dm_entity entity = dm_random_uint32(context);
    ecs_manager->entity_count++;
    
    dm_ecs_entity_insert(entity, context);
    
    if((float)ecs_manager->entity_count / (float)ecs_manager->entity_capacity < DM_ECS_MANAGER_LOAD_FACTOR) return entity;
    
    // we need to resize all entity arrays, and reinsert. This is costly!!!!
    uint32_t old_capacity         = ecs_manager->entity_capacity;
    ecs_manager->entity_capacity *= DM_ECS_MANAGER_RESIZE_FACTOR;
    uint32_t cap_dif              = ecs_manager->entity_capacity - old_capacity;
    
    // resize entities
    size_t index_size     = sizeof(dm_entity);
    size_t new_size       = index_size * ecs_manager->entity_capacity;
    size_t offset         = old_capacity;
    ecs_manager->entities = dm_realloc(ecs_manager->entities, new_size);
    void* block           = ecs_manager->entities + offset;
    dm_memset(block, DM_ECS_INVALID_ENTITY, index_size * cap_dif);
    
    // resize indices
    index_size                            = sizeof(uint32_t) * DM_ECS_MAX;
    new_size                              = index_size * ecs_manager->entity_capacity;
    ecs_manager->entity_component_indices = dm_realloc(ecs_manager->entity_component_indices, new_size);
    block                                 = ecs_manager->entity_component_indices + offset;
    dm_memset(block, DM_ECS_INVALID_ID, index_size * cap_dif);
    
    index_size                        *= DM_ECS_SYSTEM_TIMING_UNKNOWN;
    new_size                           = index_size * ecs_manager->entity_capacity;
    ecs_manager->entity_system_indices = dm_realloc(ecs_manager->entity_system_indices, new_size);
    block                              = ecs_manager->entity_system_indices + offset;
    dm_memset(block, DM_ECS_INVALID_ID, index_size * cap_dif);
    
    // resize component masks
    index_size                          = sizeof(dm_ecs_id);
    new_size                            = index_size * ecs_manager->entity_capacity;
    ecs_manager->entity_component_masks = dm_realloc(ecs_manager->entity_component_masks, new_size);
    block                               = context->ecs_manager.entity_component_masks + offset;
    dm_memset(block, 0, index_size * cap_dif);
    
    // reinsertion
    dm_ecs_reinsert_entities(old_capacity, context);

    return entity;
}

bool dm_ecs_entity_has_component(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==UINT_MAX) return false;
    
    return context->ecs_manager.entity_component_masks[entity_index] & component_id;
}

bool dm_ecs_entity_has_component_multiple(dm_entity entity, dm_ecs_id component_mask, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==UINT_MAX) return false;
    
    dm_ecs_id entity_mask = context->ecs_manager.entity_component_masks[entity_index];
    
    // NAND entity mask with opposite of component mask
    dm_ecs_id result = ~(entity_mask & ~component_mask);
    // XOR with opposite of entity mask
    result ^= ~entity_mask;
    // success only if this equals the component mask
    return (result == component_mask);
}

void* dm_ecs_get_current_component_block(dm_ecs_id component_id, uint32_t* insert_index, dm_context* context)
{
    dm_ecs_component_manager* manager = &context->ecs_manager.components[component_id];
    *insert_index = context->ecs_manager.components[component_id].entity_count % DM_ECS_COMPONENT_BLOCK_SIZE;
    
    return (char*)manager->data + manager->block_size * (manager->block_count-1);
}

void* dm_ecs_entity_get_component_block(dm_entity entity, dm_ecs_id component_id, uint32_t* index, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==UINT_MAX) return NULL;
    
    uint32_t component_index = context->ecs_manager.entity_component_indices[entity_index][component_id];

    uint32_t block_index = component_index / DM_ECS_COMPONENT_BLOCK_SIZE;
    *index = component_index % DM_ECS_COMPONENT_BLOCK_SIZE;
    
    dm_ecs_component_manager* manager = &context->ecs_manager.components[component_id];
    
    return (char*)manager->data + manager->block_size * block_index;
}

void dm_ecs_iterate_component_block(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    dm_ecs_component_manager* manager = &context->ecs_manager.components[component_id];
    
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==UINT_MAX) return;

    context->ecs_manager.entity_component_indices[entity_index][component_id] = manager->entity_count++;
    context->ecs_manager.entity_component_masks[entity_index] |= DM_BIT_SHIFT(component_id);
    
    if(!dm_ecs_entity_insert_into_systems(entity, context)) { DM_LOG_ERROR("Inserting entity failed"); return; }
    
    if(manager->entity_count==0) return;
    bool resize = manager->entity_count % DM_ECS_COMPONENT_BLOCK_SIZE == 0;
    if(!resize) return;
    
    manager->block_count++;
    manager->data = dm_realloc(manager->data, manager->block_count * manager->block_size);
    void* data = (char*)manager->data + (manager->block_count-1) * manager->block_size;
    dm_memzero(data, manager->block_size);
}

void dm_ecs_interate_system_block(dm_entity entity, dm_ecs_system_timing timing, dm_ecs_id system_id, dm_context* context)
{
    dm_ecs_system_manager* manager = &context->ecs_manager.systems[timing][system_id];
    
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(entity_index==UINT_MAX) return;

    context->ecs_manager.entity_system_indices[entity_index][timing][system_id] = manager->entity_count++;

    if(manager->entity_count==0) return;
    bool resize = manager->entity_count % DM_ECS_COMPONENT_BLOCK_SIZE == 0;
    if(!resize) return;

    manager->block_count++;
    manager->data = dm_realloc(manager->data, manager->block_count * manager->block_size);
    void* data = (char*)manager->data + (manager->block_count-1) * manager->block_size;
    dm_memzero(data, manager->block_size);
}

void dm_ecs_entity_add_transform(dm_entity entity, dm_component_transform transform, dm_context* context)
{
    if(entity==DM_ECS_INVALID_ENTITY) { DM_LOG_ERROR("Trying to add transform to invalid entity"); return; }
    
    uint32_t index;
    const dm_ecs_id transform_id = context->ecs_manager.default_components.transform;
    dm_component_transform_block* transform_block = dm_ecs_get_current_component_block(transform_id, &index, context);
    
    transform_block->pos_x[index] = transform.pos[0];
    transform_block->pos_y[index] = transform.pos[1];
    transform_block->pos_z[index] = transform.pos[2];
    
    transform_block->scale_x[index] = transform.scale[0];
    transform_block->scale_y[index] = transform.scale[1];
    transform_block->scale_z[index] = transform.scale[2];
    
    transform_block->rot_i[index] = transform.rot[0];
    transform_block->rot_j[index] = transform.rot[1];
    transform_block->rot_k[index] = transform.rot[2];
    transform_block->rot_r[index] = transform.rot[3];
    
    dm_ecs_iterate_component_block(entity, transform_id, context);
}

void dm_ecs_entity_add_physics(dm_entity entity, dm_component_physics physics, dm_context* context)
{
    if(entity==DM_ECS_INVALID_ENTITY) { DM_LOG_ERROR("Trying to add physics to invalid entity"); return; }
    
    uint32_t index;
    const dm_ecs_id physics_id = context->ecs_manager.default_components.physics;
    dm_component_physics_block* physics_block = dm_ecs_get_current_component_block(physics_id, &index, context);
    
    physics_block->vel_x[index] = physics.vel[0];
    physics_block->vel_y[index] = physics.vel[1];
    physics_block->vel_z[index] = physics.vel[2];
    
    physics_block->w_x[index] = physics.w[0];
    physics_block->w_y[index] = physics.w[1];
    physics_block->w_z[index] = physics.w[2];
    
    physics_block->l_x[index] = physics.l[0];
    physics_block->l_y[index] = physics.l[1];
    physics_block->l_z[index] = physics.l[2];
    
    physics_block->force_x[index] = physics.force[0];
    physics_block->force_y[index] = physics.force[1];
    physics_block->force_z[index] = physics.force[2];
    
    physics_block->torque_x[index] = physics.torque[0];
    physics_block->torque_y[index] = physics.torque[1];
    physics_block->torque_z[index] = physics.torque[2];
    
    physics_block->mass[index]     = physics.mass;
    physics_block->inv_mass[index] = physics.inv_mass;
    
    physics_block->i_body_0[index] = physics.i_body[0];
    physics_block->i_body_1[index] = physics.i_body[1];
    physics_block->i_body_2[index] = physics.i_body[2];
    
    physics_block->i_body_inv_0[index] = physics.i_body_inv[0];
    physics_block->i_body_inv_1[index] = physics.i_body_inv[1];
    physics_block->i_body_inv_2[index] = physics.i_body_inv[2];
    
    physics_block->i_inv_0_0[index] = physics.i_inv_0[0];
    physics_block->i_inv_0_1[index] = physics.i_inv_0[1];
    physics_block->i_inv_0_2[index] = physics.i_inv_0[2];
    
    physics_block->i_inv_1_0[index] = physics.i_inv_1[0];
    physics_block->i_inv_1_1[index] = physics.i_inv_1[1];
    physics_block->i_inv_1_2[index] = physics.i_inv_1[2];
    
    physics_block->i_inv_2_0[index] = physics.i_inv_2[0];
    physics_block->i_inv_2_1[index] = physics.i_inv_2[1];
    physics_block->i_inv_2_2[index] = physics.i_inv_2[2];
    
    physics_block->damping_v[index] = physics.damping[0];
    physics_block->damping_w[index] = physics.damping[1];
    
    physics_block->body_type[index]     = physics.body_type;
    physics_block->movement_type[index] = physics.movement_type;
    
    dm_ecs_iterate_component_block(entity, physics_id, context);
}

void dm_ecs_entity_add_collision(dm_entity entity, dm_component_collision collision, dm_context* context)
{
    if(entity==DM_ECS_INVALID_ENTITY) { DM_LOG_ERROR("Trying to add collision to invalid entity"); return; }
    
    uint32_t index;
    const dm_ecs_id collision_id = context->ecs_manager.default_components.collision;
    dm_component_collision_block* collision_block = dm_ecs_get_current_component_block(collision_id, &index, context);
    
    collision_block->aabb_local_min_x[index] = collision.aabb_local_min[0];
    collision_block->aabb_local_min_y[index] = collision.aabb_local_min[1];
    collision_block->aabb_local_min_z[index] = collision.aabb_local_min[2];
    
    collision_block->aabb_local_max_x[index] = collision.aabb_local_max[0];
    collision_block->aabb_local_max_y[index] = collision.aabb_local_max[1];
    collision_block->aabb_local_max_z[index] = collision.aabb_local_max[2];
    
    collision_block->aabb_global_min_x[index] = collision.aabb_global_min[0];
    collision_block->aabb_global_min_y[index] = collision.aabb_global_min[1];
    collision_block->aabb_global_min_z[index] = collision.aabb_global_min[2];
    
    collision_block->aabb_global_max_x[index] = collision.aabb_global_max[0];
    collision_block->aabb_global_max_y[index] = collision.aabb_global_max[1];
    collision_block->aabb_global_max_z[index] = collision.aabb_global_max[2];
    
    collision_block->center_x[index] = collision.center[0];
    collision_block->center_y[index] = collision.center[1];
    collision_block->center_z[index] = collision.center[2];
    
    collision_block->internal_0[index] = collision.internal[0];
    collision_block->internal_1[index] = collision.internal[1];
    collision_block->internal_2[index] = collision.internal[2];
    collision_block->internal_3[index] = collision.internal[3];
    collision_block->internal_4[index] = collision.internal[4];
    collision_block->internal_5[index] = collision.internal[5];
    
    collision_block->shape[index] = collision.shape;
    collision_block->flag[index]  = collision.flag;
    
    dm_ecs_iterate_component_block(entity, collision_id, context);
}

void dm_ecs_entity_add_box_collider(dm_entity entity, float center[3], float dim[3], dm_context* context)
{
    float min[3];
    float max[3];
    
    dm_vec3_sub_vec3(center, dim, min);
    dm_vec3_add_vec3(center, dim, max);
    
    dm_component_collision c = {
        .aabb_local_min[0]=min[0], .aabb_local_min[1]=min[1], .aabb_local_min[2]=min[2],
        .aabb_global_min[0]=max[0], .aabb_global_min[1]=max[1], .aabb_global_min[2]=max[2],
        .center[0]=center[0], .center[1]=center[1], .center[2]=center[2],
        .internal[0]=min[0], .internal[1]=min[1], .internal[2]=min[2], 
        .internal[3]=max[0], .internal[4]=max[1], .internal[5]=max[2], 
        .shape=DM_COLLISION_SHAPE_BOX,
    };
    
    dm_ecs_entity_add_collision(entity, c, context);
}

void dm_ecs_entity_add_sphere_collider(dm_entity entity, float center[3], float radius, dm_context* context)
{
    float min[3];
    float max[3];
    
    dm_vec3_sub_scalar(center, radius, min);
    dm_vec3_add_scalar(center, radius, max);
    
    dm_component_collision c = {
        .aabb_local_min[0]=min[0], .aabb_local_min[1]=min[1], .aabb_local_min[2]=min[2],
        .aabb_global_min[0]=max[0], .aabb_global_min[1]=max[1], .aabb_global_min[2]=max[2],
        .center[0]=center[0], .center[1]=center[1], .center[2]=center[2],
        .internal[0]=radius, .shape=DM_COLLISION_SHAPE_SPHERE,
    };
    
    dm_ecs_entity_add_collision(entity, c, context);
}

const dm_component_transform dm_ecs_entity_get_transform(dm_entity entity, dm_context* context)
{
    uint32_t index;
    dm_component_transform_block* transform_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.transform, &index, context);
    if(!transform_block) return (dm_component_transform){ 0 };

    dm_component_transform transform = { 0 };
    
    transform.pos[0] = transform_block->pos_x[index];
    transform.pos[1] = transform_block->pos_y[index];
    transform.pos[2] = transform_block->pos_z[index];
    
    transform.scale[0] = transform_block->scale_x[index];
    transform.scale[1] = transform_block->scale_y[index];
    transform.scale[2] = transform_block->scale_z[index];
    
    transform.rot[0] = transform_block->rot_i[index];
    transform.rot[1] = transform_block->rot_j[index];
    transform.rot[2] = transform_block->rot_k[index];
    transform.rot[3] = transform_block->rot_r[index];
    
    return transform;
}

const dm_component_collision dm_ecs_entity_get_collision(dm_entity entity, dm_context* context)
{
    uint32_t index;
    dm_component_collision_block* collision_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.collision, &index, context);
    
    dm_component_collision collision = { 0 };
    
    collision.aabb_local_min[0] = collision_block->aabb_local_min_x[index];
    collision.aabb_local_min[1] = collision_block->aabb_local_min_y[index];
    collision.aabb_local_min[2] = collision_block->aabb_local_min_z[index];
    
    collision.aabb_local_max[0] = collision_block->aabb_local_max_x[index];
    collision.aabb_local_max[1] = collision_block->aabb_local_max_y[index];
    collision.aabb_local_max[2] = collision_block->aabb_local_max_z[index];
    
    collision.aabb_global_min[0] = collision_block->aabb_local_min_x[index];
    collision.aabb_global_min[1] = collision_block->aabb_local_min_y[index];
    collision.aabb_global_min[2] = collision_block->aabb_local_min_z[index];
    
    collision.aabb_global_max[0] = collision_block->aabb_global_max_x[index];
    collision.aabb_global_max[1] = collision_block->aabb_global_max_y[index];
    collision.aabb_global_max[2] = collision_block->aabb_global_max_z[index];
    
    collision.center[0] = collision_block->center_x[index];
    collision.center[1] = collision_block->center_y[index];
    collision.center[2] = collision_block->center_z[index];
    
    collision.internal[0] = collision_block->internal_0[index];
    collision.internal[1] = collision_block->internal_1[index];
    collision.internal[2] = collision_block->internal_2[index];
    collision.internal[3] = collision_block->internal_3[index];
    collision.internal[4] = collision_block->internal_4[index];
    collision.internal[5] = collision_block->internal_5[index];
    
    collision.shape = collision_block->shape[index];
    collision.flag = collision_block->flag[index];
    
    return collision;
}

const dm_component_physics dm_ecs_entity_get_physics(dm_entity entity, dm_context* context)
{
    uint32_t index;
    dm_component_physics_block* physics_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.physics, &index, context);
    
    dm_component_physics physics = { 0 };
    
    physics.vel[0] = physics_block->vel_x[index];
    physics.vel[1] = physics_block->vel_y[index];
    physics.vel[2] = physics_block->vel_z[index];
    
    physics.w[0] = physics_block->w_x[index];
    physics.w[1] = physics_block->w_y[index];
    physics.w[2] = physics_block->w_z[index];
    
    physics.l[0] = physics_block->l_x[index];
    physics.l[1] = physics_block->l_y[index];
    physics.l[2] = physics_block->l_z[index];
    
    physics.force[0] = physics_block->force_x[index];
    physics.force[1] = physics_block->force_y[index];
    physics.force[2] = physics_block->force_z[index];
    
    physics.torque[0] = physics_block->torque_x[index];
    physics.torque[1] = physics_block->torque_y[index];
    physics.torque[2] = physics_block->torque_z[index];
    
    physics.mass = physics_block->mass[index];
    physics.inv_mass = physics_block->inv_mass[index];
    
    physics.i_body[0] = physics_block->i_body_0[index];
    physics.i_body[1] = physics_block->i_body_1[index];
    physics.i_body[2] = physics_block->i_body_2[index];
    
    physics.i_body_inv[0] = physics_block->i_body_inv_0[index];
    physics.i_body_inv[1] = physics_block->i_body_inv_1[index];
    physics.i_body_inv[2] = physics_block->i_body_inv_2[index];
    
    physics.i_inv_0[0] = physics_block->i_inv_0_0[index];
    physics.i_inv_0[1] = physics_block->i_inv_0_1[index];
    physics.i_inv_0[2] = physics_block->i_inv_0_2[index];
    
    physics.i_inv_1[0] = physics_block->i_inv_1_0[index];
    physics.i_inv_1[1] = physics_block->i_inv_1_1[index];
    physics.i_inv_1[2] = physics_block->i_inv_1_2[index];
    
    physics.i_inv_2[0] = physics_block->i_inv_2_0[index];
    physics.i_inv_2[1] = physics_block->i_inv_2_1[index];
    physics.i_inv_2[2] = physics_block->i_inv_2_2[index];
    
    physics.damping[0] = physics_block->damping_v[index];
    physics.damping[1] = physics_block->damping_w[index];
    
    physics.body_type     = physics_block->body_type[index];
    physics.movement_type = physics_block->movement_type[index];
    
    return physics;
}

/*********
FRAMEWORK
***********/
typedef enum dm_context_flags_t
{
    DM_CONTEXT_FLAG_IS_RUNNING,
    DM_CONTEXT_FLAG_UNKNOWN
} dm_context_flags;

dm_context* dm_init(uint32_t window_x_pos, uint32_t window_y_pos, uint32_t window_w, uint32_t window_h, const char* window_title, const char* asset_path)
{
    dm_context* context = dm_alloc(sizeof(dm_context));
    
    context->platform_data.window_data.width = window_w;
    context->platform_data.window_data.height = window_h;
    strcpy(context->platform_data.window_data.title, window_title);
    strcpy(context->platform_data.asset_path, asset_path);
    
    if(!dm_platform_init(window_x_pos, window_y_pos, &context->platform_data))
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
    
    dm_ecs_init(context);
    
    if(!dm_physics_init(&context->ecs_manager.default_components.physics, &context->ecs_manager.default_components.collision, context))
    {
        dm_renderer_shutdown(context);
        dm_platform_shutdown(&context->platform_data);
        dm_ecs_shutdown(&context->ecs_manager);
        dm_free(context);
        return NULL;
    }
    
    // random init
    init_genrand(&context->random, (uint32_t)dm_platform_get_time(&context->platform_data));
    init_genrand64(&context->random_64, (uint64_t)dm_platform_get_time(&context->platform_data));
    
    context->delta = 1.0f / DM_DEFAULT_MAX_FPS;
    context->flags |= DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
    
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
    dm_physics_shutdown(context);
    dm_ecs_shutdown(&context->ecs_manager);
    
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

bool dm_update_begin(dm_context* context)
{
    context->start = dm_platform_get_time(&context->platform_data);
    
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
    
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_RENDER_BEGIN, context)) return false;
    
    return true;
}

bool dm_renderer_end_frame(bool vsync, bool begin_frame, dm_context* context)
{
    // systems
    if(!dm_ecs_run_systems(DM_ECS_SYSTEM_TIMING_RENDER_END, context)) return false;
    
    // command submission
    if(begin_frame && !dm_renderer_submit_commands(context)) 
    { 
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        DM_LOG_FATAL("Submiting render commands failed"); 
        return false; 
    }
    
    context->renderer.command_manager.command_count = 0;
    
    if(begin_frame && !dm_renderer_backend_end_frame(vsync, context)) 
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
