#ifndef DM_RENDERER_VULKAN_H
#define DM_RENDERER_VULKAN_H

#include <vulkan/vulkan.h>

typedef struct dm_vulkan_image_desc_t
{
    uint32_t              width, height;
    
    VkImageType           type;
    VkFormat              format;
    VkImageTiling         tiling;
    VkImageUsageFlags     usage;
    VkMemoryPropertyFlags mem_flags;
    
    bool                  has_view;
    VkImageAspectFlags    view_aspect_flags;
} dm_vulkan_image_desc;

typedef struct dm_vulkan_image_t
{
    dm_vulkan_image_desc desc;
    VkImage              handle;
    VkDeviceMemory       memory;
    VkImageView          view;
} dm_vulkan_image;

typedef enum dm_vulkan_renderpass_state_t
{
    DM_VULKAN_RENDERPASS_STATE_READY,
    DM_VULKAN_RENDERPASS_STATE_RECORDING,
    DM_VULKAN_RENDERPASS_STATE_IN_RENDERPASS,
    DM_VULKAN_RENDERPASS_STATE_RECORDING_ENDED,
    DM_VULKAN_RENDERPASS_STATE_SUBMITTED,
    DM_VULKAN_RENDERPASS_STATE_NOT_ALLOCATED,
    DM_VULKAN_RENDERPASS_STATE_UNKNOWN
} dm_vulkan_renderpass_state;

typedef struct dm_vulkan_renderpass_desc_t
{
    float x, y;
    float width, height;
    float r, g, b, a;
    float depth;
    float stencil;
} dm_vulkan_renderpass_desc;

typedef struct dm_vulkan_renderpass_t
{
    VkRenderPass               handle;
    dm_vulkan_renderpass_desc  desc;
    dm_vulkan_renderpass_state state;
} dm_vulkan_renderpass;

typedef enum dm_vulkan_physical_flag_t
{
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_GRAPHICS       = 1 << 0,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_PRESENT        = 1 << 1,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_COMPUTE        = 1 << 2,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_TRANSFER       = 1 << 3,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_SAMPLER_ANISOP = 1 << 4,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_DISCRETE_GPU   = 1 << 5,
    DM_VULKAN_PHYSICAL_DEVICE_FLAG_UNKNOWN        = 1 << 6,
} dm_vulkan_physical_device_flag;

#define DM_VULKAN_DEVICE_MAX_EXTENSIONS 10
typedef struct dm_vulkan_physical_device_reqs_t
{
    dm_vulkan_physical_device_flag flags;
    uint32_t                  extension_count;
    const char*               device_extension_names[DM_VULKAN_DEVICE_MAX_EXTENSIONS];
} dm_vulkan_physical_device_reqs;

typedef struct dm_vulkan_physical_device_queue_family_info_t
{
    uint32_t graphics_index, present_index, compute_index, transfer_index;
} dm_vulkan_physical_device_queue_family_info;

typedef struct dm_vulkan_swapchain_support_info_t
{
    uint32_t format_count, present_mode_count;
    
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR*      formats;
    VkPresentModeKHR*        present_modes;
} dm_vulkan_swapchain_support_info;

typedef struct dm_vulkan_swapchain_t
{
    uint32_t           max_frames_in_flight, image_count;
    VkSurfaceFormatKHR image_format;
    VkSwapchainKHR     handle;
    dm_vulkan_image    depth;
    VkImage*           images;
    VkImageView*       views;
} dm_vulkan_swapchain;

typedef struct dm_vulkan_device_t
{
    VkPhysicalDevice                 physical;
    VkDevice                         logical;
    
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceMemoryProperties mem_props;
    
    VkQueue                          graphics_queue;
    VkQueue                          transfer_queue;
    VkQueue                          present_queue;
    
    VkFormat depth_format;
    
    dm_vulkan_swapchain_support_info swapchain_support_info;
    
    uint32_t graphics_index, present_index, transfer_index, compute_index;
} dm_vulkan_device;

typedef enum dm_vulkan_renderer_flag_t
{
    DM_VULKAN_RENDERER_FLAG_RECREATING_SWAPCHAIN,
    DM_VULKAN_RENDERER_FLAG_UNKNOWN,
} dm_vulkan_renderer_flag;

