#include "dm.h"

#ifdef DM_VULKAN

#include <string.h>

#ifdef DM_PLATFORM_WIN32
#include "platform/dm_platform_win32.h"
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#define DM_VULKAN_MAX_FRAMES_IN_FLIGHT DM_MAX_FRAMES_IN_FLIGHT 

#define DM_VULKAN_MAX_SURFACE_FORMAT_COUNT 20
#define DM_VULKAN_MAX_PRESENT_MODE_COUNT   20
typedef struct dm_vulkan_swapchain_details_t
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR       formats[DM_VULKAN_MAX_SURFACE_FORMAT_COUNT];
    VkPresentModeKHR         present_modes[DM_VULKAN_MAX_PRESENT_MODE_COUNT];
    uint32_t                 format_count, present_mode_count;
} dm_vulkan_swapchain_details;

typedef struct dm_vulkan_renderpass_t
{
    VkRenderPass renderpass;
} dm_vulkan_renderpass;

typedef struct dm_vulkan_raster_pipeline_t
{
    VkPipelineLayout layout;
} dm_vulkan_raster_pipeline;

typedef struct dm_vulkan_family_t
{
    VkQueue         queue;
    VkCommandPool   pool;
    VkCommandBuffer buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t        index;
} dm_vulkan_family;

typedef struct dm_vulkan_device_t
{
    VkPhysicalDevice physical;
    VkDevice         logical;

    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    dm_vulkan_family graphics_queue;
    dm_vulkan_family compute_queue;
    dm_vulkan_family present_queue;
    dm_vulkan_family transfer_queue;

    dm_vulkan_swapchain_details swapchain_details;
} dm_vulkan_device;

typedef struct dm_vulkan_fence_t
{
    VkFence  fence;
    uint64_t value;
} dm_vulkan_fence;

