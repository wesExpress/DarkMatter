#include "dm.h"
#include <time.h>

#ifdef DM_PLATFORM_APPLE
#include <mach/mach_time.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image/stb_image.h"

void* dm_alloc(size_t size)
{
    void* temp = malloc(size);
    return dm_memzero(temp, size);
}

void dm_free(void** ptr)
{
    free(*ptr);
    *ptr = NULL;
}

void* dm_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void dm_memcpy(void* dest, void* src, size_t size)
{
    memcpy(dest, src, size);
}

void* dm_memset(void* dst, int value, size_t size)
{
    return memset(dst, value, size);
}

void* dm_memzero(void* dst, size_t size)
{
    return dm_memset(dst, 0, size);
}

double dm_get_time()
{
#ifdef DM_PLATFORM_WIN32
    LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

    return (double)time.QuadPart / (double)frequency.QuadPart;
#elif defined(DM_PLATFORM_LINUX)
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec*0.000000001;
#elif defined(DM_PLATFORM_APPLE)
    mach_timebase_info_data_t clock_timebase;
    mach_timebase_info(&clock_timebase);
    
    uint64_t mach_absolute = mach_absolute_time();
    
    uint64_t nanos = (double)(mach_absolute * (uint64_t)clock_timebase.numer) / (double)clock_timebase.denom;
    
    return nanos / 1.0e9;
#endif
}

void dm_timer_start(dm_timer* timer)
{
    timer->start = dm_get_time();
}

double dm_timer_elapsed(dm_timer* timer)
{
    return dm_get_time() - timer->start;
}

double dm_timer_elapsed_ms(dm_timer* timer)
{
    return dm_timer_elapsed(timer) * 1000;
}

void dm_platform_write(const char* message, uint8_t color)
{
#ifdef DM_PLATFORM_WIN32
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	static uint8_t levels[6] = { 8, 1, 2, 6, 4, 64 };

	SetConsoleTextAttribute(console_handle, levels[color]);
	size_t len = strlen(message);
	LPDWORD number_written = 0;

	WriteConsoleA(console_handle, message, (DWORD)len, number_written, 0);

	// resets to white
	SetConsoleTextAttribute(console_handle, 7);
#elif defined(DM_PLATFORM_LINUX) || defined(DM_PLATFORM_APPLE)
    static char* levels[6] = {
        "1;30",   // white
        "1;34",   // blue
        "1;32",   // green
        "1;33",   // yellow
        "1;31",   // red
        "0;41"    // highlighted red
    };

    char out[5000];
    sprintf(
        out,
        "\033[%sm%s \033[0m",
        levels[color], message
    );

    printf("%s", out);
    #endif
}

void dm_log(dm_log_level level, const char* message, ...)
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

/*****************
* RENDERING IMPL *
******************/
typedef struct dm_command_param_t
{
    union
    {
        uint8_t              u8;
        uint16_t             u16;
        uint32_t             u32;
        uint64_t             u64;
        size_t               s;
        bool                 b;
        int                  i;
        float                f;
        double               d;
        void*                v;
        dm_resource_handle   rh;
        dm_renderpass_handle rph;
        dm_pipeline_handle   ph;
    };
} dm_command_param;

typedef enum dm_render_command_type_t
{
    DM_RENDER_COMMAND_TYPE_INVALID,
    DM_RENDER_COMMAND_TYPE_BEGIN_FRAME,
    DM_RENDER_COMMAND_TYPE_END_FRAME,
    DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE,
    DM_RENDER_COMMAND_TYPE_END_UPDATE,
    DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_END_RENDER_PASS,
    DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE,
    DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES,
    DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER,
    DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED,
    DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED,
#ifdef DM_HARDWARE_RAYTRACING
    DM_RENDER_COMMAND_TYPE_BIND_RAYTRACING_PIPELINE,
    DM_RENDER_COMMAND_TYPE_DISPATCH_RAYS,
    DM_RENDER_COMMAND_TYPE_UPDATE_TLAS
#endif
} dm_render_command_type;

typedef enum dm_compute_command_type_t
{
    DM_COMPUTE_COMMAND_TYPE_INVALID,
    DM_COMPUTE_COMMAND_TYPE_BEGIN_RECORDING,
    DM_COMPUTE_COMMAND_TYPE_END_RECORDING,
    DM_COMPUTE_COMMAND_BIND_COMPUTE_PIPELINE,
    DM_COMPUTE_COMMAND_SUBMIT_RESOURCES,
    DM_COMPUTE_COMMAND_DISPATCH
} dm_compute_command_type;

typedef enum dm_command_buffer_type_t
{
    DM_COMMAND_BUFFER_TYPE_INVALID,
    DM_COMMAND_BUFFER_TYPE_RENDER,
    DM_COMMAND_BUFFER_TYPE_COMPUTE
} dm_command_buffer_type;

#define DM_COMMAND_MAX_PARAMS 10
typedef struct dm_command_t
{
    union
    {
        dm_render_command_type  r_type;
        dm_compute_command_type c_type;
    };
    dm_command_param params[DM_COMMAND_MAX_PARAMS];
} dm_command;

#define DM_COMMAND_BUFFER_MAX_COMMANDS 100
typedef struct dm_command_buffer_t
{
    dm_command_buffer_type type;
    dm_command commands[DM_COMMAND_BUFFER_MAX_COMMANDS];
    uint16_t   command_count;
} dm_command_buffer;

typedef enum dm_renderer_flag_t
{
    DM_RENDERER_FLAG_NONE = 0,
    DM_RENDERER_FLAG_VSYNC_ON = 1,
} dm_renderer_flag;

typedef struct dm_renderer_t 
{
    uint32_t width, height;

    dm_command_buffer render_commands;

    void* backend;
} dm_renderer;