typedef enum dm_vulkan_command_buffer_state_t
{
    DM_VULKAN_COMMAND_BUFFER_STATE_READY,
    DM_VULKAN_COMMAND_BUFFER_STATE_RECORDING,
    DM_VULKAN_COMMAND_BUFFER_STATE_IN_RENDERPASS,
    DM_VULKAN_COMMAND_BUFFER_STATE_RECORDING_ENDED,
    DM_VULKAN_COMMAND_BUFFER_STATE_SUBMITTED,
    DM_VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED,
    DM_VULKAN_COMMAND_BUFFER_STATE_UNKNOWN
} dm_vulkan_command_buffer_state;

typedef struct dm_vulkan_command_buffer_t
{
    VkCommandBuffer                buffer;
    dm_vulkan_command_buffer_state state;
} dm_vulkan_command_buffer;

typedef struct dm_vulkan_renderer_t
{
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR           surface;
    
    dm_vulkan_device       device;
    dm_vulkan_swapchain    swapchain;
    
    uint32_t buffer_count, shader_count, texture_count, framebuffer_count, pipeline_count;
    uint32_t width, height;
    uint32_t image_index, current_frame;
    
    dm_vulkan_renderer_flag flags;
    
#ifdef DM_DEBUG
    VkDebugUtilsMessengerEXT  debug_messenger;
#endif
} dm_vulkan_renderer;

#define DM_VULKAN_GET_RENDERER dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer
#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

#define DM_VULKAN_FUNC_CHECK(FUNC_CALL) result = FUNC_CALL
#define DM_VULKAN_FUNC_SUCCESS result==VK_SUCCESS
#else
#define DM_VULKAN_FUNC_CHECK(FUNC_CALL) FUNC_CALL
#define DM_VULKAN_FUNC_SUCCESS 1
#endif

bool     dm_vulkan_device_detect_depth_buffer_range(dm_vulkan_device* device);
uint32_t dm_vulkan_device_find_memory_index(uint32_t type_filter, uint32_t property_flags, VkPhysicalDevice device);

/*************
VULKAN BUFFER
***************/
bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_renderer_backend_destroy_buffer(dm_render_handle handle, dm_renderer* renderer)
{
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_renderer_backend_destroy_uniform(dm_render_handle handle, dm_renderer* renderer)
{
}

/**************
VULKAN TEXTURE
****************/
bool dm_vulkan_image_view_create(VkFormat format, dm_vulkan_image* image, VkImageAspectFlags aspect_flags, VkDevice device, VkAllocationCallbacks* allocator)
{
    VkResult result;
    
    VkImageViewCreateInfo create_info = { 0 };
    create_info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image                       = image->handle;
    create_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format                      = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.layerCount = 1;
    
    DM_VULKAN_FUNC_CHECK(vkCreateImageView(device, &create_info, allocator, &image->view));
    if(DM_VULKAN_FUNC_SUCCESS) return true;
    
    DM_LOG_FATAL("Creating Vulkan image view failed");
    return false;
}

bool dm_vulkan_create_image(dm_vulkan_image_desc desc, dm_vulkan_image* image, dm_vulkan_device* device, VkAllocationCallbacks* allocator)
{
    VkResult result;
    
    VkImageCreateInfo create_info = { 0 };
    create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType         = desc.type;
    create_info.extent.width      = desc.width;
    create_info.extent.height     = desc.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 4;
    create_info.arrayLayers       = 1;
    create_info.format            = desc.format;
    create_info.tiling            = desc.tiling;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = desc.usage;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    
    DM_VULKAN_FUNC_CHECK(vkCreateImage(device->logical, &create_info, allocator, &image->handle));
    if(!DM_VULKAN_FUNC_SUCCESS) 
    {
        DM_LOG_FATAL("Could not create Vulkan image");
        return false;
    }
    
    //memory reqs
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device->logical, image->handle, &mem_reqs);
    
    uint32_t mem_type = dm_vulkan_device_find_memory_index(mem_reqs.memoryTypeBits, desc.mem_flags, device->physical);
    if(mem_type == -1)
    {
        DM_LOG_FATAL("Required memory type not valid. Vulkan image not valid");
        return false;
    }
    
    VkMemoryAllocateInfo mem_info = { 0 };
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_info.allocationSize = mem_reqs.size;
    mem_info.memoryTypeIndex = mem_type;
    DM_VULKAN_FUNC_CHECK(vkAllocateMemory(device->logical, &mem_info, allocator, &image->memory));
    if(!DM_VULKAN_FUNC_SUCCESS) 
    {
        DM_LOG_FATAL("vkAllocateMemory failed");
        return false;
    }
    
    DM_VULKAN_FUNC_CHECK(vkBindImageMemory(device->logical, image->handle, image->memory, 0));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    if(!desc.has_view) return true;
    
    image->view = 0;
    return dm_vulkan_image_view_create(desc.format, image, desc.view_aspect_flags, device->logical, allocator);
}