typedef struct dm_vulkan_swapchain_t
{
    VkSwapchainKHR swapchain;
    VkImage        render_targets[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkImageView    views[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkFramebuffer  frame_buffers[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    VkFormat       format;
    VkExtent2D     extents;
} dm_vulkan_swapchain;

#define DM_VULKAN_MAX_RENDERPASSES   10
#define DM_VULKAN_MAX_RASTER_PIPES   10
#define DM_VULKAN_DEFAULT_RENDERPASS 0
typedef struct dm_vulkan_renderer_t
{
    VkInstance             instance;
    VkAllocationCallbacks* allocator;

    dm_vulkan_device device;

    VkSurfaceKHR surface;

    dm_vulkan_swapchain swapchain;

    dm_vulkan_fence fences[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore wait_semaphore[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore signal_semaphore[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    dm_vulkan_renderpass renderpasses[DM_VULKAN_MAX_RENDERPASSES];
    uint32_t             rp_count;

    dm_vulkan_raster_pipeline raster_pipes[DM_VULKAN_MAX_RASTER_PIPES];
    uint32_t                  raster_pipe_count;

    uint32_t current_frame;

#ifdef DM_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} dm_vulkan_renderer;

#define DM_VULKAN_GET_RENDERER dm_vulkan_renderer* vulkan_renderer = renderer->internal_renderer

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
#endif

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_wait_for_previous_frame(dm_vulkan_renderer* vulkan_renderer)
{
    const uint8_t current_frame = vulkan_renderer->current_frame;

    vkWaitForFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->fences[current_frame].fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->fences[current_frame].fence);

    return true;
}

#ifndef DM_DEBUG
#DM_INLINE
#endif
dm_vulkan_swapchain_details dm_vulkan_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkResult vr;

    dm_vulkan_swapchain_details details;

    vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    if(vr!=VK_SUCCESS)
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, NULL);
    if(vr!=VK_SUCCESS)
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    vr = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
    if(vr!=VK_SUCCESS)
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfacePresentModesKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    if(details.format_count)
    {
        vr = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, details.formats);
        if(vr!=VK_SUCCESS)
        {
            DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
            return (dm_vulkan_swapchain_details){ 0 };
        }
    }
    if(details.present_mode_count)
    {
        vr = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
        if(vr!=VK_SUCCESS)
        {
            DM_LOG_ERROR("vkGetPhysicalDeviceSurfacePresentModesKHR failed");
            return (dm_vulkan_swapchain_details){ 0 };
        }
    }

    return details;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_is_device_suitable(VkPhysicalDevice physical_device, const char** device_extensions, const uint32_t device_extension_count, dm_vulkan_renderer* vulkan_renderer)
{
    dm_vulkan_device device;
    VkResult vr;

    device.physical = physical_device;

    vkGetPhysicalDeviceProperties(device.physical, &device.properties);
    vkGetPhysicalDeviceFeatures(device.physical, &device.features);
    vkGetPhysicalDeviceMemoryProperties(device.physical, &device.memory_properties);

    if(device.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return false;

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &family_count, NULL);
    VkQueueFamilyProperties* family_properties = dm_alloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &family_count, family_properties);
    
    for(uint32_t i=0; i<family_count; i++)
    {
        if(family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            device.graphics_queue.index = i;
        }
        else if(family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            device.transfer_queue.index = i;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device.physical, i, vulkan_renderer->surface, &present_support);
        if(present_support) device.present_queue.index = i;
    }

    dm_free((void**)&family_properties);

    // extension support
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device.physical, NULL, &extension_count, NULL);
    VkExtensionProperties* available_properties = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(device.physical, NULL, &extension_count, available_properties);

    for(uint32_t i=0; i<device_extension_count; i++)
    {
        bool found = false;

        DM_LOG_WARN("Searching for device extension: %s", device_extensions[i]);
        for(uint32_t j=0; j<extension_count; j++)
        {
            if(strcmp(device_extensions[i], available_properties[j].extensionName)==0) 
            {
                DM_LOG_INFO("found");
                found = true;
                break;
            }
        }

        if(found) continue;

        DM_LOG_ERROR("Could not find required extension: %s", device_extensions[i]);
        return false;
    }

    dm_free((void**)&available_properties);

    // swapchain support
    device.swapchain_details = dm_vulkan_query_swapchain_support(device.physical, vulkan_renderer->surface);
    if(device.swapchain_details.present_mode_count==0 && device.swapchain_details.present_mode_count==0) return false;

    vulkan_renderer->device = device;

    return true;
}

bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_INFO("Initializing vulkan backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkResult vr;

    // allocator
    {
        vulkan_renderer->allocator = NULL;
    }

    // Vulkan instance
    {
#ifdef DM_DEBUG
        uint32_t extension_count;
        vr = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
        if(vr!=VK_SUCCESS)
        {
            DM_LOG_FATAL("vkEnumerateInstanceExtensionProperties failed");
            return false;
        }

        VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
        vr = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, available_extensions);
        if(vr!=VK_SUCCESS)
        {
            DM_LOG_FATAL("vkEnumerateInstanceExtensionProperties failed");
            return false;
        }

        DM_LOG_INFO("Available Vulkan extensions: ");
        for(uint32_t i=0; i<extension_count; i++)
        {
            DM_LOG_INFO("    %s", available_extensions[i].extensionName);
        }

        dm_free((void**)&available_extensions);
#endif

    const char* required_extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef DM_PLATFORM_WIN32
            "VK_KHR_win32_surface",
#endif
#ifdef DM_DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };

#ifdef DM_DEBUG
        DM_LOG_WARN("Required Vulkan extensions: ");
        for(uint32_t i=0; i<_countof(required_extensions); i++)
        {
            DM_LOG_WARN("    %s", required_extensions[i]);
        }

        uint32_t available_count;
        vr = vkEnumerateInstanceLayerProperties(&available_count, NULL);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkEnumerateInstanceLayerProperties failed");
            return false;
        }

        VkLayerProperties* available_layers = dm_alloc(sizeof(VkLayerProperties) * available_count);

        vr = vkEnumerateInstanceLayerProperties(&available_count, available_layers);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkEnumerateInstanceLayerProperties failed");
            return false;
        }

        const char* required_layers[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        for(uint32_t i=0; i<_countof(required_layers); i++)
        {
            bool found = false;
            DM_LOG_WARN("Searching for Vulkan validation layer: %s", required_layers[i]);
            for(uint32_t j=0; j<available_count; j++)
            {
                if(strcmp(required_layers[i], available_layers[j].layerName)==0)
                {
                    DM_LOG_INFO("found");
                    found = true;
                    break;
                }
            }

            if(found) continue;

            DM_LOG_FATAL("Could not find validation layer: %s", required_layers[i]);
        }

        DM_LOG_INFO("All Vulkan validation layers found");
        dm_free((void**)&available_layers);
#endif

        VkApplicationInfo app_info  = { 0 };
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = "DarkMatter App";
        app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
        app_info.pEngineName        = "DarkMatter Framework";
        app_info.engineVersion      = VK_MAKE_VERSION(1,0,0);
        app_info.apiVersion         = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = { 0 };
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledExtensionCount   = _countof(required_extensions);
        create_info.ppEnabledExtensionNames = required_extensions;
#ifdef DM_DEBUG
        create_info.enabledLayerCount       = _countof(required_layers);
        create_info.ppEnabledLayerNames     = required_layers;
#endif

        vr = vkCreateInstance(&create_info, vulkan_renderer->allocator, &vulkan_renderer->instance);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateInstance failed");
            return false;
        }

#ifdef DM_DEBUG
        uint32_t message_severity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        message_severity          |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        uint32_t message_type  = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        message_type          |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        message_type          |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { 0 };
        debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = message_severity;
        debug_create_info.messageType     = message_type;
        debug_create_info.pfnUserCallback = dm_vulkan_debug_callback;

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkCreateDebugUtilsMessengerEXT");
        if(!func)
        {
            DM_LOG_FATAL("Failed to create Vulkan debug messenger");
            return false;
        }

        vr = func(vulkan_renderer->instance, &debug_create_info, vulkan_renderer->allocator, &vulkan_renderer->debug_messenger);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("Failed to create Vulkan debug messenger");
            return false;
        }
