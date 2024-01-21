#ifndef DM_PLATFORM_WIN32_H
#define DM_PLATFORM_WIN32_H

#include "dm.h"

#ifdef DM_PLATFORM_WIN32

#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <windowsx.h>

typedef struct dm_w32_semaphore_t
{
    CONDITION_VARIABLE cond;
    CRITICAL_SECTION   mutex;
    uint32_t v;
} dm_w32_semaphore;

typedef struct dm_w32_task_queue_t
{
    dm_thread_task tasks[DM_MAX_TASK_COUNT];
    uint32_t       count;
    
    CRITICAL_SECTION mutex;
    dm_w32_semaphore has_tasks;
} dm_w32_task_queue;

typedef struct dm_w32_threadpool_t
{
    CRITICAL_SECTION thread_count_mutex;
    CONDITION_VARIABLE all_threads_idle;
    CONDITION_VARIABLE at_least_one_idle;
    
    uint32_t num_working_threads;
    
    dm_w32_task_queue task_queue;
    
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

bool dm_platform_win32_decode_hresult(HRESULT hr);

#endif

#endif //DM_PLATFORM_WIN32_H
