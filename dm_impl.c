#include "dm.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <assert.h>

#include "mt19937/mt19937.h"
#include "mt19937/mt19937_64.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "stb_truetype/stb_truetype.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

//#define DM_MATH_TESTS

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
    h = dm_clamp(h, 0, 1);
    return h * h * (3 - 2 * h);
}

float dm_smootherstep(float edge0, float edge1, float x)
{
    float h = (x - edge0) / (edge1 - edge0);
    h = dm_clamp(h, 0, 1);
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

float dm_clamp(float x, float min, float max)
{
    const float t = DM_MIN(x, max);
    return DM_MAX(t, min);
}

float dm_rad_to_deg(float radians)
{
	return  (float)(radians * (180.0f / DM_MATH_PI));
}

float dm_deg_to_rad(float degrees)
{
	return (float)(degrees * (DM_MATH_PI / 180.0f));
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
    int old_range = INT_MAX;
    int new_range = end - start;
    
    return (dm_random_int(context) + new_range) / old_range + start;
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

#define DM_NORMAL_ITERS 5
float dm_random_float_normal(float mu, float sigma, dm_context* context)
{
    const float u1 = dm_random_float(context);
    const float u2 = dm_random_float(context);
     
    const float mag = sigma * dm_sqrtf(-2.0 * dm_logf(u1));
    const float z0  = mag * dm_cos(DM_MATH_2PI * u2) + mu;
    const float z1  = mag * dm_sin(DM_MATH_2PI * u2) + mu;
    
    return dm_random_float(context) > 0.5f ? z0 : z1;
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

void dm_input_set_mouse_scroll(float delta, dm_context* context)
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

float dm_input_get_mouse_scroll(dm_context* context)
{
	return context->input_states[0].mouse.scroll;
}

float dm_input_get_prev_mouse_scroll(dm_context* context)
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

void dm_add_mouse_scroll_event(float delta, dm_event_list* event_list)
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
extern bool   dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_context* context);
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

extern void* dm_renderer_backend_get_internal_texture_ptr(dm_render_handle handle, dm_renderer* renderer);

extern bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer);

extern bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_dynamic_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer);

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
extern void dm_render_command_backend_set_scissor_rects(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, dm_renderer* renderer);

// compute
extern bool  dm_compute_backend_create_shader(dm_compute_shader_desc desc, dm_compute_handle* handle, dm_renderer* renderer);
extern bool  dm_compute_backend_create_buffer(size_t data_size, size_t elem_size, dm_compute_buffer_type type, dm_compute_handle* handle, dm_renderer* renderer);
extern bool  dm_compute_backend_create_uniform(size_t data_size, dm_compute_handle* handle, dm_renderer* renderer);

extern bool  dm_compute_backend_command_bind_buffer(dm_compute_handle handle, uint32_t offset, uint32_t slot, dm_renderer* renderer);
extern bool  dm_compute_backend_command_update_buffer(dm_compute_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer);
extern void* dm_compute_backend_command_get_buffer_data(dm_compute_handle handle, dm_renderer* renderer);
extern bool  dm_compute_backend_command_bind_shader(dm_compute_handle handle, dm_renderer* renderer);
extern bool  dm_compute_backend_command_dispatch(uint32_t x_size, uint32_t y_size, uint32_t z_size, uint32_t x_thread_grps, uint32_t y_thread_grps, uint32_t z_thread_grps, dm_renderer* renderer);

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

