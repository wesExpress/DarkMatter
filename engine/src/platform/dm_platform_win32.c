#include "dm_platform.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_platform_win32.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_event.h"
#include "core/dm_mem.h"
#include "input/dm_input.h"

#ifdef DM_OPENGL
#include <glad/glad_wgl.h>
#endif

LRESULT CALLBACK window_callback(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static double clock_frequency;
static float start_time;
static float end_time;

dm_internal_windows_data* windows_data = NULL;

#ifdef DM_OPENGL
bool dm_wind32_load_opengl_functions(WNDCLASSEX window_class);

typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);

HDC fake_dc;
HGLRC fake_rc;
HWND fake_wnd;
#endif

bool dm_platform_init(dm_platform_data* platform_data, const char* window_name)
{
	windows_data = dm_alloc(sizeof(dm_internal_windows_data), DM_MEM_PLATFORM);
	platform_data->internal_data = windows_data;
	windows_data->h_instance = GetModuleHandleA(0);
    
	const char* window_class_name = "dm_window_class";
    
#ifdef DM_OPENGL
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = window_callback;
	wcex.hInstance = windows_data->h_instance;
	wcex.hIcon = LoadIcon(windows_data->h_instance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = window_class_name;
    
	if (!dm_wind32_load_opengl_functions(wcex)) return false;
#endif
    
#ifndef DM_OPENGL
	HICON icon = LoadIcon(windows_data->h_instance, IDI_APPLICATION);
	WNDCLASSA wc = { 0 };
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wc.lpfnWndProc = window_callback;
	wc.hInstance = windows_data->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = window_class_name;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
	if (!RegisterClassA(&wc))
	{
		DM_LOG_FATAL("Window registration failed!");
		return false;
	}
#endif
    
	// Adjust client window to be appropriate
	uint32_t client_x = platform_data->x;
	uint32_t client_y = platform_data->y;
	uint32_t client_width = platform_data->window_width;
	uint32_t client_height = platform_data->window_height;
    
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
	windows_data->hwnd = CreateWindowExA(
                                         window_ex_style, 
                                         window_class_name, window_name,
                                         window_style,
                                         window_x, window_y, 
                                         window_width, window_height,
                                         NULL, NULL, 
                                         windows_data->h_instance, 
                                         NULL
                                         );
    
	if (!windows_data->hwnd)
	{
		DM_LOG_FATAL("Window creation failed!");
		return false;
	}
    
	DM_LOG_DEBUG("Window created...");
    
#ifndef DM_OPENGL
	ShowWindow(windows_data->hwnd, SW_SHOW);
#endif
    
	// clock
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	clock_frequency = 1.0f / frequency.QuadPart;
	QueryPerformanceCounter(&start_time);
    
	return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
	DM_LOG_WARN("Platform shutdown called...");
	DM_LOG_WARN("Destroying window...");
    
	if (windows_data->hwnd)
	{
		DestroyWindow(windows_data->hwnd);
		windows_data->hwnd = 0;
	}
	
	dm_free(windows_data, sizeof(dm_internal_windows_data), DM_MEM_PLATFORM);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
	MSG message;
	while (PeekMessageA(&message, windows_data->hwnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
    
	return true;
}

void* dm_platform_alloc(size_t size)
{
	void* temp = malloc(size);
	DM_ASSERT_MSG(temp, "Malloc returned null pointer!");
	if (!temp) return NULL;
	dm_memzero(temp, size);
	return temp;
}

void* dm_platform_calloc(size_t count, size_t size)
{
	void* temp = calloc(count, size);
	DM_ASSERT_MSG(temp, "Calloc returned NULL pointer!");
	if (!temp) return NULL;
	return temp;
}

void* dm_platform_realloc(void* block, size_t size)
{
	void* temp = realloc(block, size);
	DM_ASSERT_MSG(temp, "Realloc returned null pointer!");
	if (temp) block = temp;
	else DM_LOG_FATAL("Realloc returned NULL ptr!");
	return block;
}

void dm_platform_free(void* block)
{
	free(block);
    block = NULL;
}

void* dm_platform_memzero(void* block, size_t size)
{
	return memset(block, 0, size);
}

void* dm_platform_memcpy(void* dest, const void* src, size_t size)
{
	return memcpy(dest, src, size);
}

void* dm_platform_memset(void* dest, int value, size_t size)
{
	return memset(dest, value, size);
}

void dm_platform_memmove(void* dest, const void* src, size_t size)
{
	memmove(dest, src, size);
}

float dm_platform_get_time()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
    
	return time.QuadPart * clock_frequency;
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

void dm_platform_write_error(const char* message, uint8_t color)
{
	HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
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
	switch (umsg)
	{
        case WM_ERASEBKGND:
		return 1;
        case WM_CLOSE:
        {
            dm_event_dispatch((dm_event) { DM_WINDOW_CLOSE_EVENT, NULL });
            
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
            GetWindowRect(hwnd, &rect);
            uint32_t width = rect.right - rect.left;
            uint32_t height = rect.bottom - rect.top;
            uint32_t new_rect[2] = { width, height };
            
            dm_event_dispatch((dm_event) { DM_WINDOW_RESIZE_EVENT, NULL, &new_rect });
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
                    vk = extended ? VK_RCONTROL : VK_LCONTROL;
                } break;
            }
            
            dm_key_code key = (dm_key_code)vk;
            dm_event_dispatch((dm_event) { DM_KEY_DOWN_EVENT, NULL, &key });
            
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
                    vk = extended ? VK_RCONTROL : VK_LCONTROL;
                } break;
            }
            
            dm_key_code key = (dm_key_code)vk;
            dm_event_dispatch((dm_event) { DM_KEY_UP_EVENT, NULL, &key });
        } break;
        
        case WM_MOUSEMOVE:
        {
            int32_t x = GET_X_LPARAM(lparam);
            int32_t y = GET_Y_LPARAM(lparam);
            int32_t coords[2] = { x, y };
            dm_event_dispatch((dm_event) { DM_MOUSE_MOVED_EVENT, NULL, &coords });
        } break;
        
        case WM_MOUSEWHEEL:
        {
            int32_t delta = GET_WHEEL_DELTA_WPARAM(wparam);
            if (delta != 0)
            {
                delta = (delta < 0) ? -1 : 1;
            }
            dm_event_dispatch((dm_event) { DM_MOUSE_SCROLLED_EVENT, NULL, &delta });
        } break;
        
        case WM_LBUTTONDOWN:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_L;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, &button });
        } break;
        case WM_RBUTTONDOWN:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_R;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, &button });
        } break;
        case WM_MBUTTONDOWN:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_M;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, &button });
        } break;
        
        case WM_LBUTTONUP:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_L;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, &button });
        } break;
        case WM_RBUTTONUP:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_R;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, &button });
        } break;
        case WM_MBUTTONUP:
        {
            dm_mousebutton_code button = DM_MOUSEBUTTON_M;
            dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, &button });
        } break;
        
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

