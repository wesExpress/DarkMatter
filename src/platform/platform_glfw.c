#include "platform.h"

#ifdef PLATFORM_GLFW

#include <GLFW/glfw3.h>

typedef struct internal_data
{
    GLFWwindow* window;
} internal_data;

/*
* Sets up the glfw platform specific data. 
* - GLFWwindow* interal member
*
* @param e_data -> engine_data struct (ptr)
*/
bool platform_startup(engine_data* e_data)
{
    if(!glfwInit())
    {
        return false;
    }

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

    e_data->platform_data = (internal_data*)malloc(sizeof(internal_data));
    internal_data* data = (internal_data*)e_data->platform_data;

    data->window = glfwCreateWindow(
        800, 600, "Window", NULL, NULL
    );
    if(!data->window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(data->window);
    glfwSwapInterval(1);
    
    glfwSetInputMode(data->window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(data->window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    return true;
}

void platform_shutdown(engine_data* e_data)
{
    internal_data* data = (internal_data*)e_data->platform_data;

    glfwDestroyWindow(data->window);
    glfwTerminate();

    free(e_data->platform_data);
}

/*
* named after windows specific API
* run every frame to poll OS events and check if window should close
*/
bool platform_pump_messages(engine_data* e_data)
{
    internal_data* data = (internal_data*)e_data->platform_data;

    glfwPollEvents();

    if(glfwWindowShouldClose(data->window))
    {
        return false;
    }

    return true;
}

#endif