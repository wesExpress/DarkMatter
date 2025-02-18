#include "dm.h"

#ifdef DM_VULKAN

#include <string.h>

#ifdef DM_PLATFORM_WIN32
#include "platform/dm_platform_win32.h"
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#if 0 
#include <vulkan/vulkan.h>
#else
#define VOLK_IMPLEMENTATION
#include "volk/volk.h"
#endif

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

#define DM_VULKAN_RAST_PIPE_MAX_DESCRIPTOR_SETS 4
typedef struct dm_vulkan_raster_pipeline_t
{
    VkPipeline       pipeline;
    VkPipelineLayout layout;

    VkDescriptorSetLayout descriptor_set_layout[DM_VULKAN_RAST_PIPE_MAX_DESCRIPTOR_SETS];
    size_t                descriptor_set_layout_sizes[DM_VULKAN_RAST_PIPE_MAX_DESCRIPTOR_SETS];
    size_t                descriptor_set_layout_offsets[DM_VULKAN_RAST_PIPE_MAX_DESCRIPTOR_SETS];
    uint8_t               descriptor_set_layout_count;

    VkViewport viewport;
    VkRect2D   scissor;
} dm_vulkan_raster_pipeline;

typedef struct dm_vulkan_buffer_t
{
    VkBuffer       buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory memory[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
} dm_vulkan_buffer;

typedef struct dm_vulkan_vertex_buffer_t
{
    dm_vulkan_buffer host_buffer;
    dm_vulkan_buffer device_buffer;
} dm_vulkan_vertex_buffer;

typedef struct dm_vulkan_index_buffer_t
{
    dm_vulkan_buffer host_buffer;
    dm_vulkan_buffer device_buffer;

    VkIndexType index_type;
} dm_vulkan_index_buffer;

typedef struct dm_vulkan_constant_buffer_t
{
    dm_vulkan_buffer buffer;
    void*            mapped_memory[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    size_t           size;
} dm_vulkan_constant_buffer;

#define DM_VULKAN_INVALID_QUEUE_INDEX UINT16_MAX
typedef struct dm_vulkan_family_t
{
    VkQueue         queue;
    VkCommandPool   pool;
    VkCommandBuffer buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint16_t        index;
} dm_vulkan_family;

typedef struct dm_vulkan_device_t
{
    VkPhysicalDevice physical;
    VkDevice         logical;

    VkPhysicalDeviceProperties                    properties;
    VkPhysicalDeviceProperties2                   properties2;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_props;
    VkPhysicalDeviceDescriptorBufferFeaturesEXT   descriptor_buffer_feats;
    VkPhysicalDeviceVulkan12Properties            vulkan_12_props;
    VkPhysicalDeviceVulkan12Features              vulkan_12_feats;

    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceFeatures2        features2;
    VkPhysicalDeviceMemoryProperties memory_properties;

    dm_vulkan_family graphics_family;
    dm_vulkan_family compute_family;
    dm_vulkan_family transfer_family;
    dm_vulkan_family transient_family;

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

typedef struct dm_vulkan_descriptor_buffer_mapped_memory_t
{
    void* begin;
    void* current;
} dm_vulkan_descriptor_buffer_mapped_memory;

typedef struct dm_vulkan_descriptor_buffer_t
{
    dm_vulkan_buffer                          buffer;
    VkDeviceOrHostAddressConstKHR             buffer_address[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    dm_vulkan_descriptor_buffer_mapped_memory mapped_memory[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    size_t                                    offset[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
} dm_vulkan_descriptor_buffer;

#define DM_VULKAN_MAX_RENDERPASSES     10
#define DM_VULKAN_MAX_RASTER_PIPES     10
#define DM_VULKAN_MAX_VERTEX_BUFFERS   100
#define DM_VULKAN_MAX_INDEX_BUFFERS    100
#define DM_VULKAN_MAX_CONSTANT_BUFFERS 100
#define DM_VULKAN_MAX_TEXTURES         100
#define DM_VULKAN_DEFAULT_RENDERPASS   0
typedef struct dm_vulkan_renderer_t
{
    VkInstance             instance;
    VkAllocationCallbacks* allocator;

    dm_vulkan_device device;

    VkSurfaceKHR surface;

    dm_vulkan_swapchain swapchain;

    dm_vulkan_fence fences[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore image_available_semaphore[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_finished_semaphore[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    dm_vulkan_descriptor_buffer ubo_buffer;
    dm_vulkan_descriptor_buffer sampler_buffer;

    dm_vulkan_renderpass renderpasses[DM_VULKAN_MAX_RENDERPASSES];
    uint32_t             rp_count;

    dm_vulkan_raster_pipeline raster_pipes[DM_VULKAN_MAX_RASTER_PIPES];
    uint32_t                  raster_pipe_count;

    dm_vulkan_vertex_buffer vertex_buffers[DM_VULKAN_MAX_VERTEX_BUFFERS];
    uint32_t                vb_count;

    dm_vulkan_index_buffer index_buffers[DM_VULKAN_MAX_INDEX_BUFFERS];
    uint32_t               ib_count;

    dm_vulkan_constant_buffer constant_buffers[DM_VULKAN_MAX_CONSTANT_BUFFERS];
    uint32_t                  cb_count;

    uint32_t current_frame, image_index;

    dm_render_handle bound_pipeline;

#ifdef DM_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} dm_vulkan_renderer;

#define DM_VULKAN_GET_RENDERER dm_vulkan_renderer* vulkan_renderer = renderer->internal_renderer

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
#endif

bool dm_vulkan_decode_vr(VkResult vr);
bool dm_vulkan_allocate_memory(VkMemoryRequirements memory_reqs, VkMemoryPropertyFlagBits memory_flags, VkBufferUsageFlagBits usage_flags, VkDeviceMemory* memory, dm_vulkan_renderer* vulkan_renderer);

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_wait_for_previous_frame(dm_vulkan_renderer* vulkan_renderer)
{
    const uint8_t current_frame = vulkan_renderer->current_frame;

    vkWaitForFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->fences[current_frame].fence, VK_TRUE, INFINITE);
    vkResetFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->fences[current_frame].fence);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
dm_vulkan_swapchain_details dm_vulkan_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkResult vr;

    dm_vulkan_swapchain_details details;

    vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, NULL);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    vr = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_ERROR("vkGetPhysicalDeviceSurfacePresentModesKHR failed");
        return (dm_vulkan_swapchain_details){ 0 };
    }

    if(details.format_count)
    {
        vr = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, details.formats);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_ERROR("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
            return (dm_vulkan_swapchain_details){ 0 };
        }
    }
    if(details.present_mode_count)
    {
        vr = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
        if(!dm_vulkan_decode_vr(vr))
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
    dm_vulkan_device device = { 0 };
    VkResult vr;

    device.physical = physical_device;

    vkGetPhysicalDeviceProperties(device.physical, &device.properties);
    vkGetPhysicalDeviceFeatures(device.physical, &device.features);
    vkGetPhysicalDeviceMemoryProperties(device.physical, &device.memory_properties);

    device.descriptor_buffer_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
    device.vulkan_12_feats.sType         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    device.descriptor_buffer_feats.pNext = &device.vulkan_12_feats;

    device.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device.features2.pNext = &device.descriptor_buffer_feats;
    vkGetPhysicalDeviceFeatures2(device.physical, &device.features2);

    if(device.descriptor_buffer_feats.descriptorBuffer==0) return false;
    if(device.vulkan_12_feats.bufferDeviceAddress==0)      return false;

    if(device.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return false;

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &family_count, NULL);
    VkQueueFamilyProperties* family_properties = dm_alloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &family_count, family_properties);
    
    // need graphics, transfer, and compute
    if(!(family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)) return false;
    if(!(family_properties->queueFlags & VK_QUEUE_TRANSFER_BIT)) return false;
    if(!(family_properties->queueFlags & VK_QUEUE_COMPUTE_BIT))  return false;

    device.graphics_family.index  = DM_VULKAN_INVALID_QUEUE_INDEX;
    device.transfer_family.index  = DM_VULKAN_INVALID_QUEUE_INDEX;
    device.compute_family.index   = DM_VULKAN_INVALID_QUEUE_INDEX;
    device.transient_family.index = DM_VULKAN_INVALID_QUEUE_INDEX;

    for(uint32_t i=0; i<family_count; i++)
    {
        if(family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT & family_properties[i].queueFlags & ~VK_QUEUE_COMPUTE_BIT)
        {
            //device.graphics_family.index = i;
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device.physical, i, vulkan_renderer->surface, &present_support);
            if(present_support) device.graphics_family.index = i;
        }

        if(family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT & family_properties[i].queueFlags & ~VK_QUEUE_GRAPHICS_BIT)
        {
            device.transfer_family.index = i;
        }

        if(family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            device.compute_family.index = i;
        }
    }

    // find transient queue
    for(uint32_t i=0; i<family_count; i++)
    {
        if(i == device.graphics_family.index) continue;
        if(i == device.transfer_family.index) continue;
        if(i == device.compute_family.index)  continue;

        device.transient_family.index = i;
        break;
    }

    if(device.graphics_family.index==DM_VULKAN_INVALID_QUEUE_INDEX)  return false;
    if(device.transfer_family.index==DM_VULKAN_INVALID_QUEUE_INDEX)  return false;
    if(device.compute_family.index==DM_VULKAN_INVALID_QUEUE_INDEX)   return false;
    if(device.transient_family.index==DM_VULKAN_INVALID_QUEUE_INDEX) return false;

    dm_free((void**)&family_properties);

    // extension support
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device.physical, NULL, &extension_count, NULL);
    VkExtensionProperties* available_properties = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(device.physical, NULL, &extension_count, available_properties);

#ifdef DM_DEBUG
    DM_LOG_INFO("Device available extensions: ");
    for(uint32_t i=0; i<extension_count; i++)
    {
        DM_LOG_INFO("    %s", available_properties[i].extensionName);
    }
#endif

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

    // features
    

    dm_free((void**)&available_properties);

    // swapchain support
    device.swapchain_details = dm_vulkan_query_swapchain_support(device.physical, vulkan_renderer->surface);
    if(device.swapchain_details.present_mode_count==0 && device.swapchain_details.present_mode_count==0) return false;

    DM_LOG_INFO("Vulkan API Version: %u.%u.%u", VK_API_VERSION_MAJOR(device.properties.apiVersion), VK_API_VERSION_MINOR(device.properties.apiVersion), VK_API_VERSION_PATCH(device.properties.apiVersion));

    vulkan_renderer->device = device;

    return true;
}

/************
 * SWAPCHAIN
 * ***********/
bool dm_vulkan_create_swapchain(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    vulkan_renderer->device.swapchain_details = dm_vulkan_query_swapchain_support(vulkan_renderer->device.physical, vulkan_renderer->surface);
    if(vulkan_renderer->device.swapchain_details.present_mode_count==0 && vulkan_renderer->device.swapchain_details.present_mode_count==0) return false;

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

    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = NULL;

    create_info.preTransform   = swapchain_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = present_mode;
    create_info.clipped        = VK_TRUE;
    create_info.oldSwapchain   = VK_NULL_HANDLE;

    vr = vkCreateSwapchainKHR(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->swapchain.swapchain);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateSwapchainKHR failed");
        return false;
    }

    vulkan_renderer->swapchain.format  = surface_format.format;
    vulkan_renderer->swapchain.extents = extents;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_rendertargets(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    uint32_t image_count = DM_VULKAN_MAX_FRAMES_IN_FLIGHT;
    vr = vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, &image_count, NULL);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkGetSwapchainImagesKHR failed");
        return false;
    }

    vr = vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, &image_count, vulkan_renderer->swapchain.render_targets);
    if(!dm_vulkan_decode_vr(vr))
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
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateImageView failed");
            return false;
        }
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_framebuffers(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

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
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateFrameBuffer failed");
            return false;
        }
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_command_pool(dm_vulkan_family* family, VkCommandPoolCreateFlagBits create_flags, dm_vulkan_renderer* vulkan_renderer)
{
    VkCommandPoolCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = create_flags; 
    create_info.queueFamilyIndex = family->index;

    if(dm_vulkan_decode_vr(vkCreateCommandPool(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &family->pool))) return true;

    DM_LOG_FATAL("vkCreateCommandPool failed");
    return false;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_command_buffer(VkCommandPool* pool, VkCommandBuffer* buffer, dm_vulkan_renderer* vulkan_renderer)
{
    VkCommandBufferAllocateInfo allocate_info = { 0 };
    allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = *pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(dm_vulkan_decode_vr(vkAllocateCommandBuffers(vulkan_renderer->device.logical, &allocate_info, buffer + i))) continue;

        DM_LOG_FATAL("vkAllocateCommandBuffers failed");
        return false;
    }

    return true;
}

bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_INFO("Initializing vulkan backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkResult vr;

    // volk
    {
        vr = volkInitialize();
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("Volk failed to initialized");
            return false;
        }
    }

    // allocator
    {
        vulkan_renderer->allocator = NULL;
    }

    // Vulkan instance
    {
#ifdef DM_DEBUG
        uint32_t extension_count;
        vr = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkEnumerateInstanceExtensionProperties failed");
            return false;
        }

        VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
        vr = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, available_extensions);
        if(!dm_vulkan_decode_vr(vr))
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
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkEnumerateInstanceLayerProperties failed");
            return false;
        }

        VkLayerProperties* available_layers = dm_alloc(sizeof(VkLayerProperties) * available_count);

        vr = vkEnumerateInstanceLayerProperties(&available_count, available_layers);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkEnumerateInstanceLayerProperties failed");
            return false;
        }

        const char* required_layers[] = {
            "VK_LAYER_KHRONOS_validation",
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
        app_info.apiVersion         = VK_API_VERSION_1_3;

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
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateInstance failed");
            return false;
        }

        volkLoadInstance(vulkan_renderer->instance);

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
        debug_create_info.pUserData       = context;

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkCreateDebugUtilsMessengerEXT");
        if(!func)
        {
            DM_LOG_FATAL("Failed to create Vulkan debug messenger");
            return false;
        }

        vr = func(vulkan_renderer->instance, &debug_create_info, vulkan_renderer->allocator, &vulkan_renderer->debug_messenger);
        if(!dm_vulkan_decode_vr(vr))
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
        if(!dm_vulkan_decode_vr(vr))
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
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
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

        uint32_t families[] = { 
            vulkan_renderer->device.graphics_family.index, 
            vulkan_renderer->device.transfer_family.index,
            vulkan_renderer->device.compute_family.index,
            vulkan_renderer->device.transient_family.index
        };

        const float queue_priority = 1.f;
        for(uint8_t i=0; i<_countof(families); i++)
        {
            queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = families[i];
            queue_create_infos[i].queueCount       = 1;
            queue_create_infos[i].pQueuePriorities = &queue_priority;
        }

        vulkan_renderer->device.descriptor_buffer_feats.pNext = &vulkan_renderer->device.vulkan_12_feats;

        VkDeviceCreateInfo create_info = { 0 };
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos       = queue_create_infos;
        create_info.queueCreateInfoCount    = _countof(families);
        create_info.pEnabledFeatures        = &vulkan_renderer->device.features;
        create_info.ppEnabledExtensionNames = device_extensions;
        create_info.enabledExtensionCount   = _countof(device_extensions);
        create_info.enabledLayerCount       = 0;
        create_info.pNext                   = &vulkan_renderer->device.descriptor_buffer_feats;

        vr = vkCreateDevice(vulkan_renderer->device.physical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->device.logical);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateDevice failed");
            return false;
        }

        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_family.index,  0, &vulkan_renderer->device.graphics_family.queue);
        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.transfer_family.index,  0, &vulkan_renderer->device.transfer_family.queue);
        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.compute_family.index,   0, &vulkan_renderer->device.compute_family.queue);
        vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.transient_family.index, 0, &vulkan_renderer->device.transient_family.queue);
    }

    // descriptor buffer stuff
    {
        vulkan_renderer->device.descriptor_buffer_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
        vulkan_renderer->device.properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vulkan_renderer->device.properties2.pNext = &vulkan_renderer->device.descriptor_buffer_props;

        vkGetPhysicalDeviceProperties2(vulkan_renderer->device.physical, &vulkan_renderer->device.properties2);
    }


    // swapchain
    {
        if(!dm_vulkan_create_swapchain(&context->renderer)) 
        {
            DM_LOG_FATAL("Creating swapchain failed");
            return false;
        }
    }

    // render targets
    {
        if(!dm_vulkan_create_rendertargets(vulkan_renderer))
        {
            DM_LOG_FATAL("Creating rendertargets failed");
            return false;
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
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateRenderPass failed");
            return false;
        }
        vulkan_renderer->rp_count++;
    }

    // framebuffers
    {
        if(!dm_vulkan_create_framebuffers(vulkan_renderer))
        {
            DM_LOG_FATAL("Creating framebuffers failed");
            return false;
        }
    }

    // command pool(s)
    {
        VkCommandPoolCreateFlagBits flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if(!dm_vulkan_create_command_pool(&vulkan_renderer->device.graphics_family,  flags, vulkan_renderer))  return false;
        if(!dm_vulkan_create_command_pool(&vulkan_renderer->device.transfer_family,  flags, vulkan_renderer))  return false;
        if(!dm_vulkan_create_command_pool(&vulkan_renderer->device.compute_family,   flags, vulkan_renderer))   return false;

        flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if(!dm_vulkan_create_command_pool(&vulkan_renderer->device.transient_family, flags, vulkan_renderer)) return false;

    }

    // command buffer(s)
    {
        if(!dm_vulkan_create_command_buffer(&vulkan_renderer->device.graphics_family.pool,  vulkan_renderer->device.graphics_family.buffer,  vulkan_renderer)) return false;
        if(!dm_vulkan_create_command_buffer(&vulkan_renderer->device.transfer_family.pool,  vulkan_renderer->device.transfer_family.buffer,  vulkan_renderer)) return false;
        if(!dm_vulkan_create_command_buffer(&vulkan_renderer->device.compute_family.pool,   vulkan_renderer->device.compute_family.buffer,   vulkan_renderer)) return false;
        if(!dm_vulkan_create_command_buffer(&vulkan_renderer->device.transient_family.pool, vulkan_renderer->device.transient_family.buffer, vulkan_renderer)) return false; 

        // start recording the initial transient command buffer
        VkCommandBufferBeginInfo begin_info = { 0 };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(vulkan_renderer->device.transient_family.buffer[vulkan_renderer->current_frame], &begin_info);
    }

    // semaphore stuff
    {
        VkSemaphoreCreateInfo create_info = { 0 };
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->image_available_semaphore[i]);
            if(!dm_vulkan_decode_vr(vr))
            {
                DM_LOG_FATAL("vkCreateSemaphore failed");
                return false;
            }

            vr = vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->render_finished_semaphore[i]);
            if(!dm_vulkan_decode_vr(vr))
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

    // descriptor buffers
    {
        VkMemoryPropertyFlagBits mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        // ubo buffer
        VkBufferCreateInfo create_info = { 0 };
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        create_info.size  = vulkan_renderer->device.descriptor_buffer_props.uniformBufferDescriptorSize * DM_VULKAN_MAX_CONSTANT_BUFFERS;

        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkCreateBuffer(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->ubo_buffer.buffer.buffer[i]);
            if(!dm_vulkan_decode_vr(vr))
            {
                DM_LOG_FATAL("vkCreateBuffer failed");
                DM_LOG_ERROR("Could not create ubo descriptor buffer");
                return false;
            }

            VkMemoryRequirements mem_reqs = { 0 };
            vkGetBufferMemoryRequirements(vulkan_renderer->device.logical, vulkan_renderer->ubo_buffer.buffer.buffer[i], &mem_reqs);

            if(!dm_vulkan_allocate_memory(mem_reqs, mem_flags, create_info.usage, &vulkan_renderer->ubo_buffer.buffer.memory[i], vulkan_renderer)) return false;
            vkBindBufferMemory(vulkan_renderer->device.logical, vulkan_renderer->ubo_buffer.buffer.buffer[i], vulkan_renderer->ubo_buffer.buffer.memory[i], 0);

            vkMapMemory(vulkan_renderer->device.logical, vulkan_renderer->ubo_buffer.buffer.memory[i], 0, create_info.size, 0, &vulkan_renderer->ubo_buffer.mapped_memory[i].begin);
            vulkan_renderer->ubo_buffer.mapped_memory[i].current = vulkan_renderer->ubo_buffer.mapped_memory[i].begin;

            VkBufferDeviceAddressInfoKHR address_info = { 0 };
            address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            address_info.buffer = vulkan_renderer->ubo_buffer.buffer.buffer[i];
            vulkan_renderer->ubo_buffer.buffer_address[i].deviceAddress = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &address_info);
        }

        // combined image buffer
        create_info.usage |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
        create_info.size = vulkan_renderer->device.descriptor_buffer_props.combinedImageSamplerDescriptorSize * DM_VULKAN_MAX_TEXTURES;
        
        for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vr = vkCreateBuffer(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->sampler_buffer.buffer.buffer[i]);
            if(!dm_vulkan_decode_vr(vr))
            {
                DM_LOG_FATAL("vkCreateBuffer failed");
                DM_LOG_ERROR("Could not create image descriptor buffer");
                return false;
            }

            VkMemoryRequirements mem_reqs = { 0 };
            vkGetBufferMemoryRequirements(vulkan_renderer->device.logical, vulkan_renderer->sampler_buffer.buffer.buffer[i], &mem_reqs);

            if(!dm_vulkan_allocate_memory(mem_reqs, mem_flags, create_info.usage, &vulkan_renderer->sampler_buffer.buffer.memory[i], vulkan_renderer)) return false;
            vkBindBufferMemory(vulkan_renderer->device.logical, vulkan_renderer->sampler_buffer.buffer.buffer[i], vulkan_renderer->sampler_buffer.buffer.memory[i], 0);

            vkMapMemory(vulkan_renderer->device.logical, vulkan_renderer->sampler_buffer.buffer.memory[i], 0, create_info.size, 0, &vulkan_renderer->sampler_buffer.mapped_memory[i].begin);
            vulkan_renderer->sampler_buffer.mapped_memory[i].current = vulkan_renderer->sampler_buffer.mapped_memory[i].begin;

            VkBufferDeviceAddressInfoKHR address_info = { 0 };
            address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            address_info.buffer = vulkan_renderer->sampler_buffer.buffer.buffer[i];
            vulkan_renderer->sampler_buffer.buffer_address[i].deviceAddress = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &address_info);
        }
    }

    return true;
}