// === render commands ===
void dm_render_command_submit(dm_command command, dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    if(renderer->render_commands.command_count >= DM_COMMAND_BUFFER_MAX_COMMANDS)
    {
        dm_log(DM_LOG_ERROR, "Too many render commands");
        return;
    }

    renderer->render_commands.commands[renderer->render_commands.command_count++] = command;
}

void dm_render_command_begin_frame(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_FRAME;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_frame(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_FRAME;

    dm_render_command_submit(command, context);
}

void dm_render_command_begin_update(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_update(dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_UPDATE;

    dm_render_command_submit(command, context);
}

void dm_render_command_begin_render_pass(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS;

    command.params[0].rph = handle;
    command.params[1].f = r;
    command.params[2].f = g;
    command.params[3].f = b;
    command.params[4].f = a;
    command.params[5].f = depth;

    dm_render_command_submit(command, context);
}

void dm_render_command_end_render_pass(dm_renderpass_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_END_RENDER_PASS;

    command.params[0].rph = handle;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_raster_pipeline(dm_pipeline_handle handle, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE;

    command.params[0].ph = handle;

    dm_render_command_submit(command, context);
}

void dm_render_command_submit_resources(dm_resource_handle* handles, uint16_t count, dm_context* context)
{
    dm_command command = { 0 };
    
    command.r_type = DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES;

    command.params[0].v   = (void*)handles;
    command.params[1].u16 = count;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].u8 = slot;
    command.params[2].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_bind_index_buffer(dm_resource_handle handle, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_vertex_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_index_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_constant_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_storage_buffer(dm_resource_handle handle, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command  = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER;

    command.params[0].rh = handle;
    command.params[1].v  = data;
    command.params[2].s  = size;
    command.params[3].s  = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_update_texture(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE;

    command.params[0].rh  = handle;
    command.params[1].u16 = width;
    command.params[2].u16 = height;
    command.params[3].v   = data;
    command.params[4].s   = size;
    command.params[5].s   = offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_draw_instanced(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = vertex_count;
    command.params[3].s   = vertex_offset;

    dm_render_command_submit(command, context);
}

void dm_render_command_draw_instanced_indexed(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, dm_context* context)
{
    dm_command command = { 0 };

    command.r_type = DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED;

    command.params[0].u32 = instance_count;
    command.params[1].s   = instance_offset;
    command.params[2].u32 = index_count;
    command.params[3].s   = index_offset;
    command.params[4].s   = vertex_offset;

    dm_render_command_submit(command, context);
}

#ifdef DM_DIRECTX12
#ifdef DM_DEBUG
void dm_dx12_get_debug_message(dm_renderer* renderer)
{
    const uint32_t num_messages = IDXGIInfoQueue_GetNumStoredMessages(renderer->info_queue, DXGI_DEBUG_ALL);
    for(uint32_t i=0; i<num_messages; i++)
    {
        size_t len = 0;
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, NULL, &len);
        DXGI_INFO_QUEUE_MESSAGE* buffer = dm_alloc(len);
        IDXGIInfoQueue_GetMessage(renderer->info_queue, DXGI_DEBUG_ALL, i, buffer, &len);

        dm_log(DM_LOG_ERROR, "%s\n", buffer->pDescription);
        dm_free((void**)&buffer);
    }
}
#else
void dm_dx12_get_debug_message(dm_renderer* renderer) {}
#endif

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_wait_for_previous_frame(bool advance, dm_renderer* renderer)
{
    HRESULT hr;

    dm_dx12_fence* fence = &renderer->fences[renderer->current_frame];

    hr = ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value); 
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    if(advance) renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);

    fence = &renderer->fences[renderer->current_frame];

    if(ID3D12Fence_GetCompletedValue(fence->fence) < fence->value)
    {
        hr = ID3D12Fence_SetEventOnCompletion(fence->fence, fence->value, renderer->fence_event);
        if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

        WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
    }

    fence->value++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_descriptor_heap(uint32_t descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, dm_dx12_descriptor_heap* heap, dm_renderer* renderer)
{
    HRESULT hr;
    void* temp = NULL;

    D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
    desc.Type            = type;
    desc.NumDescriptors  = descriptor_count;
    desc.Flags          |= flags;

    hr = ID3D12Device5_CreateDescriptorHeap(renderer->device, &desc, &IID_ID3D12DescriptorHeap, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateDescriptorHeap failed"); return false; }
    heap->heap = temp;
    temp = NULL;

    heap->size = ID3D12Device5_GetDescriptorHandleIncrementSize(renderer->device, type);

    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap->heap, &heap->cpu_handle.begin);
    heap->cpu_handle.current = heap->cpu_handle.begin;

    if(flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap->heap, &heap->gpu_handle.begin);
        heap->gpu_handle.current = heap->gpu_handle.begin;
    }

    return true;
}
#endif

// === backend ===
extern void* dm_renderer_init_backend(dm_context* context);
extern bool dm_renderer_finish_init_backend(void* backend);
extern void dm_renderer_shutdown_backend(void* backend);

bool dm_renderer_init(dm_context* context)
{
    context->renderer = dm_alloc(sizeof(dm_renderer));
    dm_renderer* renderer = context->renderer;

    renderer->width = dm_get_window_width(context);
    renderer->height = dm_get_window_height(context);

    renderer->backend = dm_renderer_init_backend(context);

    return renderer->backend != NULL;
#ifdef DM_DIRECTX12
#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing DirectX12 backend...");
#endif
     HRESULT hr;

    void* temp = NULL;

    // create device
#ifdef DM_DEBUG
    hr = D3D12GetDebugInterface(&IID_ID3D12Debug, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12GetDebugInterface failed"); return false; }
    renderer->debug = temp;
    temp = NULL;

    ID3D12Debug_EnableDebugLayer(renderer->debug);

    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "DXGIGetDebugInterface failed"); return false; }
    renderer->dxgi_debug = temp;
    temp = NULL;

    hr = IDXGIDebug1_QueryInterface(renderer->dxgi_debug, &IID_IDXGIInfoQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGIDebug1_QueryInterface failed"); return false; }
    renderer->info_queue = temp;
    temp = NULL;
#endif

    hr = CreateDXGIFactory1(&IID_IDXGIFactory4, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "CreateDXGIFactory1 failed"); return false; }
    renderer->factory = temp;
    temp = NULL;

    int adapter_index = 0;
    bool found = false;

    DXGI_ADAPTER_DESC1 adapter_desc = { 0 };
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_1;

    while(IDXGIFactory4_EnumAdapters1(renderer->factory, adapter_index, &renderer->adapter) != DXGI_ERROR_NOT_FOUND)
    {
        IDXGIAdapter1_GetDesc1(renderer->adapter, &adapter_desc);

        if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter_index++;
            continue;
        }

        hr = D3D12CreateDevice((IUnknown*)renderer->adapter, feature_level, &IID_ID3D12Device, NULL);
        if(SUCCEEDED(hr))
        {
            found = true;
            break;
        }

        adapter_index++;
    }

    if(!found) { dm_log(DM_LOG_FATAL, "No suitable DirectX12 adapter found"); return false; }

    hr = D3D12CreateDevice((IUnknown*)renderer->adapter, feature_level, &IID_ID3D12Device5, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "D3D12CreateDevice failed"); return false; }

    dm_log(DM_LOG_INFO, "Device: %ls", adapter_desc.Description);

    renderer->device = temp;
    temp = NULL;

    // command queue
    D3D12_COMMAND_QUEUE_DESC desc = { 0 };
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = ID3D12Device5_CreateCommandQueue(renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer->command_queue = temp;
    temp = NULL;

    hr = ID3D12Device5_CreateCommandQueue(renderer->device, &desc, &IID_ID3D12CommandQueue, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandQueue failed"); return false; }
    renderer->compute_command_queue = temp;
    temp = NULL;

    // swap chain
    DXGI_SAMPLE_DESC sample_desc = { 0 };
    sample_desc.Count = 1;

    DXGI_SWAP_CHAIN_DESC1 swap_desc = { 0 };
    swap_desc.BufferCount  = DM_MAX_FRAMES_IN_FLIGHT;
    swap_desc.Width        = renderer->width;
    swap_desc.Height       = renderer->height;
    swap_desc.BufferUsage  = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.SampleDesc   = sample_desc;
    swap_desc.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;        

    HWND hwnd = GetActiveWindow();

    hr = IDXGIFactory4_CreateSwapChainForHwnd(renderer->factory, (IUnknown*)renderer->command_queue, hwnd, &swap_desc, NULL,NULL, (IDXGISwapChain1**)&renderer->swap_chain);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGIFactory4_CreateSwapChainForHwnd failed"); return false; }

    IDXGIFactory4_MakeWindowAssociation(renderer->factory, hwnd, DXGI_MWA_NO_ALT_ENTER); 

    renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);

    // command allocators and lists
    const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer->command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer->device, 0, type, renderer->command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer->command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer->command_list[i]);
    }

    // open first for initial recording
    ID3D12GraphicsCommandList7_Reset(renderer->command_list[0], renderer->command_allocator[0], NULL);

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = ID3D12Device5_CreateCommandAllocator(renderer->device, type, &IID_ID3D12CommandAllocator, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandAllocator failed"); return false; }
        renderer->compute_command_allocator[i] = temp;
        temp = NULL;

        hr = ID3D12Device5_CreateCommandList(renderer->device, 0, type, renderer->compute_command_allocator[i], NULL, &IID_ID3D12GraphicsCommandList, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommandList failed"); return false; }
        renderer->compute_command_list[i] = temp;
        temp = NULL;

        ID3D12GraphicsCommandList7_Close(renderer->compute_command_list[i]);
    }

    // rtv heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer->rtv_heap, renderer)) return false;

    // depth stencil heap
    if(!dm_dx12_create_descriptor_heap(DM_MAX_FRAMES_IN_FLIGHT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, &renderer->depth_stencil_heap, renderer)) return false;

    // resource heap(s)
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_dx12_create_descriptor_heap(DM_DX12_MAX_RESOURCES, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer->resource_heap[i], renderer)) return false;
        if(!dm_dx12_create_descriptor_heap(DM_MAX_SAMPLERS, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, &renderer->sampler_heap[i], renderer)) return false;
    }

    // bindless root signature 
    D3D12_ROOT_PARAMETER1 root_params[1] = { 0 };
    root_params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].Constants.ShaderRegister = 0;
    root_params[0].Constants.RegisterSpace  = 0;
    root_params[0].Constants.Num32BitValues = 10;
    root_params[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = { 0 };
    root_sig_desc.Version                = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_sig_desc.Desc_1_1.Flags         = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
    root_sig_desc.Desc_1_1.NumParameters = 1;
    root_sig_desc.Desc_1_1.pParameters   = root_params;

    ID3DBlob* blob = NULL;
    hr = D3D12SerializeVersionedRootSignature(&root_sig_desc, &blob, NULL);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "D3D12SerializeVersionedRootSignature failed"); return false; }

    hr = ID3D12Device_CreateRootSignature(renderer->device, 0, ID3D10Blob_GetBufferPointer(blob), ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateRootSignature failed"); return false; }
    renderer->bindless_root_signature = temp;

    temp = NULL;
    ID3D10Blob_Release(blob);

    // render targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        hr = IDXGISwapChain4_GetBuffer(renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer->resources[renderer->resource_count] = temp;
        temp = NULL;
        ID3D12Resource* rt = renderer->resources[renderer->resource_count];
        renderer->render_targets[i] = renderer->resource_count++;

        D3D12_RENDER_TARGET_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer->rtv_heap.cpu_handle.current;

        ID3D12Device5_CreateRenderTargetView(renderer->device, rt, &view_desc, *handle);

        handle->ptr += renderer->rtv_heap.size;
    }

    // depth stencil targets
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = renderer->width;
        resource_desc.Height           = renderer->height;
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        void* temp = NULL;
        hr = ID3D12Device5_CreateCommittedResource(renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed"); dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed"); return false; }
        renderer->resources[renderer->resource_count] = temp;
        renderer->depth_stencil_targets[i] = renderer->resource_count++;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE* handle = &renderer->depth_stencil_heap.cpu_handle.current;

        ID3D12Device5_CreateDepthStencilView(renderer->device, renderer->resources[renderer->depth_stencil_targets[i]], &view_desc, *handle);

        handle->ptr += renderer->depth_stencil_heap.size;
    }

    // fence stuff
    // render
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer->fences[i].fence = temp;
        temp = NULL;
    }

    renderer->fences[0].value = 1;

    renderer->fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer->fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }

    // compute
    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        hr = ID3D12Device5_CreateFence(renderer->device, 0, flags, &IID_ID3D12Fence, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateFence failed"); return false; }
        renderer->compute_fences[i].fence = temp;
        temp = NULL;
    }

    renderer->compute_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!renderer->compute_fence_event) { dm_log(DM_LOG_FATAL, "CreateEvent failed"); return false; }
