#include "dm_platform_win32.h"

#ifdef DM_PLATFORM_WIN32

#include <stdio.h>

#define DM_WIN32_GET_DATA dm_internal_w32_data* w32_data = platform_data->internal_data

LRESULT CALLBACK window_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void* dm_win32_thread_start_func(void* args);
void  dm_win32_thread_execute_task(dm_thread_task* task);
const char* dm_win32_get_last_error();

bool dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_context* context)
{
    dm_platform_data* platform_data = &context->platform_data;
    platform_data->internal_data = dm_alloc(sizeof(dm_internal_w32_data));
    dm_internal_w32_data* w32_data = platform_data->internal_data;
    
	w32_data->h_instance = GetModuleHandleA(0);
    
	const char* window_class_name = "dm_window_class";
    
	WNDCLASSEXW wc = { 0 };
	wc.style         = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wc.lpfnWndProc   = window_callback;
	wc.hInstance     = w32_data->h_instance;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = (LPCWSTR)window_class_name;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.cbSize        = sizeof(WNDCLASSEX);
    
	if (!RegisterClassExW(&wc)) 
    {
        printf("Window registration failed\n");

        printf("%s", dm_win32_get_last_error());

        return false;
    }

    uint32_t window_width, window_height, window_x, window_y;
    
#if 1
    uint32_t window_style    = WS_OVERLAPPEDWINDOW | WS_VISIBLE; 
    uint32_t window_ex_style = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;

    RECT border_rect = { 0,0,0,0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    // Adjust client window to be appropriate
    uint32_t client_x = window_x_pos;
    uint32_t client_y = window_y_pos;
    uint32_t client_width = platform_data->window_data.width;
    uint32_t client_height = platform_data->window_data.height;

    window_x = client_x;
    window_y = client_y;
    window_width = client_width;
    window_height = client_height;

    window_x += border_rect.left;
    window_y += border_rect.top;
    window_width += (border_rect.right - border_rect.left);
    window_height += (border_rect.bottom - border_rect.top);
#else
    uint32_t window_style    = WS_POPUP | WS_VISIBLE;
    uint32_t window_ex_style = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    
    HMONITOR monitor = MonitorFromWindow(w32_data->hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = { 0 };
    info.cbSize = sizeof(info);
    if(GetMonitorInfoW(monitor, &info))
    {
        window_x = info.rcMonitor.left;
        window_y = info.rcMonitor.top;
        window_width = info.rcWork.right - info.rcWork.left;
        window_height = info.rcWork.bottom - info.rcWork.top;
    }
#endif
    
	// create the window
    wchar_t d[512];
    swprintf(d, 512, L"%hs", platform_data->window_data.title);
	w32_data->hwnd = CreateWindowExW(window_ex_style, 
                                    (LPCWSTR)window_class_name, 
                                    d, 
                                    window_style,
                                    window_x, window_y, 
                                    window_width, window_height,
                                    NULL, NULL, 
                                    w32_data->h_instance, 
                                    NULL);
    
	if (!w32_data->hwnd) 
    {
        printf("Window creation failed\n");

        printf("%s", dm_win32_get_last_error());

        return false;
    }
    
    // attach platform data to hwnd
    if(!SetPropW(w32_data->hwnd, L"platform_data", platform_data)) return false;
    
    SetWindowLongW(w32_data->hwnd, GWL_STYLE,   window_style);
    SetWindowLongW(w32_data->hwnd, GWL_EXSTYLE, window_ex_style);
	//ShowWindow(w32_data->hwnd, SW_SHOW);
    
	// clock
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	w32_data->clock_freq = 1.0f / frequency.QuadPart;
    
	return true;
}

double dm_platform_get_time(dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
    
	return time.QuadPart * w32_data->clock_freq;
}

void dm_platform_sleep(uint64_t t, dm_context* context)
{
    Sleep(t);
}

void dm_platform_shutdown(dm_platform_data* platform_data)
{
	DM_WIN32_GET_DATA;
    
    if (w32_data->hwnd)
	{
		DestroyWindow(w32_data->hwnd);
		w32_data->hwnd = 0;
	}
}

bool dm_platform_pump_events(dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
    MSG message;
    while (PeekMessageW(&message, w32_data->hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    
    return true;
}

void dm_platform_write(const char* message, uint8_t color)
{
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	static uint8_t levels[6] = { 8, 1, 2, 6, 4, 64 };

	SetConsoleTextAttribute(console_handle, levels[color]);
	size_t len = strlen(message);
	LPDWORD number_written = 0;

	WriteConsoleA(console_handle, message, (DWORD)len, number_written, 0);

	// resets to white
	SetConsoleTextAttribute(console_handle, 7);
}

LRESULT CALLBACK window_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    dm_platform_data* platform_data = GetPropW(hwnd, L"platform_data");

	if(!platform_data) return DefWindowProcW(hwnd, umsg, wparam, lparam);
    
    switch (umsg)
	{
        case WM_ERASEBKGND:
		return 1;
        case WM_CLOSE:
        dm_add_window_close_event(&platform_data->event_list);
        return 0;
        case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        
        case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            uint32_t width = rect.right - rect.left;
            uint32_t height = rect.bottom - rect.top;
            
            dm_add_window_resize_event(width, height, &platform_data->event_list);
        } break;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            WPARAM vk = wparam;;
            
            UINT scancode = (lparam & 0x00ff0000) >> 16;
            int extended = (lparam & 0x01000000) != 0;
            
            switch (vk)
            {
                case VK_SHIFT:
                {
                    vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
                } break;
                case VK_CONTROL:
                {
                    vk = !extended ? VK_RCONTROL : VK_LCONTROL;
                } break;
            }
            
            dm_add_key_down_event((dm_key_code)vk, &platform_data->event_list);
        } break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            WPARAM vk = wparam;;
            
            UINT scancode = (lparam & 0x00ff0000) >> 16;
            int extended = (lparam & 0x01000000) != 0;
            
            switch (vk)
            {
                case VK_SHIFT:
                {
                    vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
                } break;
                case VK_CONTROL:
                {
                    vk = !extended ? VK_RCONTROL : VK_LCONTROL;
                } break;
                default: 
                break;
            }
            
            dm_add_key_up_event((dm_key_code)vk, &platform_data->event_list);
        } break;
        
        case WM_MOUSEMOVE:
        {
            int32_t x = GET_X_LPARAM(lparam);
            int32_t y = GET_Y_LPARAM(lparam);
            int32_t coords[2] = { x, y };
            
            dm_add_mouse_move_event(x,y, &platform_data->event_list);
        } break;
        
        case WM_MOUSEWHEEL:
        dm_add_mouse_scroll_event((float)(short)HIWORD(wparam) / (float)WHEEL_DELTA, &platform_data->event_list);
        break;
        
        case WM_LBUTTONDOWN:
        dm_add_mousebutton_down_event(DM_MOUSEBUTTON_L, &platform_data->event_list);
        SetCapture(hwnd);
        break;
        case WM_RBUTTONDOWN:
        dm_add_mousebutton_down_event(DM_MOUSEBUTTON_R, &platform_data->event_list);
        SetCapture(hwnd);
        break;
        case WM_MBUTTONDOWN:
        dm_add_mousebutton_down_event(DM_MOUSEBUTTON_M, &platform_data->event_list);
        SetCapture(hwnd);
        break;
        case WM_LBUTTONDBLCLK:
        dm_add_mousebutton_down_event(DM_MOUSEBUTTON_DOUBLE, &platform_data->event_list);
        break;
        
        case WM_LBUTTONUP:
        dm_add_mousebutton_up_event(DM_MOUSEBUTTON_L, &platform_data->event_list);
        ReleaseCapture();
        break;
        case WM_RBUTTONUP:
        dm_add_mousebutton_up_event(DM_MOUSEBUTTON_R, &platform_data->event_list);
        ReleaseCapture();
        break;
        case WM_MBUTTONUP:
        dm_add_mousebutton_up_event(DM_MOUSEBUTTON_M, &platform_data->event_list);
        ReleaseCapture();
        break;
        
        // unhandled Windows event
        default:
        break;
	}
    
	return DefWindowProcW(hwnd, umsg, wparam, lparam);
}

LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_CREATE:
        return 0;
        case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
    
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/**********
THREADPOOL
************/
bool dm_platform_threadpool_create(dm_threadpool* threadpool)
{
    threadpool->internal_pool = dm_alloc(sizeof(dm_w32_threadpool));
    dm_w32_threadpool* internal_pool = threadpool->internal_pool;
    
    InitializeCriticalSection(&internal_pool->thread_count_mutex);
    InitializeCriticalSection(&internal_pool->task_queue.mutex);
    InitializeCriticalSection(&internal_pool->task_queue.has_tasks.mutex);
    InitializeConditionVariable(&internal_pool->task_queue.has_tasks.cond);
    InitializeConditionVariable(&internal_pool->all_threads_idle);
    InitializeConditionVariable(&internal_pool->at_least_one_idle);
    
    for(uint32_t i=0; i<threadpool->thread_count; i++)
    {
        DWORD t;
        internal_pool->threads[i] = CreateThread(0,0, (LPTHREAD_START_ROUTINE)dm_win32_thread_start_func, threadpool, 0, &t);
        
#ifdef DM_DEBUG
        DM_LOG_INFO("Created thread: %lu", t);
#endif
        
        if(internal_pool->threads[i]) continue;
        
        DM_LOG_FATAL("Could not create Win32 thread");
        return false;
    }
    
    return true;
}