void dm_vulkan_destroy_image(dm_vulkan_image* image, VkDevice device, VkAllocationCallbacks* allocator)
{
    if(image->view)   vkDestroyImageView(device, image->view, allocator);
    if(image->memory) vkFreeMemory(device, image->memory, allocator);
    if(image->handle) vkDestroyImage(device, image->handle, allocator);
}

bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_renderer_backend_destroy_texture(dm_render_handle handle, dm_renderer* renderer)
{
}

/***************
VULKAN PIPELINE
*****************/
bool dm_renderer_backend_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_renderer_backend_destroy_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
}

/*************
VULKAN SHADER
***************/
bool dm_vulkan_create_renderpass(dm_vulkan_renderer* vulkan_renderer, dm_render_handle* handle)
{
    VkResult result;
    
    dm_vulkan_renderpass internal_pass;
    
    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
    uint32_t attachment_desc_count = 2;
    VkAttachmentDescription* attachment_descs = dm_alloc(sizeof(VkAttachmentDescription) * attachment_desc_count);
    
    VkAttachmentDescription color_attachment = { 0 };
    color_attachment.format         = vulkan_renderer->swapchain.image_format.format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    attachment_descs[0] = color_attachment;
    
    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depth_attachment = { 0 };
    depth_attachment.format         = vulkan_renderer->device.depth_format;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    attachment_descs[1] = depth_attachment;
    
    VkAttachmentReference depth_attachment_reference = { 0 };
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    subpass.pDepthStencilAttachment = &depth_attachment_reference;
    
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    // create
    VkRenderPassCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = attachment_desc_count;
    create_info.pAttachments = attachment_descs;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;
    
    DM_VULKAN_FUNC_CHECK(vkCreateRenderPass(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &internal_pass.handle));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    
    
    dm_free(attachment_descs);
    
    return true;
}

void dm_vulkan_destroy_renderpass(dm_vulkan_renderer* vulkan_renderer, dm_vulkan_renderpass* renderpass)
{
    if(!renderpass->handle) return;
    
    vkDestroyRenderPass(vulkan_renderer->device.logical, renderpass->handle, vulkan_renderer->allocator);
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    return true;
}

void dm_renderer_backend_destroy_shader(dm_render_handle handle, dm_renderer* renderer)
{
}

/****************
VULKAN SWAPCHAIN
******************/
bool dm_vulkan_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, dm_vulkan_swapchain_support_info* support_info)
{
    VkResult result;
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &support_info->capabilities));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support_info->format_count, 0));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    if(support_info->format_count)
    {
        if(!support_info->formats) support_info->formats = dm_alloc(sizeof(VkSurfaceFormatKHR) * support_info->format_count);
        DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support_info->format_count, support_info->formats));
        if(!DM_VULKAN_FUNC_SUCCESS) return false;
    }
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support_info->present_mode_count, 0));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    if(support_info->present_mode_count)
    {
        if(!support_info->present_modes) support_info->present_modes = dm_alloc(sizeof(VkPresentModeKHR) * support_info->present_mode_count);
        DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support_info->present_mode_count, support_info->present_modes));
        if(!DM_VULKAN_FUNC_SUCCESS) return false;
    }
    
    return true;
}