#elif defined(DM_METAL)

    
#elif defined(DM_VULKAN)
#ifdef DM_DEBUG
    dm_log(DM_LOG_DEBUG, "Initializing Vulkan renderer...");
#endif
#endif

    return true;
}

#ifdef DM_METAL
#endif

bool dm_finish_init(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    return dm_renderer_finish_init_backend(renderer->backend);
#ifdef DM_DIRECTX12
    HRESULT hr;

    ID3D12CommandQueue* command_queue = renderer->command_queue;

    ID3D12CommandList*  command_lists[] = { (ID3D12CommandList*)renderer->command_list[0] };

    ID3D12GraphicsCommandList7_Close(renderer->command_list[0]);
    ID3D12CommandQueue_ExecuteCommandLists(command_queue, _countof(command_lists), command_lists);

    hr =ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[0].fence, renderer->fences[0].value);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12CommandQueue_Signal failed"); return false; }

    hr = ID3D12Fence_SetEventOnCompletion(renderer->fences[0].fence, renderer->fences[0].value, renderer->fence_event);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "ID3D12Fence_SetEventOnCompletion failed"); return false; }

    WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);

    renderer->fences[0].value++;
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif

    return true;
}

void dm_renderer_shutdown(dm_renderer* renderer)
{
    dm_renderer_shutdown_backend(renderer->backend);
#ifdef DM_DIRECTX12
    HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        renderer->current_frame = i;
        ID3D12CommandQueue_Signal(renderer->command_queue, renderer->fences[i].fence, renderer->fences[i].value);
        if(!dm_dx12_wait_for_previous_frame(false, renderer)) { dm_log(DM_LOG_ERROR, "Waiting for previous frame failed"); continue; }
    }

    for(uint32_t i=0; i<renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_MAX_FRAMES_IN_FLIGHT; j++)
        {
            ID3D12Resource_Unmap(renderer->resources[renderer->constant_buffers[i].resource.device[j]], 0,0);
            renderer->constant_buffers[i].mapped_addresses[j] = NULL;
        }
    }

    for(uint32_t i=0; i<renderer->resource_count; i++)
    {
        ID3D12Resource_Release(renderer->resources[i]);
    }

    for(uint32_t i=0; i<renderer->rast_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rast_pipelines[i].state);
    }