void dm_platform_threadpool_destroy(dm_threadpool* threadpool)
{
    dm_w32_threadpool* w32_threadpool = threadpool->internal_pool;
    
    for(uint32_t i=0; i<threadpool->thread_count; i++)
    {
        DWORD exit_code;
        GetExitCodeThread(w32_threadpool->threads[i], &exit_code);
#ifdef DM_DEBUG
        DM_LOG_WARN("Thread exit code: %lu", exit_code);
#endif
        CloseHandle(w32_threadpool->threads[i]);
    }
    
    dm_free(threadpool->internal_pool);
}

void dm_platform_threadpool_submit_task(dm_thread_task* task, dm_threadpool* threadpool)
{
    dm_w32_threadpool* w32_threadpool = threadpool->internal_pool;
    
    // insert task
    EnterCriticalSection(&w32_threadpool->task_queue.mutex);
    dm_memcpy(w32_threadpool->task_queue.tasks + w32_threadpool->task_queue.count, task, sizeof(dm_thread_task));
    w32_threadpool->task_queue.count++;
    
    // wake up at least one thread
    EnterCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
    w32_threadpool->task_queue.has_tasks.v = 1;
    WakeConditionVariable(&w32_threadpool->task_queue.has_tasks.cond);
    LeaveCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
    
    LeaveCriticalSection(&w32_threadpool->task_queue.mutex);
}

void dm_platform_threadpool_wait_for_completion(dm_threadpool* threadpool)
{
    dm_w32_threadpool* w32_threadpool = threadpool->internal_pool;
    
    // sleep until there are NO working threads AND the task queue is empty
    EnterCriticalSection(&w32_threadpool->thread_count_mutex);
    while(w32_threadpool->num_working_threads || w32_threadpool->task_queue.count)
    {
        SleepConditionVariableCS(&w32_threadpool->all_threads_idle, &w32_threadpool->thread_count_mutex, INFINITE);
    }
    LeaveCriticalSection(&w32_threadpool->thread_count_mutex);
}

void* dm_win32_thread_start_func(void* args)
{
    dm_threadpool* threadpool = args;
    dm_w32_threadpool* w32_threadpool = threadpool->internal_pool;
    
    while(1)
    {
        // sleep until a new task is available
        EnterCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
        while(w32_threadpool->task_queue.has_tasks.v==0)
        {
            SleepConditionVariableCS(&w32_threadpool->task_queue.has_tasks.cond, &w32_threadpool->task_queue.has_tasks.mutex, INFINITE);
        }
        w32_threadpool->task_queue.has_tasks.v = 0;
        LeaveCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
        
        // iterate thread working counter
        EnterCriticalSection(&w32_threadpool->thread_count_mutex);
        w32_threadpool->num_working_threads++;
        LeaveCriticalSection(&w32_threadpool->thread_count_mutex);
        
        // grab task
        EnterCriticalSection(&w32_threadpool->task_queue.mutex);
        dm_thread_task* task = NULL;
        
        if(w32_threadpool->task_queue.count)
        {
            task = dm_alloc(sizeof(dm_thread_task));
            dm_memcpy(task, &w32_threadpool->task_queue.tasks[0], sizeof(dm_thread_task));
            dm_memmove(w32_threadpool->task_queue.tasks, w32_threadpool->task_queue.tasks + 1, sizeof(dm_thread_task) * w32_threadpool->task_queue.count-1);
            w32_threadpool->task_queue.count--;
        }
        
        // if the task queue is still populated, need to resend signal
        if(w32_threadpool->task_queue.count) 
        {
            EnterCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
            w32_threadpool->task_queue.has_tasks.v = 1;
            WakeConditionVariable(&w32_threadpool->task_queue.has_tasks.cond);
            LeaveCriticalSection(&w32_threadpool->task_queue.has_tasks.mutex);
        }
        LeaveCriticalSection(&w32_threadpool->task_queue.mutex);
        
        // run task
        if(task) 
        {
            task->func(task->args);
            dm_free((void**)&task);
        }
        
        // decrement thread working counter
        EnterCriticalSection(&w32_threadpool->thread_count_mutex);
        w32_threadpool->num_working_threads--;
        if(w32_threadpool->num_working_threads==0) WakeConditionVariable(&w32_threadpool->all_threads_idle);
        LeaveCriticalSection(&w32_threadpool->thread_count_mutex);
    }
}