bool dm_vulkan_internal_create_swapchain(dm_vulkan_renderer* vulkan_renderer, uint32_t width, uint32_t height, dm_vulkan_swapchain* swapchain)
{
    VkResult result;
    
    VkExtent2D swapchain_extent = { width, height };
    swapchain->max_frames_in_flight = 2;
    
    bool found = false;
    for(uint32_t i=0; i<vulkan_renderer->device.swapchain_support_info.format_count; i++)
    {
        VkSurfaceFormatKHR format = vulkan_renderer->device.swapchain_support_info.formats[i];
        if(format.format != VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) continue;
        
        swapchain->image_format = format;
        found = true;
        break;
    }
    
    if(!found) swapchain->image_format = vulkan_renderer->device.swapchain_support_info.formats[0];
    
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(uint32_t i=0; i<vulkan_renderer->device.swapchain_support_info.present_mode_count; i++)
    {
        VkPresentModeKHR mode = vulkan_renderer->device.swapchain_support_info.present_modes[i];
        if(mode != VK_PRESENT_MODE_MAILBOX_KHR) continue;
        
        present_mode = mode;
        break;
    }
    
    dm_vulkan_query_swapchain_support(vulkan_renderer->device.physical, vulkan_renderer->surface, &vulkan_renderer->device.swapchain_support_info);
    
    if(vulkan_renderer->device.swapchain_support_info.capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain_extent = vulkan_renderer->device.swapchain_support_info.capabilities.currentExtent;
    }
    
    VkExtent2D min = vulkan_renderer->device.swapchain_support_info.capabilities.minImageExtent;
    VkExtent2D max = vulkan_renderer->device.swapchain_support_info.capabilities.maxImageExtent;
    swapchain_extent.width  = DM_CLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = DM_CLAMP(swapchain_extent.height, min.height, max.height);
    
    uint32_t image_count = vulkan_renderer->device.swapchain_support_info.capabilities.minImageCount + 1;
    if(vulkan_renderer->device.swapchain_support_info.capabilities.maxImageCount > 0 && image_count > vulkan_renderer->device.swapchain_support_info.capabilities.maxImageCount)
    {
        image_count = vulkan_renderer->device.swapchain_support_info.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface          = vulkan_renderer->surface;
    swapchain_create_info.minImageCount    = image_count;
    swapchain_create_info.imageFormat      = swapchain->image_format.format;
    swapchain_create_info.imageColorSpace  = swapchain->image_format.colorSpace;
    swapchain_create_info.imageExtent      = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if(vulkan_renderer->device.graphics_index != vulkan_renderer->device.present_index)
    {
        uint32_t queue_family_indices[] = {
            (uint32_t)vulkan_renderer->device.graphics_index,
            (uint32_t)vulkan_renderer->device.present_index,
        };
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    swapchain_create_info.preTransform   = vulkan_renderer->device.swapchain_support_info.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode    = present_mode;
    swapchain_create_info.clipped        = VK_TRUE;
    
    DM_VULKAN_FUNC_CHECK(vkCreateSwapchainKHR(vulkan_renderer->device.logical, &swapchain_create_info, vulkan_renderer->allocator, &swapchain->handle));
    
    // get images
    vulkan_renderer->current_frame = 0;
    swapchain->image_count = 0;
    DM_VULKAN_FUNC_CHECK(vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, swapchain->handle, &swapchain->image_count, 0));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    if(!swapchain->images) swapchain->images = dm_alloc(sizeof(VkImage) * swapchain->image_count);
    if(!swapchain->views)  swapchain->views  = dm_alloc(sizeof(VkImageView) * swapchain->image_count);
    
    DM_VULKAN_FUNC_CHECK(vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, swapchain->handle, &swapchain->image_count, swapchain->images));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    for(uint32_t i=0; i<swapchain->image_count; i++)
    {
        VkImageViewCreateInfo view_info = { 0 };
        view_info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                       = swapchain->images[i];
        view_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                      = swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;
        
        DM_VULKAN_FUNC_CHECK(vkCreateImageView(vulkan_renderer->device.logical, &view_info, vulkan_renderer->allocator, &swapchain->views[i]));
        if(!DM_VULKAN_FUNC_SUCCESS) return false;
    }
    
    // depth buffer
    if(!dm_vulkan_device_detect_depth_buffer_range(&vulkan_renderer->device))
    {
        vulkan_renderer->device.depth_format = VK_FORMAT_UNDEFINED;
        DM_LOG_FATAL("Could not find depth format");
        return false;
    }
    
    dm_vulkan_image_desc image_desc = { 0 };
    image_desc.type = VK_IMAGE_TYPE_2D;
    image_desc.format = vulkan_renderer->device.depth_format;
    image_desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_desc.mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    image_desc.has_view = true;
    image_desc.view_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    
    image_desc.width = swapchain_extent.width;
    image_desc.height = swapchain_extent.height;
    
    if(!dm_vulkan_create_image(image_desc, &swapchain->depth, &vulkan_renderer->device, vulkan_renderer->allocator)) 
    {
        DM_LOG_FATAL("Could not create depth attachment");
        return false;
    }
    
    return true;
}