bool dm_renderer_backend_finish_init(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    // close and submit transient command buffer
    vkEndCommandBuffer(vulkan_renderer->device.transient_family.buffer[vulkan_renderer->current_frame]);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &vulkan_renderer->device.transient_family.buffer[vulkan_renderer->current_frame];
    
    vkQueueSubmit(vulkan_renderer->device.transient_family.queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan_renderer->device.transient_family.queue);

    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    VkResult vr;

    const VkDevice device                  = vulkan_renderer->device.logical;
    const VkAllocationCallbacks* allocator = vulkan_renderer->allocator;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vulkan_renderer->current_frame = i;
        dm_vulkan_wait_for_previous_frame(vulkan_renderer);
    }

    vkDeviceWaitIdle(device);

    for(uint32_t i=0; i<vulkan_renderer->vb_count; i++)
    {
        for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
        {
            vkDestroyBuffer(device, vulkan_renderer->vertex_buffers[i].host_buffer.buffer[j], allocator);
            vkFreeMemory(device, vulkan_renderer->vertex_buffers[i].host_buffer.memory[j], allocator);

            vkDestroyBuffer(device, vulkan_renderer->vertex_buffers[i].device_buffer.buffer[j], allocator);
            vkFreeMemory(device, vulkan_renderer->vertex_buffers[i].device_buffer.memory[j], allocator);
        }
    }

    for(uint32_t i=0; i<vulkan_renderer->ib_count; i++)
    {
        for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
        {
            vkDestroyBuffer(device, vulkan_renderer->index_buffers[i].host_buffer.buffer[j], allocator);
            vkFreeMemory(device, vulkan_renderer->index_buffers[i].host_buffer.memory[j], allocator);

            vkDestroyBuffer(device, vulkan_renderer->index_buffers[i].device_buffer.buffer[j], allocator);
            vkFreeMemory(device, vulkan_renderer->index_buffers[i].device_buffer.memory[j], allocator);
        }
    }

    for(uint32_t i=0; i<vulkan_renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
        {
            vkUnmapMemory(device, vulkan_renderer->constant_buffers[i].buffer.memory[j]);
            vulkan_renderer->constant_buffers[i].mapped_memory[i] = NULL;
            vkDestroyBuffer(device, vulkan_renderer->constant_buffers[i].buffer.buffer[j], allocator);
            vkFreeMemory(device, vulkan_renderer->constant_buffers[i].buffer.memory[j], allocator);
        }
    }

    for(uint8_t i=0; i<vulkan_renderer->raster_pipe_count; i++)
    {
        for(uint8_t j=0; j<vulkan_renderer->raster_pipes[i].descriptor_set_layout_count; j++)
        {
            vkDestroyDescriptorSetLayout(device, vulkan_renderer->raster_pipes[i].descriptor_set_layout[j], allocator);
        }
        vkDestroyPipelineLayout(device, vulkan_renderer->raster_pipes[i].layout, allocator);
        vkDestroyPipeline(device, vulkan_renderer->raster_pipes[i].pipeline, allocator);
    }

    for(uint8_t i=0; i<vulkan_renderer->rp_count; i++)
    {
        vkDestroyRenderPass(device, vulkan_renderer->renderpasses[i].renderpass, allocator);
    }

    for(uint32_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkUnmapMemory(device, vulkan_renderer->ubo_buffer.buffer.memory[i]);
        vulkan_renderer->ubo_buffer.mapped_memory[i].begin   = NULL;
        vulkan_renderer->ubo_buffer.mapped_memory[i].current = NULL;
        vkDestroyBuffer(device, vulkan_renderer->ubo_buffer.buffer.buffer[i], allocator);
        vkFreeMemory(device, vulkan_renderer->ubo_buffer.buffer.memory[i], allocator);

        vkUnmapMemory(device, vulkan_renderer->sampler_buffer.buffer.memory[i]);
        vulkan_renderer->sampler_buffer.mapped_memory[i].begin   = NULL;
        vulkan_renderer->sampler_buffer.mapped_memory[i].current = NULL;
        vkDestroyBuffer(device, vulkan_renderer->sampler_buffer.buffer.buffer[i], allocator);
        vkFreeMemory(device, vulkan_renderer->sampler_buffer.buffer.memory[i], allocator);

        vkDestroyFence(device, vulkan_renderer->fences[i].fence, allocator);
        vkDestroySemaphore(device, vulkan_renderer->image_available_semaphore[i], allocator);
        vkDestroySemaphore(device, vulkan_renderer->render_finished_semaphore[i], allocator);
        vkDestroyFramebuffer(device, vulkan_renderer->swapchain.frame_buffers[i], allocator);
        vkDestroyImageView(device, vulkan_renderer->swapchain.views[i], allocator);
    }

    vkDestroyCommandPool(device, vulkan_renderer->device.graphics_family.pool,  allocator);
    vkDestroyCommandPool(device, vulkan_renderer->device.transfer_family.pool,  allocator);
    vkDestroyCommandPool(device, vulkan_renderer->device.compute_family.pool,   allocator);
    vkDestroyCommandPool(device, vulkan_renderer->device.transient_family.pool, allocator);

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

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const VkDevice device = vulkan_renderer->device.logical;

    vkDeviceWaitIdle(device);

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFramebuffer(device, vulkan_renderer->swapchain.frame_buffers[i], vulkan_renderer->allocator);
        vkDestroyImageView(device, vulkan_renderer->swapchain.views[i], vulkan_renderer->allocator);
    }

    vkDestroySwapchainKHR(device, vulkan_renderer->swapchain.swapchain, vulkan_renderer->allocator);

    if(!dm_vulkan_create_swapchain(renderer))
    {
        DM_LOG_FATAL("Recreating swapchain failed");
        return false;
    }

    if(!dm_vulkan_create_rendertargets(vulkan_renderer))
    {
        DM_LOG_FATAL("Recreating rendertargets failed");
        return false;
    }

    if(!dm_vulkan_create_framebuffers(vulkan_renderer))
    {
        DM_LOG_FATAL("Recreating framebuffers failed");
        return false;
    }

    return true;
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    VkDevice device = vulkan_renderer->device.logical;

    const uint8_t current_frame = vulkan_renderer->current_frame;

    vkWaitForFences(device, 1, &vulkan_renderer->fences[current_frame].fence, VK_TRUE, UINT64_MAX);

    vr = vkAcquireNextImageKHR(device, vulkan_renderer->swapchain.swapchain, UINT64_MAX, vulkan_renderer->image_available_semaphore[current_frame], VK_NULL_HANDLE, &vulkan_renderer->image_index);
    if(vr == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return dm_renderer_backend_resize(renderer->width, renderer->height, renderer);
    }
    else if(vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
    {
        dm_vulkan_decode_vr(vr);
        DM_LOG_FATAL("vkAcquireNextImageKHR failed");
        return false;
    }

    vkResetFences(device, 1, &vulkan_renderer->fences[current_frame].fence);

    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_family.buffer[current_frame];

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkResetCommandBuffer(cmd_buffer, 0);
    vr = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkBeginCommandBuffer failed");
        return false;
    }

    VkClearValue clear_color = {{{ 0.2f,0.5f,0.7f,1.f }}};

    VkRenderPassBeginInfo renderpass_begin_info = { 0 };
    renderpass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.renderPass        = vulkan_renderer->renderpasses[DM_VULKAN_DEFAULT_RENDERPASS].renderpass;
    renderpass_begin_info.framebuffer       = vulkan_renderer->swapchain.frame_buffers[vulkan_renderer->image_index];
    renderpass_begin_info.renderArea.extent = vulkan_renderer->swapchain.extents;
    renderpass_begin_info.clearValueCount   = 1;
    renderpass_begin_info.pClearValues      = &clear_color;

    vkCmdBeginRenderPass(cmd_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // descriptor buffers
    VkDescriptorBufferBindingInfoEXT binding_infos[2] = { 0 };

    binding_infos[0].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    binding_infos[0].address = vulkan_renderer->ubo_buffer.buffer_address[current_frame].deviceAddress; 
    binding_infos[0].usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

    binding_infos[1].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    binding_infos[1].address = vulkan_renderer->sampler_buffer.buffer_address[current_frame].deviceAddress;
    binding_infos[1].usage   = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

    vkCmdBindDescriptorBuffersEXT(cmd_buffer, _countof(binding_infos), binding_infos);

    vulkan_renderer->ubo_buffer.mapped_memory[current_frame].current = vulkan_renderer->ubo_buffer.mapped_memory[current_frame].begin;
    vulkan_renderer->sampler_buffer.mapped_memory[current_frame].current = vulkan_renderer->sampler_buffer.mapped_memory[current_frame].begin;

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    VkResult vr;

    const uint32_t current_frame = vulkan_renderer->current_frame;

    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_family.buffer[current_frame];

    vkCmdEndRenderPass(cmd_buffer);

    vr = vkEndCommandBuffer(cmd_buffer);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkEndCommandBuffer failed");
        return false;
    }

    VkSemaphore wait_semaphores[] = { vulkan_renderer->image_available_semaphore[vulkan_renderer->current_frame] };
    VkSemaphore signal_semaphores[] = { vulkan_renderer->render_finished_semaphore[vulkan_renderer->current_frame] };
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

    vr = vkQueueSubmit(vulkan_renderer->device.graphics_family.queue, 1, &submit_info, vulkan_renderer->fences[current_frame].fence);
    if(!dm_vulkan_decode_vr(vr))
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
    present_info.pImageIndices      = &vulkan_renderer->image_index;

    // this causes a memory leak with my NVIDIA 4070 Super on Windows 11
    // issues is caused by validation layers, so only affected in debug
    vr = vkQueuePresentKHR(vulkan_renderer->device.graphics_family.queue, &present_info);

    if(vr==VK_ERROR_OUT_OF_DATE_KHR || vr==VK_SUBOPTIMAL_KHR)
    {
        if(!dm_renderer_backend_resize(context->renderer.width, context->renderer.height, &context->renderer)) return false;
    }
    else if(!dm_vulkan_decode_vr(vr))
    {
        return false;
    }