#ifdef DM_HARDWARE_RAYTRACING
    for(uint32_t i=0; i<renderer->rt_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->rt_pipelines[i].pso);
    }
#endif

    for(uint32_t i=0; i<renderer->comp_pipe_count; i++)
    {
        ID3D12PipelineState_Release(renderer->compute_pipelines[i].state);
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12GraphicsCommandList7_Release(renderer->compute_command_list[i]);
        ID3D12CommandAllocator_Release(renderer->compute_command_allocator[i]);
        ID3D12Fence_Release(renderer->compute_fences[i].fence);

        ID3D12GraphicsCommandList7_Release(renderer->command_list[i]);
        ID3D12CommandAllocator_Release(renderer->command_allocator[i]);
        ID3D12Fence_Release(renderer->fences[i].fence);

        ID3D12DescriptorHeap_Release(renderer->resource_heap[i].heap);
        ID3D12DescriptorHeap_Release(renderer->sampler_heap[i].heap);
    }

    CloseHandle(renderer->compute_fence_event);
    CloseHandle(renderer->fence_event);

    ID3D12RootSignature_Release(renderer->bindless_root_signature);
    ID3D12DescriptorHeap_Release(renderer->rtv_heap.heap);
    ID3D12DescriptorHeap_Release(renderer->depth_stencil_heap.heap);
    IDXGISwapChain4_Release(renderer->swap_chain);
    ID3D12CommandQueue_Release(renderer->compute_command_queue);
    ID3D12CommandQueue_Release(renderer->command_queue);
    ID3D12Device5_Release(renderer->device);

#ifdef DM_DEBUG
    IDXGIDebug1* dbg = NULL;
    void* temp = NULL;
    hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_ERROR, "ID3D12Debug_QueryInterface failed"); ID3D12Debug_Release(renderer->debug); }
    else
    {
        dbg = temp;
        temp = NULL;
        ID3D12Debug_Release(renderer->debug);
        IDXGIDebug1_ReportLiveObjects(dbg, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        IDXGIDebug1_Release(dbg);
    }
#endif // DM_DEBUG
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif
}

extern bool dm_renderer_resize_backend(uint32_t width, uint32_t height, void* backend);

bool dm_renderer_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    renderer->width  = width;
    renderer->height = height;

    return dm_renderer_resize_backend(width, height, renderer->backend);
