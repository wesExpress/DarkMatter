#include "dm_platform.h"

#ifdef DM_PLATFORM_GLFW

#include <GLFW/glfw3.h>
#include "dm_mem.h"

typedef struct dm_internal_data
{
    GLFWwindow* internal_window;
} dm_internal_data;

void dm_platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


bool dm_platform_startup(dm_engine_data* e_data, int window_width, int window_height, const char* window_title)
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

    e_data->platform_data = (dm_platform_data*)dm_alloc(sizeof(dm_platform_data));
    e_data->platform_data->window_width = window_width;
    e_data->platform_data->window_height = window_height;
    e_data->platform_data->window_title = window_title;

    e_data->platform_data->internal_data = (dm_internal_data*)dm_alloc(sizeof(dm_internal_data));
    dm_internal_data* glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

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

    glfwSetKeyCallback(glfw_data->internal_window, dm_platform_glfw_key_callback);

    return true;
}

void dm_platform_shutdown(dm_engine_data* e_data)
{
    dm_internal_data* glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    glfwDestroyWindow(glfw_data->internal_window);
    glfwTerminate();

    free(e_data->platform_data);
}

/*
* named after windows specific API
* run every frame to poll OS events and check if window should close
*/
bool dm_platform_pump_messages(dm_engine_data* e_data)
{
    dm_internal_data* glfw_data = (dm_internal_data*)e_data->platform_data->internal_data;

    glfwPollEvents();

    if(glfwWindowShouldClose(glfw_data->internal_window))
    {
        return false;
    }

    return true;
}

void dm_platform_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

#endif