#ifdef DM_DEBUG
    // this fixes memory leak from present, not sure why, but is taken from this github post from 2018(!!!!!!!!!)
    // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2468
    vr = vkQueueWaitIdle(vulkan_renderer->device.graphics_family.queue);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkQueueWaitIdle failed");
        return false;
    }
#endif

    vulkan_renderer->current_frame = (vulkan_renderer->current_frame + 1) % DM_VULKAN_MAX_FRAMES_IN_FLIGHT;

    return true;
}

/*******************
 * RENDER RESOURCES
 * ******************/
#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_shader_module(const char* path, VkShaderModule* module, VkDevice device)
{
    VkResult vr;

    DM_LOG_INFO("Loading shader: %s", path);
    
    size_t size;
    void* shader_data = dm_read_bytes(path, "rb", &size);
    if(!shader_data) 
    {
        DM_LOG_ERROR("Could not load shader: %s", path);
        return false;
    }

    VkShaderModuleCreateInfo create_info = { 0 };
    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pCode    = shader_data;
    create_info.codeSize = size;

    vr = vkCreateShaderModule(device, &create_info, NULL, module);
    dm_free((void**)&shader_data);

    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateShaderModule failed");
        return false;
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_descriptor_set()
{
    return true;
}

bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_raster_pipeline pipeline = { 0 };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = { 0 };
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = { 0 };
    VkVertexInputBindingDescription bind_descriptions[2] = { 0 };
    VkVertexInputAttributeDescription vertex_attr_create_info[DM_RENDER_MAX_INPUT_ELEMENTS] = { 0 };
    VkShaderModule vs, ps;
    VkPipelineShaderStageCreateInfo vs_create_info = { 0 };
    VkPipelineShaderStageCreateInfo ps_create_info = { 0 };
    VkPipelineColorBlendStateCreateInfo blend_create_info = { 0 };
    VkPipelineColorBlendAttachmentState blend_state = { 0 };
    VkPipelineMultisampleStateCreateInfo multi_create_info = { 0 };
    VkPipelineRasterizationStateCreateInfo raster_create_info = { 0 };
    VkPipelineViewportStateCreateInfo viewport_state_info = { 0 };
    VkPipelineDynamicStateCreateInfo  dynamic_state_info  = { 0 };
    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    // === input assembler ===
    {
        input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        switch(desc.input_assembler.topology)
        {
            case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
            input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;

            default:
            DM_LOG_ERROR("Unknown topology. Assuming VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST");
            input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
        }
    }

    // === vertex input ===
    {
        vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        bind_descriptions[0].binding   = 0;
        bind_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        bind_descriptions[1].binding   = 1;
        bind_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        
        uint8_t element_count = 0;

        for(uint8_t i=0; i<desc.input_assembler.input_element_count; i++)
        {
            dm_input_element_desc d = desc.input_assembler.input_elements[i];

            uint8_t matrix_count = d.format == DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4 ? 4 : 1;

            for(uint8_t j=0; j<matrix_count; j++)
            {
                VkVertexInputAttributeDescription* input = &vertex_attr_create_info[element_count];
                input->offset   = d.offset;
                input->location = element_count;

                switch(d.format)
                {
                    case DM_INPUT_ELEMENT_FORMAT_FLOAT_2:
                    input->format = VK_FORMAT_R32G32_SFLOAT;
                    break;

                    default:
                    DM_LOG_ERROR("Unknown input format. Assuming VK_FORMAT_R32G32B32_SFLOAT");
                    case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                    input->format = VK_FORMAT_R32G32B32_SFLOAT;
                    break;

                    case DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4:
                    case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
                    input->format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                }

                switch(d.class)
                {
                    case DM_INPUT_ELEMENT_CLASS_PER_VERTEX:
                    input->binding = 0;
                    bind_descriptions[0].stride = d.stride;
                    break;

                    case DM_INPUT_ELEMENT_CLASS_PER_INSTANCE:
                    input->binding = 1;
                    bind_descriptions[1].stride = d.stride;
                    break;

                    default:
                    DM_LOG_ERROR("Unknown input element class. Assuming per vertex");
                    input->binding = 0;
                    bind_descriptions[0].stride = d.stride;
                    break;
                }

                element_count++;
            }
        }

        vertex_input_create_info.vertexBindingDescriptionCount = 2;
        vertex_input_create_info.pVertexBindingDescriptions    = bind_descriptions;

        vertex_input_create_info.vertexAttributeDescriptionCount = element_count;
        vertex_input_create_info.pVertexAttributeDescriptions    = vertex_attr_create_info;
    }

    // === shaders ===
    {
        if(!dm_vulkan_create_shader_module(desc.rasterizer.vertex_shader_desc.path, &vs, vulkan_renderer->device.logical)) 
        {
            DM_LOG_ERROR("Could not load vertex shader");
            return false;
        }
        if(!dm_vulkan_create_shader_module(desc.rasterizer.pixel_shader_desc.path, &ps, vulkan_renderer->device.logical)) 
        { 
            DM_LOG_ERROR("Could not load pixel shader");
            return false;
        }

        vs_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vs_create_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vs_create_info.module = vs;
        vs_create_info.pName  = "main";

        ps_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ps_create_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        ps_create_info.module = ps;
        ps_create_info.pName  = "main";
    }
    VkPipelineShaderStageCreateInfo shader_stages[] = { vs_create_info, ps_create_info };

    // === descriptor set ===
    // this is probably really terrible
    for(uint8_t i=0; i<desc.descriptor_group_count; i++)
    {
        dm_descriptor_group group = desc.descriptor_group[i];

        VkDescriptorSetLayoutBinding bindings[DM_DESCRIPTOR_GROUP_MAX_RANGES] = { 0 };
        VkDescriptorSetLayoutCreateInfo layout_create_info = { 0 };

        uint16_t descriptor_count = 0;

       for(uint8_t j=0; j<group.range_count; j++)
        {
            bindings[j].binding         = j; 
            bindings[j].descriptorCount = group.ranges[j].count;
            descriptor_count += group.ranges[j].count;

            switch(group.ranges[j].type)
            {
                case DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER:
                bindings[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;

                case DM_DESCRIPTOR_RANGE_TYPE_TEXTURE:
                bindings[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;

                default:
                DM_LOG_FATAL("Unsupported or uncreated resource for descriptor set");
                return false;
            }

            if(group.flags & DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER) bindings[j].stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
            if(group.flags & DM_DESCRIPTOR_GROUP_FLAG_PIXEL_SHADER)  bindings[j].stageFlags  |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_create_info.bindingCount = group.range_count;
        layout_create_info.pBindings    = bindings;
        layout_create_info.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

        vr = vkCreateDescriptorSetLayout(vulkan_renderer->device.logical, &layout_create_info, vulkan_renderer->allocator, &pipeline.descriptor_set_layout[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateDescriptorSetLayout failed");
            return false;
        }

        // descriptor layout sizes and alignment
        vkGetDescriptorSetLayoutSizeEXT(vulkan_renderer->device.logical, pipeline.descriptor_set_layout[i], &pipeline.descriptor_set_layout_sizes[i]);
        pipeline.descriptor_set_layout_sizes[i] = (pipeline.descriptor_set_layout_sizes[i] + vulkan_renderer->device.descriptor_buffer_props.descriptorBufferOffsetAlignment - 1);
        pipeline.descriptor_set_layout_sizes[i] &= ~(vulkan_renderer->device.descriptor_buffer_props.descriptorBufferOffsetAlignment - 1);

        vkGetDescriptorSetLayoutBindingOffsetEXT(vulkan_renderer->device.logical, pipeline.descriptor_set_layout[i], i, &pipeline.descriptor_set_layout_offsets[i]);

        pipeline.descriptor_set_layout_count++;
    }

    // === layout ===
    {
        // TODO: needs to be configurable
        VkPipelineLayoutCreateInfo create_info = { 0 };
        create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pSetLayouts    = pipeline.descriptor_set_layout;
        create_info.setLayoutCount = pipeline.descriptor_set_layout_count;

        vr = vkCreatePipelineLayout(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &pipeline.layout);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreatePipelineLayout failed");
            return false;
        }
    }

    // === blend === 
    {   
        blend_state.blendEnable = VK_FALSE;

        blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blend_create_info.sType   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend_create_info.logicOp = VK_LOGIC_OP_COPY;

        blend_create_info.attachmentCount = 1;
        blend_create_info.pAttachments    = &blend_state;
    }

    // === multisampling ===
    {
        multi_create_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_create_info.sampleShadingEnable  = VK_FALSE;
        multi_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    }

    // === rasterizer ===
    {
        raster_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster_create_info.lineWidth = 1.f;
        
        switch(desc.rasterizer.polygon_fill)
        {
            case DM_RASTERIZER_POLYGON_FILL_FILL:
            raster_create_info.polygonMode = VK_POLYGON_MODE_FILL;
            break;

            case DM_RASTERIZER_POLYGON_FILL_WIREFRAME:
            raster_create_info.polygonMode = VK_POLYGON_MODE_LINE;
            break;

            default:
            DM_LOG_ERROR("Unknown polygon fill mode. Assuming VK_POLYGON_MODE_FILL");
            raster_create_info.polygonMode = VK_POLYGON_MODE_FILL;
            break;
        }

        switch(desc.rasterizer.cull_mode)
        {
            default:
            DM_LOG_ERROR("Unknown cull mode. Assuming VK_CULL_MODE_BIT");
            case DM_RASTERIZER_CULL_MODE_BACK:
            raster_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
            break;

            case DM_RASTERIZER_CULL_MODE_FRONT:
            raster_create_info.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;

            case DM_RASTERIZER_CULL_MODE_NONE:
            raster_create_info.cullMode = VK_CULL_MODE_NONE;
        }

        switch(desc.rasterizer.front_face)
        {
            case DM_RASTERIZER_FRONT_FACE_CLOCKWISE:
            raster_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
            break;

            case DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE:
            raster_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            break;

            default:
            DM_LOG_ERROR("Unknown front face. Assuming VK_FRONT_FACE_CLOCKWISE");
            break;
        }
    }

    // === viewport and scissor ===
    {
        viewport_state_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info.viewportCount = 1;
        viewport_state_info.scissorCount  = 1;
        
        switch(desc.viewport.type)
        {
            case DM_VIEWPORT_TYPE_DEFAULT:
            default:
            pipeline.viewport.x        = 0;
            pipeline.viewport.y        = 0;
            pipeline.viewport.width    = (float)renderer->width;
            pipeline.viewport.height   = (float)renderer->height;
            pipeline.viewport.minDepth = 0.f;
            pipeline.viewport.maxDepth = 1.f;
            break;

            case DM_VIEWPORT_TYPE_CUSTOM:
            DM_LOG_FATAL("Not supported yet");
            return false;
        }

        switch(desc.scissor.type)
        {
            case DM_SCISSOR_TYPE_DEFAULT:
            default:
            pipeline.scissor.offset.x      = 0;
            pipeline.scissor.offset.y      = 0;
            pipeline.scissor.extent.width  = renderer->width;
            pipeline.scissor.extent.height = renderer->height;
            break;

            case DM_SCISSOR_TYPE_CUSTOM:
            DM_LOG_FATAL("Not supported yet");
            return false;
        }

        viewport_state_info.pViewports = &pipeline.viewport;
        viewport_state_info.pScissors  = &pipeline.scissor;
    }

    // === dynamic states ===
    {
        dynamic_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.dynamicStateCount = 2;
        dynamic_state_info.pDynamicStates    = dynamic_states;
    }


    // === pipeline object ===
    {
        VkGraphicsPipelineCreateInfo create_info = { 0 };
        create_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = 2;
        create_info.pStages    = shader_stages;
        create_info.renderPass = vulkan_renderer->renderpasses[DM_VULKAN_DEFAULT_RENDERPASS].
        renderpass;

        create_info.pVertexInputState   = &vertex_input_create_info;
        create_info.pInputAssemblyState = &input_assembly_create_info;
        create_info.layout              = pipeline.layout;
        create_info.pColorBlendState    = &blend_create_info;
        create_info.pMultisampleState   = &multi_create_info;
        create_info.pRasterizationState = &raster_create_info;
        create_info.pViewportState      = &viewport_state_info;
        create_info.pDynamicState       = &dynamic_state_info;
        create_info.flags               = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

        vr = vkCreateGraphicsPipelines(vulkan_renderer->device.logical, VK_NULL_HANDLE, 1, &create_info, vulkan_renderer->allocator, &pipeline.pipeline);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateGraphicsPipeline failed");
            return false;
        }
    }

    // === cleanup ===
    vkDestroyShaderModule(vulkan_renderer->device.logical, vs, vulkan_renderer->allocator);
    vkDestroyShaderModule(vulkan_renderer->device.logical, ps, vulkan_renderer->allocator);

    //
    {
        dm_memcpy(vulkan_renderer->raster_pipes + vulkan_renderer->raster_pipe_count, &pipeline, sizeof(pipeline));
        handle->index = vulkan_renderer->raster_pipe_count++;
    }

    return true;
}

/*************
 *  RESOURCES
 * ************/
bool dm_vulkan_find_memory_type(uint32_t type_filter, VkPhysicalDeviceMemoryProperties properties, VkMemoryPropertyFlags flags, uint32_t* index)
{
    for(uint8_t i=0; i<properties.memoryTypeCount; i++)
    {
        if((type_filter & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags)==flags)
        {
            *index = i;
            return true;
        }
    }
    
    DM_LOG_FATAL("Could not find suitable memory");
    return false;
}

bool dm_vulkan_allocate_memory(VkMemoryRequirements memory_reqs, VkMemoryPropertyFlagBits memory_flags, VkBufferUsageFlagBits usage_flags, VkDeviceMemory* memory, dm_vulkan_renderer* vulkan_renderer)
{
    VkMemoryAllocateInfo allocate_info = { 0 };
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = memory_reqs.size;
    if(!dm_vulkan_find_memory_type(memory_reqs.memoryTypeBits, vulkan_renderer->device.memory_properties, memory_flags, &allocate_info.memoryTypeIndex)) return false;

    VkMemoryAllocateFlagsInfoKHR allocate_flags_info = { 0 };
    if(usage_flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocate_info.pNext       = &allocate_flags_info;
    }

    if(!dm_vulkan_decode_vr(vkAllocateMemory(vulkan_renderer->device.logical, &allocate_info, vulkan_renderer->allocator, memory)))
    {
        DM_LOG_FATAL("vkAllocateMemory failed");
        return false;
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_copy_memory(VkDeviceMemory* memory, void* data, const size_t size, dm_vulkan_renderer* vulkan_renderer)
{
    void* temp = NULL;

    vkMapMemory(vulkan_renderer->device.logical, *memory, 0, size, 0, &temp);
    if(!temp)
    {
        DM_LOG_FATAL("vkMapMemory failed");
        return false;
    }
    dm_memcpy(temp, data, size);
    vkUnmapMemory(vulkan_renderer->device.logical, *memory);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_buffer(const size_t size, VkBufferUsageFlagBits buffer_type, VkMemoryPropertyFlagBits memory_flags, VkSharingMode sharing_mode, VkBuffer* buffer, VkDeviceMemory* memory, dm_vulkan_renderer* vulkan_renderer)
{
    uint32_t family_indices[3] = { 0 };

    if(sharing_mode == VK_SHARING_MODE_CONCURRENT)
    {
        family_indices[0] = vulkan_renderer->device.graphics_family.index;
        family_indices[1] = vulkan_renderer->device.transfer_family.index;
        family_indices[2] = vulkan_renderer->device.transient_family.index;
    }

    VkBufferCreateInfo create_info    = { 0 };
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size                  = size;
    create_info.usage                 = buffer_type;
    create_info.sharingMode           = sharing_mode;
    create_info.pQueueFamilyIndices   = family_indices;
    create_info.queueFamilyIndexCount = sharing_mode==VK_SHARING_MODE_CONCURRENT ? _countof(family_indices) : 0;

    if(!dm_vulkan_decode_vr(vkCreateBuffer(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, buffer)))
    {
        DM_LOG_FATAL("vkCreateBuffer failed");
        return false;
    }

    // device memory
    VkMemoryRequirements memory_reqs ={ 0 };
    vkGetBufferMemoryRequirements(vulkan_renderer->device.logical, *buffer, &memory_reqs);

    if(!dm_vulkan_allocate_memory(memory_reqs, memory_flags, create_info.usage, memory, vulkan_renderer)) return false;

    vkBindBufferMemory(vulkan_renderer->device.logical, *buffer, *memory, 0);

    return true;
}

bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_vertex_buffer buffer = { 0 }; 

    VkBufferUsageFlagBits host_buffer_usage   = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits device_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VkMemoryPropertyFlagBits host_memory_flags   = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    VkMemoryPropertyFlagBits device_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
    
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer
        if(!dm_vulkan_create_buffer(desc.size, host_buffer_usage, host_memory_flags, sharing_mode, &buffer.host_buffer.buffer[i], &buffer.host_buffer.memory[i], vulkan_renderer)) return false;

        // device buffer
        if(!dm_vulkan_create_buffer(desc.size, device_buffer_usage, device_memory_flags, sharing_mode, &buffer.device_buffer.buffer[i], &buffer.device_buffer.memory[i], vulkan_renderer)) return false;

        // buffer data
        if(!desc.data) continue;

        if(!dm_vulkan_copy_memory(&buffer.host_buffer.memory[i], desc.data, desc.size, vulkan_renderer)) return false;

        VkBufferCopy copy_region = { 0 };
        copy_region.size = desc.size;

        vkCmdCopyBuffer(vulkan_renderer->device.transient_family.buffer[vulkan_renderer->current_frame], buffer.host_buffer.buffer[i], buffer.device_buffer.buffer[i], 1, &copy_region);
    }

    //
    {
        dm_memcpy(vulkan_renderer->vertex_buffers + vulkan_renderer->vb_count, &buffer, sizeof(buffer));
        handle->index = vulkan_renderer->vb_count++;
    }

    return true;
}

bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_index_buffer buffer = { 0 }; 

    switch(desc.index_type)
    {
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT16:
        buffer.index_type = VK_INDEX_TYPE_UINT16;
        break;

        default:
        DM_LOG_ERROR("Unknown index buffer index type, assuming uint32");
        case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
        buffer.index_type = VK_INDEX_TYPE_UINT32;
        break;
    }

    VkBufferUsageFlagBits host_buffer_usage   = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits device_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkMemoryPropertyFlagBits host_memory_flags   = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    VkMemoryPropertyFlagBits device_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
    
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // host buffer
        if(!dm_vulkan_create_buffer(desc.size, host_buffer_usage, host_memory_flags, sharing_mode, &buffer.host_buffer.buffer[i], &buffer.host_buffer.memory[i], vulkan_renderer)) return false;

        // device buffer
        if(!dm_vulkan_create_buffer(desc.size, device_buffer_usage, device_memory_flags, sharing_mode, &buffer.device_buffer.buffer[i], &buffer.device_buffer.memory[i], vulkan_renderer)) return false;

        // buffer data
        if(!desc.data) continue;

        if(!dm_vulkan_copy_memory(&buffer.host_buffer.memory[i], desc.data, desc.size, vulkan_renderer)) return false;

        VkBufferCopy copy_region = { 0 };
        copy_region.size = desc.size;

        vkCmdCopyBuffer(vulkan_renderer->device.transient_family.buffer[vulkan_renderer->current_frame], buffer.host_buffer.buffer[i], buffer.device_buffer.buffer[i], 1, &copy_region);
    }

    //
    {
        dm_memcpy(vulkan_renderer->index_buffers + vulkan_renderer->ib_count, &buffer, sizeof(buffer));
        handle->index = vulkan_renderer->ib_count++;
    }

    return true;
}

bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_constant_buffer buffer = { 0 };
    
    VkBufferUsageFlagBits    buffer_usage   = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkMemoryPropertyFlagBits memory_flags   = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    
    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
    
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(!dm_vulkan_create_buffer(desc.size, buffer_usage, memory_flags, sharing_mode, &buffer.buffer.buffer[i], &buffer.buffer.memory[i], vulkan_renderer)) return false;

        // buffer data
        if(!desc.data) continue;

        if(!dm_vulkan_copy_memory(&buffer.buffer.memory[i], desc.data, desc.size, vulkan_renderer)) return false;

        vkMapMemory(vulkan_renderer->device.logical, buffer.buffer.memory[i], 0, desc.size, 0, &buffer.mapped_memory[i]);
        if(!buffer.mapped_memory[i])
        {
            DM_LOG_FATAL("vkMapMemory failed");
            return false;
        }
    }

    buffer.size = desc.size;

    //
    dm_memcpy(vulkan_renderer->constant_buffers + vulkan_renderer->cb_count, &buffer, sizeof(buffer));
    handle->index = vulkan_renderer->cb_count++;

    return true;
}

