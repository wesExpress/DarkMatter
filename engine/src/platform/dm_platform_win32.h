#ifndef __DM_PLATFORM_WIN32_H__
#define __DM_PLATFORM_WIN32_H__

#include "core/dm_defines.h"

#ifdef DM_PLATFORM_WIN32

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <dxgi.h>

const char* dm_get_win32_error_msg(HRESULT hr);
const char* dm_get_win32_last_error();

#endif

#endif