#endif
    }

    // surface
    {
#ifdef DM_PLATFORM_WIN32
        dm_internal_w32_data* w32_data = context->platform_data.internal_data;

        VkWin32SurfaceCreateInfoKHR create_info = { 0 };
        create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.hwnd      = w32_data->hwnd;
        create_info.hinstance = w32_data->h_instance;

        vr = vkCreateWin32SurfaceKHR(vulkan_renderer->instance, &create_info, vulkan_renderer->allocator, &vulkan_renderer->surface);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateWin32SurfaceKHR failed");
            return false;
        }
#endif
    }

    // device
    {
        vulkan_renderer->device.physical = VK_NULL_HANDLE;

        const char* device_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, NULL);
        if(device_count == 0)
        {
            DM_LOG_FATAL("No GPUs available with Vulkan support");
            return false;
        }

        VkPhysicalDevice* devices = dm_alloc(sizeof(VkPhysicalDevice) * device_count);

        vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, devices);

        for(uint32_t i=0; i<device_count; i++)
        {
            if(dm_vulkan_is_device_suitable(devices[i], device_extensions, _countof(device_extensions), vulkan_renderer)) break;
        }

        dm_free((void**)&devices);

        if(vulkan_renderer->device.physical == VK_NULL_HANDLE)
        {
            DM_LOG_FATAL("Failed to find a suitable GPU");
            return false;
        }

#ifdef DM_DEBUG
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(vulkan_renderer->device.physical, &properties);
        DM_LOG_INFO("GPU selected: %s", properties.deviceName);
