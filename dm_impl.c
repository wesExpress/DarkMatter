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

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

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
extern bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer);
extern bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer);

bool dm_renderer_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_render_handle* handle, dm_context* context)
{
    handle->type = DM_RENDER_RESOURCE_TYPE_RASTER_PIPELINE;

    if(dm_renderer_backend_create_raster_pipeline(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Could not create raster pipeline");
    return false;
}

bool dm_renderer_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_context* context)
{
    handle->type = DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER;

    if(dm_renderer_backend_create_vertex_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating vertex buffer failed");
    return false;
}

bool dm_renderer_create_index_buffer(dm_index_buffer_desc desc, dm_render_handle* handle, dm_context* context)
{
    handle->type = DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER;

    if(dm_renderer_backend_create_index_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating index buffer failed");
    return false;
}

bool dm_renderer_create_constant_buffer(dm_constant_buffer_desc desc, dm_render_handle* handle, dm_context* context)
{
    handle->type = DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER;

    if(dm_renderer_backend_create_constant_buffer(desc, handle, &context->renderer)) return true;

    DM_LOG_FATAL("Creating constant buffer failed");
    return false;
}

/***************
RENDER COMMANDS
*****************/
extern bool dm_render_command_backend_bind_raster_pipeline(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, dm_renderer* renderer);
extern bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_render_handle, dm_renderer* renderer);
extern bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer);
extern bool dm_render_command_backend_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer);
extern bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_render_handle handle, dm_renderer* renderer);

void _dm_render_command_submit(dm_render_command command, dm_render_command_manager* manager)
{
    if(!manager->commands)
    {
        manager->capacity = 16;
        manager->commands = dm_alloc(sizeof(dm_render_command) * manager->capacity);
    }

    manager->commands[manager->count++] = command;

    if((float)manager->count / (float)manager->capacity > 0.75f)
    {
        manager->capacity *= 2;
        manager->commands = dm_realloc(manager->commands, sizeof(dm_render_command) * manager->capacity);
    }
}

#define DM_RENDER_COMMAND_SUBMIT _dm_render_command_submit(command, &context->renderer.command_manager)

void dm_render_command_bind_raster_pipeline(dm_render_handle handle, dm_context* context)
{
    dm_render_command command = { 0 };
    
    command.type = DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE;

    command.params[0].render_handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_vertex_buffer(dm_render_handle handle, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;

    command.params[0].render_handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_bind_index_buffer(dm_render_handle handle, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;

    command.params[0].render_handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_vertex_buffer(void* data, size_t size, dm_render_handle handle, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER;

    command.params[0].void_val          = data;
    command.params[1].size_t_val        = size;
    command.params[2].render_handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_update_constant_buffer(void* data, size_t size, dm_render_handle handle, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER;

    command.params[0].void_val          = data;
    command.params[1].size_t_val        = size;
    command.params[2].render_handle_val = handle;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED;

    command.params[0].uint32_val = instance_count;
    command.params[1].uint32_val = instance_offset;
    command.params[2].uint32_val = vertex_count;
    command.params[3].uint32_val = vertex_offset;

    DM_RENDER_COMMAND_SUBMIT;
}

void dm_render_command_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_context* context)
{
    dm_render_command command = { 0 };

    command.type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED;

    command.params[0].uint32_val = instance_count;
    command.params[1].uint32_val = instance_offset;
    command.params[2].uint32_val = index_count;
    command.params[3].uint32_val = index_offset;
    command.params[4].uint32_val = vertex_offset;

    DM_RENDER_COMMAND_SUBMIT;
}

bool dm_renderer_submit_commands(dm_context* context)
{
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    
    dm_renderer* renderer = &context->renderer;
    
    for(uint32_t i=0; i<context->renderer.command_manager.count; i++)
    {
        dm_render_command command = context->renderer.command_manager.commands[i];
        
        switch(command.type)
        {
            case DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE:
            if(command.params[0].render_handle_val.type != DM_RENDER_RESOURCE_TYPE_RASTER_PIPELINE) return false;
            if(dm_render_command_backend_bind_raster_pipeline(command.params[0].render_handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind raster pipeline failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER:
            if(command.params[0].render_handle_val.type != DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER) return false;
            if(dm_render_command_backend_bind_vertex_buffer(command.params[0].render_handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER:
            if(command.params[2].render_handle_val.type != DM_RENDER_RESOURCE_TYPE_VERTEX_BUFFER) return false;
            if(dm_render_command_backend_update_vertex_buffer(command.params[0].void_val, command.params[1].size_t_val, command.params[2].render_handle_val, renderer)) continue;
            DM_LOG_FATAL("Update vertex buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER:
            if(command.params[0].render_handle_val.type != DM_RENDER_RESOURCE_TYPE_INDEX_BUFFER) return false;
            if(dm_render_command_backend_bind_index_buffer(command.params[0].render_handle_val, renderer)) continue;
            DM_LOG_FATAL("Bind index buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER:
            if(command.params[2].render_handle_val.type != DM_RENDER_RESOURCE_TYPE_CONSTANT_BUFFER) return false;
            if(dm_render_command_backend_update_constant_buffer(command.params[0].void_val, command.params[1].size_t_val, command.params[2].render_handle_val, renderer)) continue;
            DM_LOG_FATAL("Update constant buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED:
            if(dm_render_command_backend_draw_instanced(command.params[0].uint32_val, command.params[1].uint32_val, command.params[2].uint32_val, command.params[3].uint32_val, renderer)) continue;
            DM_LOG_FATAL("Draw instanced failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED:
            if(dm_render_command_backend_draw_instanced_indexed(command.params[0].uint32_val, command.params[1].uint32_val, command.params[2].uint32_val, command.params[3].uint32_val, command.params[5].uint32_val, renderer)) continue;
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
    DM_CONTEXT_FLAG_IS_RUNNING,
    DM_CONTEXT_FLAG_UNKNOWN
} dm_context_flags;

bool dm_init(dm_context_init_packet init_packet, dm_context** context)
{
    *context = dm_alloc(sizeof(dm_context));
    
    (*context)->platform_data.window_data.width = init_packet.window_width;
    (*context)->platform_data.window_data.height = init_packet.window_height;
    dm_strcpy((*context)->platform_data.window_data.title, init_packet.window_title);
    dm_strcpy((*context)->platform_data.asset_path, init_packet.asset_folder);
    
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
    
    // misc
    (*context)->delta = 1.0f / 60.f;
    (*context)->flags |= DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);

    if(init_packet.app_data_size) (*context)->app_data = dm_alloc(init_packet.app_data_size);
    
    return true;
}

void dm_shutdown(dm_context* context)
{
    if(context->app_data) dm_free((void**)&context->app_data);

    dm_free((void**)&context->renderer.command_manager.commands);
    
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
                context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
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
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        return false;
    }
    
    if(!dm_poll_events(context))
    {
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        return false;
    }
    context->platform_data.event_list.num = 0;
    
    return true;
}

bool dm_update_end(dm_context* context)
{
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
        context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
        DM_LOG_FATAL("Submiting render commands failed"); 
        return false; 
    }
    
    context->renderer.command_manager.count = 0;
    
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
    context->flags &= ~DM_BIT_SHIFT(DM_CONTEXT_FLAG_IS_RUNNING);
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
                DM_LOG_FATAL("DarkMatter end fram failed");

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
    
#ifdef DM_DEBUG
    DM_LOG_WARN("Press any key to exit...");
#endif
    int r = getchar();
    
    return exit_code;
}