bool dm_renderer_create_static_index_buffer(void* data, size_t data_size, size_t index_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_INDEX,
        .usage=DM_BUFFER_USAGE_STATIC,
        .cpu_access=DM_BUFFER_CPU_WRITE,
        .buffer_size=data_size,
        .elem_size=index_size
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_renderer_create_dynamic_index_buffer(void* data, size_t data_size, size_t index_size, dm_render_handle* handle, dm_context* context)
{
    dm_buffer_desc desc = {
        .type=DM_BUFFER_TYPE_INDEX,
        .usage=DM_BUFFER_USAGE_DYNAMIC,
        .cpu_access=DM_BUFFER_CPU_READ,
        .buffer_size=data_size,
        .elem_size=index_size
    };
    
    return dm_renderer_create_buffer(desc, data, handle, context);
}

bool dm_compute_create_buffer(size_t data_size, size_t elem_size, dm_compute_buffer_type type, dm_compute_handle* handle, dm_context* context)
{
    if(dm_compute_backend_create_buffer(data_size, elem_size, type, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not create compute buffer");
    return false;
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

bool dm_renderer_create_texture_from_data(uint32_t width, uint32_t height, uint32_t n_channels, const void* data, const char* name, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_texture(width, height, n_channels, data, name, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Failed to create texture from data");
    return false;
}

bool dm_renderer_create_dynamic_texture(uint32_t width, uint32_t height, uint32_t n_channels, const void* data, const char* name, dm_render_handle* handle, dm_context* context)
{
    if(dm_renderer_backend_create_dynamic_texture(width, height, n_channels, data, name, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not create dynamic texture");
    return false;
}

void* dm_renderer_get_internal_texture_ptr(dm_render_handle handle, dm_context* context)
{
    return dm_renderer_backend_get_internal_texture_ptr(handle, &context->renderer);
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

bool dm_compute_create_uniform(size_t data_size, dm_compute_handle* handle, dm_context* context)
{
    if(dm_compute_backend_create_uniform(data_size, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not create compute uniform");
    return false;
}

bool dm_compute_create_shader(dm_compute_shader_desc desc, dm_render_handle* handle, dm_context* context)
{
    if(dm_compute_backend_create_shader(desc, handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not create compute shader from: %s", desc.path);
    return false;
}

bool dm_compute_command_bind_buffer(dm_compute_handle handle, uint32_t offset, uint32_t slot, dm_context* context)
{
    if(dm_compute_backend_command_bind_buffer(handle, offset, slot, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not bind compute buffer");
    return false;
}

bool dm_compute_command_update_buffer(dm_compute_handle handle, void* data, size_t data_size, size_t offset, dm_context* context)
{
    if(dm_compute_backend_command_update_buffer(handle, data, data_size, offset, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not update compute buffer");
    return false;
}

void* dm_compute_command_get_buffer_data(dm_compute_handle handle, dm_context* context)
{
    return dm_compute_backend_command_get_buffer_data(handle, &context->renderer);
}

bool dm_compute_command_bind_shader(dm_compute_handle handle, dm_context* context)
{
    if(dm_compute_backend_command_bind_shader(handle, &context->renderer)) return true;
    
    DM_LOG_FATAL("Could not bind compute shader");
    return false;
}

bool dm_compute_command_dispatch(uint32_t x_size, uint32_t y_size, uint32_t z_size, uint32_t x_thread_grps, uint32_t y_thread_grps, uint32_t z_thread_grps, dm_context* context)
{
    if(dm_compute_backend_command_dispatch(x_size, y_size, z_size, x_thread_grps, y_thread_grps, z_thread_grps, &context->renderer)) return true;
    
    DM_LOG_FATAL("Compute dispatch failed");
    return false;
}

/*************
MODEL LOADING
***************/
bool dm_renderer_load_obj_model(const char* path, const dm_mesh_vertex_attrib* attribs, uint32_t attrib_count, dm_mesh_index_type index_type, float** vertices, void** indices, uint32_t* vertex_count, uint32_t* index_count, uint32_t index_offset)
{
    fastObjMesh* m = fast_obj_read(path);
    if(!m) { DM_LOG_ERROR("Could not create mesh from file \'%s\'", path); return false; }
    
    assert(m->group_count==1);
    
    int indx = 0;
    
    int pos_offset, norm_offset, tex_offset; 
    pos_offset = norm_offset = tex_offset = -1;
    size_t vertex_size = 0;
    
    for(uint32_t i=0; i<attrib_count; i++)
    {
        switch(attribs[i])
        {
            case DM_MESH_VERTEX_ATTRIB_POSITION:
            pos_offset = vertex_size;
            vertex_size += 3;
            break;
            
            case DM_MESH_VERTEX_ATTRIB_NORMAL:
            norm_offset = vertex_size;
            vertex_size += 3;
            break;
            
            case DM_MESH_VERTEX_ATTRIB_TEXCOORD:
            tex_offset = vertex_size;
            vertex_size += 2;
            break;
            
            default:
            DM_LOG_ERROR("Invalid vertex attribute for obj format");
            return false;
        }
    }
    
    *vertices = dm_alloc(sizeof(float) * vertex_size * 3 * m->groups[0].face_count);
    switch(index_type)
    {
        case DM_MESH_INDEX_TYPE_UINT16:
        *indices  = dm_alloc(sizeof(uint16_t) * m->index_count);
        break;
        
        case DM_MESH_INDEX_TYPE_UINT32:
        *indices  = dm_alloc(sizeof(uint32_t) * m->index_count);
        break;
        
        default:
        dm_free(*vertices);
        fast_obj_destroy(m);
        return false;
    }
    
    fastObjIndex mi;
    
    *vertex_count = 0;
    // over each face
    for(uint32_t j=0; j<m->groups[0].face_count; j++)
    {
        const uint32_t fv = m->face_vertices[m->groups[0].face_offset + j];
        
        // over each vertex
        for(uint32_t k=0; k < fv; k++)
        {
            mi = m->indices[m->groups[0].index_offset + indx];
            
            if(pos_offset >= 0)
            {
                (*vertices)[indx * vertex_size + pos_offset + 0] = m->positions[3 * mi.p + 0];
                (*vertices)[indx * vertex_size + pos_offset + 1] = m->positions[3 * mi.p + 1];
                (*vertices)[indx * vertex_size + pos_offset + 2] = m->positions[3 * mi.p + 2];
            }
            
            if(norm_offset >= 0)
            {
                (*vertices)[indx * vertex_size + norm_offset + 0] = m->normals[3 * mi.n + 0];
                (*vertices)[indx * vertex_size + norm_offset + 1] = m->normals[3 * mi.n + 1];
                (*vertices)[indx * vertex_size + norm_offset + 2] = m->normals[3 * mi.n + 2];
            }
            
            if(tex_offset >= 0)
            {
                (*vertices)[indx * vertex_size + tex_offset + 0] = m->texcoords[2 * mi.t + 0];
                (*vertices)[indx * vertex_size + tex_offset + 1] = m->texcoords[2 * mi.t + 1];
            }
            
            switch(index_type)
            {
                case DM_MESH_INDEX_TYPE_UINT16:
                {
                    uint16_t* inds = *indices;
                    inds[indx] = index_offset + indx;
                } break;
                
                case DM_MESH_INDEX_TYPE_UINT32:
                {
                    uint32_t* inds = *indices;
                    inds[indx] = index_offset + indx;
                } break;
                
                default:
                return false;
            }
            indx++;
            (*vertex_count)++;
        }
    }
    
    assert(indx == m->index_count);
    assert(*vertex_count == m->groups[0].face_count * 3);
    
    *index_count = m->index_count;
    
    fast_obj_destroy(m);
    
    return true;
}

bool dm_renderer_load_gltf_model(const char* path, const dm_mesh_vertex_attrib* attribs, uint32_t attrib_count, dm_mesh_index_type index_type, float** vertices, void** indices, uint32_t* vertex_count, uint32_t* index_count, uint32_t index_offset)
{
    cgltf_options options = { 0 };
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success)
    {
        DM_LOG_FATAL("Could not load file: %s", path);
        cgltf_free(data);
        return false;
    }
    
    result = cgltf_validate(data);
    if(result!=cgltf_result_success) 
    {
        DM_LOG_FATAL("Validating gltf data failed: %s", path);
        cgltf_free(data);
        return false;
    }
    
    result = cgltf_load_buffers(&options, data, path);
    if(result!=cgltf_result_success)
    {
        DM_LOG_FATAL("Could not load gltf buffers: %s", path);
        cgltf_free(data);
        return false;
    }
    
    // only supporting single meshes at this point
    uint32_t mesh_count = data->meshes_count;
    assert(mesh_count==1);
    
    cgltf_mesh mesh = data->meshes[0];
    
    // only supporting triangles (faces) for now
    uint32_t primitive_count = mesh.primitives_count;
    assert(primitive_count==1);
    assert(mesh.primitives[0].type==cgltf_primitive_type_triangles);
    
    cgltf_primitive primitive = mesh.primitives[0];
    
    // only supporting up to 3 attribute types for now
    //assert(primitive.attributes_count<=3);
    
    // make sure that attributes all have the same number of counts
    for(uint32_t i=0; i<primitive.attributes_count; i++)
    {
        for(uint32_t j=i+1; j<primitive.attributes_count; j++)
        {
            assert(primitive.attributes[i].data->count==primitive.attributes[j].data->count);
        }
    }
    
    // we now know how many vertices we have
    const uint32_t count = primitive.attributes[0].data->count;
    
    // get attribute offsets into our vertices buffer
    int pos_offset, norm_offset, tex_offset;
    pos_offset = norm_offset = tex_offset = -1;
    
    size_t vertex_stride = 0;
    
    for(uint32_t i=0; i<attrib_count; i++)
    {
        switch(attribs[i])
        {
            case DM_MESH_VERTEX_ATTRIB_POSITION:
            pos_offset = vertex_stride;
            vertex_stride += 3;
            break;
            
            case DM_MESH_VERTEX_ATTRIB_NORMAL:
            norm_offset = vertex_stride;
            vertex_stride += 3;
            break;
            
            case DM_MESH_VERTEX_ATTRIB_TEXCOORD:
            tex_offset = vertex_stride;
            vertex_stride += 2;
            break;
            
            default:
            DM_LOG_ERROR("Invalid vertex attribute for obj format");
            return false;
        }
    }
    
    *vertices = dm_alloc(sizeof(float) * vertex_stride * count);
    
    // put data into our vertices buffer    
    cgltf_accessor*    accessor = NULL;
    cgltf_attribute    attribute;
    cgltf_buffer_view* buffer_view = NULL;
    float* buffer = NULL;
    
    size_t stride = 0;
    size_t size = 0;
    size_t offset = 0;
    
    for(uint32_t v=0; v<count; v++)
    {
        for(uint32_t i=0; i<primitive.attributes_count; i++)
        {
            attribute   = primitive.attributes[i];
            accessor    = attribute.data;
            buffer_view = accessor->buffer_view;
            buffer      = (float*)((char*)buffer_view->buffer->data + buffer_view->offset);
            stride      = accessor->stride / sizeof(float);
            
            switch(primitive.attributes[i].type)
            {
                case cgltf_attribute_type_position:
                if(pos_offset>=0) 
                {
                    size   = sizeof(float) * 3;
                    offset = pos_offset;
                } break;
                
                case cgltf_attribute_type_normal:
                if(norm_offset>=0) 
                {
                    size   = sizeof(float) * 3;
                    offset = norm_offset;
                } break;
                
                case cgltf_attribute_type_texcoord:
                if(tex_offset>=0)
                {
                    size   = sizeof(float) * 2;
                    offset = tex_offset;
                } break;
                
                default:
                continue;
            }
            
            dm_memcpy(*vertices + v * vertex_stride + offset, buffer + v * stride, size);
        }
    }
    
    // get indices
    size_t indices_size = primitive.indices->buffer_view->size;
    *indices = dm_alloc(indices_size);
    dm_memcpy(*indices, (char*)primitive.indices->buffer_view->buffer->data + primitive.indices->buffer_view->offset, indices_size);
    
    *vertex_count = count;
    *index_count = primitive.indices->buffer_view->size / sizeof(uint16_t);
    
    // cleanup
    cgltf_free(data);
    
    return true;
}

bool dm_renderer_load_model(const char* path, const dm_mesh_vertex_attrib* attribs, uint32_t attrib_count, dm_mesh_index_type index_type, float** vertices, void** indices, uint32_t* vertex_count, uint32_t* index_count, uint32_t index_offset, dm_context* context)
{
    const char* ext = strrchr(path, '.');
    ext++;
    
    if(strcmp(ext, "obj")==0) 
        return dm_renderer_load_obj_model(path, attribs, attrib_count, index_type, vertices, indices, vertex_count, index_count, index_offset);
    else if(strcmp(ext, "gltf")==0 || strcmp(ext, "glb")==0) 
        return dm_renderer_load_gltf_model(path, attribs, attrib_count, index_type, vertices, indices, vertex_count, index_count, index_offset);
    
    DM_LOG_FATAL("Unknown model extension");
    return false;
}

/***************
RENDER COMMANDS
*****************/
void __dm_renderer_submit_render_command(dm_render_command* command, dm_render_command_manager* manager)
{
    dm_render_command* c = &manager->commands[manager->command_count++];
    
    if(!c->params.data) c->params.data = dm_alloc(command->params.size);
    else if(c->params.size != command->params.size) c->params.data = dm_realloc(c->params.data, command->params.size);
    dm_memcpy(c->params.data, command->params.data, command->params.size);
    
    c->type = command->type;
    c->params.size = command->params.size;
    
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

void dm_render_command_set_scissor_rects(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, dm_context* context)
{
    if(DM_TOO_MANY_COMMANDS) return;
    
    dm_render_command command = { 0 };
    command.type = DM_RENDER_COMMAND_SET_SCISSOR_RECTS;
    
    DM_BYTE_POOL_INSERT(command.params, left);
    DM_BYTE_POOL_INSERT(command.params, right);
    DM_BYTE_POOL_INSERT(command.params, top);
    DM_BYTE_POOL_INSERT(command.params, bottom);
    
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
            
            case DM_RENDER_COMMAND_SET_SCISSOR_RECTS:
            {
                DM_BYTE_POOL_POP(command.params, uint32_t, bottom);
                DM_BYTE_POOL_POP(command.params, uint32_t, top);
                DM_BYTE_POOL_POP(command.params, uint32_t, right);
                DM_BYTE_POOL_POP(command.params, uint32_t, left);
                
                dm_render_command_backend_set_scissor_rects(left, right, top, bottom, &context->renderer);
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

/***
ECS
*****/
bool dm_ecs_init(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
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
    
    dm_ecs_system* system = NULL;
    for(i=0; i<DM_ECS_SYSTEM_TIMING_UNKNOWN; i++)
    {
        for(uint32_t j=0; j<ecs_manager->num_registered_systems[i]; j++)
        {
            system = &ecs_manager->systems[i][j];
            
            system->shutdown_func((void*)system,(void*)context);
            
            if(system->system_data) dm_free(system->system_data);
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

dm_ecs_id dm_ecs_register_system(dm_ecs_id* component_ids, uint32_t component_count, dm_ecs_system_timing timing,  bool (*run_func)(void*,void*), void (*shutdown_func)(void*,void*), void (*insert_func)(uint32_t,void*,void*), dm_context* context)
{
    if(context->ecs_manager.num_registered_systems[timing] >= DM_ECS_MAX) return DM_ECS_INVALID_ID;
    
    dm_ecs_id id = context->ecs_manager.num_registered_systems[timing]++;
    dm_ecs_system* system = &context->ecs_manager.systems[timing][id];
    
    for(uint32_t i=0; i<component_count; i++)
    {
        system->component_mask   |= DM_BIT_SHIFT(component_ids[i]);
        system->component_ids[i]  = component_ids[i];
    }
    
    system->component_count = component_count;
    
    system->run_func      = run_func;
    system->shutdown_func = shutdown_func;
    system->insert_func   = insert_func;
    
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
        context->ecs_manager.entity_count++;
        return;
    }
    
    uint32_t runner = index + 1;
    if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    
    while(runner != index)
    {
        if(entities[runner]==DM_ECS_INVALID_ENTITY) 
        {
            entities[runner] = entity;
            context->ecs_manager.entity_count++;
            return;
        }
        
        runner++;
        if(runner >= DM_ECS_MAX_ENTITIES) runner = 0;
    }
    
    DM_LOG_FATAL("Could not insert entity, should not be here...");
}

void dm_ecs_insert_entities_into_systems(dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    dm_ecs_system*  system = NULL;
    uint32_t        comp_id, comp_index;
    
    dm_entity entity = DM_ECS_INVALID_ENTITY;
    
    // for all entities
    for(uint32_t e=0; e<DM_ECS_MAX_ENTITIES; e++)
    {
        entity = ecs_manager->entities[e];
        if(entity == DM_ECS_INVALID_ENTITY) continue;
        
        //entity_index = dm_ecs_entity_get_index(entity, context);
        
        // for all system timings
        for(uint32_t t=0; t<DM_ECS_SYSTEM_TIMING_UNKNOWN; t++)
        {
            // for all systems in timing
            for(uint32_t s=0; s<ecs_manager->num_registered_systems[t]; s++)
            {
                system = &ecs_manager->systems[t][s];
                
                // early out if we don't have what this system needs
                if(!dm_ecs_entity_has_component_multiple_via_index(e, system->component_mask, context)) continue;
                
                // get all component indices
                for(uint32_t c=0; c<system->component_count; c++)
                {
                    comp_id    = system->component_ids[c];
                    comp_index = ecs_manager->entity_component_indices[e][comp_id];
                    
                    system->entity_indices[system->entity_count][comp_id] = comp_index;
                }
                
                system->insert_func(e, (void*)system, (void*)context);
                
                system->entity_count++;
            }
        }
    }
    
    ecs_manager->flags &= ~DM_ECS_FLAG_REINSERT_ENTITIES;
}

bool dm_ecs_run_systems(dm_ecs_system_timing timing, dm_context* context)
{
    dm_ecs_system* system = NULL;
    for(uint32_t i=0; i<context->ecs_manager.num_registered_systems[timing]; i++)
    {
        system = &context->ecs_manager.systems[timing][i];
        if(!system->run_func(system, context)) return false;
        
        // reset entity_count
        system->entity_count = 0;
    }
    
    return true;
}

dm_entity dm_ecs_entity_create(dm_context* context)
{
    dm_entity entity;
    
    while(true)
    {
        entity = dm_random_uint32(context);
        if(entity!=DM_ECS_INVALID_ENTITY) break;
    }
    
    dm_ecs_entity_insert(entity, context);
    
    return entity;
}

void dm_ecs_entity_destroy(dm_entity entity, dm_context* context)
{
    dm_ecs_manager* ecs_manager = &context->ecs_manager;
    
    uint32_t index = dm_ecs_entity_get_index(entity, context);
    
    // remove components
    for(uint32_t c=0; c<ecs_manager->num_registered_components; c++)
    {
        dm_ecs_entity_remove_component_via_index(index, c, context);
    }
    
    // entity array position set to invalid
    ecs_manager->entities[index] = DM_ECS_INVALID_ENTITY;
    // component indices set to invalid
    dm_memset(ecs_manager->entity_component_indices[index], DM_ECS_INVALID_ID, sizeof(uint32_t) * DM_ECS_MAX);
    // component mask reset to 0
    ecs_manager->entity_component_masks[index] = 0;
    
    ecs_manager->entity_count--;
}

void* dm_ecs_get_component_block(dm_ecs_id component_id, dm_context* context)
{
    if(component_id==DM_ECS_INVALID_ID) return NULL;
    
    return context->ecs_manager.components[component_id].data;
}

void dm_ecs_get_component_insert_index(dm_ecs_id component_id, uint32_t* index, dm_context* context)
{
    if(component_id==DM_ECS_INVALID_ID) return;
    
    dm_ecs_component* component = &context->ecs_manager.components[component_id];
    
    // if we have dead entities, use their indices.
    // otherwise iterate
    if(component->tombstone_count==0) *index = component->entity_count;
    else                              *index = component->tombstones[component->tombstone_count--];
}

void dm_ecs_entity_add_component(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    if(component_id==DM_ECS_INVALID_ID) return;
    if(dm_ecs_entity_has_component_via_index(entity_index, component_id, context)) return;
    
    uint32_t c_index = context->ecs_manager.components[component_id].entity_count;
    
    context->ecs_manager.entity_component_indices[entity_index][component_id] = c_index;
    context->ecs_manager.entity_component_masks[entity_index] |= DM_BIT_SHIFT(component_id);
    context->ecs_manager.components[component_id].entity_count++;
    
    context->ecs_manager.flags |= DM_ECS_FLAG_REINSERT_ENTITIES;
}

void dm_ecs_entity_remove_component(dm_entity entity, dm_ecs_id component_id, dm_context* context)
{
    uint32_t entity_index = dm_ecs_entity_get_index(entity, context);
    
    dm_ecs_entity_remove_component_via_index(entity_index, component_id, context);
}

void dm_ecs_entity_remove_component_via_index(uint32_t index, dm_ecs_id component_id, dm_context* context)
{
    if(!dm_ecs_entity_has_component_via_index(index, component_id, context)) return;
    
    dm_ecs_component* component = &context->ecs_manager.components[component_id];
    component->tombstones[component->tombstone_count++] = index;
    
    context->ecs_manager.flags |= DM_ECS_FLAG_REINSERT_ENTITIES;
}

/*********
THREADING
***********/
extern bool dm_platform_threadpool_create(dm_threadpool* threadpool);
extern void dm_platform_threadpool_destroy(dm_threadpool* threadpool);
extern void dm_platform_threadpool_submit_task(dm_thread_task* task, dm_threadpool* threadpool);
extern void dm_platform_threadpool_wait_for_completion(dm_threadpool* threadpool);
extern void dm_platform_clipboard_copy(const char* text, int len);
extern void dm_platform_clipboard_paste(void (*callback)(char*,int,void*), void* edit);

bool dm_threadpool_create(const char* tag, uint32_t num_threads, dm_threadpool* threadpool)
{
    strcpy(threadpool->tag, tag);
    
    // want at least one thread
    num_threads = DM_CLAMP(num_threads, 1, DM_MAX_THREAD_COUNT);
    threadpool->thread_count = num_threads;
    
    return dm_platform_threadpool_create(threadpool);
}

void dm_threadpool_destroy(dm_threadpool* threadpool)
{
    dm_platform_threadpool_destroy(threadpool);
}

void dm_threadpool_submit_task(dm_thread_task* task, dm_threadpool* threadpool)
{
    dm_platform_threadpool_submit_task(task, threadpool);
}

void dm_threadpool_wait_for_completion(dm_threadpool* threadpool)
{
    dm_platform_threadpool_wait_for_completion(threadpool);
}

/*****
IMGUI
*******/
extern bool dm_imgui_init(dm_context* context);
extern void dm_imgui_shutdown(dm_context* context);
extern void dm_imgui_render(dm_context* context);

extern void dm_imgui_input_begin(dm_context* context);
extern void dm_imgui_input_end(dm_context* context);
extern void dm_imgui_input_event(dm_event e, dm_context* context);

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
    
    if(!dm_platform_init(init_packet.window_x, init_packet.window_y, context))
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
    
    context->renderer.width  = init_packet.window_width;
    context->renderer.height = init_packet.window_height;
    context->renderer.vsync  = init_packet.vsync;
    
    // ecs
    dm_ecs_init(context);
    
    // random init
    init_genrand(&context->random, (uint32_t)dm_platform_get_time(&context->platform_data));
    init_genrand64(&context->random_64, (uint64_t)dm_platform_get_time(&context->platform_data));
    
    // misc
    context->delta = 1.0f / DM_DEFAULT_MAX_FPS;
    context->flags |= DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
    
    // nuklear
    if(!dm_imgui_init(context)) return false;
    
    return context;
}

void dm_shutdown(dm_context* context)
{
    for(uint32_t i=0; i<DM_MAX_RENDER_COMMANDS; i++)
    {
        if(context->renderer.command_manager.commands[i].params.data) dm_free(context->renderer.command_manager.commands[i].params.data);
    }
    
    dm_imgui_shutdown(context);
    
    dm_renderer_shutdown(context);
    dm_platform_shutdown(&context->platform_data);
    dm_ecs_shutdown(context);
    
    dm_free(context);
}

// also pass through nuklear inputs
void dm_poll_events(dm_context* context)
{
    dm_imgui_input_begin(context);
    
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
            if(e.key == DM_KEY_ESCAPE)
            {
                dm_event* e2 = &context->platform_data.event_list.events[context->platform_data.event_list.num++];
                e2->type = DM_EVENT_WINDOW_CLOSE;
            }
            if(!dm_input_is_key_pressed(e.key, context)) dm_input_set_key_pressed(e.key, context);
            break;
            case DM_EVENT_KEY_UP:
            if(dm_input_is_key_pressed(e.key, context)) dm_input_set_key_released(e.key, context);
            break;
            
            case DM_EVENT_MOUSEBUTTON_DOWN:
            if(!dm_input_is_mousebutton_pressed(e.button, context)) dm_input_set_mousebutton_pressed(e.button, context);
            break;
            
            case DM_EVENT_MOUSEBUTTON_UP:
            if(dm_input_is_mousebutton_pressed(e.button, context)) 
            {
                if(e.button==DM_MOUSEBUTTON_L) dm_input_set_mousebutton_released(DM_MOUSEBUTTON_DOUBLE, context);
                dm_input_set_mousebutton_released(e.button, context);
            } break;
            
            case DM_EVENT_MOUSE_MOVE:
            dm_input_set_mouse_x(e.coords[0], context);
            dm_input_set_mouse_y(e.coords[1], context);
            break;
            
            case DM_EVENT_MOUSE_SCROLL:
            dm_input_set_mouse_scroll(e.delta, context);
            break;
            
            case DM_EVENT_WINDOW_RESIZE:
            context->platform_data.window_data.width  = e.new_rect[0];
            context->platform_data.window_data.height = e.new_rect[1];
            
            context->renderer.width = e.new_rect[0];
            context->renderer.height = e.new_rect[1];
            
            dm_renderer_backend_resize(e.new_rect[0], e.new_rect[1], &context->renderer);
            break;
            
            case DM_EVENT_UNKNOWN:
            DM_LOG_ERROR("Unknown event! Shouldn't be here...");
            break;
        }
        
        dm_imgui_input_event(e, context);
    }
    
    dm_imgui_input_end(context);
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
    dm_ecs_insert_entities_into_systems(context);
    
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
    
    // imgui
    dm_imgui_render(context);
    
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

#ifdef DM_MATH_TESTS
void dm_math_tests();
#endif

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
#ifdef DM_MATH_TESTS
    dm_math_tests();
#endif
    
    dm_context_init_packet init_packet = {
        DM_DEFAULT_SCREEN_X, DM_DEFAULT_SCREEN_Y,
        DM_DEFAULT_SCREEN_WIDTH, DM_DEFAULT_SCREEN_HEIGHT,
        DM_DEFAULT_TITLE,
        DM_DEFAULT_ASSETS_FOLDER
    };
    
    dm_application_setup(&init_packet);
    
    dm_context* context = dm_init(init_packet);
    if(!context) 
    {
        getchar();
        return DM_EXIT_CODE_INIT_FAIL;
    }
    
    if(!dm_application_init(context)) 
    {
        DM_LOG_FATAL("Application init failed");
        
        dm_shutdown(context);
        getchar();
        return DM_EXIT_CODE_INIT_FAIL;
    }
    
    while(dm_context_is_running(context))
    {
        dm_start(context);
        
        // updating
        if(!dm_update_begin(context)) break;
        if(!dm_application_update(context))
        {
            DM_LOG_FATAL("Application update failed");
            break;
        }
        if(!dm_update_end(context)) break;
        
        // rendering
        if(dm_renderer_begin_frame(context))
        {
            if(!dm_application_render(context))
            {
                DM_LOG_FATAL("Application render failed");
                
                break;
            }
            if(!dm_renderer_end_frame(context)) break;
        }
        
        dm_end(context);
    }
    
    dm_application_shutdown(context);
    dm_shutdown(context);
    
    getchar();
    
    return DM_EXIT_CODE_SUCCESS;
}

#ifdef DM_MATH_TESTS
#define DM_MATH_ASSERT(COND, MESSAGE) if(!(COND)) { DM_LOG_FATAL(MESSAGE); assert(false); }
#define DM_MATH_CLOSE_ENOUGH(X,Y) (dm_fabs((X)-(Y)) < 0.001f)

void dm_vec3_tests()
{
    const dm_vec3 v1 = { 1,1,1 };
    const dm_vec3 v2 = { 2,2,2 };
    const dm_vec3 v3 = { 0.1f,0.1f,0.1f };
    const dm_vec3 v4 = { -0.1f,-0.1f,-0.1f };
    
    DM_MATH_ASSERT(dm_vec3_dot(v1,v2)==6, "Vec3 dot product failed");
    DM_MATH_ASSERT(DM_MATH_CLOSE_ENOUGH(dm_vec3_dot(v2,v3),0.6f), "Vec3 dot product failed");
    DM_MATH_ASSERT(DM_MATH_CLOSE_ENOUGH(dm_vec3_dot(v3,v4),-0.03f), "Vec3 dot product failed");
    
    dm_vec3 result;
    dm_vec3_sub_vec3(v1,v2, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],-1.0f)) && (DM_MATH_CLOSE_ENOUGH(result[1],-1.0f)) && (DM_MATH_CLOSE_ENOUGH(result[2],-1.0f)), "Vec3 sub vec3 failed");
    
    dm_vec3_sub_vec3(v2,v3, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],1.9f)) && (DM_MATH_CLOSE_ENOUGH(result[1],1.9f)) && (DM_MATH_CLOSE_ENOUGH(result[2],1.9f)), "Vec3 sub vec3 failed");
    
    dm_vec3_add_vec3(v1,v2, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],3)) && (DM_MATH_CLOSE_ENOUGH(result[1],3)) && (DM_MATH_CLOSE_ENOUGH(result[2],3)), "Vec3 sub vec3 failed");
    
    dm_vec3_add_vec3(v2,v3, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],2.1f)) && (DM_MATH_CLOSE_ENOUGH(result[1],2.1f)) && (DM_MATH_CLOSE_ENOUGH(result[2],2.1f)), "Vec3 sub vec3 failed");
    
    dm_vec3_norm(v1,result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],DM_MATH_INV_SQRT3)) && (DM_MATH_CLOSE_ENOUGH(result[1],DM_MATH_INV_SQRT3)) && (DM_MATH_CLOSE_ENOUGH(result[2],DM_MATH_INV_SQRT3)), "Vec3 norm failed");
    
    dm_vec3_scale(v1, 4, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],4)) && (DM_MATH_CLOSE_ENOUGH(result[1],4)) && (DM_MATH_CLOSE_ENOUGH(result[2],4)), "Vec3 scale failed");
}