#ifdef DM_DIRECTX12
     HRESULT hr;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_dx12_fence* fence = &renderer->fences[i];
        size_t fence_value = fence->value++;
        ID3D12CommandQueue_Signal(renderer->command_queue, fence->fence, fence->value);
        if(ID3D12Fence_GetCompletedValue(fence->fence) < fence_value)
        {
            ID3D12Fence_SetEventOnCompletion(fence->fence, fence_value, renderer->fence_event);
            WaitForSingleObjectEx(renderer->fence_event, INFINITE, FALSE);
        }
    }

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource_Release(renderer->resources[renderer->render_targets[i]]);
        ID3D12Resource_Release(renderer->resources[renderer->depth_stencil_targets[i]]);
    }

    hr = IDXGISwapChain4_ResizeBuffers(renderer->swap_chain, DM_MAX_FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if(!dm_platform_win32_decode_hresult(hr)) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_ResizeBuffers failed"); return false; }

    void* temp = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle = renderer->rtv_heap.cpu_handle.begin;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_target_handle  = renderer->depth_stencil_heap.cpu_handle.begin;

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        ID3D12Resource* render_target = renderer->resources[renderer->render_targets[i]];

        hr = IDXGISwapChain4_GetBuffer(renderer->swap_chain, i, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "IDXGISwapChain4_GetBuffer failed"); return false; }
        renderer->resources[renderer->render_targets[i]] = temp;
        temp = NULL;

        D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        ID3D12Device_CreateRenderTargetView(renderer->device, renderer->resources[renderer->render_targets[i]], &desc, render_target_handle);
        render_target_handle.ptr += renderer->rtv_heap.size;

        // depth stencil
        D3D12_CLEAR_VALUE clear_value = { 0 };
        clear_value.Format             = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.f;

        D3D12_HEAP_PROPERTIES heap_properties = { 0 };
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = { 0 };
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Format           = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;

        hr = ID3D12Device5_CreateCommittedResource(renderer->device, &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, &IID_ID3D12Resource, &temp);
        if(!dm_platform_win32_decode_hresult(hr) || !temp)
        {
            dm_log(DM_LOG_FATAL, "ID3D12Device_CreateCommittedResource failed");
            dm_log(DM_LOG_ERROR, "Creating depth stencil buffer failed");
            return false;
        }
        renderer->resources[renderer->depth_stencil_targets[i]] = temp;

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = { 0 };
        view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags         = D3D12_DSV_FLAG_NONE;

        ID3D12Device5_CreateDepthStencilView(renderer->device, renderer->resources[renderer->depth_stencil_targets[i]], &view_desc, depth_target_handle);
        depth_target_handle.ptr += renderer->depth_stencil_heap.size;
    }

    renderer->current_frame = IDXGISwapChain4_GetCurrentBackBufferIndex(renderer->swap_chain);
#elif defined(DM_METAL)
    //CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    //renderer->swapchain.contentsScale = scale;
    //CGSize drawable_size;
    //drawable_size.width  = renderer->width;
    //drawable_size.height = renderer->height;
#elif defined(DM_VULKAN)
#endif

    return true;
}

/*************
 * RESOURCES *
 *************/
