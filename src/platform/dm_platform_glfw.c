#include "dm_platform.h"

#ifdef DM_PLATFORM_GLFW

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_event.h"
#include "core/dm_assert.h"
#include "input/dm_input.h"

typedef struct dm_internal_data
{
    GLFWwindow* internal_window;
} dm_internal_data;

dm_internal_data* glfw_data = NULL;

// forward declaration of glfw callbacks
void dm_platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void dm_platform_glfw_char_callback(GLFWwindow* window, unsigned int key);
void dm_platform_glfw_window_close_callback(GLFWwindow* window);
void dm_platform_glfw_window_resize_callback(GLFWwindow* window, int width, int height);
void dm_platform_glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void dm_platform_glfw_mouse_move_callback(GLFWwindow* window, double xPos, double yPos);
void dm_platform_glfw_mouse_scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

dm_key_code dm_translate_key_code(uint32_t glfw_key_code);

bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title, int start_x, int start_y)
{
    if(!glfwInit())
    {
        DM_LOG_FATAL("GLFW could not be initialized");
        return false;
    }

    // setting the opengl version. apple has stopped supporting past 4.1
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#endif

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    e_data->platform_data = (dm_platform_data*)dm_alloc(sizeof(dm_platform_data), DM_MEM_PLATFORM);
    e_data->platform_data->window_width = window_width;
    e_data->platform_data->window_height = window_height;
    e_data->platform_data->window_title = window_title;

    e_data->platform_data->internal_data = (dm_internal_data*)dm_alloc(sizeof(dm_internal_data), DM_MEM_PLATFORM);
    glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    glfw_data->internal_window = glfwCreateWindow(
        e_data->platform_data->window_width, e_data->platform_data->window_height, 
        e_data->platform_data->window_title, 
        NULL, NULL
    );
    if(!glfw_data->internal_window)
    {
        DM_LOG_FATAL("GLFW window is NULL!");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(glfw_data->internal_window);
    glfwSwapInterval(1);
    
    glfwSetInputMode(glfw_data->internal_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(glfw_data->internal_window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    // set callbacks
    glfwSetKeyCallback(glfw_data->internal_window, dm_platform_glfw_key_callback);
    glfwSetCharCallback(glfw_data->internal_window, dm_platform_glfw_char_callback);
    glfwSetWindowCloseCallback(glfw_data->internal_window, dm_platform_glfw_window_close_callback);
#ifdef __APPLE__
    glfwSetFramebufferSizeCallback(glfw_data->internal_window, dm_platform_glfw_window_resize_callback);
#else
    glfwSetWindowSizeCallback(glfw_data->internal_window, dm_platform_glfw_window_resize_callback);
#endif
    glfwSetMouseButtonCallback(glfw_data->internal_window, dm_platform_glfw_mouse_button_callback);
    glfwSetCursorPosCallback(glfw_data->internal_window, dm_platform_glfw_mouse_move_callback);
    glfwSetScrollCallback(glfw_data->internal_window, dm_platform_glfw_mouse_scroll_callback);

    glfwSetWindowPos(glfw_data->internal_window, start_x, start_y);
    glfwShowWindow(glfw_data->internal_window);

    return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
    DM_LOG_WARN("Platform shutdown called...");

    DM_LOG_WARN("Destroying GLFW window...");
    glfwDestroyWindow(glfw_data->internal_window);
    dm_free(e_data->platform_data->internal_data, sizeof(dm_internal_data), DM_MEM_PLATFORM);
    DM_LOG_WARN("Terminating GLFW...");
    glfwTerminate();

    dm_free(e_data->platform_data, sizeof(dm_platform_data), DM_MEM_PLATFORM);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
    glfwPollEvents();

    if(glfwWindowShouldClose(glfw_data->internal_window))
    {
        DM_LOG_WARN("GLFW received close event!");
        return false;
    }

    return true;
}

void* dm_platform_alloc(size_t size)
{
    void* temp = malloc(size);
    DM_ASSERT_MSG(temp, "Malloc returned null pointer!");
    if (!temp) return NULL;
    dm_platform_memzero(temp, size);
    return temp;
}

void* dm_platform_calloc(size_t count, size_t size)
{
    void* temp = calloc(count, size);
    DM_ASSERT_MSG(temp, "Calloc return null pointer!");
    if (!temp) return NULL;
    return temp;
}

void* dm_platform_realloc(void* block, size_t size)
{
    void* temp = realloc(block, size);
    DM_ASSERT_MSG(temp, "Realloc returned null pointer!");
    if (temp) block = temp;
    else DM_LOG_FATAL("Realloc returned NULL ptr!");
    return temp;
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
}

void dm_platform_write_error(const char* message, uint8_t color)
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
    sprintf(
        out,
        "\033[%sm%s \033[0m",
        levels[color], message
    );

    fprintf(stderr, "%s", out);
}

void dm_platform_swap_buffers()
{
    glfwSwapBuffers(glfw_data->internal_window);
}

// glfw callbacks
void dm_platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    dm_key_code key_code = dm_translate_key_code(key);

    switch (action)
    {
    case GLFW_PRESS:
    {
        dm_event_dispatch((dm_event) { DM_KEY_DOWN_EVENT, NULL, (void*)key_code });
    } break;
    case GLFW_REPEAT:
    {

    } break;
    case GLFW_RELEASE:
    {
        dm_event_dispatch((dm_event) { DM_KEY_UP_EVENT, NULL, (void*)key_code });
    } break;
    }
}

void dm_platform_glfw_char_callback(GLFWwindow* window, unsigned int key)
{
    dm_key_code key_code = dm_translate_key_code(key);

    dm_event_dispatch((dm_event) { DM_KEY_TYPE_EVENT, NULL, (void*)key_code });
}

void dm_platform_glfw_window_close_callback(GLFWwindow* window)
{
    dm_event_dispatch((dm_event) { DM_WINDOW_CLOSE_EVENT, NULL });
}

void dm_platform_glfw_window_resize_callback(GLFWwindow* window, int width, int height)
{
    uint32_t new_rect[2] = { width, height };

    dm_event_dispatch((dm_event) { DM_WINDOW_RESIZE_EVENT, NULL, (void*)new_rect });
}

void dm_platform_glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    dm_mousebutton_code b = -1;

    // determine the button
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        b = DM_MOUSEBUTTON_R;
    }break;
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        b = DM_MOUSEBUTTON_L;
    }break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        b = DM_MOUSEBUTTON_M;
    }break;
    }

    // send the appropriate event
    switch (action)
    {
    case GLFW_PRESS:
    {
        dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_DOWN_EVENT, NULL, (void*)b });
    }break;
    case GLFW_RELEASE:
    {
        dm_event_dispatch((dm_event) { DM_MOUSEBUTTON_UP_EVENT, NULL, (void*)b });
    }break;
    }
}