void dm_vec4_tests()
{
    const dm_vec4 v1 = { 1,1,1,1 };
    const dm_vec4 v2 = { 2,2,2,2 };
    const dm_vec4 v3 = { 0.1f,0.1f,0.1f,0.1f };
    const dm_vec4 v4 = { -0.1f,-0.1f,-0.1f,-0.1f };
    
    float t = dm_vec4_dot(v1,v2);
    DM_MATH_ASSERT(t==8, "Vec4 dot product failed");
    
    t = dm_vec4_dot(v2,v3);
    DM_MATH_ASSERT(DM_MATH_CLOSE_ENOUGH(t,0.8f), "Vec4 dot product failed");
    
    t = dm_vec4_dot(v3,v4);
    DM_MATH_ASSERT(DM_MATH_CLOSE_ENOUGH(t,-0.04f), "Vec4 dot product failed");
    
    dm_vec4 result;
    dm_vec4_sub_vec4(v1,v2, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],-1.0f)) && (DM_MATH_CLOSE_ENOUGH(result[1],-1.0f)) && (DM_MATH_CLOSE_ENOUGH(result[2],-1.0f)) && (DM_MATH_CLOSE_ENOUGH(result[3],-1.0f)), "Vec4 sub vec4 failed");
    
    dm_vec4_sub_vec4(v2,v3, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],1.9f)) && (DM_MATH_CLOSE_ENOUGH(result[1],1.9f)) && (DM_MATH_CLOSE_ENOUGH(result[2],1.9f)) && (DM_MATH_CLOSE_ENOUGH(result[3],1.9f)), "Vec4 sub vec4 failed");
    
    dm_vec4_add_vec4(v1,v2, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],3)) && (DM_MATH_CLOSE_ENOUGH(result[1],3)) && (DM_MATH_CLOSE_ENOUGH(result[2],3)) && (DM_MATH_CLOSE_ENOUGH(result[3],3)), "Vec4 sub vec4 failed");
    
    dm_vec4_add_vec4(v2,v3, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],2.1f)) && (DM_MATH_CLOSE_ENOUGH(result[1],2.1f)) && (DM_MATH_CLOSE_ENOUGH(result[2],2.1f)) && (DM_MATH_CLOSE_ENOUGH(result[3],2.1f)), "Vec4 sub vec4 failed");
    
    dm_vec4_norm(v1,result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],0.5f)) && (DM_MATH_CLOSE_ENOUGH(result[1],0.5f)) && (DM_MATH_CLOSE_ENOUGH(result[2],0.5f)) && (DM_MATH_CLOSE_ENOUGH(result[3],0.5f)), "Vec4 norm failed");
    
    dm_vec4_scale(v1,4,result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],4)) && (DM_MATH_CLOSE_ENOUGH(result[1],4)) && (DM_MATH_CLOSE_ENOUGH(result[2],4)) && (DM_MATH_CLOSE_ENOUGH(result[3],4)), "Vec4 norm failed");
}