const char* dm_get_win32_error_msg(HRESULT hr)
{
	char* message = NULL;
    
	DWORD msg_len = FormatMessageA(
                                   FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPSTR)(&message), 0, NULL
                                   );
    
	if (msg_len == 0)
	{
		return "Unidentified Error Code";
	}
    
	return message;
}

const char* dm_get_win32_last_error()
{
	DWORD error_message_id = GetLastError();
    
	LPSTR message = NULL;
    
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, error_message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)(&message), 0, NULL);
    
	return message;
    
}

#ifdef DM_OPENGL
bool dm_wind32_load_opengl_functions(WNDCLASSEX window_class)
{
	// fake window
	if (!RegisterClassEx(&window_class))
	{
		DM_LOG_FATAL("Registering window class failed!");
		return false;
	}
    
	fake_wnd = CreateWindow(
                            "dm_window_class",
                            "Fake Window",
                            WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                            0, 0,
                            1, 1,
                            NULL, NULL,
                            windows_data->h_instance, NULL
                            );
    
	// rendering context
	fake_dc = GetDC(fake_wnd);
    
	PIXELFORMATDESCRIPTOR fake_pfd = { 0 };
	fake_pfd.nSize = sizeof(fake_pfd);
	fake_pfd.nVersion = 1;
	fake_pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	fake_pfd.iPixelType = PFD_TYPE_RGBA;
	fake_pfd.cColorBits = 32;
	fake_pfd.cDepthBits = 24;
	fake_pfd.cAlphaBits = 8;
    
	int format = ChoosePixelFormat(fake_dc, &fake_pfd);
	if (format == 0)
	{
		DM_LOG_FATAL("ChoosePixelFormat() failed!");
		return false;
	}
	if (!SetPixelFormat(fake_dc, format, &fake_pfd))
	{
		DM_LOG_FATAL("SetPixelFormat() failed!");
		return false;
	}
    
	fake_rc = wglCreateContext(fake_dc);
	if (!fake_rc)
	{
		DM_LOG_FATAL("Failed to create render context!");
		return false;
	}
    
	if (!wglMakeCurrent(fake_dc, fake_rc))
	{
		DM_LOG_FATAL("Failed to make context current!");
		return false;
	}
    
	// load opengl functions with glad
	if (!gladLoadWGL(fake_dc))
	{
		DM_LOG_FATAL("Could not initialize WGL GLAD functions!");
		return false;
	}
    
	return true;
}

bool dm_platform_init_opengl()
{
	windows_data->hdc = GetDC(windows_data->hwnd);
    
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
    
	bool status = wglChoosePixelFormatARB(windows_data->hdc, pixel_attribs, NULL, 1, &pixel_format_id, &num_formats);
	if (!status || (num_formats == 0))
	{
		DM_LOG_FATAL("wglChoosePixelFormatsARB failed!");
		return false;
	}
    
	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(windows_data->hdc, pixel_format_id, sizeof(PFD), &PFD);
	SetPixelFormat(windows_data->hdc, pixel_format_id, &PFD);
    
	// opengl context
	const int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, DM_OPENGL_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB, DM_OPENGL_MINOR,
		WGL_CONTEXT_FLAGS_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB,
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0,
	};
    
	windows_data->hrc = wglCreateContextAttribsARB(windows_data->hdc, 0, context_attribs);
	if (!windows_data->hrc)
	{
		DM_LOG_FATAL("wglCreateContextAttribsARB failed!");
		return false;
	}
    
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fake_rc);
	ReleaseDC(fake_wnd, fake_dc);
	DestroyWindow(fake_wnd);
    
	if (!wglMakeCurrent(windows_data->hdc, windows_data->hrc))
	{
		DM_LOG_FATAL("Could not make context current!");
		return false;
	}
    
	// load opengl functions with glad
	if (!gladLoadGL())
	{
		DM_LOG_FATAL("Could not initialize GLAD!");
		return false;
	}
    
	ShowWindow(windows_data->hwnd, SW_SHOW);
    
	return true;
}

void dm_platform_shutdown_opengl()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(windows_data->hrc);
}

void dm_platform_swap_buffers()
{
	SwapBuffers(windows_data->hdc);
}

#endif

#endif