void dm_platform_glfw_mouse_move_callback(GLFWwindow* window, double xPos, double yPos)
{
    int32_t coords[2] = { (int32_t)xPos, (int32_t)yPos };
    
    dm_event_dispatch((dm_event) { DM_MOUSE_MOVED_EVENT, NULL, (void*)coords });
}

void dm_platform_glfw_mouse_scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
    // ???
}

float dm_platform_get_time()
{
    return glfwGetTime();
}

void dm_platform_set_vsync(bool enabled)
{
    if (enabled)
    {
        glfwSwapInterval(1);
    }
    else
    {
        glfwSwapInterval(0);
    }
}

#ifdef DM_OPENGL
bool dm_platform_init_opengl()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        DM_LOG_FATAL("Failed to initialize GLAD!");
        return false;
    }

    return true;
}

void dm_platform_shutdown_opengl()
{

}
#endif

dm_key_code dm_translate_key_code(uint32_t glfw_key_code)
{
    switch (glfw_key_code)
    {
    case GLFW_KEY_BACKSPACE: return DM_KEY_BACKSPACE;
    case GLFW_KEY_ENTER: return DM_KEY_ENTER;
    case GLFW_KEY_TAB: return DM_KEY_TAB;
    case GLFW_KEY_ESCAPE: return DM_KEY_ESCAPE;
    case GLFW_KEY_SPACE: return DM_KEY_SPACE;
    case GLFW_KEY_END: return DM_KEY_END;
    case GLFW_KEY_HOME: return DM_KEY_HOME;
    case GLFW_KEY_LEFT: return DM_KEY_LEFT;
    case GLFW_KEY_RIGHT: return DM_KEY_RIGHT;
    case GLFW_KEY_UP: return DM_KEY_UP;
    case GLFW_KEY_DOWN: return DM_KEY_DOWN;
    case GLFW_KEY_PRINT_SCREEN: return DM_KEY_PRINT;
    case GLFW_KEY_INSERT: return DM_KEY_INSERT;
    case GLFW_KEY_DELETE: return DM_KEY_DELETE;

    case GLFW_KEY_KP_0: return DM_KEY_NUMPAD_0;
    case GLFW_KEY_KP_1: return DM_KEY_NUMPAD_1;
    case GLFW_KEY_KP_2: return DM_KEY_NUMPAD_2;
    case GLFW_KEY_KP_3: return DM_KEY_NUMPAD_3;
    case GLFW_KEY_KP_4: return DM_KEY_NUMPAD_4;
    case GLFW_KEY_KP_5: return DM_KEY_NUMPAD_5;
    case GLFW_KEY_KP_6: return DM_KEY_NUMPAD_6;
    case GLFW_KEY_KP_7: return DM_KEY_NUMPAD_7;
    case GLFW_KEY_KP_8: return DM_KEY_NUMPAD_8;
    case GLFW_KEY_KP_9: return DM_KEY_NUMPAD_9;
    case GLFW_KEY_KP_ADD: return DM_KEY_ADD;
    case GLFW_KEY_MINUS: return DM_KEY_MINUS;
    case GLFW_KEY_KP_SUBTRACT: return DM_KEY_SUBTRACT;
    case GLFW_KEY_KP_DECIMAL: return DM_KEY_DECIMAL;
    case GLFW_KEY_KP_DIVIDE: return DM_KEY_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY: return DM_KEY_MULTIPLY;
    case GLFW_KEY_NUM_LOCK: return DM_KEY_NUMLCK;
    case GLFW_KEY_KP_ENTER: return DM_KEY_ENTER;
    case GLFW_KEY_PAUSE: return DM_KEY_PAUSE;

    case GLFW_KEY_RIGHT_SHIFT: return DM_KEY_RSHIFT;
    case GLFW_KEY_LEFT_SHIFT: return DM_KEY_LSHIFT;
    case GLFW_KEY_RIGHT_CONTROL: return DM_KEY_RCTRL;
    case GLFW_KEY_LEFT_CONTROL: return DM_KEY_LCTRL;
    case GLFW_KEY_LAST: return DM_KEY_ALT;
    case GLFW_KEY_CAPS_LOCK: return DM_KEY_CAPSLOCK;

    case GLFW_KEY_COMMA: return DM_KEY_COMMA;
    case GLFW_KEY_PERIOD: return DM_KEY_PERIOD;
    case GLFW_KEY_SLASH: return DM_KEY_LSLASH;
    case GLFW_KEY_APOSTROPHE: return DM_KEY_QUOTE;

    case GLFW_KEY_F1: return DM_KEY_F1;
    case GLFW_KEY_F2: return DM_KEY_F2;
    case GLFW_KEY_F3: return DM_KEY_F3;
    case GLFW_KEY_F4: return DM_KEY_F4;
    case GLFW_KEY_F5: return DM_KEY_F5;
    case GLFW_KEY_F6: return DM_KEY_F6;
    case GLFW_KEY_F7: return DM_KEY_F7;
    case GLFW_KEY_F8: return DM_KEY_F8;
    case GLFW_KEY_F9: return DM_KEY_F9;
    case GLFW_KEY_F10: return DM_KEY_F10;
    case GLFW_KEY_F11: return DM_KEY_F11;
    case GLFW_KEY_F12: return DM_KEY_F12;
    }

    return glfw_key_code;
}

#endif