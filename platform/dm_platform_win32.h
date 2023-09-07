#ifndef DM_PLATFORM_WIN32_H
#define DM_PLATFORM_WIN32_H

#include "dm.h"

#ifdef DM_PLATFORM_WIN32

#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <windowsx.h>

typedef struct dm_w32_threadpool_t
{
    CONDITION_VARIABLE queue_condition;
    CONDITION_VARIABLE queue_empty;
    CRITICAL_SECTION   queue_mutex;
    
    LONG queue_increment;
    
    HANDLE   threads[DM_MAX_THREAD_COUNT];
} dm_w32_threadpool;

typedef struct dm_internal_w32_data_t
{
	HINSTANCE h_instance;
	HWND      hwnd;
	HDC       hdc;
	HGLRC     hrc;
    double    clock_freq;
    
#ifdef DM_OPENGL
    HDC   fake_dc;
    HGLRC fake_rc;
    HWND  fake_wnd;
#endif
} dm_internal_w32_data;

#endif

#endif //DM_PLATFORM_WIN32_H
