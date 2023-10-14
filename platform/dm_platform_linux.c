#include "dm.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

#include <pthread.h>
#include <errno.h>
#include <sys/sysinfo.h>

#ifdef DM_OPENGL
#include <glad/glad.h>
#include <GL/glx.h>

typedef GLXContext (*glXCreateContextAttribARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#endif

#include <pthread.h>

typedef struct dm_linux_semaphore_t
{
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    uint32_t v;
} dm_linux_semaphore;

typedef struct dm_linux_task_queue_t
{
    dm_thread_task tasks[DM_MAX_THREAD_COUNT];
    uint32_t       count;

    pthread_mutex_t    mutex;
    dm_linux_semaphore has_tasks;
} dm_linux_task_queue;

typedef struct dm_linux_threadpool_t
{
    pthread_mutex_t thread_count_mutex;
    pthread_cond_t  all_threads_idle;
    pthread_cond_t  at_least_one_idle;

    uint32_t num_working_threads;

    dm_linux_task_queue task_queue;

    pthread_t threads[DM_MAX_THREAD_COUNT];
} dm_linux_threadpool;

typedef struct dm_internal_linux_data_t
{
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} dm_internal_linux_data;

#define DM_LINUX_GET_DATA dm_internal_linux_data* linux_data = platform_data->internal_data

dm_key_code dm_linux_translate_key_code(uint32_t x_keycode);
void* dm_linux_thread_start_func(void* args);

bool dm_platform_init(uint32_t window_x_pos, uint32_t window_y_pos, dm_platform_data* platform_data)
{
    platform_data->internal_data = dm_alloc(sizeof(dm_internal_linux_data));
    dm_internal_linux_data* linux_data = platform_data->internal_data;
    
    linux_data->display = XOpenDisplay(NULL);
    
    //XAutoRepeatOff(linux_data->display);
    
    int screen_p = 0;
    linux_data->connection = XGetXCBConnection(linux_data->display);
    
    if(xcb_connection_has_error(linux_data->connection))
    {
        DM_LOG_FATAL("Failed to connect ot X server via XCB!");
        return false;
    }
    
    const struct xcb_setup_t* setup = xcb_get_setup(linux_data->connection);
    
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    for(uint32_t s=screen_p;s>0;s--)
    {
        xcb_screen_next(&it);    
    }
    
    linux_data->screen = it.data;
    
    linux_data->window = xcb_generate_id(linux_data->connection);
    
    uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    
    uint32_t event_values = FocusChangeMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
        PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask |
        EnterWindowMask | LeaveWindowMask;
    
    uint32_t value_list[] = {linux_data->screen->black_pixel, event_values};
    
    xcb_create_window(linux_data->connection,
                      XCB_COPY_FROM_PARENT,
                      linux_data->window,
                      linux_data->screen->root,
                      window_x_pos, window_y_pos,
                      platform_data->window_data.width, platform_data->window_data.height,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      linux_data->screen->root_visual,
                      event_mask,
                      value_list);
    
    xcb_change_property(linux_data->connection,
                        XCB_PROP_MODE_REPLACE,
                        linux_data->window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(platform_data->window_data.title),
                        platform_data->window_data.title);
    
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(linux_data->connection,
                                                                0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(linux_data->connection,
                                                                   0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(linux_data->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(linux_data->connection, wm_protocols_cookie, NULL);
    
    linux_data->wm_delete_win = wm_delete_reply->atom;
    linux_data->wm_protocols = wm_protocols_reply->atom;
    
    xcb_change_property(linux_data->connection,
                        XCB_PROP_MODE_REPLACE,
                        linux_data->window,
                        wm_protocols_reply->atom,
                        4, 32, 1,
                        &wm_delete_reply->atom);
    
    xcb_map_window(linux_data->connection, linux_data->window);
    
    int stream_result = xcb_flush(linux_data->connection);
    if(stream_result<=0)
    {
        DM_LOG_FATAL("Error occured when flushing the screen: %d", stream_result);
        return false;
    }
    
    return true;
}

double dm_platform_get_time(dm_platform_data* platform_data)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec*0.000000001;
}

void dm_platform_shutdown(dm_platform_data* platform_data)
{
    DM_LINUX_GET_DATA;
    
    xcb_destroy_window(linux_data->connection, linux_data->window);
    dm_free(platform_data->internal_data);
}

bool dm_platform_pump_events(dm_platform_data* platform_data)
{
    DM_LINUX_GET_DATA;
    
    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;
    
    while((event=xcb_poll_for_event(linux_data->connection)))
    {
        switch(event->response_type & ~0x80)
        {
            case XCB_CLIENT_MESSAGE:
            {
                cm = (xcb_client_message_event_t*)event;
                
                if(cm->data.data32[0]==linux_data->wm_delete_win) dm_add_window_close_event(&platform_data->event_list);
            } break;
            
            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t* kb_event = (xcb_key_press_event_t*)event;
                xcb_keycode_t code = kb_event->detail;
                
                KeySym key_sym = XkbKeycodeToKeysym(linux_data->display, (KeyCode)code, 0, code & ShiftMask ? 1 : 0);
                
                dm_key_code key = dm_linux_translate_key_code(key_sym);
                dm_add_key_down_event(key, &platform_data->event_list);
            } break;
            case XCB_KEY_RELEASE:
            {
                xcb_key_release_event_t* kb_event = (xcb_key_release_event_t*)event;
                xcb_keycode_t code = kb_event->detail;
                
                KeySym key_sym = XkbKeycodeToKeysym(linux_data->display, (KeyCode)code, 0, code & ShiftMask ? 1 : 0);
                
                dm_key_code key = dm_linux_translate_key_code(key_sym);
                dm_add_key_up_event(key, &platform_data->event_list);
            } break;
            
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t* m_event = (xcb_button_press_event_t*)event;
                
                dm_mousebutton_code button = DM_MOUSEBUTTON_UNKNOWN;
                
                switch (m_event->detail)
                {
                    case XCB_BUTTON_INDEX_1: 
                    button = DM_MOUSEBUTTON_L;
                    break;
                    case XCB_BUTTON_INDEX_2:
                    button = DM_MOUSEBUTTON_M;
                    break;
                    case XCB_BUTTON_INDEX_3:
                    button = DM_MOUSEBUTTON_R;
                    break;
                }
                
                if (button!=DM_MOUSEBUTTON_UNKNOWN) dm_add_mousebutton_down_event(button, &platform_data->event_list); 
            } break;
            case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t* m_event = (xcb_button_release_event_t*)event;
                
                dm_mousebutton_code button = DM_MOUSEBUTTON_UNKNOWN;
                
                switch (m_event->detail)
                {
                    case XCB_BUTTON_INDEX_1: 
                    button = DM_MOUSEBUTTON_L;
                    break;
                    case XCB_BUTTON_INDEX_2:
                    button = DM_MOUSEBUTTON_M;
                    break;
                    case XCB_BUTTON_INDEX_3:
                    button = DM_MOUSEBUTTON_R;
                    break;
                }
                
                if (button!=DM_MOUSEBUTTON_UNKNOWN) dm_add_mousebutton_up_event(button, &platform_data->event_list); 
            } break;
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t* m_event = (xcb_motion_notify_event_t*)event;
                dm_add_mouse_move_event(m_event->event_x,m_event->event_y, &platform_data->event_list);
            } break;
            
            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t* c_event = (xcb_configure_notify_event_t*)event;
                dm_add_window_resize_event(c_event->width, c_event->height, &platform_data->event_list);
            } break;
            
            default:
            break;
        }
        
        free(event);
    }
    
    return true;
}