extern bool dm_create_renderpass_backend(dm_renderpass_desc desc, dm_renderpass_handle* handle, void* backend);
extern bool dm_create_raster_pipeline_backend(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, void* backend);
extern bool dm_create_vertex_buffer_backend(dm_vertex_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_index_buffer_backend(dm_index_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_constant_buffer_backend(dm_constant_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_storage_buffer_backend(dm_storage_buffer_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_texture_backend(dm_texture_desc desc, dm_resource_handle* handle, void* backend);
extern bool dm_create_sampler_backend(dm_sampler_desc desc, dm_resource_handle* handle, void* backend);

bool dm_create_renderpass(dm_renderpass_desc desc, dm_renderpass_handle* handle, dm_context* context)
{
    dm_renderer* renderer =  context->renderer;

    return dm_create_renderpass_backend(desc, handle, renderer->backend);
}

#ifdef DM_DIRECTX12
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_load_shader_data(const char* path, ID3D10Blob** blob)
{
    HRESULT hr;
    wchar_t ws[512];
    swprintf(ws, 512, L"%hs", path);
    hr = D3DReadFileToBlob(ws, blob);
    if(!dm_platform_win32_decode_hresult(hr))
    {
        dm_log(DM_LOG_FATAL, "D3DReadFileToBlob failed");
        dm_log(DM_LOG_ERROR, "Could not load shader: %s", path);
        return false;
    }

    return true;
}
#elif defined(DM_METAL)
#endif // DM_METAL

bool dm_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_pipeline_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_PIPELINE_TYPE_RASTER;

    return dm_create_raster_pipeline_backend(desc, handle, renderer->backend);
#ifdef DM_DIRECTX12
    HRESULT hr;

    dm_dx12_raster_pipeline pipeline = { 0 };

    void* temp = NULL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { 0 };
    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    // === rasterizer ===
    switch(desc.rasterizer.cull_mode)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown cull mode. Assuming D3D12_CULL_MODE_BACK");
        case DM_RASTERIZER_CULL_MODE_BACK:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        break;

        case DM_RASTERIZER_CULL_MODE_FRONT:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        break;

        case DM_RASTERIZER_CULL_MODE_NONE:
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        break;
    }

    switch(desc.rasterizer.polygon_fill)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown polygon fill mode. Assuming D3D12_FILL_MODE_SOLID");
        case DM_RASTERIZER_POLYGON_FILL_FILL:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        break;

        case DM_RASTERIZER_POLYGON_FILL_WIREFRAME:
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    }

    switch(desc.rasterizer.front_face)
    {
        case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        break;

        default:
        dm_log(DM_LOG_ERROR, "Unknown front face. Assuming counter clockwise");
        case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
        pso_desc.RasterizerState.FrontCounterClockwise = TRUE;
        break;
    }

    // TODO: needs to be configurable
    // === blending ===
    D3D12_RENDER_TARGET_BLEND_DESC blend_desc = { 0 };
    
    blend_desc.BlendEnable           = TRUE;
    blend_desc.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOp               = D3D12_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha         = D3D12_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.LogicOpEnable         = FALSE;
    blend_desc.LogicOp               = D3D12_LOGIC_OP_NOOP;

    // TODO: needs to be configurable
    // === depth/stencil ===
    D3D12_DEPTH_STENCIL_DESC depth_desc = { 0 };

    depth_desc.DepthEnable      = desc.depth_stencil.depth ? TRUE : FALSE;
    depth_desc.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
    depth_desc.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_desc.StencilEnable    = FALSE;
    depth_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_desc.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;

    // === shaders ===
    ID3DBlob* vs = NULL;
    ID3DBlob* ps = NULL;
    
    const char* vertex_path = desc.rasterizer.vertex_shader_desc.path;
    const char* pixel_path  = desc.rasterizer.pixel_shader_desc.path;

    if(!dm_dx12_load_shader_data(vertex_path, &vs)) { dm_log(DM_LOG_ERROR, "Could not load vertex shader"); return false; }
    if(!dm_dx12_load_shader_data(pixel_path,  &ps)) { dm_log(DM_LOG_ERROR, "Could not load pixel shader"); return false; }
    
    pso_desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vs);
    pso_desc.VS.BytecodeLength  = ID3D10Blob_GetBufferSize(vs);

    pso_desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(ps);
    pso_desc.PS.BytecodeLength  = ID3D10Blob_GetBufferSize(ps);

    // === input assembler ===
    D3D12_INPUT_ELEMENT_DESC input_element_descs[DM_RENDER_MAX_INPUT_ELEMENTS] = { 0 };
    uint8_t element_index = 0;

    for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
    {
        uint8_t matrix_count = desc.input_assembler.input_elements[i].format == DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4 ? 4 : 1;

        for(uint8_t j=0; j<matrix_count; j++)
        {
            input_element_descs[element_index].SemanticName      = desc.input_assembler.input_elements[i].name; 
            input_element_descs[element_index].SemanticIndex     = j;
            input_element_descs[element_index].InputSlot         = desc.input_assembler.input_elements[i].slot;
            input_element_descs[element_index].AlignedByteOffset = i==0 ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;

            switch(desc.input_assembler.input_elements[i].format)
            {
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32_FLOAT;
                break;

                default:
                dm_log(DM_LOG_ERROR, "Unknown input element format. Assuming DXGI_FORMAT_R32G32B32_FLOAT");
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;

                case DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4:
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
                input_element_descs[element_index].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            }

            switch(desc.input_assembler.input_elements[i].class)
            {
                default:
                dm_log(DM_LOG_ERROR, "Unknown input element class. Assuming D3D12_INPUT_CLASSIFICATION_PER_VERTEX");
                case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
                input_element_descs[element_index].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                break;

                case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
                input_element_descs[element_index].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                input_element_descs[element_index].InstanceDataStepRate = 1;
                input_element_descs[element_index].InputSlot            = 1;
                break;
            }

            element_index++;
        }
    }

    pso_desc.InputLayout.pInputElementDescs = input_element_descs;
    pso_desc.InputLayout.NumElements        = element_index;

    switch(desc.input_assembler.topology)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknow primitive topology. Assuming D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST");
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        pipeline.topology              = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    }

    // === viewport and scissor ===
    switch(desc.viewport.type)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown viewport type. Assumin DM_VIEWPORT_TYPE_DEFAULT");
        case DM_VIEWPORT_TYPE_DEFAULT:
        pipeline.viewport.Width    = (float)renderer->width;
        pipeline.viewport.Height   = (float)renderer->height;
        pipeline.viewport.MaxDepth = 1.f;
        break;

        case DM_VIEWPORT_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom viewport for dx12 pipeline not supported yet");
        return false;
    }

    switch(desc.scissor.type)
    {
        default:
        dm_log(DM_LOG_ERROR, "Unknown scissor type. Assuming DM_SCISSOR_TYPE_DEFAULT");
        case DM_SCISSOR_TYPE_DEFAULT:
        pipeline.scissor.right  = (float)renderer->width;
        pipeline.scissor.bottom = (float)renderer->height;
        break;

        case DM_SCISSOR_TYPE_CUSTOM:
        dm_log(DM_LOG_FATAL, "Custom scissor for dx12 not supported yet");
        return false;
    }
    
    // === pipeline state ===
    pso_desc.RTVFormats[0]              = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count           = 1;
    pso_desc.SampleMask                 = 0xffffffff;
    pso_desc.NumRenderTargets           = 1;
    pso_desc.BlendState.RenderTarget[0] = blend_desc;
    pso_desc.DepthStencilState          = depth_desc;
    pso_desc.DSVFormat                  = DXGI_FORMAT_D32_FLOAT;
    pso_desc.pRootSignature             = renderer->bindless_root_signature;

    hr = ID3D12Device5_CreateGraphicsPipelineState(renderer->device, &pso_desc, &IID_ID3D12PipelineState, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp)
    {
        dm_log(DM_LOG_FATAL, "ID3D12Device5_CreatePipelineState failed");
        dm_dx12_get_debug_message(renderer);
        return false;
    }
    pipeline.state = temp;
    temp = NULL;
    
    ID3D10Blob_Release(vs);
    ID3D10Blob_Release(ps);

    // 
    renderer->rast_pipelines[renderer->rast_pipe_count] = pipeline;
    handle->index = renderer->rast_pipe_count++;
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif
}

