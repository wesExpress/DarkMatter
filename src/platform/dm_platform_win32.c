#include "dm_platform.h"

#ifdef DM_PLATFORM_WIN32

#include "dm_platform_win32.h"
#include "dm_logger.h"
#include "dm_assert.h"
#include "dm_event.h"
#include "input/dm_input.h"
#include "dm_mem.h"

typedef struct dm_internal_data
{
	HINSTANCE h_instance;
	HWND hwnd;
} dm_internal_data;

LRESULT CALLBACK window_callback(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title, int start_x, int start_y)
{
	e_data->platform_data = (dm_platform_data*)dm_alloc(sizeof(dm_platform_data), DM_MEM_PLATFORM);
	e_data->platform_data->window_width = window_width;
	e_data->platform_data->window_height = window_height;
	e_data->platform_data->window_title = window_title;

	e_data->platform_data->internal_data = (dm_internal_data*)dm_alloc(sizeof(dm_internal_data), DM_MEM_PLATFORM);
	dm_internal_data* internal_data = (dm_internal_data*)e_data->platform_data->internal_data;

	HICON icon = LoadIcon(internal_data->h_instance, IDI_APPLICATION);
	WNDCLASSA wc = { 0 };
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wc.lpfnWndProc = window_callback;
	wc.hInstance = internal_data->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = "dm_window_class";
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	if (!RegisterClassA(&wc))
	{
		MessageBoxA(0, "Window registration failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Adjust client window to be appropriate
	uint32_t client_x = start_x;
	uint32_t client_y = start_y;
	uint32_t client_width = window_width;
	uint32_t client_height = window_height;

	uint32_t window_x = client_x;
	uint32_t window_y = client_y;
	uint32_t window_w = client_width;
	uint32_t window_h = client_height;

	uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
	uint32_t window_ex_style = WS_EX_APPWINDOW;

	RECT border_rect = { 0,0,0,0 };
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

	window_x += border_rect.left;
	window_y += border_rect.top;
	window_width += (border_rect.right - border_rect.left);
	window_height += (border_rect.bottom - border_rect.top);

	// create the window
	HWND wnd_handle = CreateWindowExA(
		window_ex_style, wc.lpszClassName, window_title, window_style,
		window_x, window_y, window_w, window_h,
		0, 0, internal_data->h_instance, 0
	);

	if (!wnd_handle)
	{
		MessageBoxA(NULL, "Window creation failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		DM_LOG_FATAL("Window creation failed!");
		return false;
	}
	internal_data->hwnd = wnd_handle;

#if DM_OPENGL
	if (!dm_platform_create_context(plat_state)) return false;
#endif

	DM_LOG_DEBUG("Window created...");

	ShowWindow(internal_data->hwnd, SW_SHOW);

	return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
	DM_LOG_WARN("Platform shutdown called...");

	dm_internal_data* internal_data = (dm_internal_data*)e_data->platform_data->internal_data;

	DM_LOG_WARN("Destroying window...");

	if (internal_data->hwnd)
	{
		DestroyWindow(internal_data->hwnd);
		internal_data->hwnd = 0;
	}
	
	dm_free(e_data->platform_data->internal_data, sizeof(dm_internal_data), DM_MEM_PLATFORM);
	dm_free(e_data->platform_data, sizeof(dm_platform_data), DM_MEM_PLATFORM);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
	dm_internal_data* internal_data = (dm_internal_data*)e_data->platform_data->internal_data;

	MSG message;
	while (PeekMessageA(&message, internal_data->hwnd, 0, 0, PM_REMOVE))
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

		dm_event_dispatch((dm_event) { DM_WINDOW_RESIZE_EVENT, NULL, (void*)new_rect });
	} break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		dm_key_code key = (dm_key_code)wparam;
		dm_event_dispatch((dm_event) { DM_KEY_DOWN_EVENT, NULL, (void*)key });

	} break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		dm_key_code key = (dm_key_code)wparam;
		dm_event_dispatch((dm_event) { DM_KEY_UP_EVENT, NULL, (void*)key });
	} break;

	case WM_MOUSEMOVE:
	{
		int32_t x = GET_X_LPARAM(lparam);
		int32_t y = GET_Y_LPARAM(lparam);
		int32_t coords[2] = { x, y };
		dm_event_dispatch((dm_event) { DM_MOUSE_MOVED_EVENT, NULL, (void*)coords });
	} break;

	case WM_MOUSEWHEEL:
	{
		int32_t delta = GET_WHEEL_DELTA_WPARAM(wparam);
		if (delta != 0)
		{
			delta = (delta < 0) ? -1 : 1;
		}
		dm_event_dispatch((dm_event) { DM_MOUSE_SCROLLED_EVENT, NULL, (void*)(size_t)delta });
	} break;

	case WM_LBUTTONDOWN:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_L;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
	} break;
	case WM_RBUTTONDOWN:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_R;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
	} break;
	case WM_MBUTTONDOWN:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_M;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)button });
	} break;

	case WM_LBUTTONUP:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_L;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
	} break;
	case WM_RBUTTONUP:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_R;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
	} break;
	case WM_MBUTTONUP:
	{
		dm_mousebutton_code button = DM_MOUSEBUTTON_M;
		dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)button });
	} break;

	}

	return DefWindowProcA(hwnd, umsg, wparam, lparam);
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

#endif