void dm_platform_write(const char* message, uint8_t color)
{
    static char* levels[6] = {
        "1;30",   // white
        "1;34",   // blue
        "1;32",   // green
        "1;33",   // yellow
        "1;31",   // red
        "0;41"    // highlighted red
    };
    
    char out[5000];
    sprintf(out,
            "\033[%sm%s \033[0m",
            levels[color], message);
    
    printf("%s", out);
}

#ifdef DM_OPENGL
bool dm_platform_init_opengl(dm_platform_data* platform_data)
{
    DM_LINUX_GET_DATA;
    
    int visual_attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };
    
    int num_fbc = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(linux_data->display, DefaultScreen(linux_data->display), visual_attribs, &num_fbc);
    if(!fbc)
    {
        DM_LOG_FATAL("glxChooseFBConfig failed!");
        return false;
    }
    
    glXCreateContextAttribARBProc glxCreateContextAttribsARB = 0;
    glxCreateContextAttribsARB = (glXCreateContextAttribARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
    if(!glxCreateContextAttribsARB)
    {
        DM_LOG_FATAL("glxCreateContextAttribsARB failed!");
        return false;
    }
    
    int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, DM_OPENGL_MAJOR,
        GLX_CONTEXT_MINOR_VERSION_ARB, DM_OPENGL_MINOR,
        None
    };
    
    GLXContext context = glxCreateContextAttribsARB(linux_data->display, fbc[0], NULL, true, context_attribs);
    
    if(!context)
    {
        DM_LOG_FATAL("Failed to create OpenGL context!");
        return false;
    }
    
    glXMakeCurrent(linux_data->display, linux_data->window, context);
    
    if(!gladLoadGL())
    {
        DM_LOG_FATAL("Failed to initialize GLAD!");
        return false;
    }
    
    XFree(fbc);
    
    return true;
}

