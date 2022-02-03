#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "core/dm_defines.h"
#include "core/dm_engine_types.h"

/*
Sets up the platform specific data.

@param e_data -> dm_engine_data struct (ptr)
@param window_width 
@param window_height 
@param window_title
@param start_x - pixel x start of window
@param start_y - pixel y start of window
*/
//bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title, int start_x, int start_y);
bool dm_platform_init(dm_platform_data* platform_data, const char* window_name);
/*
frees up all allocated memory for the platform

@param e_data -> dm_engine_data struct (ptr)
*/
void dm_platform_shutdown(dm_engine_data* e_data);

/*
named after windows specific API
run every frame to poll OS events and check if window should close

@param e_data -> dm_engine_data struct (ptr)
*/
bool dm_platform_pump_messages(dm_engine_data* e_data);

// various platform memory functions
void* dm_platform_alloc(size_t size);
void* dm_platform_calloc(size_t count, size_t size);
void* dm_platform_realloc(void* block, size_t size);
void dm_platform_free(void* block);
void* dm_platform_memzero(void* block, size_t size);
void* dm_platform_memcpy(void* dest, const void* src, size_t size);
void* dm_platform_memset(void* dest, int value, size_t size);
void dm_platform_memmove(void* dest, const void* src, size_t size);

void dm_platform_set_vsync(bool enabled);

/*
platform specific write functionality

@param message
@param color - index of color array
*/
void dm_platform_write(const char* message, uint8_t color);

/*
platform specific stderr functionality

@param message
@param color - index of color array
*/
void dm_platform_write_error(const char* message, uint8_t color);

/*
platform specific means to get time
*/
float dm_platform_get_time();

/*
platform specific buffer swapping
*/
void dm_platform_swap_buffers();

#ifdef DM_OPENGL
/*
platform specific opengl initialization
*/
bool dm_platform_init_opengl();

/*
platform specific opengl shutdown
*/
void dm_platform_shutdown_opengl();
#endif

#endif