bool dm_renderer_backend_create_texture(dm_texture_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    return true;
}

/******************
 * RENDER COMMANDS
 * *****************/
bool dm_render_command_backend_bind_raster_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame      = vulkan_renderer->current_frame;
    const VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_family.buffer[current_frame];

    dm_vulkan_raster_pipeline pipeline = vulkan_renderer->raster_pipes[handle.index];

    vkCmdBindPipeline(cmd_buffer, 0, pipeline.pipeline);
    vkCmdSetViewport(cmd_buffer, 0,1, &pipeline.viewport);
    vkCmdSetScissor(cmd_buffer, 0,1, &pipeline.scissor);

    return true;
}

bool dm_render_command_backend_bind_descriptor_group(dm_render_handle handle, uint8_t group_index, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame      = vulkan_renderer->current_frame;
    const VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_family.buffer[current_frame];

    dm_vulkan_raster_pipeline pipeline = vulkan_renderer->raster_pipes[handle.index];


    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, uint8_t slot, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_vertex_buffer buffer = vulkan_renderer->vertex_buffers[handle.index];
    
    VkBuffer buffers[]     = { buffer.device_buffer.buffer[current_frame] };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(vulkan_renderer->device.graphics_family.buffer[current_frame], slot,_countof(buffers),buffers, offsets);

    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_index_buffer buffer = vulkan_renderer->index_buffers[handle.index];

    vkCmdBindIndexBuffer(vulkan_renderer->device.graphics_family.buffer[current_frame], buffer.device_buffer.buffer[current_frame], 0, buffer.index_type);

    return true;
}

bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_render_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_vertex_buffer buffer = vulkan_renderer->vertex_buffers[handle.index];
        
    if(!dm_vulkan_copy_memory(&buffer.host_buffer.memory[current_frame], data, size, vulkan_renderer)) return false;

    VkBufferCopy copy_region = { 0 };
    copy_region.size = size;

    vkCmdCopyBuffer(vulkan_renderer->device.graphics_family.buffer[vulkan_renderer->current_frame], buffer.host_buffer.buffer[current_frame], buffer.device_buffer.buffer[current_frame], 1, &copy_region);

    return true;
}

bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_render_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_constant_buffer buffer = vulkan_renderer->constant_buffers[handle.index];

    if(!buffer.mapped_memory[current_frame])
    {
        DM_LOG_FATAL("Constant buffer has invalid mapped memory for frame: %d", current_frame);
        return false;
    }

    dm_memcpy(buffer.mapped_memory[current_frame], data, size);

    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle buffer, uint8_t slot, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer  = vulkan_renderer->device.graphics_family.buffer[current_frame];

    dm_vulkan_constant_buffer internal_buffer = vulkan_renderer->constant_buffers[buffer.index];
    dm_vulkan_raster_pipeline bound_pipeline = vulkan_renderer->raster_pipes[vulkan_renderer->bound_pipeline.index];

    uint32_t index = slot;

    char* buffer_ptr = vulkan_renderer->ubo_buffer.mapped_memory[current_frame].current;

    VkBufferDeviceAddressInfo buffer_address_info = { 0 };
    buffer_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_address_info.buffer = internal_buffer.buffer.buffer[current_frame]; 

    VkDescriptorAddressInfoEXT address_info = { 0 };
    address_info.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT;
    address_info.format  = VK_FORMAT_UNDEFINED;
    address_info.address = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &buffer_address_info);
    address_info.range   = internal_buffer.size;

    VkDescriptorGetInfoEXT descriptor_info = { 0 };
    descriptor_info.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    descriptor_info.type                = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_info.data.pUniformBuffer = &address_info;

    vkGetDescriptorEXT(vulkan_renderer->device.logical, &descriptor_info, vulkan_renderer->device.descriptor_buffer_props.uniformBufferDescriptorSize, buffer_ptr);

    vkCmdSetDescriptorBufferOffsetsEXT(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bound_pipeline.layout, 0, 1, &index, &vulkan_renderer->ubo_buffer.offset[current_frame]);

    buffer_ptr += vulkan_renderer->device.descriptor_buffer_props.uniformBufferDescriptorSize;
    vulkan_renderer->ubo_buffer.mapped_memory[current_frame].current = buffer_ptr;

    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle texture, uint8_t slot, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer  = vulkan_renderer->device.graphics_family.buffer[current_frame];

    dm_vulkan_raster_pipeline bound_pipeline = vulkan_renderer->raster_pipes[vulkan_renderer->bound_pipeline.index];

    uint32_t index = slot;

    char* buffer_ptr = vulkan_renderer->sampler_buffer.mapped_memory[current_frame].current;

    VkDescriptorGetInfoEXT descriptor_info = { 0 };
    descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    descriptor_info.type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    vkCmdSetDescriptorBufferOffsetsEXT(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bound_pipeline.layout, 0, 1, &index, &vulkan_renderer->sampler_buffer.offset[current_frame]);

    buffer_ptr += vulkan_renderer->device.descriptor_buffer_props.combinedImageSamplerDescriptorSize;
    vulkan_renderer->sampler_buffer.mapped_memory[current_frame].current = buffer_ptr;

    return true;
}

bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    vkCmdDraw(vulkan_renderer->device.graphics_family.buffer[vulkan_renderer->current_frame], vertex_count, instance_count, vertex_offset, instance_offset);

    return true;
}

bool dm_render_command_backend_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    vkCmdDrawIndexed(vulkan_renderer->device.graphics_family.buffer[vulkan_renderer->current_frame], index_count, instance_count, index_offset, vertex_offset, instance_offset);

    return true;
}

/***************
 * VULKAN DEBUG
*****************/
#ifdef DM_DEBUG
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
    
    dm_kill(user_data);

    return VK_FALSE;
}
#endif

bool dm_vulkan_decode_vr(VkResult vr)
{
    if(vr==VK_SUCCESS) return true;
    
    switch(vr)
    {
        case VK_NOT_READY:
        DM_LOG_ERROR("VK_NOT_READY: fence or query has not yet completed");
        break;

        case VK_TIMEOUT:
        DM_LOG_ERROR("VK_TIMEOUT: a wait operation has not completed in the specified time");
        break;

        case VK_EVENT_SET:
        DM_LOG_ERROR("VK_EVENT_SET: an event is signaled");
        break;

        case VK_EVENT_RESET:
        DM_LOG_ERROR("VK_EVENT_RESET: an event is unsignaled");
        break;
        
        case VK_INCOMPLETE:
        DM_LOG_ERROR("VK_INCOMPLETE: a return array was too small for the result");
        break;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        DM_LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY: a host memory allocation has failed");
        break;

        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        DM_LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY: a device memory allocation has failed");
        break;

        case VK_ERROR_INITIALIZATION_FAILED:
        DM_LOG_ERROR("VK_ERROR_INITIALIZATION_FAILED: initialization of an object could not be completed for implementation-specific reasons");
        break;

        case VK_ERROR_DEVICE_LOST:
        DM_LOG_ERROR("VK_ERROR_DEVICE_LOST: the logical or physical device has been lost");
        break;

        case VK_ERROR_MEMORY_MAP_FAILED:
        DM_LOG_ERROR("VK_ERROR_MEMORY_MAP_FAILED: mapping of a memory object has failed");
        break;

        case VK_ERROR_LAYER_NOT_PRESENT:
        DM_LOG_ERROR("VK_ERROR_LAYER_NOT_PRESENT: a requested layer is not present or could not be loade");
        break;

        case VK_ERROR_EXTENSION_NOT_PRESENT:
        DM_LOG_ERROR("VK_ERROR_EXTENSION_NOT_PRESENT: a requested extension is not supported");
        break;

        case VK_ERROR_FEATURE_NOT_PRESENT:
        DM_LOG_ERROR("VK_ERROR_FEATURE_NOT_PRESENT: a requested features is not supported");
        break;

        case VK_ERROR_INCOMPATIBLE_DRIVER:
        DM_LOG_ERROR("VK_ERROR_INCOMPATIBLE_DRIVER: the requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons");    
        break;

        case VK_ERROR_TOO_MANY_OBJECTS:
        DM_LOG_ERROR("VK_ERROR_TOO_MANY_OBJECTS: too many objects of the type have already been created");
        break;

        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        DM_LOG_ERROR("VK_ERROR_FORMAT_NOT_SUPPORTED: a requested format is not supported on this device");
        break;

        case VK_ERROR_FRAGMENTED_POOL:
        DM_LOG_ERROR("VK_ERROR_FRAGMENTED_POOL: A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.");
        break;

        default:
        DM_LOG_FATAL("Unknown error");
        break;
    }

    return false;
}

#endif