void dm_platform_shutdown_opengl(dm_platform_data* platform_data)
{
    
}

void dm_platform_swap_buffers(bool vsync, dm_platform_data* platform_data)
{
    DM_LINUX_GET_DATA;
    
    glXSwapBuffers(linux_data->display, linux_data->window);
}
#endif

dm_key_code dm_linux_translate_key_code(uint32_t x_keycode)
{
    switch(x_keycode)
    {
        case XK_BackSpace: return DM_KEY_BACKSPACE;
        case XK_Return: return DM_KEY_ENTER;
        case XK_Tab: return DM_KEY_TAB;
        case XK_Escape: return DM_KEY_ESCAPE;
        case XK_space: return DM_KEY_SPACE;
        case XK_End: return DM_KEY_END;
        case XK_Home: return DM_KEY_HOME;
        case XK_Left: return DM_KEY_LEFT;
        case XK_Right: return DM_KEY_RIGHT;
        case XK_Up: return DM_KEY_UP;
        case XK_Down: return DM_KEY_DOWN;
        case XK_Print: return DM_KEY_PRINT;
        case XK_Insert: return DM_KEY_INSERT;
        case XK_Delete: return DM_KEY_DELETE;
        
        case XK_KP_0: return DM_KEY_NUMPAD_0;
        case XK_KP_1: return DM_KEY_NUMPAD_1;
        case XK_KP_2: return DM_KEY_NUMPAD_2;
        case XK_KP_3: return DM_KEY_NUMPAD_3;
        case XK_KP_4: return DM_KEY_NUMPAD_4;
        case XK_KP_5: return DM_KEY_NUMPAD_5;
        case XK_KP_6: return DM_KEY_NUMPAD_6;
        case XK_KP_7: return DM_KEY_NUMPAD_7;
        case XK_KP_8: return DM_KEY_NUMPAD_8;
        case XK_KP_9: return DM_KEY_NUMPAD_9;
        case XK_minus: return DM_KEY_MINUS;
        case XK_KP_Add: return DM_KEY_ADD;
        case XK_KP_Subtract: return DM_KEY_SUBTRACT;
        case XK_KP_Decimal: return DM_KEY_DECIMAL;
        case XK_KP_Divide: return DM_KEY_DIVIDE;
        case XK_KP_Multiply: return DM_KEY_MULTIPLY;
        
        case XK_F1: return DM_KEY_F1;
        case XK_F2: return DM_KEY_F1;
        case XK_F3: return DM_KEY_F1;
        case XK_F4: return DM_KEY_F1;
        case XK_F5: return DM_KEY_F1;
        case XK_F6: return DM_KEY_F1;
        case XK_F7: return DM_KEY_F1;
        case XK_F8: return DM_KEY_F1;
        case XK_F9: return DM_KEY_F1;
        case XK_F10: return DM_KEY_F1;
        case XK_F11: return DM_KEY_F1;
        case XK_F12: return DM_KEY_F1;
        
        case XK_Num_Lock: return DM_KEY_NUMLCK;
        case XK_Scroll_Lock: return DM_KEY_SCRLLCK;
        
        case XK_Shift_L: return DM_KEY_LSHIFT;
        case XK_Shift_R: return DM_KEY_RSHIFT;
        case XK_Control_L: return DM_KEY_LCTRL;
        case XK_Control_R: return DM_KEY_RCTRL;
        
        case XK_plus: return DM_KEY_PLUS;
        case XK_comma: return DM_KEY_COMMA;
        case XK_period: return DM_KEY_PERIOD;
        case XK_slash: return DM_KEY_LSLASH;
        
        case XK_a:
        case XK_A: return DM_KEY_A;
        case XK_b:
        case XK_B: return DM_KEY_B;
        case XK_c:
        case XK_C: return DM_KEY_C;
        case XK_d:
        case XK_D: return DM_KEY_D;
        case XK_e:
        case XK_E: return DM_KEY_E;
        case XK_f:
        case XK_F: return DM_KEY_F;
        case XK_g:
        case XK_G: return DM_KEY_G;
        case XK_h:
        case XK_H: return DM_KEY_H;
        case XK_i:
        case XK_I: return DM_KEY_I;
        case XK_j:
        case XK_J: return DM_KEY_J;
        case XK_k:
        case XK_K: return DM_KEY_K;
        case XK_l:
        case XK_L: return DM_KEY_L;
        case XK_m:
        case XK_M: return DM_KEY_M;
        case XK_n:
        case XK_N: return DM_KEY_N;
        case XK_o:
        case XK_O: return DM_KEY_O;
        case XK_p:
        case XK_P: return DM_KEY_P;
        case XK_q:
        case XK_Q: return DM_KEY_Q;
        case XK_r:
        case XK_R: return DM_KEY_R;
        case XK_s:
        case XK_S: return DM_KEY_S;
        case XK_t:
        case XK_T: return DM_KEY_T;
        case XK_u:
        case XK_U: return DM_KEY_U;
        case XK_v:
        case XK_V: return DM_KEY_V;
        case XK_w:
        case XK_W: return DM_KEY_W;
        case XK_x:
        case XK_X: return DM_KEY_X;
        case XK_y:
        case XK_Y: return DM_KEY_Y;
        case XK_z:
        case XK_Z: return DM_KEY_Z;
        
        default: return 0;
    }
}