void dm_vulkan_destroy_swapchain(dm_vulkan_renderer* vulkan_renderer, dm_vulkan_swapchain* swapchain)
{
    dm_vulkan_destroy_image(&swapchain->depth, vulkan_renderer->device.logical, vulkan_renderer->allocator);
    
    for(uint32_t i=0; i<swapchain->image_count; i++)
    {
        vkDestroyImageView(vulkan_renderer->device.logical, swapchain->views[i], vulkan_renderer->allocator);
    }
    
    vkDestroySwapchainKHR(vulkan_renderer->device.logical, swapchain->handle, vulkan_renderer->allocator);
    
    if(swapchain->images) dm_free(swapchain->images);
    if(swapchain->views)  dm_free(swapchain->views);
}

bool dm_vulkan_create_swapchain(dm_vulkan_renderer* vulkan_renderer, uint32_t width, uint32_t height, dm_vulkan_swapchain* swapchain)
{
    return dm_vulkan_internal_create_swapchain(vulkan_renderer, width, height, swapchain);
}

bool dm_vulkan_recreate_swapchain(dm_vulkan_renderer* vulkan_renderer, uint32_t width, uint32_t height, dm_vulkan_swapchain* swapchain)
{
    dm_vulkan_destroy_swapchain(vulkan_renderer, swapchain);
    return dm_vulkan_internal_create_swapchain(vulkan_renderer, width, height, swapchain) ;
}


bool dm_vulkan_swapchain_next_image_index(dm_vulkan_renderer* vulkan_renderer, dm_vulkan_swapchain* swapchain, uint32_t timeout_ms, VkSemaphore image_available_semaphore, VkFence fence, uint32_t* image_index)
{
    VkResult result;
    
    DM_VULKAN_FUNC_CHECK(vkAcquireNextImageKHR(vulkan_renderer->device.logical, swapchain->handle, timeout_ms, image_available_semaphore, fence, image_index));
    
    switch(result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        dm_vulkan_recreate_swapchain(vulkan_renderer, vulkan_renderer->width, vulkan_renderer->height, swapchain);
        return false;
        
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
        break;
        
        default:
        DM_LOG_FATAL("Faied to acquire swapchain image");
        return false;
    }
    
    return true;
}

void dm_vulkan_swapchain_present(dm_vulkan_renderer* vulkan_renderer, dm_vulkan_swapchain* swapchain, VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t image_index)
{
    VkResult result;
    
    VkPresentInfoKHR present_info  = { 0 };
    present_info.sType             = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.swapchainCount    = 1;
    present_info.pSwapchains       = &swapchain->handle;
    present_info.pImageIndices     = &image_index;
    
    DM_VULKAN_FUNC_CHECK(vkQueuePresentKHR(present_queue, &present_info));
    switch(result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
        dm_vulkan_recreate_swapchain(vulkan_renderer, vulkan_renderer->width, vulkan_renderer->height, swapchain);
        break;
        
        case VK_SUCCESS:
        break;
        
        default:
        DM_LOG_FATAL("Failed to present swap chain image");
        return;
    }
}