#ifdef DM_DIRECTX12
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_committed_resource(D3D12_HEAP_PROPERTIES properties, D3D12_RESOURCE_DESC desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** resource, ID3D12Device5* device)
{
    void* temp = NULL;
    HRESULT hr = ID3D12Device5_CreateCommittedResource(device, &properties, flags, &desc, state, 0, &IID_ID3D12Resource, &temp);
    if(!dm_platform_win32_decode_hresult(hr) || !temp) { dm_log(DM_LOG_FATAL, "ID3D12Device5_CreateCommittedResource failed"); return false; }

    *resource = temp;
    temp = NULL;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_copy_memory(ID3D12Resource* resource, const void* data, size_t size)
{
    void* temp = NULL;

    D3D12_RANGE range = { 0 };
    range.End = size-1;
    
    ID3D12Resource_Map(resource, 0, &range, &temp);
    if(!temp) { dm_log(DM_LOG_FATAL, "ID3D12Resource_Map failed"); return false; }

    dm_memcpy(temp, data, size);
    ID3D12Resource_Unmap(resource, 0, &range);
    
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_dx12_create_buffer(const size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Resource** resource, dm_renderer* renderer)
{
    D3D12_RESOURCE_DESC desc = { 0 };
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width            = size;
    desc.Height           = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags            = flags;

    D3D12_HEAP_PROPERTIES heap_desc = { 0 };
    heap_desc.Type = heap_type;

    if(!dm_dx12_create_committed_resource(heap_desc, desc, D3D12_HEAP_FLAG_NONE, state, resource, renderer->device)) return false;  

    return true;
}
#elif defined(DM_METAL)
#endif  

bool dm_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_VERTEX_BUFFER;

    return dm_create_vertex_buffer_backend(desc, handle, renderer->backend);
#ifdef DM_DIRECTX12
    dm_dx12_vertex_buffer buffer = { 0 };

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "vertex_host_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "vertex_device_buffer", renderer->vb_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;

        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].StrideInBytes  = desc.stride;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.NumElements         = desc.size / desc.stride;
        view_desc.Buffer.StructureByteStride = desc.stride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    } 

    //
    renderer->vertex_buffers[renderer->vb_count] = buffer;
    handle->index = renderer->vb_count++;
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif
}

bool dm_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_INDEX_BUFFER;

    return dm_create_index_buffer_backend(desc, handle, renderer->backend);
#ifdef DM_DIRECTX12
    dm_dx12_index_buffer buffer = { 0 };
    
    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_format = DXGI_FORMAT_R16_UINT;
        break;

        default:
        dm_log(DM_LOG_ERROR, "Unknown index type. Assuming uint32");
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_format = DXGI_FORMAT_R32_UINT;
        break;
    }

    ID3D12GraphicsCommandList7* command_list = renderer->command_list[0];

    for(uint8_t i=0; i<DM_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer 
        ID3D12Resource** host_buffer   = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, host_buffer, renderer)) return false;
#ifdef DM_DEBUG
        wchar_t name[512];
        swprintf(name, 512, L"%hs %d.%d", "index_host_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*host_buffer, name);
#endif
        buffer.resource.host[i] = renderer->resource_count++;

        // device buffer and its view
        ID3D12Resource** device_buffer = &renderer->resources[renderer->resource_count];
        if(!dm_dx12_create_buffer(desc.size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE, device_buffer, renderer)) return false;
#ifdef DM_DEBUG
        swprintf(name, 512, L"%hs %d.%d", "index_device_buffer", renderer->ib_count, i);
        ID3D12Resource_SetName(*device_buffer, name);
#endif
        buffer.resource.device[i] = renderer->resource_count++;
        
        buffer.views[i].BufferLocation = ID3D12Resource_GetGPUVirtualAddress(*device_buffer);
        buffer.views[i].SizeInBytes    = desc.size;
        buffer.views[i].Format         = buffer.index_format;

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
        view_desc.Format                     = DXGI_FORMAT_UNKNOWN;
        view_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        switch(desc.index_type)
        {
            case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
            view_desc.Buffer.StructureByteStride = sizeof(uint16_t);
            break;

            case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
            view_desc.Buffer.StructureByteStride = sizeof(uint32_t);
            break;

            default:
            dm_log(DM_LOG_FATAL, "Should NOT be here");
            return false;
        }
        view_desc.Buffer.NumElements         = desc.size / view_desc.Buffer.StructureByteStride;
        
        buffer.resource.descriptor_index[i] = renderer->resource_heap[i].count;
        ID3D12Device5_CreateShaderResourceView(renderer->device, *device_buffer, &view_desc, renderer->resource_heap[i].cpu_handle.current);
        renderer->resource_heap[i].cpu_handle.current.ptr += renderer->resource_heap[i].size;
        renderer->resource_heap[i].count++;

        // data copy
        if(!desc.data) continue;

        if(!dm_dx12_copy_memory(*host_buffer, desc.data, desc.size)) return false;

        ID3D12GraphicsCommandList7_CopyBufferRegion(command_list, *device_buffer, 0, *host_buffer, 0, desc.size);

        D3D12_RESOURCE_BARRIER barrier = { 0 };

        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.pResource   = *device_buffer;

        ID3D12GraphicsCommandList7_ResourceBarrier(command_list, 1, &barrier);
    }

    //
    renderer->index_buffers[renderer->ib_count] = buffer;
    handle->index = renderer->ib_count++;
#elif defined(DM_METAL)
#elif defined(DM_VULKAN)
#endif
}

bool dm_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_CONSTANT_BUFFER;

    return dm_create_constant_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_STORAGE_BUFFER;

    return dm_create_storage_buffer_backend(desc, handle, renderer->backend);
}

bool dm_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    
    handle->type = DM_RESOURCE_TYPE_TEXTURE;

    return dm_create_texture_backend(desc, handle, renderer->backend);
}