#ifdef DM_VULKAN
bool dm_platform_create_vulkan_surface(dm_platform_data* platform_data, VkInstance* instance, VkSurfaceKHR* surface)
{
    DM_LINUX_GET_DATA;
    
    VkXcbSurfaceCreateInfoKHR create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.connection = linux_data->connection;
    create_info.window = linux_data->window;
    
    VkResult result = vkCreateXcbSurfaceKHR(*instance, &create_info, NULL, surface);
    if(result==VK_SUCCESS) return true;
    
    DM_LOG_FATAL("Could not create Win32 Vulkan surface");
    return false;
}
#endif

/**********
THREADPOOL
************/
/**********
THREADPOOL
************/
bool dm_platform_threadpool_create(dm_threadpool* threadpool)
{
    threadpool->internal_pool = dm_alloc(sizeof(dm_linux_threadpool));
    dm_linux_threadpool* linux_threadpool = threadpool->internal_pool;

    pthread_mutex_init(&linux_threadpool->thread_count_mutex, NULL);
    pthread_mutex_init(&linux_threadpool->task_queue.mutex, NULL);
    pthread_mutex_init(&linux_threadpool->task_queue.has_tasks.mutex, NULL);
    pthread_cond_init(&linux_threadpool->task_queue.has_tasks.cond, NULL);
    pthread_cond_init(&linux_threadpool->all_threads_idle, NULL);
    pthread_cond_init(&linux_threadpool->at_least_one_idle, NULL);

    for(uint32_t i=0; i<threadpool->thread_count; i++)
    {
        if(pthread_create(&linux_threadpool->threads[i], NULL, &dm_linux_thread_start_func, threadpool) == 0) continue;

        DM_LOG_FATAL("Could not create pthread");
        return false;
    }

    return true;
}

void dm_platform_threadpool_destroy(dm_threadpool* threadpool)
{
    dm_linux_threadpool* linux_threadpool = threadpool->internal_pool;

    pthread_mutex_destroy(&linux_threadpool->thread_count_mutex);
    pthread_mutex_destroy(&linux_threadpool->task_queue.mutex);
    pthread_mutex_destroy(&linux_threadpool->task_queue.has_tasks.mutex);
    pthread_cond_destroy(&linux_threadpool->task_queue.has_tasks.cond);
    pthread_cond_destroy(&linux_threadpool->all_threads_idle);
    pthread_cond_destroy(&linux_threadpool->at_least_one_idle);

    dm_free(threadpool->internal_pool);
}

