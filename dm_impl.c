#include "dm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <assert.h>

#include "mt19937/mt19937.h"
#include "mt19937/mt19937_64.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

//#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobjloader-c/tinyobj_loader_c.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"

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

void dm_free(void** block)
{
    free(*block);
    *block = NULL;
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
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_WINDOW_CLOSE;
}

void dm_add_window_resize_event(uint32_t new_width, uint32_t new_height, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_WINDOW_RESIZE;
    e->new_rect[0] = new_width;
    e->new_rect[1] = new_height;
}

void dm_add_mousebutton_down_event(dm_mousebutton_code button, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSEBUTTON_DOWN;
    e->button = button;
}

void dm_add_mousebutton_up_event(dm_mousebutton_code button, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSEBUTTON_UP;
    e->button = button;
}

void dm_add_mouse_move_event(uint32_t mouse_x, uint32_t mouse_y, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSE_MOVE;
    
    e->coords[0] = mouse_x;
    e->coords[1] = mouse_y;
}

void dm_add_mouse_scroll_event(float delta, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_MOUSE_SCROLL;
    e->delta = delta;
}

void dm_add_key_down_event(dm_key_code key, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

    dm_event* e = &event_list->events[event_list->num++];
    e->type = DM_EVENT_KEY_DOWN;
    e->key = key;
}

void dm_add_key_up_event(dm_key_code key, dm_event_list* event_list)
{
    if(event_list->num>=DM_MAX_EVENTS_PER_FRAME) return;

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

/**********
 * LOGGING
************/
void dm_strcpy(char* dest, const char* src)
{
    strcpy(dest, src);
}

/*********
RENDERING
***********/
extern bool dm_renderer_backend_init(dm_context* context);
extern bool dm_renderer_backend_finish_init(dm_context* context);
extern void dm_renderer_backend_shutdown(dm_context* context);
extern bool dm_renderer_backend_begin_frame(dm_renderer* renderer);
extern bool dm_renderer_backend_end_frame(dm_context* context);
extern bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer);
extern bool dm_renderer_backend_begin_update(dm_renderer* renderer);
extern bool dm_renderer_backend_end_update(dm_renderer* renderer);

bool dm_renderer_init(dm_context* context)
{
    if(!dm_renderer_backend_init(context)) { DM_LOG_FATAL("Could not initialize renderer backend"); return false; }
    
    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_renderer_backend_shutdown(context);
}