bool dm_create_texture_from_file(const char* path, dm_resource_handle* handle, dm_context* context)
{
    int x,y,n;
    void* data = stbi_load(path, &x,&y,&n,4);
    if(!data)
    {
        dm_log(DM_LOG_FATAL, "Could not load texture: %s", path);
        return false;
    }

    dm_texture_desc desc = { 
        .width=x,.height=y,.n_channels=4,
        .data=data,
        .format=DM_TEXTURE_FORMAT_BYTE_4_UNORM
    };

    bool result = dm_create_texture(desc, handle, context);

    stbi_image_free(data);
    return result;
}

#ifdef DM_METAL
#endif // DM_METAL

bool dm_create_sampler(dm_sampler_desc desc, dm_resource_handle* handle, dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    handle->type = DM_RESOURCE_TYPE_SAMPLER;

    return dm_create_sampler_backend(desc, handle, renderer->backend);
}

extern bool dm_render_command_begin_frame_backend(void* backend);
extern bool dm_render_command_end_frame_backend(bool vsync, void* backend);
extern bool dm_render_command_begin_update_backend(void* backend);
extern bool dm_render_command_end_update_backend(void* backend);
extern bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, void* backend);
extern bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, void* backend);
extern bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, void* backend);
extern bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, void* backend);
extern bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, void* backend);
extern bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, void* backend);
extern bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, void* backend);
extern bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, void* backend);
extern bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, void* backend);

bool dm_submit_render_commands(dm_context* context)
{
    dm_renderer* renderer = context->renderer;
    dm_command_buffer buffer = renderer->render_commands;
    void* backend = renderer->backend;

    for(uint32_t i=0; i<buffer.command_count; i++)
    {
        dm_command command = buffer.commands[i];
        dm_command_param* params = command.params;

        switch(command.r_type)
        {
            case DM_RENDER_COMMAND_TYPE_BEGIN_FRAME:
            if(dm_render_command_begin_frame_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin frame failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_FRAME:
            if(dm_render_command_end_frame_backend((context->flags & DM_CONTEXT_FLAG_VSYNC_ON), backend)) continue;
            dm_log(DM_LOG_FATAL, "End frame failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BEGIN_UPDATE:
            if(dm_render_command_begin_update_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin update failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_UPDATE:
            if(dm_render_command_end_update_backend(backend)) continue;
            dm_log(DM_LOG_FATAL, "End update failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BEGIN_RENDER_PASS:
            if(dm_render_command_begin_render_pass_backend(params[0].rph,params[1].f,params[2].f,params[3].f,params[4].f,params[5].f, backend)) continue;
            dm_log(DM_LOG_FATAL, "Begin render pass failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_END_RENDER_PASS:
            if(dm_render_command_end_render_pass_backend(params[0].rph, backend)) continue;
            dm_log(DM_LOG_FATAL, "End render pass failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_RASTER_PIPELINE:
            if(dm_render_command_bind_raster_pipeline_backend(params[0].ph, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind raster pipeline failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_SUBMIT_RESOURCES:
            if(dm_render_command_submit_resources_backend(params[0].v,params[1].u16, backend)) continue;
            dm_log(DM_LOG_FATAL, "Submit resources failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER:
            if(dm_render_command_bind_vertex_buffer_backend(params[0].rh, params[1].u32, params[2].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER:
            if(dm_render_command_bind_index_buffer_backend(params[0].rh, params[1].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Bind index buffer failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_UPDATE_VERTEX_BUFFER:
            if(dm_render_command_update_vertex_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update vertex buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_INDEX_BUFFER:
            if(dm_render_command_update_index_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update index buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_CONSTANT_BUFFER:
            if(dm_render_command_update_constant_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update constant buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_STORAGE_BUFFER:
            if(dm_render_command_update_storage_buffer_backend(params[0].rh, params[1].v,params[2].s,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update storage buffer failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_UPDATE_TEXTURE:
            if(dm_render_command_update_texture_backend(params[0].rh, params[1].u16,params[2].u16,params[3].v,params[4].s,params[5].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Update texture failed");
            return false;

            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED:
            if(dm_render_command_draw_instanced_backend(params[0].u32,params[1].s,params[2].u32,params[3].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Draw instanced failed");
            return false;
            case DM_RENDER_COMMAND_TYPE_DRAW_INSTANCED_INDEXED:
            if(dm_render_command_draw_instanced_indexed_backend(params[0].u32,params[1].s,params[2].u32,params[3].s,params[4].s, backend)) continue;
            dm_log(DM_LOG_FATAL, "Draw instanced indexed failed");
            return false;

            default:
            dm_log(DM_LOG_ERROR, "Unknown render command, shouldn't be here...");
            break;
        }
    }

    renderer->render_commands.command_count = 0;
    return true;
}

/**********
* CONTEXT *
***********/
extern bool dm_window_create(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags, dm_context* context);
extern void dm_window_shutdown(void* window);
extern bool dm_window_poll_events(void* window);

dm_context* dm_init(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title, dm_window_create_flag flags)
{
    dm_context* context = dm_alloc(sizeof(dm_context));
    if(!context) { dm_log(DM_LOG_FATAL, "creating context failed"); return NULL; }

    if(!dm_window_create(x,y,width,height,title,flags, context)) return false;
    if(!dm_renderer_init(context)) return false;

    return context;
}

void dm_shutdown(dm_context* context)
{
    dm_renderer_shutdown(context->renderer);
    dm_window_shutdown(context->window);

    dm_free((void**)&context->renderer);
    dm_free((void**)&context->window);
    dm_free((void**)&context);
}

bool dm_update(dm_context* context)
{
    dm_renderer* renderer = context->renderer;

    if(!dm_window_poll_events(context->window)) return false;

    uint16_t width = dm_get_window_width(context);
    uint16_t height = dm_get_window_height(context);

    if(renderer->width != width || renderer->height != height)
    {
        if(!dm_renderer_resize(width, height, renderer)) return false;
    }

    return true;
}