void dm_platform_threadpool_submit_task(dm_thread_task* task, dm_threadpool* threadpool)
{
    dm_linux_threadpool* linux_threadpool = threadpool->internal_pool;

    // insert task
    pthread_mutex_lock(&linux_threadpool->task_queue.mutex);
    dm_memcpy(linux_threadpool->task_queue.tasks + linux_threadpool->task_queue.count, task, sizeof(dm_thread_task));
    linux_threadpool->task_queue.count++;

    // wake up at least one thread
    pthread_mutex_lock(&linux_threadpool->task_queue.has_tasks.mutex);
    linux_threadpool->task_queue.has_tasks.v = 1;
    pthread_cond_signal(&linux_threadpool->task_queue.has_tasks.cond);
    pthread_mutex_unlock(&linux_threadpool->task_queue.has_tasks.mutex);

    pthread_mutex_unlock(&linux_threadpool->task_queue.mutex);
}

void dm_platform_threadpool_wait_for_completion(dm_threadpool* threadpool)
{
    dm_linux_threadpool* linux_threadpool = threadpool->internal_pool;

    // sleep until there are NO working threads AND the task queue is empty
    pthread_mutex_lock(&linux_threadpool->thread_count_mutex);
    while(linux_threadpool->num_working_threads || linux_threadpool->task_queue.count) 
    {
        pthread_cond_wait(&linux_threadpool->all_threads_idle, &linux_threadpool->thread_count_mutex);
    }
    pthread_mutex_unlock(&linux_threadpool->thread_count_mutex);
}

void* dm_linux_thread_start_func(void* args)
{
    dm_threadpool* threadpool = args;
    dm_linux_threadpool* linux_threadpool = threadpool->internal_pool;
    
    while(1)
    {
        // sleep until a new task is available
        pthread_mutex_lock(&linux_threadpool->task_queue.has_tasks.mutex);
        while(linux_threadpool->task_queue.has_tasks.v==0)
        {
            pthread_cond_wait(&linux_threadpool->task_queue.has_tasks.cond, &linux_threadpool->task_queue.has_tasks.mutex);
        }
        linux_threadpool->task_queue.has_tasks.v = 0;
        pthread_mutex_unlock(&linux_threadpool->task_queue.has_tasks.mutex);

        // iterate thread working counter
        pthread_mutex_lock(&linux_threadpool->thread_count_mutex);
        linux_threadpool->num_working_threads++;
        pthread_mutex_unlock(&linux_threadpool->thread_count_mutex);

        // grab task
        pthread_mutex_lock(&linux_threadpool->task_queue.mutex);
        dm_thread_task* task = NULL;

        // is there still a task since we've woken up?
        if(linux_threadpool->task_queue.count)
        {
            task = dm_alloc(sizeof(dm_thread_task));
            dm_memcpy(task, &linux_threadpool->task_queue.tasks[0], sizeof(dm_thread_task));
            dm_memmove(linux_threadpool->task_queue.tasks, linux_threadpool->task_queue.tasks + 1, sizeof(dm_thread_task) * linux_threadpool->task_queue.count-1);
            linux_threadpool->task_queue.count--;
        }

        // if queue is still populated, need to set semaphore back
        if(linux_threadpool->task_queue.count)
        {
            pthread_mutex_lock(&linux_threadpool->task_queue.has_tasks.mutex);
            linux_threadpool->task_queue.has_tasks.v = 1;
            pthread_cond_signal(&linux_threadpool->task_queue.has_tasks.cond);
            pthread_mutex_unlock(&linux_threadpool->task_queue.has_tasks.mutex);
        }
        pthread_mutex_unlock(&linux_threadpool->task_queue.mutex);

        if(task)
        {
            task->func(task->args);
            dm_free(task);
        }

        // decrement thread working counter
        pthread_mutex_lock(&linux_threadpool->thread_count_mutex);
        linux_threadpool->num_working_threads--;
        if(linux_threadpool->num_working_threads==0) pthread_cond_signal(&linux_threadpool->all_threads_idle);
        //else pthread_cond_signal(&linux_threadpool->at_least_one_idle);
        pthread_mutex_unlock(&linux_threadpool->thread_count_mutex);
    }

    return NULL;
}