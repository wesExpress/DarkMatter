#include "dm_platform.h"

#ifdef DM_PLATFORM_GLFW

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dm_mem.h"
#include "dm_logger.h"
#include "dm_event.h"
#include "dm_assert.h"
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


bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title, int start_x, int start_y)
{
    if(!glfwInit())
    {
        DM_FATAL("GLFW could not be initialized");
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

    e_data->platform_data = (dm_platform_data*)dm_alloc(sizeof(dm_platform_data));
    e_data->platform_data->window_width = window_width;
    e_data->platform_data->window_height = window_height;
    e_data->platform_data->window_title = window_title;

    e_data->platform_data->internal_data = (dm_internal_data*)dm_alloc(sizeof(dm_internal_data));
    glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    glfw_data->internal_window = glfwCreateWindow(
        e_data->platform_data->window_width, e_data->platform_data->window_height, 
        e_data->platform_data->window_title, 
        NULL, NULL
    );
    if(!glfw_data->internal_window)
    {
        DM_FATAL("GLFW window is NULL!");
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
    DM_WARN("Platform shutdown called...");

    //glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    DM_WARN("Destroying GLFW window...");
    glfwDestroyWindow(glfw_data->internal_window);
    free(e_data->platform_data->internal_data);
    DM_WARN("Terminating GLFW...");
    glfwTerminate();

    free(e_data->platform_data);
}

bool dm_platform_pump_messages(dm_engine_data* e_data)
{
    //glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    glfwPollEvents();

    if(glfwWindowShouldClose(glfw_data->internal_window))
    {
        DM_ERROR("GLFW received close event!");
        return false;
    }

    return true;
}

void* dm_platform_alloc(size_t size)
{
    void* temp = malloc(size);
    DM_ASSERT_MSG(temp, "Malloc returned null pointer!");
    if (!temp) return NULL;
    return temp;
}

void* dm_platform_realloc(void* block, size_t size)
{
    void* temp = realloc(block, size);
    DM_ASSERT_MSG(temp, "Realloc returned null pointer!");
    if (temp) block = temp;
    else DM_FATAL("Realloc returned NULL ptr!");
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

#ifdef DM_OPENGL
bool dm_platform_init_opengl()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        DM_FATAL("Failed to initialize GLAD!");
        return false;
    }

    return true;
}

void dm_platform_shutdown_opengl()
{

}
#endif

// glfw callbacks
void dm_platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (action)
    {
    case GLFW_PRESS:
    {
        dm_event_dispatch((dm_event) { DM_KEY_DOWN_EVENT, NULL, (void*)key });
    } break;
    case GLFW_REPEAT:
    {

    } break;
    case GLFW_RELEASE:
    {
        dm_event_dispatch((dm_event) { DM_KEY_UP_EVENT, NULL, (void*)key });
    } break;
    }
}

void dm_platform_glfw_char_callback(GLFWwindow* window, unsigned int key)
{
    dm_event_dispatch((dm_event) { DM_KEY_TYPE_EVENT, NULL, (void*)key });
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

#endif