#endif

        VkDeviceQueueCreateInfo queue_create_infos[4] = { 0 };

        uint32_t families[] = { vulkan_renderer->device.graphics_queue.index, vulkan_renderer->device.present_queue.index };

        const float queue_priority = 1.f;
        for(uint8_t i=0; i<_countof(families); i++)
        {
            queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = families[i];
            queue_create_infos[i].queueCount       = 1;
            queue_create_infos[i].pQueuePriorities = &queue_priority;
        }

        VkDeviceCreateInfo create_info = { 0 };
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos       = queue_create_infos;
        create_info.queueCreateInfoCount    = _countof(families);
        create_info.pEnabledFeatures        = &vulkan_renderer->device.features;
        create_info.ppEnabledExtensionNames = device_extensions;
        create_info.enabledExtensionCount   = _countof(device_extensions);
        create_info.enabledLayerCount       = 0;

        vr = vkCreateDevice(vulkan_renderer->device.physical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->device.logical);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateDevice failed");
            return false;
        }

        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_queue.index, 0, &vulkan_renderer->device.graphics_queue.queue);
        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.present_queue.index, 0, &vulkan_renderer->device.present_queue.queue);
    }

    // swapchain
    {
        const dm_vulkan_swapchain_details swapchain_details = vulkan_renderer->device.swapchain_details;

        VkSurfaceFormatKHR surface_format;
        bool found = false;
        for(uint32_t i=0; i<swapchain_details.format_count; i++)
        {
            VkSurfaceFormatKHR format = swapchain_details.formats[i];

            if(format.format==VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                found = true;
                surface_format = format;
                break;
            }
        }

        if(!found)
        {
            DM_LOG_FATAL("Desired surface_format not found on device");
            return false;
        }

        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
        found = false;
        for(uint32_t i=0; i<swapchain_details.present_mode_count; i++)
        {
            VkPresentModeKHR mode = swapchain_details.present_modes[i];

            if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = mode;
                found = true;
                break;
            }
        }

        if(swapchain_details.capabilities.currentExtent.width==UINT_MAX || swapchain_details.capabilities.currentExtent.height==UINT_MAX)
        {
            DM_LOG_FATAL("Swapchain extents are invalid");
            return false;
        }

        VkExtent2D extents = { 0 };
        extents.height = swapchain_details.capabilities.currentExtent.height;
        extents.width  = swapchain_details.capabilities.currentExtent.width;

        VkSwapchainCreateInfoKHR create_info = { 0 };
        create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface          = vulkan_renderer->surface;
        create_info.minImageCount    = DM_VULKAN_MAX_FRAMES_IN_FLIGHT;
        create_info.imageFormat      = surface_format.format;
        create_info.imageColorSpace  = surface_format.colorSpace;
        create_info.imageExtent      = extents;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t indices[] = { vulkan_renderer->device.graphics_queue.index, vulkan_renderer->device.present_queue.index };

        if(vulkan_renderer->device.graphics_queue.index != vulkan_renderer->device.present_queue.index)
        {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = indices;
        }
        else
        {
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = NULL;
        }

        create_info.preTransform   = swapchain_details.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode    = present_mode;
        create_info.clipped        = VK_TRUE;
        create_info.oldSwapchain   = VK_NULL_HANDLE;

        vr = vkCreateSwapchainKHR(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->swapchain.swapchain);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateSwapchainKHR failed");
            return false;
        }

        vulkan_renderer->swapchain.format  = surface_format.format;
        vulkan_renderer->swapchain.extents = extents;
    }

    // render targets
    {
        uint32_t image_count = DM_VULKAN_MAX_FRAMES_IN_FLIGHT;
        vr = vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, &image_count, NULL);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkGetSwapchainImagesKHR failed");
            return false;
        }

        vr = vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, &image_count, vulkan_renderer->swapchain.render_targets);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkGetSwapchainImagesKHR failed");
            return false;
        }

        for(uint32_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkImageViewCreateInfo create_info = { 0 };
            create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image    = vulkan_renderer->swapchain.render_targets[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format   = vulkan_renderer->swapchain.format;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            vr = vkCreateImageView(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->swapchain.views[i]);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkCreateImageView failed");
                return false;
            }
        }
    }

    // default renderpass
    {
        VkAttachmentDescription color_attachment = { 0 };
        color_attachment.format         = vulkan_renderer->swapchain.format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref = { 0 };
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = { 0 };
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_attachment_ref;

        VkSubpassDependency dependency = { 0 };
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo create_info = { 0 };
        create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments    = &color_attachment;
        create_info.subpassCount    = 1;
        create_info.pSubpasses      = &subpass;
        create_info.dependencyCount = 1;
        create_info.pDependencies   = &dependency;

        vr = vkCreateRenderPass(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->renderpasses[DM_VULKAN_DEFAULT_RENDERPASS].renderpass);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateRenderPass failed");
            return false;
        }
        vulkan_renderer->rp_count++;
    }

    // framebuffers
    {
        for(uint32_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkImageView attachments[] = { vulkan_renderer->swapchain.views[i] };

            VkFramebufferCreateInfo create_info = { 0 };
            create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.renderPass      = vulkan_renderer->renderpasses[DM_VULKAN_DEFAULT_RENDERPASS].renderpass;
            create_info.attachmentCount = 1;
            create_info.pAttachments    = attachments;
            create_info.width           = vulkan_renderer->swapchain.extents.width;
            create_info.height          = vulkan_renderer->swapchain.extents.height;
            create_info.layers          = 1;

            vr = vkCreateFramebuffer(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->swapchain.frame_buffers[i]);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkCreateFrameBuffer failed");
                return false;
            }
        }
    }

    // command pool(s)
    {
        VkCommandPoolCreateInfo create_info = { 0 };
        create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = vulkan_renderer->device.graphics_queue.index;

        vr = vkCreateCommandPool(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->device.graphics_queue.pool);
        if(vr != VK_SUCCESS)
        {
            DM_LOG_FATAL("vkCreateCommandPool failed");
            return false;
        }
    }

    // command buffer(s)
    {
        VkCommandBufferAllocateInfo allocate_info = { 0 };
        allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool        = vulkan_renderer->device.graphics_queue.pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkAllocateCommandBuffers(vulkan_renderer->device.logical, &allocate_info, &vulkan_renderer->device.graphics_queue.buffer[i]);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkAllocateCommandBuffers failed");
                return false;
            }
        }
    }

    // semaphore stuff
    {
        VkSemaphoreCreateInfo create_info = { 0 };
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->wait_semaphore[i]);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkCreateSemaphore failed");
                return false;
            }

            vr = vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->signal_semaphore[i]);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkCreateSemaphore failed");
                return false;
            }
        }
    }

    // fence stuff
    {
        VkFenceCreateInfo create_info = { 0 };
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkCreateFence(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->fences[i].fence);
            if(vr != VK_SUCCESS)
            {
                DM_LOG_FATAL("vkCreateFence failed");
                return false;
            }
        }
    }

    return true;
}

