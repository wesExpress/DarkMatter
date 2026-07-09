#include "dm.h"

#ifdef DM_VULKAN
#define GLFW_INCLUDE_VULKAN
#else
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3.h>
#ifdef DM_METAL
#include <GLFW/glfw3native.h>
#endif

typedef struct dm_glfw_window_t
{
    GLFWwindow* window;
} dm_glfw_window;

void glfw_error_callback(int error, const char* description)
{
    LOG_ERROR("Error: %s\n", description);
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    dm_context* context = glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) 
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);

        context->flags &= ~DM_CONTEXT_FLAG_IS_RUNNING;
    }
}

void dm_glfw_window_resize_callback(GLFWwindow* window, int width, int height)
{
    dm_context *context = glfwGetWindowUserPointer(window);

    context->window.width  = width;
    context->window.height = height;

    context->flags |= DM_CONTEXT_FLAG_WINDOW_RESIZED;
}

#ifdef DM_VULKAN
VkSurfaceKHR dm_window_create_vulkan_surface(dm_context* context, VkInstance instance)
{
    dm_glfw_window* window = dm_arena_get_ptr(context->arena, context->window.offset);

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if(glfwCreateWindowSurface(instance, window->window, NULL, &surface)==VK_SUCCESS) return surface;

    LOG_ERROR("glfwCreateWindowSurface failed");
    return VK_NULL_HANDLE;
}
#elif defined(DM_METAL)
void *dm_window_get_native_window(dm_context *context)
{
    dm_glfw_window* window = dm_arena_get_ptr(context->arena, context->window.offset);

    return glfwGetCocoaWindow(window->window);
}
#endif

#ifdef DM_VULKAN
const char** dm_window_get_vulkan_extensions(u32* glfw_ext_count)
{
    return glfwGetRequiredInstanceExtensions(glfw_ext_count);
}
#endif

bool dm_is_key_pressed(dm_context *context, int key)
{
    dm_glfw_window* window = dm_arena_get_ptr(context->arena, context->window.offset);

    return glfwGetKey(window->window, key)==GLFW_PRESS;
}

bool dm_window_create(dm_context* context, u16 width, u16 height, const char* title)
{
    LOG_INFO("Creating glfw window...");

    dm_glfw_window* window = dm_arena_alloc(&context->arena, sizeof(dm_glfw_window), &context->window.offset);
    if(!window) return false;

    if(!glfwInit()) 
    { 
        LOG_FATAL("glfwInit failed"); 
        return false; 
    }

    glfwSetErrorCallback(glfw_error_callback);

#ifdef DM_VULKAN
    if(!glfwVulkanSupported()) 
    { 
        LOG_FATAL("Vulkan is not supported"); 
        return false; 
    }
#endif

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if(!window->window) return false;

    glfwSetKeyCallback(window->window, glfw_key_callback);
    glfwSetWindowSizeCallback(window->window, dm_glfw_window_resize_callback);

    glfwSetWindowUserPointer(window->window, context);

    return true;
}

void dm_window_destroy(dm_context* context)
{
    dm_glfw_window* window = dm_arena_get_ptr(context->arena, context->window.offset);

    glfwDestroyWindow(window->window);
}

void dm_window_poll_events(dm_context* context)
{
    glfwPollEvents();
}

size_t dm_window_get_internal_size()
{
    return sizeof(dm_glfw_window);

}

double dm_window_get_time()
{
    return glfwGetTime();
}
