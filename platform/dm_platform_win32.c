#include "dm_platform_win32.h"

#ifdef DM_PLATFORM_WIN32

#ifdef DM_OPENGL
#include "glad/glad_wgl.h"

bool dm_win32_load_opengl_functions(WNDCLASSEX window_class, dm_platform_data* platform_data);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);

#elif defined(DM_VULKAN)
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#endif

#define DM_WIN32_GET_DATA dm_internal_w32_data* w32_data = platform_data->internal_data

LRESULT CALLBACK window_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void* dm_win32_thread_start_func(void* args);
void  dm_win32_thread_execute_task(dm_thread_task* task);

bool dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_context* context)
{
    dm_platform_data* platform_data = &context->platform_data;
    platform_data->internal_data = dm_alloc(sizeof(dm_internal_w32_data));
    dm_internal_w32_data* w32_data = platform_data->internal_data;
    
	w32_data->h_instance = GetModuleHandleA(0);
    
	const char* window_class_name = "dm_window_class";
    
#ifdef DM_OPENGL
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = window_callback;
	wcex.hInstance = w32_data->h_instance;
	wcex.hIcon = LoadIcon(w32_data->h_instance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = window_class_name;
    
	if (!dm_win32_load_opengl_functions(wcex, platform_data)) 
    {
        DM_LOG_FATAL("Win32 load OpenGL functions failed");
        return false;
    }
#else
	HICON icon = LoadIcon(w32_data->h_instance, IDI_APPLICATION);
	WNDCLASSA wc = { 0 };
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wc.lpfnWndProc = window_callback;
	wc.hInstance = w32_data->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = window_class_name;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
	if (!RegisterClassA(&wc)) 
    {
        //DM_LOG_FATAL("Window registering failed");
        printf("Window registration failed");
        return false;
    }
#endif
    
	// Adjust client window to be appropriate
	uint32_t client_x = window_x_pos;
	uint32_t client_y = window_y_pos;
	uint32_t client_width = platform_data->window_data.width;
	uint32_t client_height = platform_data->window_data.height;
    
	uint32_t window_x = client_x;
	uint32_t window_y = client_y;
	uint32_t window_width = client_width;
	uint32_t window_height = client_height;
    
	uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
	uint32_t window_ex_style = WS_EX_APPWINDOW;
    
	RECT border_rect = { 0,0,0,0 };
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
    
	window_x += border_rect.left;
	window_y += border_rect.top;
	window_width += (border_rect.right - border_rect.left);
	window_height += (border_rect.bottom - border_rect.top);
    
	// create the window
	w32_data->hwnd = CreateWindowExA(window_ex_style, 
                                     window_class_name, platform_data->window_data.title,
                                     window_style,
                                     window_x, window_y, 
                                     window_width, window_height,
                                     NULL, NULL, 
                                     w32_data->h_instance, 
                                     NULL);
    
	if (!w32_data->hwnd) 
    {
        //DM_LOG_FATAL("Window creation failed");
        printf("Window creation failed");
        return false;
    }
    
    // attach platform data to hwnd
    if(!SetPropA(w32_data->hwnd, "platform_data", platform_data)) return false;
    
#ifndef DM_OPENGL
	ShowWindow(w32_data->hwnd, SW_SHOW);
#endif
    
    //ShowCursor(FALSE);
    
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
    
    dm_free(platform_data->internal_data);
}

bool dm_platform_pump_events(dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
    MSG message;
    while (PeekMessageA(&message, w32_data->hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
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
    dm_platform_data* platform_data = GetPropA(hwnd, "platform_data");
    
    switch (umsg)
	{
        case WM_ERASEBKGND:
		return 1;
        case WM_CLOSE:
        {
            dm_add_window_close_event(&platform_data->event_list);
            return 0;
        } break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
        
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
    
	return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_CREATE:
        {
            return 0;
        } 
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
            dm_free(task);
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
const char* dm_get_win32_error_msg(HRESULT hr)
{
	char* message = NULL;
    
	DWORD msg_len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)(&message), 0, NULL);
    
	if (msg_len == 0) return "Unidentified Error Code";
    
	return message;
}

const char* dm_get_win32_last_error()
{
	DWORD error_message_id = GetLastError();
	LPSTR message = NULL;
    
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, error_message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)(&message), 0, NULL);
    
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
    dm_free(utf8);
    
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

    default:
        break;
    }

    return false;
}