/*************
VULKAN DEVICE
***************/
bool dm_vulkan_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface, dm_vulkan_device* device, dm_vulkan_physical_device_reqs* reqs, dm_vulkan_physical_device_queue_family_info* queue_info, dm_vulkan_swapchain_support_info* swapchain_support_info)
{
    VkResult result;
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(physical_device, &feats);
    
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    
    // quick checks
    if((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_SAMPLER_ANISOP) && !feats.samplerAnisotropy) return false;
    
    if((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_DISCRETE_GPU) && (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) return false;
    
    // the big check
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);
    VkQueueFamilyProperties* queue_families = dm_alloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
    
    uint8_t min_transfer_score = -1;
    uint8_t current_score;
    for(uint32_t i=0; i<queue_family_count; i++)
    {
        current_score = 0;
        
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_info->graphics_index = i;
            current_score++;
        }
        
        if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            queue_info->compute_index = i;
            current_score++;
        }
        
        if((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && (current_score <= min_transfer_score))
        {
            queue_info->transfer_index = i;
            min_transfer_score = current_score;
        }
        
        VkBool32 supports_present = VK_FALSE;
        DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_present));
        if(!DM_VULKAN_FUNC_SUCCESS) 
        {
            dm_free(queue_families);
            return false;
        }
        if(supports_present) queue_info->present_index = i;
        
        bool graphics_met = (reqs->flags & ~DM_VULKAN_PHYSICAL_DEVICE_FLAG_GRAPHICS) || ((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_GRAPHICS) && (queue_info->graphics_index != -1));
        bool compute_met = (reqs->flags & ~DM_VULKAN_PHYSICAL_DEVICE_FLAG_COMPUTE) || ((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_COMPUTE) && (queue_info->compute_index != -1));
        bool transfer_met = (reqs->flags & ~DM_VULKAN_PHYSICAL_DEVICE_FLAG_TRANSFER) || ((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_TRANSFER) && (queue_info->transfer_index != -1));
        bool present_met = (reqs->flags & ~DM_VULKAN_PHYSICAL_DEVICE_FLAG_PRESENT) || ((reqs->flags & DM_VULKAN_PHYSICAL_DEVICE_FLAG_PRESENT) && (queue_info->present_index != -1));
        
        // met all requirements
        if(!graphics_met || !compute_met || !transfer_met || !present_met) return false;
        
        if(!dm_vulkan_query_swapchain_support(physical_device, surface, swapchain_support_info)) return false;
        
        if(swapchain_support_info->format_count < 1 || swapchain_support_info->present_mode_count < 1)
        {
            dm_free(swapchain_support_info->formats);
            dm_free(swapchain_support_info->present_modes);
            return false;
        }
    }
    dm_free(queue_families);
    
    // extensions
    if(reqs->extension_count)
    {
        uint32_t available_count = 0;
        DM_VULKAN_FUNC_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0, &available_count, 0));
        if(!DM_VULKAN_FUNC_SUCCESS) return false;
        if(!available_count) return false;
        
        VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * available_count);
        DM_VULKAN_FUNC_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0, &available_count, available_extensions));
        if(!DM_VULKAN_FUNC_SUCCESS) return false;
        
        bool found;
        for(uint32_t i=0; i<reqs->extension_count; i++)
        {
            found = false;
            for(uint32_t j=0; j<available_count; j++)
            {
                if(strcmp(reqs->device_extension_names[i], available_extensions[j].extensionName)!=0) continue;
                
                found = true;
                break;
            }
            
            if(found) continue;
            
            dm_free(available_extensions);
            return false;
        }
        
        dm_free(available_extensions);
        
        DM_LOG_INFO("Device \'%s\' meets requirements", props.deviceName);
        
        DM_LOG_INFO("GPU driver version: %d.%d.%d", 
                    VK_VERSION_MAJOR(props.driverVersion),
                    VK_VERSION_MINOR(props.driverVersion),
                    VK_VERSION_PATCH(props.driverVersion));
        
        DM_LOG_INFO("Vulkan API version: %d.%d.%d", 
                    VK_VERSION_MAJOR(props.apiVersion),
                    VK_VERSION_MINOR(props.apiVersion),
                    VK_VERSION_PATCH(props.apiVersion));
        
        for(uint32_t i=0; i<mem_props.memoryHeapCount; i++)
        {
            float size = (float)mem_props.memoryHeaps[i].size * DM_MATH_1024_INV * DM_MATH_1024_INV * DM_MATH_1024_INV;
            if(mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) DM_LOG_INFO("Local GPU memory: %0.2f GiB", size);
            else DM_LOG_INFO("Shared system memory: %0.2f GiB", size);
        }
        
        device->physical = physical_device;
        device->properties = props;
        device->features = feats;
        device->mem_props = mem_props;
        
        device->graphics_index = queue_info->graphics_index;
        device->transfer_index = queue_info->transfer_index;
        device->compute_index = queue_info->compute_index;
        device->present_index = queue_info->present_index;
        
        return true;
    }
    
    return false;
}