/***************
ERROR MESSAGING
*****************/
const char* dm_win32_get_last_error()
{
	DWORD error_message_id = GetLastError();
	LPSTR message = NULL;
    
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error_message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)(&message), 0, NULL);
    
	return message;
}

/*********
CLIPBOARD
***********/
void dm_platform_clipboard_copy(const char* text, int len)
{
    if(!OpenClipboard(NULL)) return;
    
    int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    if(!wsize) { CloseClipboard(); return; }
    
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (wsize  + 1) * sizeof(wchar_t));
    if(!mem) { CloseClipboard(); return; }
    
    wchar_t* wstr = (wchar_t*)GlobalLock(mem);
    if(!wstr) { CloseClipboard(); return; }
    
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
    wstr[wsize] = 0;
    GlobalUnlock(mem);
    SetClipboardData(CF_UNICODETEXT, mem);
    
    CloseClipboard();
}

void dm_platform_clipboard_paste(void (*callback)(char*,int,void*), void* edit)
{
    if(!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(NULL)) return;
    
    HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
    if(!mem) { CloseClipboard(); return; }
    
    SIZE_T size = GlobalSize(mem) - 1;
    if(!size) { CloseClipboard(); return; }
    
    LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
    if(!wstr) { CloseClipboard(); return; }
    
    int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, size / sizeof(wchar_t), NULL, 0, NULL, NULL);
    if(!utf8size) { GlobalUnlock(mem); CloseClipboard(); return; }
    char* utf8 = (char*)malloc(utf8size);
    
    if(!utf8) { GlobalUnlock(mem); CloseClipboard(); return; }
    
    WideCharToMultiByte(CP_UTF8, 0, wstr, size / sizeof(wchar_t), utf8, utf8size, NULL, NULL);
    
    callback(utf8, utf8size, edit);
    //nk_textedit_paste(edit, utf8, utf8size);
    dm_free((void**)&utf8);
    
    GlobalUnlock(mem);
    CloseClipboard();
}

uint32_t dm_get_available_processor_count(dm_context* context)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    
    return sysinfo.dwNumberOfProcessors;
}

bool dm_platform_win32_decode_hresult(HRESULT hr)
{
    if (hr == S_OK) return true;
    
    DM_LOG_ERROR("Hresult failed with:");
    
    switch (hr)
    {
        case E_ABORT: 
        DM_LOG_ERROR("Operation aborted");
        break;
        
        case E_ACCESSDENIED:
        DM_LOG_ERROR("General access denied error");
        break;
        
        case E_FAIL:
        DM_LOG_ERROR("Unspecified failure");
        break;
        
        case E_HANDLE:
        DM_LOG_ERROR("Handle that is not valid");
        break;
        
        case E_INVALIDARG:
        DM_LOG_ERROR("One or more arguments are not valid");
        break;
        
        case E_NOINTERFACE:
        DM_LOG_ERROR("No such interface supported");
        break;
        
        case E_NOTIMPL:
        DM_LOG_ERROR("Not implemented");
        break;
        
        case E_OUTOFMEMORY:
        DM_LOG_ERROR("Failed to allocate necessary memory");
        break;
        
        case E_POINTER:
        DM_LOG_ERROR("Pointer that is not valid");
        break;
        
        case E_UNEXPECTED:
        DM_LOG_ERROR("Unexpected failure");
        break;
        
        case 0x80070002:
        DM_LOG_ERROR("File not found");
        break;
        
        case 0x887A0005:
        DM_LOG_ERROR("DXGI Error: Device removed: Device hung");
        break;
        
        case 0x887A0006:
        DM_LOG_ERROR("GPU will not respond to more commands due to invalid command");
        break;

        case 0x8876086c:
        DM_LOG_ERROR("Invalid call");
        break;
        
        default:
        DM_LOG_ERROR("Unknown error: %u", hr);
        break;
    }
    
    return false;
}
#endif