// render resources
extern bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_blas(dm_blas_desc desc, dm_resource_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_tlas(dm_tlas_desc desc, dm_resource_handle* handle, dm_renderer* renderer);

extern bool dm_renderer_backend_get_blas_gpu_address(dm_resource_handle blas, size_t* address, dm_renderer* renderer);

bool dm_renderer_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_RASTER_PIPELINE;

    if(dm_renderer_backend_create_raster_pipeline(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Could not create raster pipeline");
    return false;
}

bool dm_renderer_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    if(dm_renderer_backend_create_vertex_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating vertex buffer failed");
    return false;
}

bool dm_renderer_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    if(dm_renderer_backend_create_index_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating index buffer failed");
    return false;
}

bool dm_renderer_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    if(dm_renderer_backend_create_constant_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating constant buffer failed");
    return false;
}

bool dm_renderer_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    if(dm_renderer_backend_create_texture(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating texture failed");
    return false;
}

bool dm_renderer_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_SAMPLER;

    if(dm_renderer_backend_create_sampler(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating sampler failed");
    return false;
}

bool dm_renderer_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    if(dm_renderer_backend_create_storage_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating storage buffer failed");
    return false;
}

bool dm_renderer_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_RAYTRACING_PIPELINE;

    if(dm_renderer_backend_create_raytracing_pipeline(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating raytracing pipeline failed");
    return false;
}

bool dm_renderer_create_blas(dm_blas_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_BLAS;

    if(dm_renderer_backend_create_blas(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating blas failed");
    return false;
}

bool dm_renderer_create_tlas(dm_tlas_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_TLAS;

    if(dm_renderer_backend_create_tlas(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating tlas failed");
    return false;
}

bool dm_renderer_get_blas_gpu_address(dm_resource_handle handle, size_t* address, dm_context* context)
{
    if(handle.type != DM_RESOURCE_TYPE_BLAS)
    {
        DM_LOG_FATAL("Resource is not a bottom-level acceleration structure");
        return false;
    }

    return dm_renderer_backend_get_blas_gpu_address(handle, address, &context->renderer);
}

/***************
RENDER COMMANDS
*****************/
extern bool dm_render_command_backend_begin_render_pass(float r, float g, float b, float a, dm_renderer* renderer);
extern bool dm_render_command_backend_end_render_pass(dm_renderer* renderer);
extern bool dm_render_command_backend_bind_raster_pipeline(dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_raytracing_pipeline(dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_index_buffer(dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_update_index_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer);
extern bool dm_render_command_backend_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer);
extern bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_update_storage_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_resize_texture(uint32_t width, uint32_t height, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_update_tlas(uint32_t instance_count, dm_resource_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_dispatch_rays(uint16_t x, uint16_t y, dm_resource_handle pipeline, dm_renderer* renderer);

void _dm_render_command_submit(dm_command command, dm_command_manager* manager)
{
    if(!manager->commands)
    {
        manager->capacity = 16;
        manager->commands = dm_alloc(sizeof(dm_command) * manager->capacity);
    }

    manager->commands[manager->count++] = command;

    if((float)manager->count / (float)manager->capacity > 0.75f)
    {
        manager->capacity *= 2;
        manager->commands = dm_realloc(manager->commands, sizeof(dm_command) * manager->capacity);
    }
}

#define DM_RENDER_COMMAND_SUBMIT _dm_render_command_submit(command, &context->renderer.render_command_manager)

void dm_render_command_begin_render_pass(float r, float g, float b, float a, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS;

    command.params[0].float_val = r;
    command.params[1].float_val = g;
    command.params[2].float_val = b;
    command.params[3].float_val = a;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_end_render_pass(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_RENDER_PASS;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_raster_pipeline(dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };
    
    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE;

    command.params[0].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_raytracing_pipeline(dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_RAYTRACING_PIPELINE;

    command.params[0].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_SET_ROOT_CONSTANTS;

    command.params[0].u8_val     = slot;
    command.params[1].u32_val    = count;
    command.params[2].size_t_val = offset;
    command.params[3].void_val   = data;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;

    command.params[0].handle_val = handle;
    command.params[1].u8_val     = slot;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_index_buffer(dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;

    command.params[0].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_vertex_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER;

    command.params[0].void_val   = data;
    command.params[1].size_t_val = size;
    command.params[2].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_index_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER;

    command.params[0].void_val   = data;
    command.params[1].size_t_val = size;
    command.params[2].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER;

    command.params[0].void_val   = data;
    command.params[1].size_t_val = size;
    command.params[2].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_storage_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER;

    command.params[0].void_val   = data;
    command.params[1].size_t_val = size;
    command.params[2].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_resize_texture(uint32_t width, uint32_t height, dm_resource_handle handle, dm_context *context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_RESIZE_TEXTURE;

    command.params[0].u32_val    = width;
    command.params[1].u32_val    = height;
    command.params[2].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_tlas(uint32_t instance_count, dm_resource_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_TLAS;

    command.params[0].u32_val    = instance_count;
    command.params[1].handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_dispatch_rays(uint16_t x, uint16_t y, dm_resource_handle pipeline, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DISPATCH_RAYS;

    command.params[0].u16_val    = x;
    command.params[1].u16_val    = y;
    command.params[2].handle_val = pipeline;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED;

    command.params[0].u32_val = instance_count;
    command.params[1].u32_val = instance_offset;
    command.params[2].u32_val = vertex_count;
    command.params[3].u32_val = vertex_offset;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED;

    command.params[0].u32_val = instance_count;
    command.params[1].u32_val = instance_offset;
    command.params[2].u32_val = index_count;
    command.params[3].u32_val = index_offset;
    command.params[4].u32_val = vertex_offset;

    DM_RENDER_COMMAND_SUBMIT;
}

bool dm_renderer_submit_commands(dm_context* context)
{
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    
    dm_renderer* renderer = &context->renderer;
    
    for(uint32_t i=0; i<context->renderer.render_command_manager.count && dm_context_is_running(context); i++)
    {
        dm_command command = context->renderer.render_command_manager.commands[i];
        dm_command_param* params = command.params;
        
        switch(command.r_type)
        {
            case DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS:
            if(dm_render_command_backend_begin_render_pass(params[0].float_val, params[1].float_val, params[2].float_val, params[3].float_val, renderer)) continue;
            DM_LOG_FATAL("Begin renderpass failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_END_RENDER_PASS:
            if(dm_render_command_backend_end_render_pass(renderer)) continue;
            DM_LOG_FATAL("End renderpass failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE:
            if(params[0].handle_val.type != DM_RESOURCE_TYPE_RASTER_PIPELINE) { DM_LOG_FATAL("Resource is not a raster pipeline."); return false; }
            if(dm_render_command_backend_bind_raster_pipeline(params[0].handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind raster pipeline failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_BIND_RAYTRACING_PIPELINE:
            if(params[0].handle_val.type != DM_RESOURCE_TYPE_RAYTRACING_PIPELINE) { DM_LOG_FATAL("Resource is not a raytracing pipeline"); return false; }
            if(dm_render_command_backend_bind_raytracing_pipeline(params[0].handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind raytracig pipeline failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_SET_ROOT_CONSTANTS:
            if(dm_render_command_backend_set_root_constants(params[0].u8_val, params[1].u32_val, params[2].size_t_val, params[3].void_val, renderer)) continue;
            DM_LOG_FATAL("Set root constants failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_CONSTANT_BUFFER) { DM_LOG_FATAL("Resource is not a constant buffer."); return false; }
            if(dm_render_command_backend_update_constant_buffer(params[0].void_val, params[1].size_t_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Update constant buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_STORAGE_BUFFER) { DM_LOG_FATAL("Resource is not a storage buffer"); return false; }
            if(dm_render_command_backend_update_storage_buffer(params[0].void_val, params[1].size_t_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Update storage buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER:
            if(params[0].handle_val.type != DM_RESOURCE_TYPE_VERTEX_BUFFER) { DM_LOG_FATAL("Resource is not a vertex buffer"); return false; }
            if(dm_render_command_backend_bind_vertex_buffer(params[0].handle_val, params[1].u8_val, renderer)) continue;
            DM_LOG_FATAL("Bind vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_VERTEX_BUFFER) { DM_LOG_FATAL("Resource is not a vertex buffer."); return false; }
            if(dm_render_command_backend_update_vertex_buffer(params[0].void_val, params[1].size_t_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Update vertex buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER:
            if(params[0].handle_val.type != DM_RESOURCE_TYPE_INDEX_BUFFER) { DM_LOG_FATAL("Resource is not an index buffer."); return false; }
            if(dm_render_command_backend_bind_index_buffer(params[0].handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind index buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_INDEX_BUFFER) { DM_LOG_FATAL("Resource is not an index buffer."); return false; }
            if(dm_render_command_backend_update_index_buffer(params[0].void_val, params[1].size_t_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Update vertex buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_RESIZE_TEXTURE:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_TEXTURE) { DM_LOG_FATAL("Resource is not a texture"); return false; }
            if(dm_render_command_backend_resize_texture(params[0].u32_val, params[1].u32_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Resize texture failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_TLAS:
            if(params[1].handle_val.type != DM_RESOURCE_TYPE_TLAS) { DM_LOG_FATAL("Resource is not a top level acceleration structure"); return false; }
            if(dm_render_command_backend_update_tlas(params[0].u32_val, params[1].handle_val, renderer)) continue;
            DM_LOG_FATAL("Update acceleration structure failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DISPATCH_RAYS:
            if(params[2].handle_val.type != DM_RESOURCE_TYPE_RAYTRACING_PIPELINE) { DM_LOG_FATAL("Resource is not a raytracing pipeline"); return false; }
            if(dm_render_command_backend_dispatch_rays(params[0].u16_val, params[1].u16_val, params[2].handle_val, renderer)) continue;
            DM_LOG_FATAL("Dispatch rays failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED:
            if(dm_render_command_backend_draw_instanced(params[0].u32_val, params[1].u32_val, params[2].u32_val, params[3].u32_val, renderer)) continue;
            DM_LOG_FATAL("Draw instanced failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED:
            if(dm_render_command_backend_draw_instanced_indexed(params[0].u32_val, params[1].u32_val, params[2].u32_val, params[3].u32_val, params[5].u32_val, renderer)) continue;
            DM_LOG_FATAL("Draw instanced indexed failed");
            return false;

            default:
            DM_LOG_ERROR("Unknown render command! Shouldn't be here...");
            // TODO: do we kill here? Probably not, just ignore...
            //return false;
            break;
        }
    }
    
    return true;
    dm_font font = { 0 };
}

/**********
 * COMPUTE
 ***********/
extern bool dm_compute_backend_create_compute_pipeline(dm_compute_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer);

bool dm_compute_create_compute_pipeline(dm_compute_pipeline_desc desc, dm_resource_handle* handle, dm_context* context)
{
    handle->type = DM_RESOURCE_TYPE_COMPUTE_PIPELINE;

    if(dm_compute_backend_create_compute_pipeline(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating compute pipeline failed");
    return false;
}

extern bool dm_compute_command_backend_begin_recording(dm_renderer* renderer);
extern bool dm_compute_command_backend_end_recording(dm_renderer* renderer);
extern void dm_compute_command_backend_bind_compute_pipeline(dm_resource_handle handle, dm_renderer* renderer);
extern void dm_compute_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer);
extern void dm_compute_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer);
extern void dm_compute_command_backend_dispatch(const uint16_t x, const uint16_t y, const uint16_t z, dm_renderer* renderer);
 
bool dm_compute_command_begin_recording(dm_context* context)
{
    return dm_compute_command_backend_begin_recording(&context->renderer);
}

bool dm_compute_command_end_recording(dm_context* context)
{
    return dm_compute_command_backend_end_recording(&context->renderer);
}

void dm_compute_command_bind_compute_pipeline(dm_resource_handle handle, dm_context* context)
{
    dm_compute_command_backend_bind_compute_pipeline(handle, &context->renderer);
}

void dm_compute_command_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_context* context)
{
    dm_compute_command_backend_update_constant_buffer(data, size, handle, &context->renderer);
}

void dm_compute_command_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_context* context)
{   
    dm_compute_command_backend_set_root_constants(slot, count, offset, data, &context->renderer);
}

void dm_compute_command_dispatch(const uint16_t x, const uint16_t y, const uint16_t z, dm_context* context)
{
    dm_compute_command_backend_dispatch(x,y,z, &context->renderer);
}

/***************
 * FONT 
 ****************/
bool dm_renderer_load_font(dm_font_desc font_desc, dm_resource_handle sampler, dm_font* font, dm_context* context)
{
    DM_LOG_INFO("Loading font: %s", font_desc.path);
    
    size_t size = 0;
    void* buffer = dm_read_bytes(font_desc.path, "rb", &size);
    
    if(!buffer) 
    {
        DM_LOG_FATAL("Font file '%s' not found");
        return false;
    }
    
    stbtt_fontinfo info;
    if(!stbtt_InitFont(&info, buffer, 0))
    {
        DM_LOG_FATAL("Could not initialize font: %s", font_desc.path);
        return false;
    }
    
    uint16_t w = 512;
    uint16_t h = 512;
    uint8_t n_channels = 4;
    
    unsigned char* alpha_bitmap = dm_alloc(w * h);
    unsigned char* bitmap = dm_alloc(w * h * n_channels);
    dm_memzero(alpha_bitmap, w * h);
    dm_memzero(bitmap, w * h * n_channels);
    
    stbtt_BakeFontBitmap(buffer, 0, font_desc.size, alpha_bitmap, w,h, 32, 96, (stbtt_bakedchar*)font->glyphs);
    
    // bitmaps are single alpha values, so make 4 channel texture
    for(uint16_t y=0; y<h; y++)
    {
        for(uint16_t x=0; x<w; x++)
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

    bitmap[0] = 1.f;
    
    dm_texture_desc desc = { 0 };
    desc.width      = w;
    desc.height     = h;
    desc.n_channels = n_channels;
    desc.data       = bitmap;
    desc.format     = DM_TEXTURE_FORMAT_BYTE_4_UNORM;
    desc.sampler    = sampler;

    if(!dm_renderer_create_texture(desc, &font->texture_handle, context))
    { 
        DM_LOG_FATAL("Could not create texture for font: %s"); 
        return false; 
    }

    dm_free((void**)&alpha_bitmap);
    dm_free((void**)&bitmap);
    dm_free(&buffer);

    return true;
}

dm_font_aligned_quad dm_font_get_aligned_quad(dm_font font, const char text, float* xf, float* yf)
{
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad((stbtt_bakedchar*)font.glyphs, 512,512, text-32, xf,yf, &q, 1);

    return *(dm_font_aligned_quad*)&q;
}

/********
 * MESH *
 ********/
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_renderer_gltf_load_material(const char* directory, dm_material_type type, cgltf_material material, dm_scene* scene, dm_context* context)
{
    cgltf_texture* texture;
    switch(type)
    {
        case DM_MATERIAL_TYPE_ALBEDO:
        texture = material.pbr_metallic_roughness.base_color_texture.texture;
        break;

        case DM_MATERIAL_TYPE_METALLIC_ROUGHNESS:
        if(!material.pbr_metallic_roughness.metallic_roughness_texture.texture) return true;
        texture = material.pbr_metallic_roughness.metallic_roughness_texture.texture;
        break;

        case DM_MATERIAL_TYPE_NORMAL_MAP:
        if(!material.normal_texture.texture) return true;
        texture = material.normal_texture.texture;
        break;

        case DM_MATERIAL_TYPE_EMISSION:
        if(!material.emissive_texture.texture) return true;
        texture = material.emissive_texture.texture;
        break;

        default:
        return true;
    }

    dm_material* m = &scene->materials[scene->material_count];

    int width, height, n_channels;

    void* image_data = NULL;
    if(texture->image->buffer_view)
    {
        void* src = texture->image->buffer_view->buffer->data + texture->image->buffer_view->offset;
        size_t size = texture->image->buffer_view->size; 
        image_data = stbi_load_from_memory(src, size, &width, &height, &n_channels, 4);
    }
    else
    {
        char full_path[512];
        sprintf(full_path, "%s/%s", directory, texture->image->uri);
        size_t size;
        void* temp = dm_read_bytes(full_path, "rb", &size);
        if(!temp) return false;
        image_data = stbi_load_from_memory(temp, size, &width, &height, &n_channels, 4);
    }

    dm_sampler_desc sampler_desc = { 0 };
    if(texture->sampler)
    {
        switch(texture->sampler->wrap_s)
        {
            case cgltf_wrap_mode_clamp_to_edge:
            sampler_desc.address_u = DM_SAMPLER_ADDRESS_MODE_BORDER;
            break;

            case cgltf_wrap_mode_repeat:
            sampler_desc.address_u = DM_SAMPLER_ADDRESS_MODE_WRAP;
            break;

            default:
            return false;
        }

        switch(texture->sampler->wrap_t)
        {
            case cgltf_wrap_mode_clamp_to_edge:
            sampler_desc.address_v = DM_SAMPLER_ADDRESS_MODE_BORDER;
            break;

            case cgltf_wrap_mode_repeat:
            sampler_desc.address_v = DM_SAMPLER_ADDRESS_MODE_WRAP;
            break;

            default:
            return false;
        }
        sampler_desc.address_w = sampler_desc.address_u;
    }
    else
    {
        sampler_desc.address_u = DM_SAMPLER_ADDRESS_MODE_WRAP;
        sampler_desc.address_v = DM_SAMPLER_ADDRESS_MODE_WRAP;
        sampler_desc.address_w = DM_SAMPLER_ADDRESS_MODE_WRAP;
    }

    if(!dm_renderer_create_sampler(sampler_desc, &scene->materials[scene->material_count].samplers[type], context)) return false;

    dm_texture_desc desc = { 0 };
    desc.width      = width;
    desc.height     = height;
    desc.n_channels = 4;
    desc.format     = DM_TEXTURE_FORMAT_BYTE_4_UNORM; 
    desc.sampler    = m->samplers[type]; 
    desc.data       = image_data;

    if(!dm_renderer_create_texture(desc, &m->textures[type], context)) 
    {
        stbi_image_free(image_data);
        return false;
    }

    stbi_image_free(image_data);

    return true;
}

bool dm_renderer_load_gltf_mesh(cgltf_data* data, uint8_t mesh_index, dm_mesh_vertex_attribute* mesh_attributes, uint8_t attribute_count, dm_scene* scene, dm_context* context)
{
    if(mesh_index>data->meshes_count)
    {
        DM_LOG_FATAL("Trying to access an invalid mesh from gltf model");
        return false;
    }

    dm_mesh* mesh = &scene->meshes[scene->mesh_count];

    // begin
    float* vertex_data = NULL;
    void*  index_data  = NULL;

    size_t vertex_size = 0;
    size_t vertex_pos_offset=-1, vertex_normal_offset=-1, vertex_tangent_offset=-1, vertex_uv_offset=-1, vertex_color_offset=-1;
    bool calculate_tangents = true;
    bool packed_uv = false;
    for(uint8_t i=0; i<attribute_count; i++)
    {
        switch(mesh_attributes[i])
        {
            case DM_MESH_VERTEX_ATTRIBUTE_TEX_COORDS_2:
            vertex_uv_offset = vertex_size;
            vertex_size += 2;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_2:
            vertex_pos_offset = vertex_size;
            vertex_size += 2;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_TANGENT_3:
            vertex_tangent_offset = vertex_size;
            vertex_size += 3;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3:
            vertex_pos_offset = vertex_size;
            vertex_size += 3;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3:
            vertex_normal_offset = vertex_size;
            vertex_size += 3;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3_TEX_COORD_U:
            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_4:
            vertex_pos_offset = vertex_size;
            vertex_size += 4;
            packed_uv = true;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3_TEX_COOR_V:
            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_4:
            vertex_pos_offset = vertex_size;
            vertex_size += 4;
            packed_uv = true;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_TANGENT_4:
            vertex_tangent_offset = vertex_size;
            vertex_size += 4;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_COLOR_4:
            vertex_color_offset = vertex_size;
            vertex_size += 4;
            break;

            default:
            DM_LOG_FATAL("Unknown or unsupported mesh vertex attribute");
            return false;
        }
    }

    mesh->vertex_stride = vertex_size * sizeof(float);

    // data array
    float* vertices = NULL;

    size_t position_stride=0, normal_stride=0, uv_stride=0, color_stride=0, tangent_stride=0;
    float* position_buffer, *normal_buffer, *uv_buffer, *color_buffer, *tangent_buffer;

    // set up attributes arrays
    cgltf_mesh cm = data->meshes[mesh_index];

    // for each primitive in mesh
    // only supporting triangles for now
    cgltf_primitive cp;
    cp.type = cgltf_primitive_type_invalid;

    for(uint32_t p=0; p<cm.primitives_count; p++)
    {
        if(cm.primitives[p].type != cgltf_primitive_type_triangles) continue;

        cp = cm.primitives[p];
    }

    if(cp.type==cgltf_primitive_type_invalid)
    {
        DM_LOG_FATAL("GLTF mesh does not contain triangle primitives");
        return false;
    }

    // set up attributes
    for(uint32_t a=0; a<cp.attributes_count; a++)
    {
        cgltf_attribute attribute = cp.attributes[a];
        cgltf_accessor* accessor = attribute.data;
        void* buffer = accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
        size_t stride = accessor->stride / sizeof(float);

        switch(attribute.type)
        {
            case cgltf_attribute_type_position:
            position_stride = stride;
            position_buffer = buffer;
            break;

            case cgltf_attribute_type_normal:
            normal_stride = stride;
            normal_buffer = buffer;
            break;

            case cgltf_attribute_type_texcoord:
            uv_stride = stride;
            uv_buffer = buffer;
            break;

            case cgltf_attribute_type_color:
            color_stride = stride;
            color_buffer = buffer;
            break;

            case cgltf_attribute_type_tangent:
            tangent_stride = stride;
            tangent_buffer = buffer;
            calculate_tangents = false;
            break;

            default:
            break;
        }
    }

    if(calculate_tangents) { DM_LOG_FATAL("DarkMatter does not calculate tangents for you."); return false; }

    // indices
    mesh->index_count  = cp.indices->count;

    cgltf_accessor* index_accessor = cp.indices;    
    void* src = index_accessor->buffer_view->buffer->data + index_accessor->offset + index_accessor->buffer_view->offset;
    size_t size = index_accessor->stride * index_accessor->count;

    dm_index_buffer_desc ib_desc = { 0 };

    mesh->index_type = DM_INDEX_BUFFER_INDEX_TYPE_UINT32;
    index_data  = dm_alloc(sizeof(uint32_t) * index_accessor->count);
    uint32_t* indices = index_data;

    uint32_t max_vertex = 0;
    for(uint32_t i=0; i<index_accessor->count; i++)
    {
        switch(cm.primitives->indices->component_type)
        {
            case cgltf_component_type_r_16u:
            indices[i] = *((uint16_t*)src + i);   
            break;

            case cgltf_component_type_r_32u:
            indices[i] = *((uint32_t*)src + i);
            break;

            default:
            DM_LOG_FATAL("Unsupported index type (is not u16 or u32)");
            return false;
        }

        max_vertex = indices[i] > max_vertex ? indices[i] : max_vertex;
    }

    ib_desc.data         = index_data;
    ib_desc.index_type   = mesh->index_type;
    ib_desc.size         = sizeof(uint32_t) * index_accessor->count;

    // fill in vertex array
    mesh->vertex_count = max_vertex + 1; 
    vertex_data = dm_alloc(mesh->vertex_stride * mesh->vertex_count);

    uint32_t index=0;
    uint32_t vertex_data_count = vertex_size * mesh->vertex_count;

    while(index < vertex_data_count)
    {
        for(uint8_t a=0; a<attribute_count; a++)
        {
            switch(mesh_attributes[a])
            {
                case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3_TEX_COORD_U:
                vertex_data[index++] = position_buffer ? *(position_buffer + 0) : 0;
                vertex_data[index++] = position_buffer ? *(position_buffer + 1) : 0;
                vertex_data[index++] = position_buffer ? *(position_buffer + 2) : 0;
                vertex_data[index++] = uv_buffer ? *(uv_buffer + 0) : 0;

                if(position_buffer) position_buffer += position_stride;
                break;

                case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3_TEX_COOR_V:
                vertex_data[index++] = normal_buffer ? *(normal_buffer + 0) : 0;
                vertex_data[index++] = normal_buffer ? *(normal_buffer + 1) : 0;
                vertex_data[index++] = normal_buffer ? *(normal_buffer + 2) : 0;
                vertex_data[index++] = uv_buffer ? *(uv_buffer + 1) : 0;

                if(normal_buffer) normal_buffer += normal_stride;
                if(uv_buffer)     uv_buffer += uv_stride;
                break;

                case DM_MESH_VERTEX_ATTRIBUTE_TANGENT_4:
                vertex_data[index++] = tangent_buffer ? *(tangent_buffer + 0) : 0;
                vertex_data[index++] = tangent_buffer ? *(tangent_buffer + 1) : 0;
                vertex_data[index++] = tangent_buffer ? *(tangent_buffer + 2) : 0;
                vertex_data[index++] = 0;

                if(tangent_buffer) tangent_buffer += tangent_stride;
                break;

                case DM_MESH_VERTEX_ATTRIBUTE_COLOR_4:
                vertex_data[index++] = color_buffer ? *(color_buffer + 0) : 0;
                vertex_data[index++] = color_buffer ? *(color_buffer + 1) : 0;
                vertex_data[index++] = color_buffer ? *(color_buffer + 2) : 0;
                vertex_data[index++] = color_buffer ? *(color_buffer + 3) : 0;

                if(color_buffer) color_buffer += color_stride;
                break;

                default:
                continue;
            }
        }
    }

    dm_vertex_buffer_desc vb_desc = { 0 };
    vb_desc.stride       = mesh->vertex_stride;
    vb_desc.size         = mesh->vertex_count * mesh->vertex_stride;
    vb_desc.data         = vertex_data;

    if(!dm_renderer_create_vertex_buffer(vb_desc, &mesh->vb, context)) return false;
    if(!dm_renderer_create_index_buffer(ib_desc, &mesh->ib, context)) return false;

    dm_free((void**)&vertex_data);
    dm_free((void**)&index_data);

    return true;
}

bool dm_renderer_load_gltf_file(const char* file, dm_mesh_vertex_attribute* mesh_attributes, uint8_t attribute_count, dm_scene* scene, dm_context* context)
{
    DM_LOG_INFO("Loading gltf/glfb file: %s", file);

    // check if valid path
    FILE* fp = fopen(file, "r");
    if(!fp)
    {
        DM_LOG_FATAL("Cannot find file: %s", file);
        return false;
    }

    // check if file is valid 
    const char* dot = strrchr(file, '.');
    const char* ext = dot + 1;
    if(strcmp(ext, "gltf")==1 && strcmp(ext, "glb")==1)
    {
        DM_LOG_FATAL("Trying to load invalid file for gltf/glb format: %s", file);
        return false;
    }


    // begin
    cgltf_options options = { 0 };
    cgltf_data*   data = NULL;
    cgltf_result  result = cgltf_parse_file(&options, file, &data);
    if(result != cgltf_result_success)
    {
        DM_LOG_FATAL("cgltf_parse_file failed");
        cgltf_free(data);
        return false;
    }

    // load buffers
    result = cgltf_load_buffers(&options, data, file);
    if(result != cgltf_result_success)
    {
        DM_LOG_FATAL("cgltf_load_buffers failed");
        cgltf_free(data);
        return false;
    }

    // meshes
    for(uint16_t i=0; i<data->meshes_count; i++)
    {
        DM_LOG_INFO("Loading mesh: %u", i);
        if(!dm_renderer_load_gltf_mesh(data, i, mesh_attributes, attribute_count, scene, context)) { cgltf_free(data); return false; }
        scene->mesh_count++;
    }

    // materials
    // TODO: this *NEEDS* to be optimized, very slow to create all the textures
    char directory[512];
    strcpy(directory, file);
    *(strrchr(directory, '/')) = '\0';

    for(uint16_t i=0; i<data->materials_count; i++)
    {
        DM_LOG_INFO("Loading material: %u", i);
        for(uint8_t m=0; m<DM_MATERIAL_TYPE_UNKNOWN; m++)
        {
            if(!dm_renderer_gltf_load_material(directory, m, data->materials[i], scene, context)) { cgltf_free(data); return false; }
        }
        scene->material_count++;
    }

    // nodes 
    for(uint16_t i=0; i<data->scene->nodes_count; i++)
    {
        DM_LOG_INFO("Loading node: %u", i);
        cgltf_node* node = data->scene->nodes[i];
        dm_scene_node* scene_node = &scene->nodes[scene->node_count++];

        // find mesh index
        for(uint32_t m=0; m<data->meshes_count; m++)
        {
            if(strcmp(node->mesh->name, data->meshes[m].name)!=0) continue;

            scene_node->mesh_index = m;
            break;
        }

        cgltf_mesh      mesh = data->meshes[scene_node->mesh_index];
        cgltf_primitive primitive;
        for(uint8_t p=0; p<mesh.primitives_count; p++)
        {
            if(mesh.primitives[p].type != cgltf_primitive_type_triangles) continue;

            primitive = mesh.primitives[p];
        }

        // find material index
        for(uint32_t m=0; m<data->materials_count; m++)
        {
            if(strcmp(primitive.material->name, data->materials[m].name)!=0) continue;

            scene_node->material_index = m;
            break;
        }

        // model matrix
        if(node->has_matrix)
        {
            dm_memcpy(scene_node->model_matrix, node->matrix, sizeof(dm_mat4));
        }
        else
        {
            dm_mat4 translation, rotation, scale;

            dm_mat4_identity(translation);
            dm_mat4_identity(rotation);
            dm_mat4_identity(scale);
            
            if(node->has_translation) dm_mat_translate_make(node->translation, translation);
            if(node->has_rotation)    dm_mat4_rotate_from_quat(node->rotation, rotation);
            if(node->has_scale)       dm_mat_scale_make(node->scale, scale);

            dm_mat4_identity(scene_node->model_matrix);

            dm_mat4_mul_mat4(scene_node->model_matrix, scale, scene_node->model_matrix);
            dm_mat4_mul_mat4(scene_node->model_matrix, rotation, scene_node->model_matrix);
            dm_mat4_mul_mat4(scene_node->model_matrix, translation, scene_node->model_matrix);
        }
    }

    //
    cgltf_free(data);

    return true;
}

bool dm_renderer_load_obj_model(const char* file, dm_mesh_vertex_attribute* mesh_attributes, uint8_t attribute_count, dm_mesh* mesh, dm_context* context)
{
    assert(mesh);

    DM_LOG_INFO("Loading obj model: %s", file);

    // check if valid path
    FILE* fp = fopen(file, "r");
    if(!fp)
    {
        DM_LOG_FATAL("Cannot find file: %s", file);
        return false;
    }

    // check if file is obj
    const char* dot = strrchr(file, '.');
    const char* ext = dot + 1;
    if(strcmp(ext, "obj")==1)
    {
        DM_LOG_FATAL("Trying to load invalid file for obj format: %s", file);
        return false;
    }

    // begin
    fastObjMesh* obj_mesh = fast_obj_read(file);

    size_t vertex_size = 0;
    for(uint8_t i=0; i<attribute_count; i++)
    {
        switch(mesh_attributes[i])
        {
            case DM_MESH_VERTEX_ATTRIBUTE_TEX_COORDS_2:
            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_2:
            vertex_size += sizeof(float) * 2;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3:
            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3:
            vertex_size += sizeof(float) * 3;
            break;

            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3_TEX_COORD_U:
            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3_TEX_COOR_V:
            case DM_MESH_VERTEX_ATTRIBUTE_POSITION_4:
            case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_4:
            case DM_MESH_VERTEX_ATTRIBUTE_COLOR_4:
            vertex_size += sizeof(float) * 4;
            break;

            default:
            return false;
        }
    }

    float* vertex_data = dm_alloc(vertex_size * 3 * obj_mesh->face_count);
    
    // loop over mesh data, fill in vertex_data
    uint32_t index = 0, idx=0;

    // for all faces
    for(uint32_t v=0; v<obj_mesh->face_count; v++)
    {
        uint32_t fv = obj_mesh->face_vertices[v];

        // for all vertices in face
        for(uint32_t f=0; f<fv; f++)
        {
            fastObjIndex i = obj_mesh->indices[idx];
    
            for(uint8_t a=0; a<attribute_count; a++)
            {
                switch(mesh_attributes[a])
                {
                    case DM_MESH_VERTEX_ATTRIBUTE_TEX_COORDS_2:
                    vertex_data[index++] = i.t ? obj_mesh->texcoords[2 * i.t + 0] : 0;
                    vertex_data[index++] = i.t ? obj_mesh->texcoords[2 * i.t + 1] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_POSITION_2:
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 0] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 1] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3:
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 0] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 1] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 2] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_POSITION_4:
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 0] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 1] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 2] : 0;
                    vertex_data[index++] = 1;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_2:
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 0] : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 1] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3:
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 0] : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 1] : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 2] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_4:
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 0] : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 1] : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 2] : 0;
                    vertex_data[index++] = 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_POSITION_3_TEX_COORD_U:
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 0] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 1] : 0;
                    vertex_data[index++] = i.p ? obj_mesh->positions[3 * i.p + 2] : 0;
                    vertex_data[index++] = i.t ? obj_mesh->texcoords[2 * i.t + 0] : 0;
                    break;

                    case DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3_TEX_COOR_V:
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 0]   : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 1]   : 0;
                    vertex_data[index++] = i.n ? obj_mesh->normals[3 * i.n + 2]   : 0;
                    vertex_data[index++] = i.t ? obj_mesh->texcoords[2 * i.t + 1] : 0;
                    break;
                        
                    case DM_MESH_VERTEX_ATTRIBUTE_COLOR_4:
                    vertex_data[index++] = 1.f; 
                    vertex_data[index++] = 1.f; 
                    vertex_data[index++] = 1.f; 
                    vertex_data[index++] = 1.f;
                    break;
                        
                    default:
                    return false;
                }
            }

            idx++;
        }
    }

    mesh->vertex_count  = obj_mesh->face_count * 3;
    mesh->vertex_stride = vertex_size;
    mesh->index_type    = DM_INDEX_BUFFER_INDEX_TYPE_UNKNOWN;

    // create mesh
    dm_vertex_buffer_desc vb_desc = { 0 };
    vb_desc.size         = mesh->vertex_count * vertex_size;
    vb_desc.stride       = vertex_size;
    vb_desc.data         = vertex_data;

    if(!dm_renderer_create_vertex_buffer(vb_desc, &mesh->vb, context)) return false;

    // cleanup
    fast_obj_destroy(obj_mesh);

    dm_free((void**)&vertex_data);

    return true;
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

/*********
FRAMEWORK
***********/
typedef enum dm_context_flags_t
{
    DM_CONTEXT_FLAG_IS_RUNNING = 1,
} dm_context_flags;

bool dm_init(dm_context_init_packet init_packet, dm_context** context)
{
    *context = dm_alloc(sizeof(dm_context));
    
    (*context)->platform_data.window_data.width = init_packet.window_width;
    (*context)->platform_data.window_data.height = init_packet.window_height;
    dm_strcpy((*context)->platform_data.window_data.title, init_packet.window_title);
    dm_strcpy((*context)->platform_data.asset_path, init_packet.asset_folder);
    
    // misc
    (*context)->delta = 1.0f / 60.f;
    (*context)->flags |= DM_CONTEXT_FLAG_IS_RUNNING;

    if(!dm_platform_init(init_packet.window_x, init_packet.window_y, *context)) return false;
    
    (*context)->renderer.width  = (*context)->platform_data.window_data.width;
    (*context)->renderer.height = (*context)->platform_data.window_data.height;
    if(!dm_renderer_init(*context)) return false;
    
    (*context)->renderer.width  = init_packet.window_width;
    (*context)->renderer.height = init_packet.window_height;
    (*context)->renderer.vsync  = init_packet.vsync;
    
    // random init
    init_genrand(&(*context)->random, (uint32_t)dm_platform_get_time(&(*context)->platform_data));
    init_genrand64(&(*context)->random_64, (uint64_t)dm_platform_get_time(&(*context)->platform_data));
    
    if(init_packet.app_data_size) (*context)->app_data = dm_alloc(init_packet.app_data_size);
    
    return true;
}

void dm_shutdown(dm_context* context)
{
    if(context->app_data) dm_free((void**)&context->app_data);

    dm_free((void**)&context->renderer.render_command_manager.commands);
    
    dm_renderer_shutdown(context);
    dm_platform_shutdown(&context->platform_data);
    
    dm_free((void**)&context->renderer.internal_renderer);
    dm_free((void**)&context->platform_data.internal_data);
    dm_free((void**)&context);
}

bool dm_poll_events(dm_context* context)
{
    for(uint32_t i=0; i<context->platform_data.event_list.num && i<DM_MAX_EVENTS_PER_FRAME; i++)
    {
        dm_event e = context->platform_data.event_list.events[i];
        switch(e.type)
        {
            case DM_EVENT_WINDOW_CLOSE:
            {
                context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
                DM_LOG_WARN("Window close event received");
                return true;
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
            //DM_LOG_INFO("Window resize event received");
            context->platform_data.window_data.width  = e.new_rect[0];
            context->platform_data.window_data.height = e.new_rect[1];
            
            context->renderer.width = e.new_rect[0];
            context->renderer.height = e.new_rect[1];

            if(!dm_renderer_backend_resize(e.new_rect[0], e.new_rect[1], &context->renderer))
            {
                DM_LOG_FATAL("Resize failed");
                return false;
            }
            break;
            
            case DM_EVENT_UNKNOWN:
            DM_LOG_ERROR("Unknown event! Shouldn't be here...");
            break;
        }
    }
    
    return true;
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
    context->input_states[0].mouse.scroll = 0;
    
    if(!dm_platform_pump_events(&context->platform_data)) 
    {
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
        return false;
    }
    
    if(!dm_poll_events(context))
    {
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
        return false;
    }

    // render udpates
    if(!dm_renderer_backend_begin_update(&context->renderer))
    {
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
        return false;
    }
    
    return true;
}

bool dm_update_end(dm_context* context)
{
    if(!dm_renderer_backend_end_update(&context->renderer))
    {
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
        return false;
    }

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
    
    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    // command submission
    if(!dm_renderer_submit_commands(context)) 
    { 
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
        DM_LOG_FATAL("Submiting render commands failed"); 
        return false; 
    }
    
    context->renderer.render_command_manager.count  = 0;
    context->renderer.compute_command_manager.count = 0;
    
    if(!dm_renderer_backend_end_frame(context)) 
    {
        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
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
        char d[512];
        dm_memcpy(d, buffer, 512);
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
    return context->flags & DM_CONTEXT_FLAG_IS_RUNNING;
}

uint32_t __dm_get_screen_width(dm_context* context)
{
    return context->platform_data.window_data.width;
}

uint32_t __dm_get_screen_height(dm_context* context)
{
    return context->platform_data.window_data.height;
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

void dm_assert(bool condition)
{
    assert(condition);
}

void dm_assert_msg(bool condition, const char* message)
{
    if(condition) return;
    
    DM_LOG_FATAL("%s", message);
    assert(false);
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

/****
HASH
******/
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
dm_hash dm_hash_32bit(uint32_t key)
{
    dm_hash hash = ((key >> 16) ^ key)   * 0x119de1f3;
    hash         = ((hash >> 16) ^ hash) * 0x119de1f3;
    hash         = (hash >> 16) ^ hash;
    
    return hash;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
dm_hash64 dm_hash_64bit(uint64_t key)
{
	dm_hash64 hash = (key ^ (key >> 30))   * UINT64_C(0xbf58476d1ce4e5b9);
	hash           = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
	hash           = hash ^ (hash >> 31);
    
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

// alternative to modulo: https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
uint32_t dm_hash_reduce(uint32_t x, uint32_t n)
{
    return ((uint64_t)x * (uint64_t)n) >> 32;
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

void dm_kill(dm_context* context)
{
    DM_LOG_FATAL("DarkMatter kill received");
    context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
}

#ifdef DM_MATH_TESTS
void dm_math_tests();
#endif

typedef enum dm_exit_code_t
{
    DM_EXIT_CODE_SUCCESS                 =  0,
    DM_EXIT_CODE_INIT_FAIL               = -1,
    DM_EXIT_CODE_APPLICATION_INIT_FAIL   = -2,
    DM_EXIT_CODE_BEGIN_UPDATE_FAIL       = -3,
    DM_EXIT_CODE_APPLICATION_UPDATE_FAIL = -4,
    DM_EXIT_CODE_END_UPDATE_FAIL         = -5,
    DM_EXIT_CODE_BEGIN_FRAME_FAIL        = -6,
    DM_EXIT_CODE_APPLICATION_RENDER_FAIL = -7,
    DM_EXIT_CODE_END_FRAME_FAIL          = -8,
    DM_EXIT_CODE_UNKNOWN                 = -9
} dm_exit_code;

#define DM_DEFAULT_SCREEN_X      100
#define DM_DEFAULT_SCREEN_Y      100
#define DM_DEFAULT_SCREEN_WIDTH  1920
#define DM_DEFAULT_SCREEN_HEIGHT 1080
#define DM_DEFAULT_TITLE         "DarkMatter Application"
#define DM_DEFAULT_ASSETS_FOLDER "assets"
int main(int argc, char** argv)
{
    int exit_code = DM_EXIT_CODE_SUCCESS;

    dm_context_init_packet init_packet = {
        DM_DEFAULT_SCREEN_X, DM_DEFAULT_SCREEN_Y,
        DM_DEFAULT_SCREEN_WIDTH, DM_DEFAULT_SCREEN_HEIGHT,
        DM_DEFAULT_TITLE,
        DM_DEFAULT_ASSETS_FOLDER
    };
    
    dm_application_setup(&init_packet);
    
    dm_context* context = NULL;

    if(!dm_init(init_packet, &context))
    {
        exit_code = DM_EXIT_CODE_INIT_FAIL;
        goto DM_EXIT;
    }
    
    if(!dm_application_init(context)) 
    {
        DM_LOG_FATAL("Application init failed");
        
        exit_code = DM_EXIT_CODE_APPLICATION_INIT_FAIL;
        goto DM_EXIT;
    }
   
    if(!dm_renderer_backend_finish_init(context))
    {
        DM_LOG_FATAL("Renderer backend finish init failed");

        exit_code = DM_EXIT_CODE_INIT_FAIL;
        goto DM_EXIT;
    }

    while(dm_context_is_running(context))
    {
        dm_start(context);
        
        // updating
        if(!dm_update_begin(context)) 
        {
            DM_LOG_FATAL("DarkMatter begin update failed");

            exit_code = DM_EXIT_CODE_BEGIN_UPDATE_FAIL;
            goto DM_EXIT;
        }
        if(!dm_application_update(context))
        {
            DM_LOG_FATAL("Application update failed");

            exit_code = DM_EXIT_CODE_APPLICATION_UPDATE_FAIL;
            goto DM_EXIT;
        }
        if(!dm_update_end(context))
        {
            DM_LOG_FATAL("DarkMatter end update failed");

            exit_code = DM_EXIT_CODE_END_UPDATE_FAIL;
            goto DM_EXIT;
        }
        
        // rendering
        if(dm_renderer_begin_frame(context))
        {
            if(!dm_application_render(context))
            {
                DM_LOG_FATAL("Application render failed");
                
                exit_code = DM_EXIT_CODE_APPLICATION_RENDER_FAIL;
                goto DM_EXIT;
            }
            if(!dm_renderer_end_frame(context)) 
            {
                DM_LOG_FATAL("DarkMatter end frame failed");

                exit_code = DM_EXIT_CODE_END_FRAME_FAIL;
                goto DM_EXIT;
            }
        }
        else
        {
            DM_LOG_FATAL("DarkMatter begin frame failed");

            exit_code = DM_EXIT_CODE_BEGIN_FRAME_FAIL;
            goto DM_EXIT;
        }
        
        dm_end(context);
    }
    
DM_EXIT:
    dm_application_shutdown(context);
    dm_shutdown(context);
    
    DM_LOG_WARN("Press enter to exit...");
    int r = getchar();
    
    return exit_code;
}