void dm_mat4_tests()
{
    const dm_mat4 m1 = {
        {1,2,3,4},
        {1,2,3,4},
        {1,2,3,4},
        {1,2,3,4}
    };
    
    const dm_vec4 v1 = {
        2,2,2,2
    };
    
    dm_vec4 result;
    dm_mat4_mul_vec4(m1,v1, result);
    DM_MATH_ASSERT((DM_MATH_CLOSE_ENOUGH(result[0],20.0f)) && (DM_MATH_CLOSE_ENOUGH(result[1],20.0f)) && (DM_MATH_CLOSE_ENOUGH(result[2],20.0f)) && (DM_MATH_CLOSE_ENOUGH(result[3],20.0f)), "Mat4 mul vec4 failed");
    
    const dm_mat4 m2 = {
        { 1,0,10,0 },
        { 0,1,0,0  },
        { 0,0,1,0  },
        { 0,10,0,1 }
    };
    const dm_mat4 m3 = {
        { 1,0,-10,0 },
        { 0,1,0,0  },
        { 0,0,1,0  },
        { 0,-10,0,1 }
    };
    
    dm_mat4 r;
    dm_mat4_inverse(m2,r);
    DM_MATH_ASSERT(dm_mat4_is_equal(m3,r), "Mat4 inverse failed");
}

void dm_math_tests()
{
    dm_vec3_tests();
    dm_vec4_tests();
    dm_mat4_tests();
}
#endif