bool dm_vulkan_create_device(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult result;
    
    vulkan_renderer->device.physical = VK_NULL_HANDLE;
    
    uint32_t device_count;
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, NULL);
    if(!device_count) { DM_LOG_FATAL("No GPUs with Vulkan support"); return false; }
    
    VkPhysicalDevice* physical_devices = dm_alloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, physical_devices);
    
    dm_vulkan_physical_device_reqs reqs = { 0 };
    reqs.flags = DM_VULKAN_PHYSICAL_DEVICE_FLAG_GRAPHICS | DM_VULKAN_PHYSICAL_DEVICE_FLAG_PRESENT | DM_VULKAN_PHYSICAL_DEVICE_FLAG_COMPUTE  | DM_VULKAN_PHYSICAL_DEVICE_FLAG_TRANSFER | DM_VULKAN_PHYSICAL_DEVICE_FLAG_SAMPLER_ANISOP | DM_VULKAN_PHYSICAL_DEVICE_FLAG_DISCRETE_GPU;
    
    reqs.device_extension_names[reqs.extension_count] = dm_alloc(strlen(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    strcpy(reqs.device_extension_names[reqs.extension_count++], VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    
    dm_vulkan_physical_device_queue_family_info queue_info = { 0 };
    
    for(uint32_t i=0; i<device_count; i++)
    {
        if(dm_vulkan_is_device_suitable(physical_devices[i], vulkan_renderer->surface, &vulkan_renderer->device, &reqs, &queue_info, &vulkan_renderer->device.swapchain_support_info)) break;
    }
    
    dm_free(physical_devices);
    if(vulkan_renderer->device.physical == VK_NULL_HANDLE) { DM_LOG_FATAL("No suitable GPU"); return false; }
    
    // logical device
    bool transfer_shares_graphics = vulkan_renderer->device.graphics_index == vulkan_renderer->device.transfer_index;
    bool present_shares_graphics = vulkan_renderer->device.graphics_index == vulkan_renderer->device.present_index;
    uint32_t index_count = 1;
    if(!transfer_shares_graphics) index_count++;
    if(!present_shares_graphics)  index_count++;
    
    uint32_t* indices = dm_alloc(sizeof(uint32_t) * index_count);
    uint32_t index = 0;
    
    indices[index++] = vulkan_renderer->device.graphics_index;
    if(!present_shares_graphics)  indices[index++] = vulkan_renderer->device.present_index;
    if(!transfer_shares_graphics) indices[index++] = vulkan_renderer->device.transfer_index;
    
    VkDeviceQueueCreateInfo* create_info = dm_alloc(sizeof(VkDeviceQueueCreateInfo) * index_count);
    for(uint32_t i=0; i<index_count; i++)
    {
        create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info[i].queueFamilyIndex = indices[i];
        create_info[i].queueCount = 1;
        
        float queue_priority = 1.0f;
        create_info[i].pQueuePriorities = &queue_priority;
    }
    
    VkPhysicalDeviceFeatures device_features = { 0 };
    device_features.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = create_info;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_names;
    
    DM_VULKAN_FUNC_CHECK(vkCreateDevice(vulkan_renderer->device.physical, &device_create_info, vulkan_renderer->allocator, &vulkan_renderer->device.logical));
    dm_free(indices);
    dm_free(create_info);
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_index, 0, &vulkan_renderer->device.graphics_queue); 
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.present_index, 0, &vulkan_renderer->device.present_queue); 
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.transfer_index, 0, &vulkan_renderer->device.transfer_queue); 
    
    return true;
}

bool dm_vulkan_device_detect_depth_buffer_range(dm_vulkan_device* device)
{
    VkResult result;
    
    static const uint32_t candidate_count = 3;
    static VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for(uint32_t i=0; i<candidate_count; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->physical, candidates[i], &props);
        
        if(((props.linearTilingFeatures & flags) != flags) && ((props.optimalTilingFeatures & flags) != flags)) continue;
        
        device->depth_format = candidates[i];
        return true;
    }
    
    return false;
}

uint32_t dm_vulkan_device_find_memory_index(uint32_t type_filter, uint32_t property_flags, VkPhysicalDevice device)
{
    if (type_filter & (1 << 1)) return -1;
    
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(device, &mem_props);
    
    for(uint32_t i=0; i<mem_props.memoryTypeCount; i++)
    {
        if((mem_props.memoryTypes[i].propertyFlags & property_flags) == property_flags) return i;
    }
    
    DM_LOG_WARN("Unable to find suitable Vulkan memory type");
    return -1;
}

/***************
VULKAN INSTANCE
*****************/
bool dm_vulkan_create_instance(dm_platform_data* platform_data, dm_vulkan_renderer* vulkan_renderer)
{
    vulkan_renderer->allocator = NULL;
    VkResult result;
    
    // extensions
    
#ifdef DM_DEBUG
    // available
    uint32_t extension_count;
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, available_extensions));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    DM_LOG_INFO("Available Vulkan extensions:");
    for(uint32_t i=0; i<extension_count; i++)
    {
        DM_LOG_INFO("     %s", available_extensions[i].extensionName);
    }
    
    dm_free(available_extensions);
#endif
    
    // required
    extension_count = 0;
    const char* required_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef DM_PLATFORM_WIN32
        "VK_KHR_win32_surface",
