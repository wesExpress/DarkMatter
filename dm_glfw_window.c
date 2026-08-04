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
#include "imgui/dcimgui_impl_glfw.h"

typedef struct dm_glfw_window_t
{
    GLFWwindow* window;
} dm_glfw_window;

void glfw_error_callback(int error, const char* description)
{
    LOG_ERROR("Error: %s\n", description);
}

dm_key_code     dm_glfw_convert_key(int key);
dm_mouse_button dm_glfw_convert_button(int button);

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    dm_context* context = glfwGetWindowUserPointer(window);

    switch(action)
    {
        case GLFW_PRESS:
            context->window.input_states[0].keys[dm_glfw_convert_key(key)] = 1;
            break;
        case GLFW_RELEASE:
            context->window.input_states[0].keys[dm_glfw_convert_key(key)] = 0;
            break;
    }
}

static void dm_glfw_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    dm_context* context = glfwGetWindowUserPointer(window);

    switch(action)
    {
        case GLFW_PRESS:
            context->window.input_states[0].buttons[dm_glfw_convert_button(button)] = 1;
            break;
        case GLFW_RELEASE:
            context->window.input_states[0].buttons[dm_glfw_convert_button(button)] = 0;
            break;
    }
}

static void dm_glfw_mouse_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    dm_context* context = glfwGetWindowUserPointer(window);

    context->window.input_states[0].mouse_x = xpos;
    context->window.input_states[0].mouse_y = ypos;
}

static void dm_glfw_mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    dm_context* context = glfwGetWindowUserPointer(window);

    context->window.input_states[0].scroll_x = xoffset;
    context->window.input_states[0].scroll_y = yoffset;
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
    dm_glfw_window* window = context->window.internal_window;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if(glfwCreateWindowSurface(instance, window->window, NULL, &surface)==VK_SUCCESS) return surface;

    LOG_ERROR("glfwCreateWindowSurface failed");
    return VK_NULL_HANDLE;
}
#elif defined(DM_METAL)
void *dm_window_get_native_window(dm_context *context)
{
    dm_glfw_window* window = context->window.internal_window;

    return glfwGetCocoaWindow(window->window);
}
#endif

#ifdef DM_VULKAN
const char** dm_window_get_vulkan_extensions(u32* glfw_ext_count)
{
    return glfwGetRequiredInstanceExtensions(glfw_ext_count);
}
#endif

bool dm_window_create(dm_context* context, u16 width, u16 height, const char* title)
{
    LOG_INFO("Creating glfw window...");

    context->window.internal_window = dm_arena_alloc(&context->arena, sizeof(dm_glfw_window));
    if(!context->window.internal_window) return false;

    dm_glfw_window *window = context->window.internal_window;

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
    glfwSetMouseButtonCallback(window->window, dm_glfw_button_callback);
    glfwSetCursorPosCallback(window->window, dm_glfw_mouse_pos_callback);
    glfwSetScrollCallback(window->window, dm_glfw_mouse_scroll_callback);

    glfwSetWindowSizeCallback(window->window, dm_glfw_window_resize_callback);

    glfwSetWindowUserPointer(window->window, context);

    int w,h,display_w,display_h;
    glfwGetWindowSize(window->window, &w, &h);
    glfwGetFramebufferSize(window->window, &display_w, &display_h);

    context->window.width  = w;
    context->window.height = h;

    context->window.scale_w = (w > 0) ? (float)display_w / (float)w : 1.f;
    context->window.scale_h = (h > 0) ? (float)display_h / (float)h : 1.f;

    return true;
}

void dm_window_destroy(dm_context* context)
{
    dm_glfw_window* window = context->window.internal_window;

    glfwDestroyWindow(window->window);
}

void dm_window_poll_events(dm_context* context)
{
    dm_glfw_window* window = context->window.internal_window;

    int w,h,display_w,display_h;
    glfwPollEvents();

    glfwGetWindowSize(window->window, &w, &h);
    glfwGetFramebufferSize(window->window, &display_w, &display_h);

    context->window.width  = w;
    context->window.height = h;

    context->window.scale_w = (w > 0) ? (float)display_w / (float)w : 1.f;
    context->window.scale_h = (h > 0) ? (float)display_h / (float)h : 1.f;
}

double dm_window_get_time()
{
    return glfwGetTime();
}

void dm_platform_imgui_init(dm_context *context)
{
    dm_glfw_window* window = context->window.internal_window;

#ifdef DM_METAL
    cImGui_ImplGlfw_InitForOther(window->window, true);
#else
    cImGui_ImplGlfw_InitForVulkan(window->window, true);
#endif
}

void dm_platform_imgui_shutdown(dm_context *context)
{
    cImGui_ImplGlfw_Shutdown();
}

void dm_platform_imgui_new_frame(dm_context *context)
{
    cImGui_ImplGlfw_NewFrame();
}