bool dm_renderer_backend_finish_init(dm_context* context)
{
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    VkResult vr;

    const VkDevice device                  = vulkan_renderer->device.logical;
    const VkAllocationCallbacks* allocator = vulkan_renderer->allocator;

    vkDeviceWaitIdle(device);

    for(uint8_t i=0; i<vulkan_renderer->raster_pipe_count; i++)
    {
        vkDestroyPipelineLayout(device, vulkan_renderer->raster_pipes[i].layout, allocator);
    }

    for(uint8_t i=0; i<vulkan_renderer->rp_count; i++)
    {
        vkDestroyRenderPass(device, vulkan_renderer->renderpasses[i].renderpass, allocator);
    }

    for(uint32_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(device, vulkan_renderer->fences[i].fence, allocator);
        vkDestroySemaphore(device, vulkan_renderer->signal_semaphore[i], allocator);
        vkDestroySemaphore(device, vulkan_renderer->wait_semaphore[i], allocator);
        vkDestroyFramebuffer(device, vulkan_renderer->swapchain.frame_buffers[i], allocator);
        vkDestroyImageView(device, vulkan_renderer->swapchain.views[i], allocator);
    }


    vkDestroyCommandPool(device, vulkan_renderer->device.graphics_queue.pool, allocator);

    vkDestroySwapchainKHR(device, vulkan_renderer->swapchain.swapchain, allocator);
    vkDestroyDevice(vulkan_renderer->device.logical, allocator);
    vkDestroySurfaceKHR(vulkan_renderer->instance, vulkan_renderer->surface, vulkan_renderer->allocator);

#ifdef DM_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func)
    {
        func(vulkan_renderer->instance, vulkan_renderer->debug_messenger, vulkan_renderer->allocator);
    }
#endif

    vkDestroyInstance(vulkan_renderer->instance, vulkan_renderer->allocator);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_wait_for_previous_frame(vulkan_renderer);

    const uint8_t current_frame = vulkan_renderer->current_frame;

    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkResetCommandBuffer(cmd_buffer, 0);

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vr = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    if(vr != VK_SUCCESS)
    {
        DM_LOG_FATAL("vkBeginCommandBuffer failed");
        return false;
    }

    VkClearValue clear_color = {{{ 0.2f,0.5f,0.7f,1.f }}};

    VkRenderPassBeginInfo renderpass_begin_info = { 0 };
    renderpass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.renderPass        = vulkan_renderer->renderpasses[DM_VULKAN_DEFAULT_RENDERPASS].renderpass;
    renderpass_begin_info.framebuffer       = vulkan_renderer->swapchain.frame_buffers[current_frame];
    renderpass_begin_info.renderArea.extent = vulkan_renderer->swapchain.extents;
    renderpass_begin_info.clearValueCount   = 1;
    renderpass_begin_info.pClearValues      = &clear_color;

    vkCmdBeginRenderPass(cmd_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    VkResult vr;

    const uint32_t current_frame = vulkan_renderer->current_frame;

    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkCmdEndRenderPass(cmd_buffer);

    vr = vkEndCommandBuffer(cmd_buffer);
    if(vr != VK_SUCCESS)
    {
        DM_LOG_FATAL("vkEndCommandBuffer failed");
        return false;
    }

    vkAcquireNextImageKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, UINT64_MAX, vulkan_renderer->wait_semaphore[vulkan_renderer->current_frame], VK_NULL_HANDLE, &vulkan_renderer->current_frame);

    VkSemaphore wait_semaphores[] = { vulkan_renderer->wait_semaphore[vulkan_renderer->current_frame] };
    VkSemaphore signal_semaphores[] = { vulkan_renderer->signal_semaphore[vulkan_renderer->current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    vr = vkQueueSubmit(vulkan_renderer->device.graphics_queue.queue, 1, &submit_info, vulkan_renderer->fences[vulkan_renderer->current_frame].fence);
    if(vr != VK_SUCCESS)
    {
        DM_LOG_FATAL("vkQueueSubmit failed");
        return false;
    }

    // finally present
    VkSwapchainKHR swapchains[] = { vulkan_renderer->swapchain.swapchain };
    
    VkPresentInfoKHR present_info = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores; 
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = swapchains;
    present_info.pImageIndices      = &vulkan_renderer->current_frame;

    vkQueuePresentKHR(vulkan_renderer->device.present_queue.queue, &present_info);

    vulkan_renderer->current_frame = (vulkan_renderer->current_frame + 1) % DM_VULKAN_MAX_FRAMES_IN_FLIGHT;

    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    return true;
}

/***************
 * VULKAN DEBUG
*****************/
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    switch(types)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        DM_LOG_WARN("Validation message");
        break;
        
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        DM_LOG_WARN("General message");
        break;
        
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        DM_LOG_WARN("Performance message:");
        break;
    }
    
    switch(severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        DM_LOG_ERROR(callback_data->pMessage);
        break;
        
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        DM_LOG_WARN(callback_data->pMessage);
        break;
        
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        DM_LOG_INFO(callback_data->pMessage);
        break;
        
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        DM_LOG_TRACE(callback_data->pMessage);
        break;
        
        default:
        DM_LOG_ERROR("Shouldn't be here");
        break;
    }
    
    return VK_FALSE;
}

#endif