#elif defined(DM_PLATFORM_LINUX)
        "VK_KHR_xcb_surface",
#endif
#ifdef DM_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    
#ifdef DM_DEBUG
    DM_LOG_WARN("Required Vulkan extensions:");
    for(uint32_t i=0; i<DM_ARRAY_LEN(required_extensions); i++)
    {
        DM_LOG_WARN("     %s", required_extensions[i]);
    }
#endif
    
    // validation layers
#ifdef DM_DEBUG
    uint32_t available_count;
    
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceLayerProperties(&available_count, NULL));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    VkLayerProperties* available_layers = dm_alloc(sizeof(VkLayerProperties) * available_count);
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceLayerProperties(&available_count, available_layers));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    const char* required_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    for(uint32_t i=0; i<DM_ARRAY_LEN(required_layers); i++)
    {
        bool found = false;
        
        for(uint32_t j=0; j<available_count; j++)
        {
            DM_LOG_WARN("Searching for Vulkan layer: %s", available_layers[j].layerName);
            if(strcmp(required_layers[i], available_layers[j].layerName))
            {
                DM_LOG_INFO("found");
                found = true;
                break;
            }
        }
        
        if(found) continue;
        
        DM_LOG_FATAL("Could not find validation layer: %s", required_layers[i]);
        return false;
    }
    DM_LOG_INFO("All Vulkan validation layers found");
    dm_free(available_layers);
#endif
    
    // instance creation
    VkApplicationInfo app_info = { 0 };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = platform_data->window_data.title;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "DarkMatter";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = DM_ARRAY_LEN(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;
#ifdef DM_DEBUG
    create_info.enabledLayerCount   = DM_ARRAY_LEN(required_layers);
    create_info.ppEnabledLayerNames = required_layers;  
#endif
    
    DM_VULKAN_FUNC_CHECK(vkCreateInstance(&create_info, vulkan_renderer->allocator, &vulkan_renderer->instance));
    if(!DM_VULKAN_FUNC_SUCCESS) return false;
    
    // debugger
#ifdef DM_DEBUG
    uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { 0 };
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = dm_vulkan_debug_callback;
    
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkCreateDebugUtilsMessengerEXT");
    if(!func) 
    {
        DM_LOG_FATAL("Failed to create Vulkan debugger messenger");
        return false;
    }
    
    DM_VULKAN_FUNC_CHECK(func(vulkan_renderer->instance, &debug_create_info, vulkan_renderer->allocator,  &vulkan_renderer->debug_messenger));
#endif
    
    return true;
}

/**************
VULKAN BACKEND
****************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Vulkan backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    
    if(!dm_vulkan_create_instance(&context->platform_data, vulkan_renderer)) return false;
    if(!dm_platform_create_vulkan_surface(&context->platform_data, &vulkan_renderer->instance, &vulkan_renderer->surface)) return false;
    if(!dm_vulkan_create_device(vulkan_renderer)) return false;
    if(!dm_vulkan_create_swapchain(vulkan_renderer, context->platform_data.window_data.width, context->platform_data.window_data.height, &vulkan_renderer->swapchain)) return false;
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    DM_VULKAN_GET_RENDERER;
    
    dm_vulkan_destroy_swapchain(vulkan_renderer, &vulkan_renderer->swapchain);
    
    if(vulkan_renderer->device.swapchain_support_info.formats) dm_free(vulkan_renderer->device.swapchain_support_info.formats);
    if(vulkan_renderer->device.swapchain_support_info.present_modes) dm_free(vulkan_renderer->device.swapchain_support_info.present_modes);
    
    if(vulkan_renderer->device.logical) vkDestroyDevice(vulkan_renderer->device.logical, vulkan_renderer->allocator);
    
    vkDestroySurfaceKHR(vulkan_renderer->instance, vulkan_renderer->surface, 0);
#ifdef DM_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkDestroyDebugUtilsMessengerEXT");
    func(vulkan_renderer->instance, vulkan_renderer->debug_messenger, vulkan_renderer->allocator);
#endif
    vkDestroyInstance(vulkan_renderer->instance, NULL);
    
    dm_free(context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_end_frame(bool vsync, dm_context* context)
{
    return true;
}

/********
COMMANDS
**********/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
}

/************
VULKAN DEBUG
**************/
#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
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
    }
    
    return VK_FALSE;
}
#endif

#endif //DM_RENDERER_VULKAN_H