void dm_window_clipboard_copy(dm_context *context, const char *text, int len)
{
    dm_glfw_window *window = context->window.internal_window;

    char *str = 0;
    if (!len) return;
    str = (char *)malloc((size_t)len + 1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    glfwSetClipboardString(window->window, str);
    free(str);
}

const char *dm_window_clipboard_paste(dm_context *context)
{
    dm_glfw_window *window = context->window.internal_window;

    return glfwGetClipboardString(window->window);
}

/////////////////////////////////////////////
dm_key_code dm_glfw_convert_key(int key)
{
    switch(key)
    {
        default:
        case GLFW_KEY_A: return DM_KEY_A;
        case GLFW_KEY_B: return DM_KEY_B;
        case GLFW_KEY_C: return DM_KEY_C;
        case GLFW_KEY_D: return DM_KEY_D;
        case GLFW_KEY_E: return DM_KEY_E;
        case GLFW_KEY_F: return DM_KEY_F;
        case GLFW_KEY_G: return DM_KEY_G;
        case GLFW_KEY_H: return DM_KEY_H;
        case GLFW_KEY_I: return DM_KEY_I;
        case GLFW_KEY_J: return DM_KEY_J;
        case GLFW_KEY_K: return DM_KEY_K;
        case GLFW_KEY_L: return DM_KEY_L;
        case GLFW_KEY_M: return DM_KEY_M;
        case GLFW_KEY_N: return DM_KEY_N;
        case GLFW_KEY_O: return DM_KEY_O;
        case GLFW_KEY_P: return DM_KEY_P;
        case GLFW_KEY_Q: return DM_KEY_Q;
        case GLFW_KEY_R: return DM_KEY_R;
        case GLFW_KEY_S: return DM_KEY_S;
        case GLFW_KEY_T: return DM_KEY_T;
        case GLFW_KEY_U: return DM_KEY_U;
        case GLFW_KEY_V: return DM_KEY_V;
        case GLFW_KEY_W: return DM_KEY_W;
        case GLFW_KEY_X: return DM_KEY_X;
        case GLFW_KEY_Y: return DM_KEY_Y;
        case GLFW_KEY_Z: return DM_KEY_Z;

        case GLFW_KEY_0: return DM_KEY_0;
        case GLFW_KEY_1: return DM_KEY_1;
        case GLFW_KEY_2: return DM_KEY_2;
        case GLFW_KEY_3: return DM_KEY_3;
        case GLFW_KEY_4: return DM_KEY_4;
        case GLFW_KEY_5: return DM_KEY_5;
        case GLFW_KEY_6: return DM_KEY_6;
        case GLFW_KEY_7: return DM_KEY_7;
        case GLFW_KEY_8: return DM_KEY_8;
        case GLFW_KEY_9: return DM_KEY_9;

        case GLFW_KEY_F1:  return DM_KEY_F1;
        case GLFW_KEY_F2:  return DM_KEY_F2;
        case GLFW_KEY_F3:  return DM_KEY_F3;
        case GLFW_KEY_F4:  return DM_KEY_F4;
        case GLFW_KEY_F5:  return DM_KEY_F5;
        case GLFW_KEY_F6:  return DM_KEY_F6;
        case GLFW_KEY_F7:  return DM_KEY_F7;
        case GLFW_KEY_F8:  return DM_KEY_F8;
        case GLFW_KEY_F9:  return DM_KEY_F9;
        case GLFW_KEY_F10: return DM_KEY_F10;
        case GLFW_KEY_F11: return DM_KEY_F11;
        case GLFW_KEY_F12: return DM_KEY_F12;

        case GLFW_KEY_LEFT:  return DM_KEY_LEFT;
        case GLFW_KEY_RIGHT: return DM_KEY_RIGHT;
        case GLFW_KEY_UP:    return DM_KEY_UP;
        case GLFW_KEY_DOWN:  return DM_KEY_DOWN;

        case GLFW_KEY_ESCAPE:        return DM_KEY_ESC;
        case GLFW_KEY_ENTER:         return DM_KEY_ENTER;
        case GLFW_KEY_SPACE:         return DM_KEY_SPACE;
        case GLFW_KEY_LEFT_SHIFT:    return DM_KEY_LSHIFT;
        case GLFW_KEY_RIGHT_SHIFT:   return DM_KEY_RSHIFT;
        case GLFW_KEY_LEFT_ALT:      return DM_KEY_LALT;
        case GLFW_KEY_RIGHT_ALT:     return DM_KEY_RALT;
        case GLFW_KEY_LEFT_CONTROL:  return DM_KEY_LCTRL;
        case GLFW_KEY_RIGHT_CONTROL: return DM_KEY_RCTRL;
        case GLFW_KEY_LEFT_SUPER:    return DM_KEY_SUPER;
        case GLFW_KEY_TAB:           return DM_KEY_TAB;
        case GLFW_KEY_CAPS_LOCK:     return DM_KEY_CAPS;
        case GLFW_KEY_BACKSPACE:     return DM_KEY_BACKSPACE;

        case GLFW_KEY_PERIOD:        return DM_KEY_PERIOD;
        case GLFW_KEY_COMMA:         return DM_KEY_COMMA;
        case GLFW_KEY_SEMICOLON:     return DM_KEY_SEMICOLON;
        case GLFW_KEY_APOSTROPHE:    return DM_KEY_APOSTROPHE;
        case GLFW_KEY_MINUS:         return DM_KEY_DASH;
        case GLFW_KEY_EQUAL:         return DM_KEY_EQUALS;
        case GLFW_KEY_BACKSLASH:     return DM_KEY_BSLASH;
        case GLFW_KEY_SLASH:         return DM_KEY_FSLASH;
        case GLFW_KEY_LEFT_BRACKET:  return DM_KEY_LBRACKET;
        case GLFW_KEY_RIGHT_BRACKET: return DM_KEY_RBRACKET;
    }
}

dm_mouse_button dm_glfw_convert_button(int button)
{
    switch(button)
    {
        default:
        case GLFW_MOUSE_BUTTON_LEFT:   return DM_MOUSE_LEFT;
        case GLFW_MOUSE_BUTTON_RIGHT:  return DM_MOUSE_RIGHT;
        case GLFW_MOUSE_BUTTON_MIDDLE: return DM_MOUSE_MIDDLE;
    }
}
