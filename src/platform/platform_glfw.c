#include "platform.h"

#ifdef PLATFORM_GLFW

#include <GLFW/glfw3.h>
#include "mem.h"

typedef struct internal_data
{
    GLFWwindow* internal_window;
} internal_data;

void platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

/*
* Sets up the glfw platform specific data. 
* - GLFWwindow* interal member
*
* @param e_data -> engine_data struct (ptr)
*/
bool platform_startup(engine_data* e_data, int window_width, int window_height, const char* window_title)
{
    if(!glfwInit())
    {
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

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    e_data->platform_data = (platform_data*)mem_alloc(sizeof(platform_data));
    e_data->platform_data->window_width = window_width;
    e_data->platform_data->window_height = window_height;
    e_data->platform_data->window_title = window_title;

    e_data->platform_data->internal_data = (internal_data*)mem_alloc(sizeof(internal_data));
    internal_data* glfw_data = (internal_data*)e_data->platform_data->internal_data;

    glfw_data->internal_window = glfwCreateWindow(
        e_data->platform_data->window_width, e_data->platform_data->window_height, 
        e_data->platform_data->window_title, 
        NULL, NULL
    );
    if(!glfw_data->internal_window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(glfw_data->internal_window);
    glfwSwapInterval(1);
    
    glfwSetInputMode(glfw_data->internal_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(glfw_data->internal_window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    glfwSetKeyCallback(glfw_data->internal_window, platform_glfw_key_callback);

    return true;
}

void platform_shutdown(engine_data* e_data)
{
    internal_data* glfw_data = (internal_data*)e_data->platform_data->internal_data;

    glfwDestroyWindow(glfw_data->internal_window);
    glfwTerminate();

    free(e_data->platform_data);
}

/*
* named after windows specific API
* run every frame to poll OS events and check if window should close
*/
bool platform_pump_messages(engine_data* e_data)
{
    internal_data* glfw_data = (internal_data*)e_data->platform_data->internal_data;

    glfwPollEvents();

    if(glfwWindowShouldClose(glfw_data->internal_window))
    {
        return false;
    }

    return true;
}

void platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

#endif