/******
OPENGL
********/
#ifdef DM_OPENGL
bool dm_win32_load_opengl_functions(WNDCLASSEX window_class, dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
	// fake window
	if (!RegisterClassEx(&window_class)) return false;
    
	w32_data->fake_wnd = CreateWindow("dm_window_class",
                                      "Fake Window",
                                      WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                      0, 0,
                                      1, 1,
                                      NULL, NULL,
                                      w32_data->h_instance, NULL);
    
	// rendering context
	w32_data->fake_dc = GetDC(w32_data->fake_wnd);
    
	PIXELFORMATDESCRIPTOR fake_pfd = { 0 };
	fake_pfd.nSize = sizeof(fake_pfd);
	fake_pfd.nVersion = 1;
	fake_pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	fake_pfd.iPixelType = PFD_TYPE_RGBA;
	fake_pfd.cColorBits = 32;
	fake_pfd.cDepthBits = 24;
	fake_pfd.cAlphaBits = 8;
    
	int format = ChoosePixelFormat(w32_data->fake_dc, &fake_pfd);
	if (format == 0) return false;
    if (!SetPixelFormat(w32_data->fake_dc, format, &fake_pfd)) 
    {
        DM_LOG_FATAL("SetPixelFormat failed");
        return false;
    }
    
	w32_data->fake_rc = wglCreateContext(w32_data->fake_dc);
	if (!w32_data->fake_rc) return false;
	if (!wglMakeCurrent(w32_data->fake_dc, w32_data->fake_rc)) 
    {
        DM_LOG_FATAL("wglMakeCurrent failed");
        return false;
    }
    
	// load opengl functions with glad
	if (!gladLoadWGL(w32_data->fake_dc)) 
    {
        DM_LOG_FATAL("gladLoadWGL failed");
        return false;
    }
    
	return true;
}

bool dm_platform_init_opengl(dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
	w32_data->hdc = GetDC(w32_data->hwnd);
    
	const int pixel_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		0
	};
    
	int pixel_format_id;
	uint32_t num_formats;
    
	bool status = wglChoosePixelFormatARB(w32_data->hdc, pixel_attribs, NULL, 1, &pixel_format_id, &num_formats);
	if (!status || (num_formats == 0)) return false;
    
	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(w32_data->hdc, pixel_format_id, sizeof(PFD), &PFD);
	SetPixelFormat(w32_data->hdc, pixel_format_id, &PFD);
    
	// opengl context
	const int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, DM_OPENGL_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB, DM_OPENGL_MINOR,
		WGL_CONTEXT_FLAGS_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB,
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0,
	};
    
	w32_data->hrc = wglCreateContextAttribsARB(w32_data->hdc, 0, context_attribs);
	if (!w32_data->hrc) return false;
    
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(w32_data->fake_rc);
	ReleaseDC(w32_data->fake_wnd, w32_data->fake_dc);
	DestroyWindow(w32_data->fake_wnd);
    
	if (!wglMakeCurrent(w32_data->hdc, w32_data->hrc)) return false;
    
	// load opengl functions with glad
	if (!gladLoadGL()) 
    {
        DM_LOG_FATAL("gladLoadGL failed");
        return false;
    }
    
	ShowWindow(w32_data->hwnd, SW_SHOW);
    
	return true;
}

void dm_platform_shutdown_opengl(dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(w32_data->hrc);
}

void dm_platform_swap_buffers(bool vsync, dm_platform_data* platform_data)
{
    DM_WIN32_GET_DATA;
    
    uint32_t v = vsync ? 1 : 0;
    wglSwapIntervalEXT(v);
	SwapBuffers(w32_data->hdc);
}
#endif

/******
VULKAN
********/
#ifdef DM_VULKAN
bool dm_platform_create_vulkan_surface(dm_platform_data* platform_data, VkInstance* instance, VkSurfaceKHR* surface)
{
    DM_WIN32_GET_DATA;
    
    VkWin32SurfaceCreateInfoKHR create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hinstance = w32_data->h_instance;
    create_info.hwnd = w32_data->hwnd;
    
    VkResult result = vkCreateWin32SurfaceKHR(*instance, &create_info, NULL, surface);
    if(result==VK_SUCCESS) return true;
    
    DM_LOG_FATAL("Could not create Win32 Vulkan surface");
    return false;
}
#endif

#endif