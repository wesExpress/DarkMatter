#include "dm.h"

#ifdef DM_VULKAN

#include <string.h>

#ifdef DM_PLATFORM_WIN32
#include "platform/dm_platform_win32.h"
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#include "VulkanMemoryAllocator/include/vk_mem_alloc.h"

#define DM_VULKAN_API_VERSION VK_API_VERSION_1_3
#define DM_VULKAN_MAX_FRAMES_IN_FLIGHT DM_MAX_FRAMES_IN_FLIGHT 

typedef struct dm_vulkan_buffer_t
{
    VkBuffer      buffer;
    VmaAllocation memory; 
} dm_vulkan_buffer;

typedef struct dm_vulkan_image_t
{
    VkImage       image;
    VkImageView   view;
    VmaAllocation memory;
} dm_vulkan_image;

#define DM_VULKAN_MAX_SURFACE_FORMAT_COUNT 20
#define DM_VULKAN_MAX_PRESENT_MODE_COUNT   20
typedef struct dm_vulkan_swapchain_details_t
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR       formats[DM_VULKAN_MAX_SURFACE_FORMAT_COUNT];
    VkPresentModeKHR         present_modes[DM_VULKAN_MAX_PRESENT_MODE_COUNT];
    uint32_t                 format_count, present_mode_count;
} dm_vulkan_swapchain_details;

typedef struct dm_vulkan_swapchain_t
{
    VkSwapchainKHR swapchain;
    VkExtent2D     extents;
    VkFormat       format;

    VkImage     render_targets[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkImageView image_views[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    dm_vulkan_swapchain_details details;
} dm_vulkan_swapchain;

typedef struct dm_vulkan_queue_family_t
{
    VkQueue         queue;
    VkCommandPool   pool;
    VkCommandBuffer buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t        index;
} dm_vulkan_queue_family;

typedef struct dm_vulkan_device_properties_t
{
    VkPhysicalDeviceProperties                           properties;
    VkPhysicalDeviceProperties2                          properties2;
    VkPhysicalDeviceVulkan13Properties                   vulkan_1_3;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR   acceleration_structure;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR      raytracing_pipeline;
} dm_vulkan_device_properties;

typedef struct dm_vulkan_device_features_t
{
    VkPhysicalDeviceFeatures                         features; 
    VkPhysicalDeviceFeatures2                        features2;
    VkPhysicalDeviceVulkan13Features                 vulkan_1_3;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR    raytracing_pipeline;
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR   buffer_device_address;
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutable_descriptor_type;
} dm_vulkan_device_features;

typedef struct dm_vulkan_device_t
{
    VkPhysicalDevice physical;
    VkDevice         logical;

    dm_vulkan_device_properties      properties;
    dm_vulkan_device_features        features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    dm_vulkan_queue_family graphics_queue;
    dm_vulkan_queue_family compute_queue;
    dm_vulkan_queue_family transfer_queue;
} dm_vulkan_device;

typedef struct dm_vulkan_raster_pipeline_t
{
    VkPipeline pipeline;

    VkViewport viewport;
    VkRect2D   scissor;
} dm_vulkan_raster_pipeline;

typedef struct dm_vulkan_sbt_t 
{
    uint8_t index[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkDeviceAddress address[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    size_t  stride, size;
} dm_vulkan_sbt;

typedef struct dm_vulkan_raytracing_pipeline_t
{
    VkPipeline pipeline;

    dm_vulkan_sbt raygen_sbt, miss_sbt, hit_group_sbt;
} dm_vulkan_raytracing_pipeline;

typedef struct dm_vulkan_compute_pipeline_t
{
    VkPipeline       pipeline;
} dm_vulkan_compute_pipeline;

typedef struct dm_vulkan_vertex_buffer_t 
{
    uint32_t host[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t device[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
} dm_vulkan_vertex_buffer;

typedef struct dm_vulkan_index_buffer_t
{
    uint32_t host[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t device[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    VkIndexType index_type;
} dm_vulkan_index_buffer;

typedef struct dm_vulkan_constant_buffer_t
{
    uint32_t         buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    void*            mapped_memory[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    size_t           size;
} dm_vulkan_constant_buffer;

typedef struct dm_vulkan_storage_buffer_t
{
    uint32_t host[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t device[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    size_t   size;
} dm_vulkan_storage_buffer;

typedef struct dm_vulkan_texture_t
{
    uint32_t staging_buffer[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    uint32_t image[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkFormat format;
} dm_vulkan_texture;

typedef struct dm_vulkan_sampler_t
{
    VkSampler sampler;
} dm_vulkan_sampler;

typedef struct dm_vulkan_as_t
{
    uint32_t                   result;
    uint32_t                   scratch;
    VkAccelerationStructureKHR as;
    size_t                     result_size, scratch_size;
} dm_vulkan_as;

typedef struct dm_vulkan_blas_t
{
    dm_vulkan_as as[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
} dm_vulkan_blas;

typedef struct dm_vulkan_tlas_t
{
    dm_vulkan_as as[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    dm_resource_handle instance_buffer;
} dm_vulkan_tlas;

#define DM_VULKAN_MAX_DESCRIPTORS               100
#define DM_VULKAN_MAX_DESCRIPTOR_TYPES          10

#define DM_VULKAN_MAX_RASTER_PIPES  10
#define DM_VULKAN_MAX_RT_PIPES      10
#define DM_VULKAN_MAX_COMPUTE_PIPES 10
#define DM_VULKAN_MAX_VBS           10
#define DM_VULKAN_MAX_IBS           10
#define DM_VULKAN_MAX_CBS           10
#define DM_VULKAN_MAX_SBS           10
#define DM_VULKAN_MAX_TEXTURES      10
#define DM_VULKAN_MAX_SAMPLERS      10
#define DM_VULKAN_MAX_BLAS          10
#define DM_VULKAN_MAX_TLAS          10

#define DM_VULKAN_MAX_BUFFERS DM_VULKAN_MAX_DESCRIPTORS * DM_VULKAN_MAX_FRAMES_IN_FLIGHT * 10
#define DM_VULKAN_MAX_IMAGES  DM_VULKAN_MAX_DESCRIPTORS * DM_VULKAN_MAX_FRAMES_IN_FLIGHT
typedef struct dm_vulkan_renderer_t
{
    VkInstance   instance;
    VmaAllocator allocator;
    VkSurfaceKHR surface;

    dm_vulkan_device    device;
    dm_vulkan_swapchain swapchain;

    dm_vulkan_image depth_stencil;

    VkSemaphore wait_semaphores[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore signal_semaphores[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkFence     front_fences[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkFence     back_fences[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    uint32_t current_frame, image_index;

    VkDescriptorSetLayout resource_bindless_layout, sampler_bindless_layout;
    VkPipelineLayout      bindless_pipeline_layout;

    VkDescriptorPool      resource_bindless_pool[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorPool      sampler_bindless_pool[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSet resource_bindless_set[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet sampler_bindless_set[DM_VULKAN_MAX_FRAMES_IN_FLIGHT];

    uint32_t resource_heap_count, sampler_heap_count;

    dm_vulkan_buffer buffers[DM_VULKAN_MAX_BUFFERS];
    uint32_t         buffer_count;

    dm_vulkan_image images[DM_VULKAN_MAX_IMAGES];
    uint32_t        image_count;

    dm_vulkan_raster_pipeline raster_pipes[DM_VULKAN_MAX_RASTER_PIPES];
    uint32_t                  raster_pipe_count;

    dm_vulkan_raytracing_pipeline rt_pipes[DM_VULKAN_MAX_RT_PIPES];
    uint32_t                      rt_pipe_count;

    dm_vulkan_compute_pipeline compute_pipes[DM_VULKAN_MAX_COMPUTE_PIPES];
    uint32_t                   compute_pipe_count;

    dm_vulkan_vertex_buffer vertex_buffers[DM_VULKAN_MAX_VBS];
    uint32_t                vb_count;

    dm_vulkan_index_buffer index_buffers[DM_VULKAN_MAX_IBS];
    uint32_t               ib_count;

    dm_vulkan_constant_buffer constant_buffers[DM_VULKAN_MAX_CBS];
    uint32_t                  cb_count;

    dm_vulkan_storage_buffer storage_buffers[DM_VULKAN_MAX_SBS];
    uint32_t                 sb_count;

    dm_vulkan_texture textures[DM_VULKAN_MAX_TEXTURES];
    uint32_t          texture_count;

    dm_vulkan_sampler samplers[DM_VULKAN_MAX_SAMPLERS];
    uint32_t          sampler_count;

    dm_vulkan_blas blas[DM_VULKAN_MAX_BLAS];
    uint32_t       blas_count;

    dm_vulkan_tlas tlas[DM_VULKAN_MAX_TLAS];
    uint32_t       tlas_count;

    dm_resource_handle bound_pipeline;
    dm_pipeline_type   bound_pipeline_type;

#ifdef DM_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} dm_vulkan_renderer;

bool dm_vulkan_decode_vr(VkResult vr);

// boilerplate declerations
bool dm_vulkan_create_instance(dm_context* context);
bool dm_vulkan_create_surface(dm_context* context);
bool dm_vulkan_create_device(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_allocator(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_swapchain(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_render_targets(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_depth_stencil_target(const uint32_t width, const uint32_t height, dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_command_pools_and_buffers(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_synchronization_objects(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_bindless_layout(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_bindless_descriptor_pool(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_bindless_descriptor_sets(dm_vulkan_renderer* vulkan_renderer);
bool dm_vulkan_create_bindless_pipeline_layout(dm_vulkan_renderer* vulkan_renderer);

dm_vulkan_swapchain_details dm_vulkan_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
#endif

#define DM_VULKAN_GET_RENDERER dm_vulkan_renderer* vulkan_renderer = renderer->internal_renderer

/***********
 * BACKEND *
 ***********/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_INFO("Initializing Vulkan render backend...");

    context->renderer.internal_renderer = dm_alloc(sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkResult vr;

    // === Volk ===
    vr = volkInitialize();
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("Initializing Volk failed"); return false; }
    
    // boilerplate hidden away
    if(!dm_vulkan_create_instance(context)) { DM_LOG_FATAL("Create Vulkan instance failed"); return false; }
    if(!dm_vulkan_create_surface(context)) { DM_LOG_FATAL("Could not create Vulkan surface"); return false; }
    if(!dm_vulkan_create_device(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan device"); return false; }
    if(!dm_vulkan_create_allocator(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan allocator"); return false; }
    if(!dm_vulkan_create_swapchain(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan swapchain"); return false; }
    if(!dm_vulkan_create_render_targets(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan render targets"); return false; }
    if(!dm_vulkan_create_depth_stencil_target(context->renderer.width, context->renderer.height, vulkan_renderer)) { DM_LOG_FATAL("Could not create depth stencil target"); return false; }
    if(!dm_vulkan_create_command_pools_and_buffers(vulkan_renderer)) { DM_LOG_FATAL("Could not create vulkan command pools"); return false; }
    if(!dm_vulkan_create_synchronization_objects(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan synchronization objects"); return false; };
    if(!dm_vulkan_create_bindless_layout(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan bindless descriptor set layout"); return false; }
    if(!dm_vulkan_create_bindless_descriptor_pool(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan bindless descriptor pools"); return false; }
    if(!dm_vulkan_create_bindless_descriptor_sets(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan bindless descriptor sets"); return false; }
    if(!dm_vulkan_create_bindless_pipeline_layout(vulkan_renderer)) { DM_LOG_FATAL("Could not create Vulkan bindless pipeline layout"); return false; }

    return true;
}

bool dm_renderer_backend_finish_init(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkResult vr;

    vkEndCommandBuffer(vulkan_renderer->device.graphics_queue.buffer[0]);
    vkEndCommandBuffer(vulkan_renderer->device.transfer_queue.buffer[0]);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pCommandBuffers    = vulkan_renderer->device.graphics_queue.buffer;
    submit_info.commandBufferCount = 1;

    vr = vkQueueSubmit(vulkan_renderer->device.graphics_queue.queue, 1, &submit_info, NULL);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueSubmit failed"); return false; }
    vr = vkQueueWaitIdle(vulkan_renderer->device.graphics_queue.queue);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueWaitIdle failed"); return false; }

    submit_info.pCommandBuffers = vulkan_renderer->device.transfer_queue.buffer;

    vkQueueSubmit(vulkan_renderer->device.transfer_queue.queue, 1, &submit_info, NULL);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueSubmit failed"); return false; }
    vr = vkQueueWaitIdle(vulkan_renderer->device.transfer_queue.queue);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueWaitIdle failed"); return false; }

    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkDevice     device    = vulkan_renderer->device.logical;
    VmaAllocator allocator = vulkan_renderer->allocator;

    vkWaitForFences(device, 1, &vulkan_renderer->front_fences[vulkan_renderer->current_frame], VK_TRUE, INFINITE);

    // === resources ===
    for(uint32_t i=0; i<vulkan_renderer->raster_pipe_count; i++)
    {
        vkDestroyPipeline(device, vulkan_renderer->raster_pipes[i].pipeline, NULL);
    }

    for(uint32_t i=0; i<vulkan_renderer->rt_pipe_count; i++)
    {
        vkDestroyPipeline(device, vulkan_renderer->rt_pipes[i].pipeline, NULL);
    }

    for(uint32_t i=0; i<vulkan_renderer->compute_pipe_count; i++)
    {
        vkDestroyPipeline(device, vulkan_renderer->compute_pipes[i].pipeline, NULL);
    }

    for(uint32_t i=0; i<vulkan_renderer->cb_count; i++)
    {
        for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
        {
            vmaUnmapMemory(allocator, vulkan_renderer->buffers[vulkan_renderer->constant_buffers[i].buffer[j]].memory);
        }
    }

    for(uint32_t i=0; i<vulkan_renderer->sampler_count; i++)
    {
        vkDestroySampler(device, vulkan_renderer->samplers[i].sampler, NULL);
    }

    for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
    {
        for(uint32_t i=0; i<vulkan_renderer->tlas_count; i++)
        {
            vkDestroyAccelerationStructureKHR(device, vulkan_renderer->tlas[i].as[j].as, NULL);
        }
        for(uint32_t i=0; i<vulkan_renderer->blas_count; i++)
        {
            vkDestroyAccelerationStructureKHR(device, vulkan_renderer->blas[i].as[j].as, NULL);
        }
    }

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyDescriptorPool(device, vulkan_renderer->resource_bindless_pool[i], NULL);
        vkDestroyDescriptorPool(device, vulkan_renderer->sampler_bindless_pool[i], NULL);
    }

    // throws a validation error for not deleting device memory?
    // not sure how
    for(uint32_t i=0; i<vulkan_renderer->buffer_count; i++)
    {
        vmaDestroyBuffer(allocator, vulkan_renderer->buffers[i].buffer, vulkan_renderer->buffers[i].memory);
    }

    for(uint32_t i=0; i<vulkan_renderer->image_count; i++)
    {
        vmaDestroyImage(allocator, vulkan_renderer->images[i].image, vulkan_renderer->images[i].memory);
        vkDestroyImageView(device, vulkan_renderer->images[i].view, NULL);
    }

    // === backend stuff ===
    vkDestroyPipelineLayout(device, vulkan_renderer->bindless_pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device, vulkan_renderer->sampler_bindless_layout, NULL);
    vkDestroyDescriptorSetLayout(device, vulkan_renderer->resource_bindless_layout, NULL);

    vkDestroyCommandPool(device, vulkan_renderer->device.graphics_queue.pool, NULL);
    vkDestroyCommandPool(device, vulkan_renderer->device.compute_queue.pool, NULL);
    vkDestroyCommandPool(device, vulkan_renderer->device.transfer_queue.pool, NULL);

    // throws a validation error for not deleting device memory?
    // not sure how
    vmaDestroyImage(allocator, vulkan_renderer->depth_stencil.image, vulkan_renderer->depth_stencil.memory);
    vkDestroyImageView(device, vulkan_renderer->depth_stencil.view, NULL);

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(device, vulkan_renderer->front_fences[i], NULL);
        vkDestroySemaphore(device, vulkan_renderer->wait_semaphores[i], NULL);
        vkDestroySemaphore(device, vulkan_renderer->signal_semaphores[i], NULL);
        vkDestroyImageView(device, vulkan_renderer->swapchain.image_views[i], NULL);
    }

    vkDestroySwapchainKHR(device, vulkan_renderer->swapchain.swapchain, NULL);
    vkDestroySurfaceKHR(vulkan_renderer->instance, vulkan_renderer->surface, NULL);
    vkDestroyDevice(device, NULL);

#ifdef DM_DEBUG
    vkDestroyDebugUtilsMessengerEXT(vulkan_renderer->instance, vulkan_renderer->debug_messenger, NULL);
#endif
    vkDestroyInstance(vulkan_renderer->instance, NULL);
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const VkDevice device = vulkan_renderer->device.logical;

    vkDeviceWaitIdle(device);

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyImageView(device, vulkan_renderer->swapchain.image_views[i], NULL);
    }

    vkDestroySwapchainKHR(device, vulkan_renderer->swapchain.swapchain, NULL);

    if(!dm_vulkan_create_swapchain(vulkan_renderer)) { DM_LOG_FATAL("Recreating swapchain failed"); return false; }
    if(!dm_vulkan_create_render_targets(vulkan_renderer)) { DM_LOG_FATAL("Recreating rendertargets failed"); return false;}

    // depth stencil target
    vmaDestroyImage(vulkan_renderer->allocator, vulkan_renderer->depth_stencil.image, vulkan_renderer->depth_stencil.memory);
    vkDestroyImageView(vulkan_renderer->device.logical, vulkan_renderer->depth_stencil.view, NULL);

    if(!dm_vulkan_create_depth_stencil_target(width, height, vulkan_renderer)) { DM_LOG_FATAL("Recreating depthstencil target failed"); return false; }

    return true;
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    uint32_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkWaitForFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->front_fences[current_frame], VK_TRUE, INFINITE); 

    vr = vkResetCommandBuffer(cmd_buffer, 0);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkResetCommandBuffer failed"); return false; }

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vr = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkBeginCommandBuffer failed"); return false; }

    vr = vkAcquireNextImageKHR(vulkan_renderer->device.logical, vulkan_renderer->swapchain.swapchain, UINT64_MAX, vulkan_renderer->wait_semaphores[current_frame], VK_NULL_HANDLE, &vulkan_renderer->image_index);
    if(vr == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if(!dm_renderer_backend_resize(renderer->width, renderer->height, renderer)) { return false; }
    }
    else if(vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
    {
        dm_vulkan_decode_vr(vr);
        DM_LOG_FATAL("vkAcquireNextImageKHR failed");
        return false;
    }
    if(vulkan_renderer->back_fences[vulkan_renderer->image_index] != VK_NULL_HANDLE)
    {
        vkWaitForFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->back_fences[vulkan_renderer->image_index], VK_TRUE, INFINITE);
    }
    vulkan_renderer->back_fences[vulkan_renderer->image_index] = vulkan_renderer->front_fences[vulkan_renderer->current_frame];
    vkResetFences(vulkan_renderer->device.logical, 1, &vulkan_renderer->front_fences[vulkan_renderer->current_frame]);

    VkImageSubresourceRange range = { 0 };
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    // must transition swapchain images and depth image
    VkImageMemoryBarrier swapchain_barrier = { 0 };
    swapchain_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapchain_barrier.image               = vulkan_renderer->swapchain.render_targets[vulkan_renderer->image_index];
    swapchain_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapchain_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapchain_barrier.srcAccessMask       = 0;
    swapchain_barrier.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    swapchain_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    swapchain_barrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swapchain_barrier.subresourceRange    = range;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  0,0,NULL,0,NULL, 1, &swapchain_barrier);

    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    VkImageMemoryBarrier depth_barrier = { 0 };
    depth_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depth_barrier.image               = vulkan_renderer->depth_stencil.image;
    depth_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_barrier.srcAccessMask       = 0;
    depth_barrier.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depth_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_barrier.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_barrier.subresourceRange    = range;

    VkPipelineStageFlags depth_stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; 

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, depth_stage_flags, 0,0,NULL,0,NULL, 1, &depth_barrier);

    VkDescriptorSet sets[] = { vulkan_renderer->resource_bindless_set[current_frame], vulkan_renderer->sampler_bindless_set[current_frame] };
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_renderer->bindless_pipeline_layout, 0, _countof(sets), sets, 0, NULL);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan_renderer->bindless_pipeline_layout, 0, _countof(sets), sets, 0, NULL);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vulkan_renderer->bindless_pipeline_layout, 0, _countof(sets), sets, 0, NULL);

    // misc
    vulkan_renderer->bound_pipeline_type = DM_PIPELINE_TYPE_UNKNOWN;

    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    VkResult vr;

    uint32_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    // transition swapchain image
    VkImageSubresourceRange range = { 0 };
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer = 0;
    range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier swapchain_barrier = { 0 };
    swapchain_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapchain_barrier.image               = vulkan_renderer->swapchain.render_targets[vulkan_renderer->image_index];
    swapchain_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapchain_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapchain_barrier.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    swapchain_barrier.dstAccessMask       = 0;
    swapchain_barrier.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swapchain_barrier.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapchain_barrier.subresourceRange    = range;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  0,0,NULL,0,NULL, 1, &swapchain_barrier);

    vr = vkEndCommandBuffer(cmd_buffer);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkEndCommandBuffer failed"); return false; }

    VkPipelineStageFlags pipeline_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pCommandBuffers      = &cmd_buffer;
    submit_info.commandBufferCount   = 1;
    submit_info.pWaitSemaphores      = &vulkan_renderer->wait_semaphores[current_frame];
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pSignalSemaphores    = &vulkan_renderer->signal_semaphores[current_frame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pWaitDstStageMask    = &pipeline_stage;

    vr = vkQueueSubmit(vulkan_renderer->device.graphics_queue.queue, 1, &submit_info, vulkan_renderer->front_fences[current_frame]);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueSubmit failed"); return false; }

    VkSwapchainKHR swapchains[] = { vulkan_renderer->swapchain.swapchain };

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pWaitSemaphores    = &vulkan_renderer->signal_semaphores[current_frame];
    present_info.waitSemaphoreCount = 1;
    present_info.pSwapchains        = swapchains;
    present_info.swapchainCount     = 1;
    present_info.pImageIndices      = &vulkan_renderer->image_index;

    vr = vkQueuePresentKHR(vulkan_renderer->device.graphics_queue.queue, &present_info);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueuePresentKHR failed"); return false; }

    if(vr==VK_ERROR_OUT_OF_DATE_KHR || vr==VK_SUBOPTIMAL_KHR)
    {
        if(!dm_renderer_backend_resize(context->renderer.width, context->renderer.height, &context->renderer)) return false;
    }
    else if(!dm_vulkan_decode_vr(vr)) return false;

#ifdef DM_DEBUG
    // this fixes memory leak from present, not sure why, but is taken from this github post (from 2018!!!!!!!!!)
    // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2468
    vr = vkQueueWaitIdle(vulkan_renderer->device.graphics_queue.queue);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkQueueWaitIdle failed"); return false; }
#endif

    vulkan_renderer->current_frame = (vulkan_renderer->current_frame + 1) % DM_VULKAN_MAX_FRAMES_IN_FLIGHT;

    return true;
}

/********************
 * RENDER RESOURCES *
 ********************/
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

bool dm_renderer_backend_create_raster_pipeline(dm_raster_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
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
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = { 0 };
    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    // === input assembler ===
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch(desc.input_assembler.topology)
    {
        case DM_INPUT_TOPOLOGY_TRIANGLE_LIST:
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;

        case DM_INPUT_TOPOLOGY_LINE_LIST:
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;

        default:
        DM_LOG_ERROR("Unknown topology. Assuming VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST");
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    }

    // === vertex input ===
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
                d.offset += 8;
                break;

                default:
                DM_LOG_ERROR("Unknown input format. Assuming VK_FORMAT_R32G32B32_SFLOAT");
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_3:
                input->format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset += 12;
                break;

                case DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4:
                case DM_INPUT_ELEMENT_FORMAT_FLOAT_4:
                input->format = VK_FORMAT_R32G32B32A32_SFLOAT;
                d.offset += 16;
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

    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions    = bind_descriptions;

    vertex_input_create_info.vertexAttributeDescriptionCount = element_count;
    vertex_input_create_info.pVertexAttributeDescriptions    = vertex_attr_create_info;

    // === shaders ===
    if(!dm_vulkan_create_shader_module(desc.rasterizer.vertex_shader_desc.path, &vs, vulkan_renderer->device.logical)) return false;
    if(!dm_vulkan_create_shader_module(desc.rasterizer.pixel_shader_desc.path, &ps, vulkan_renderer->device.logical))  return false;

    vs_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_create_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vs_create_info.module = vs;
    vs_create_info.pName  = "main";

    ps_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_create_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ps_create_info.module = ps;
    ps_create_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vs_create_info, ps_create_info };

    // === depth stencil state ===
    // TODO: needs to be configurable
    depth_stencil_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; 
    depth_stencil_state_info.depthTestEnable   = desc.depth_stencil.depth;
    depth_stencil_state_info.depthWriteEnable  = desc.depth_stencil.depth;
    depth_stencil_state_info.stencilTestEnable = desc.depth_stencil.stencil;
    depth_stencil_state_info.depthCompareOp    = VK_COMPARE_OP_LESS;

    // === blend === 
    // TODO: needs to be configurable
    blend_state.blendEnable = VK_TRUE;

    blend_state.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.colorBlendOp        = VK_BLEND_OP_ADD;
    blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.alphaBlendOp        = VK_BLEND_OP_ADD;

    blend_create_info.sType   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create_info.logicOp = VK_LOGIC_OP_COPY;

    blend_create_info.attachmentCount = 1;
    blend_create_info.pAttachments    = &blend_state;

    // === multisampling ===
    multi_create_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_create_info.sampleShadingEnable  = VK_FALSE;
    multi_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // === rasterizer ===
    raster_create_info.sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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

    // === viewport and scissor ===
    viewport_state_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount  = 1;
    
    switch(desc.viewport.type)
    {
        case DM_VIEWPORT_TYPE_DEFAULT:
        default:
        pipeline.viewport.x        = 0;
        pipeline.viewport.y        = (float)renderer->height;
        pipeline.viewport.width    = (float)renderer->width;
        pipeline.viewport.height   = -(float)renderer->height;
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

    // === dynamic states ===
    dynamic_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates    = dynamic_states;

    // === pipeline object ===
    VkPipelineRenderingCreateInfo render_create_info = { 0 };
    render_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    render_create_info.colorAttachmentCount    = 1;
    render_create_info.pColorAttachmentFormats = &vulkan_renderer->swapchain.format;
    render_create_info.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkGraphicsPipelineCreateInfo create_info = { 0 };
    create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount          = _countof(shader_stages);
    create_info.pStages             = shader_stages;
    create_info.pVertexInputState   = &vertex_input_create_info;
    create_info.pInputAssemblyState = &input_assembly_create_info;
    create_info.layout              = vulkan_renderer->bindless_pipeline_layout;
    create_info.pColorBlendState    = &blend_create_info;
    create_info.pMultisampleState   = &multi_create_info;
    create_info.pRasterizationState = &raster_create_info;
    create_info.pViewportState      = &viewport_state_info;
    create_info.pDynamicState       = &dynamic_state_info;
    create_info.pDepthStencilState  = &depth_stencil_state_info;
    create_info.pNext               = &render_create_info;

    vr = vkCreateGraphicsPipelines(vulkan_renderer->device.logical, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.pipeline);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateGraphicsPipeline failed"); return false; }

    // === cleanup ===
    vkDestroyShaderModule(vulkan_renderer->device.logical, vs, NULL);
    vkDestroyShaderModule(vulkan_renderer->device.logical, ps, NULL);

    //
    dm_memcpy(vulkan_renderer->raster_pipes + vulkan_renderer->raster_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = vulkan_renderer->raster_pipe_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_copy_memory(VmaAllocation allocation, void* data, const size_t size, dm_vulkan_renderer* vulkan_renderer)
{
    void* temp = NULL;

    vmaMapMemory(vulkan_renderer->allocator, allocation, &temp); 
    if(!temp)
    {
        DM_LOG_FATAL("vkMapMemory failed");
        return false;
    }
    dm_memcpy(temp, data, size);
    vmaUnmapMemory(vulkan_renderer->allocator, allocation);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_buffer(const size_t size, VkBufferUsageFlagBits buffer_type, VkSharingMode sharing_mode, dm_vulkan_buffer* buffer, dm_vulkan_renderer* vulkan_renderer)
{
    uint32_t family_indices[2] = { 0 };

    if(sharing_mode == VK_SHARING_MODE_CONCURRENT)
    {
        family_indices[0] = vulkan_renderer->device.graphics_queue.index;
        family_indices[1] = vulkan_renderer->device.transfer_queue.index;
    }

    VkBufferCreateInfo create_info    = { 0 };
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size                  = size;
    create_info.usage                 = buffer_type;
    create_info.sharingMode           = sharing_mode;
    create_info.pQueueFamilyIndices   = family_indices;
    create_info.queueFamilyIndexCount = sharing_mode==VK_SHARING_MODE_CONCURRENT ? _countof(family_indices) : 0;

    VmaAllocationCreateInfo allocation_create_info = { 0 };
    allocation_create_info.usage  = VMA_MEMORY_USAGE_AUTO;
    if(buffer_type && VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    {
        allocation_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VkResult vr = vmaCreateBuffer(vulkan_renderer->allocator, &create_info, &allocation_create_info, &buffer->buffer, &buffer->memory, NULL); 

    if(!dm_vulkan_decode_vr(vr) || !buffer) { DM_LOG_FATAL("vmaCreateBuffer failed"); return false;}

    return true;
}

bool dm_renderer_backend_create_vertex_buffer(dm_vertex_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_vertex_buffer buffer = { 0 }; 

    VkBufferUsageFlagBits host_buffer_usage   = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits device_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
    
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* host_buffer   = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];
        dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count+1];

        // host buffer
        if(!dm_vulkan_create_buffer(desc.size, host_buffer_usage, sharing_mode, host_buffer, vulkan_renderer)) return false;
        // device buffer
        if(!dm_vulkan_create_buffer(desc.size, device_buffer_usage, sharing_mode, device_buffer, vulkan_renderer)) return false;

        buffer.host[i] = vulkan_renderer->buffer_count++;
        buffer.device[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(!desc.data) continue;

        if(!dm_vulkan_copy_memory(host_buffer->memory, desc.data, desc.size, vulkan_renderer)) return false;

        VkBufferCopy copy_region = { 0 };
        copy_region.size = desc.size;

        vkCmdCopyBuffer(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], host_buffer->buffer, device_buffer->buffer, 1, &copy_region);
    }

    //
    dm_memcpy(vulkan_renderer->vertex_buffers + vulkan_renderer->vb_count, &buffer, sizeof(buffer));
    handle->index = vulkan_renderer->vb_count++;

    return true;
}

bool dm_renderer_backend_create_index_buffer(dm_index_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
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
    VkBufferUsageFlagBits device_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
    
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* host_buffer   = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];
        dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count+1];

        // host buffer
        if(!dm_vulkan_create_buffer(desc.size, host_buffer_usage, sharing_mode, host_buffer, vulkan_renderer)) return false;

        // device buffer
        if(!dm_vulkan_create_buffer(desc.size, device_buffer_usage, sharing_mode, device_buffer, vulkan_renderer)) return false;

        buffer.host[i]   = vulkan_renderer->buffer_count++;
        buffer.device[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(!desc.data) continue;

        if(!dm_vulkan_copy_memory(host_buffer->memory, desc.data, desc.size, vulkan_renderer)) return false;

        VkBufferCopy copy_region = { 0 };
        copy_region.size = desc.size;

        vkCmdCopyBuffer(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], host_buffer->buffer, device_buffer->buffer, 1, &copy_region);
    }

    //
    dm_memcpy(vulkan_renderer->index_buffers + vulkan_renderer->ib_count, &buffer, sizeof(buffer));
    handle->index = vulkan_renderer->ib_count++;
    return true;
}

bool dm_renderer_backend_create_constant_buffer(dm_constant_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_constant_buffer buffer = { 0 };
    
    VkBufferUsageFlagBits buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkSharingMode         sharing_mode = VK_SHARING_MODE_CONCURRENT;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* host_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];

        if(!dm_vulkan_create_buffer(desc.size, buffer_usage, sharing_mode, host_buffer, vulkan_renderer)) return false;
        buffer.buffer[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(desc.data)
        {
            if(!dm_vulkan_copy_memory(host_buffer->memory, desc.data, desc.size, vulkan_renderer)) return false;
        }

        vmaMapMemory(vulkan_renderer->allocator, host_buffer->memory, &buffer.mapped_memory[i]);
        if(!buffer.mapped_memory[i])
        {
            DM_LOG_FATAL("vkMapMemory failed");
            return false;
        }

        // descriptors
        VkDescriptorBufferInfo buffer_info = { 0 };
        buffer_info.buffer = host_buffer->buffer;
        buffer_info.range  = desc.size;

        VkWriteDescriptorSet write_set = { 0 };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.descriptorCount = 1;
        write_set.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_set.dstSet          = vulkan_renderer->resource_bindless_set[i];
        write_set.dstArrayElement = vulkan_renderer->resource_heap_count;
        write_set.dstBinding      = 0;
        write_set.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &write_set, 0, NULL);
    }

    buffer.size = desc.size;

    //
    dm_memcpy(vulkan_renderer->constant_buffers + vulkan_renderer->cb_count, &buffer, sizeof(buffer));
    handle->index = vulkan_renderer->cb_count++;
    handle->descriptor_index = vulkan_renderer->resource_heap_count++;

    return true;
}

bool dm_renderer_backend_create_storage_buffer(dm_storage_buffer_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    VkBufferUsageFlagBits host_buffer_usage   = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits device_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;

    dm_vulkan_storage_buffer buffer = { 0 };
    buffer.size = desc.size;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* host_buffer   = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];
        dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count+1];

        if(!dm_vulkan_create_buffer(desc.size, host_buffer_usage, sharing_mode, host_buffer, vulkan_renderer)) return false;
        if(!dm_vulkan_create_buffer(desc.size, device_buffer_usage, sharing_mode, device_buffer, vulkan_renderer)) return false; 

        buffer.host[i]   = vulkan_renderer->buffer_count++;
        buffer.device[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(desc.data)
        {
            if(!dm_vulkan_copy_memory(host_buffer->memory, desc.data, desc.size, vulkan_renderer)) return false;

            VkBufferCopy copy_region = { 0 };
            copy_region.size = desc.size;

            vkCmdCopyBuffer(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], host_buffer->buffer, device_buffer->buffer, 1, &copy_region);
        }

        // descriptors
        VkDescriptorBufferInfo buffer_info = { 0 };
        buffer_info.buffer = device_buffer->buffer;
        buffer_info.range  = desc.size;

        VkWriteDescriptorSet write_set = { 0 };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.descriptorCount = 1;
        write_set.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_set.dstSet          = vulkan_renderer->resource_bindless_set[i];
        write_set.dstArrayElement = vulkan_renderer->resource_heap_count;
        write_set.dstBinding      = 0;
        write_set.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &write_set, 0, NULL);
    }

    //
    dm_memcpy(vulkan_renderer->storage_buffers + vulkan_renderer->sb_count, &buffer, sizeof(buffer));
    handle->index = vulkan_renderer->sb_count++;
    handle->descriptor_index = vulkan_renderer->resource_heap_count++;

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_copy_buffer_to_image(dm_vulkan_buffer* buffer, dm_vulkan_image* image, VkBufferImageCopy copy, bool sampled, dm_vulkan_renderer* vulkan_renderer)
{
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[0];

    // transition image layout
    VkImageMemoryBarrier barrier = { 0 };
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.image                       = image->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL, 1, &barrier);

    // copy over data
    vkCmdCopyBufferToImage(cmd_buffer, buffer->buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    // transition again
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = sampled ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    VkPipelineStageFlagBits flags = sampled ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,flags,0,0,NULL,0,NULL, 1, &barrier);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_texture(dm_texture_desc desc, VkImageUsageFlagBits usage_flags, dm_vulkan_texture* texture, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkBufferUsageFlagBits buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkSharingMode         sharing_mode = VK_SHARING_MODE_CONCURRENT;

    const size_t size = desc.height * desc.width * desc.n_channels;

    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        texture->format = VK_FORMAT_R8G8B8A8_UINT;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        texture->format = VK_FORMAT_R8G8B8A8_UNORM;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_3:
        texture->format = VK_FORMAT_R32G32B32_SFLOAT;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        texture->format = VK_FORMAT_R32G32B32A32_SFLOAT;
        break;

        default:
        DM_LOG_FATAL("Unknown or unsupported texture format");
        return false;
    }

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width  = desc.width;
    image_create_info.extent.height = desc.height;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.usage         = usage_flags;
    image_create_info.format        = texture->format;

    VmaAllocationCreateInfo allocation_create_info = { 0 };
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* staging_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];

        if(!dm_vulkan_create_buffer(size, buffer_usage, sharing_mode, staging_buffer, vulkan_renderer)) return false;
        texture->staging_buffer[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(desc.data) 
        {
            if(!dm_vulkan_copy_memory(staging_buffer->memory, desc.data, size, vulkan_renderer)) return false;
        }

        // sampled image
        dm_vulkan_image* image = &vulkan_renderer->images[vulkan_renderer->image_count];

        vr = vmaCreateImage(vulkan_renderer->allocator, &image_create_info, &allocation_create_info, &image->image, &image->memory, NULL);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vmaCreateImage failed");
            return false;
        }
        texture->image[i] = vulkan_renderer->image_count++;

        if(desc.data)
        {
            VkBufferImageCopy copy = { 0 };
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent.width           = desc.width;
            copy.imageExtent.height          = desc.height;
            copy.imageExtent.depth           = 1;

            if(!dm_vulkan_copy_buffer_to_image(staging_buffer, image, copy, true, vulkan_renderer)) return false;
        }

        // view(s)
        VkImageViewCreateInfo view_info = { 0 };
        view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image    = image->image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format   = texture->format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        vr = vkCreateImageView(vulkan_renderer->device.logical, &view_info, NULL, &image->view);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateImageView failed");
            return false;
        }
    }

    dm_memcpy(vulkan_renderer->textures + vulkan_renderer->texture_count, texture, sizeof(dm_vulkan_texture));

    return true;
}

bool dm_renderer_backend_create_texture(dm_texture_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_texture texture = { 0 };

    VkImageUsageFlagBits  image_usage  = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    VkBufferUsageFlagBits buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkSharingMode         sharing_mode = VK_SHARING_MODE_CONCURRENT;

    const size_t size = desc.height * desc.width * desc.n_channels;

    switch(desc.format)
    {
        case DM_TEXTURE_FORMAT_BYTE_4_UINT:
        texture.format = VK_FORMAT_R8G8B8A8_UINT;
        break;

        case DM_TEXTURE_FORMAT_BYTE_4_UNORM:
        texture.format = VK_FORMAT_R8G8B8A8_UNORM;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_3:
        texture.format = VK_FORMAT_R32G32B32_SFLOAT;
        break;

        case DM_TEXTURE_FORMAT_FLOAT_4:
        texture.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        break;

        default:
        DM_LOG_FATAL("Unknown or unsupported texture format");
        return false;
    }

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width  = desc.width;
    image_create_info.extent.height = desc.height;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.usage         = image_usage;
    image_create_info.format        = texture.format;

    VmaAllocationCreateInfo allocation_create_info = { 0 };
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dm_vulkan_buffer* staging_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];

        if(!dm_vulkan_create_buffer(size, buffer_usage, sharing_mode, staging_buffer, vulkan_renderer)) return false;
        texture.staging_buffer[i] = vulkan_renderer->buffer_count++;

        // buffer data
        if(desc.data) 
        {
            if(!dm_vulkan_copy_memory(staging_buffer->memory, desc.data, size, vulkan_renderer)) return false;
        }

        // sampled image
        dm_vulkan_image* image = &vulkan_renderer->images[vulkan_renderer->image_count];

        vr = vmaCreateImage(vulkan_renderer->allocator, &image_create_info, &allocation_create_info, &image->image, &image->memory, NULL);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vmaCreateImage failed");
            return false;
        }
        texture.image[i] = vulkan_renderer->image_count++;

        if(desc.data)
        {
            VkBufferImageCopy copy = { 0 };
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent.width           = desc.width;
            copy.imageExtent.height          = desc.height;
            copy.imageExtent.depth           = 1;

            if(!dm_vulkan_copy_buffer_to_image(staging_buffer, image, copy, true, vulkan_renderer)) return false;
        }

        // view(s)
        VkImageViewCreateInfo view_info = { 0 };
        view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image    = image->image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format   = texture.format;

        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        vr = vkCreateImageView(vulkan_renderer->device.logical, &view_info, NULL, &image->view);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateImageView failed");
            return false;
        }
    }

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // storage image descriptor
        VkDescriptorImageInfo storage_info = { 0 };
        storage_info.imageView   = vulkan_renderer->images[texture.image[i]].view;
        storage_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet storage_descriptor = { 0 };
        storage_descriptor.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        storage_descriptor.descriptorCount = 1;
        storage_descriptor.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        storage_descriptor.dstSet          = vulkan_renderer->resource_bindless_set[i];
        storage_descriptor.dstArrayElement = vulkan_renderer->resource_heap_count;
        storage_descriptor.dstBinding      = 0;
        storage_descriptor.pImageInfo      = &storage_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &storage_descriptor, 0, NULL);

        // sampled image descriptor
        VkDescriptorImageInfo sampled_info = { 0 };
        sampled_info.imageView   = vulkan_renderer->images[texture.image[i]].view;
        sampled_info.sampler     = vulkan_renderer->samplers[desc.sampler.index].sampler;
        sampled_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet sampled_descriptor = { 0 };
        sampled_descriptor.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sampled_descriptor.descriptorCount = 1;
        sampled_descriptor.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sampled_descriptor.dstSet          = vulkan_renderer->resource_bindless_set[i];
        sampled_descriptor.dstArrayElement = vulkan_renderer->resource_heap_count + 1;
        sampled_descriptor.dstBinding      = 0;
        sampled_descriptor.pImageInfo      = &sampled_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &sampled_descriptor, 0, NULL);
    }

    //
    dm_memcpy(vulkan_renderer->textures + vulkan_renderer->texture_count, &texture, sizeof(texture));
    handle->index = vulkan_renderer->texture_count++;

    handle->descriptor_index = vulkan_renderer->resource_heap_count;
    vulkan_renderer->resource_heap_count += 2;

    return true;
}

bool dm_renderer_backend_create_sampler(dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_sampler sampler = { 0 };

    VkSamplerCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter        = VK_FILTER_LINEAR;
    sampler_create_info.minFilter        = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy    = vulkan_renderer->device.properties.properties.limits.maxSamplerAnisotropy;
    sampler_create_info.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.compareOp        = VK_COMPARE_OP_NEVER;

    vr = vkCreateSampler(vulkan_renderer->device.logical, &sampler_create_info, NULL, &sampler.sampler);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateSampler failed");
        return false;
    }

    // descriptor
    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo image_info = { 0 };
        image_info.sampler = sampler.sampler;

        VkWriteDescriptorSet write_set = { 0 };
        write_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.descriptorCount = 1;
        write_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
        write_set.dstSet          = vulkan_renderer->sampler_bindless_set[i];
        write_set.dstArrayElement = vulkan_renderer->sampler_heap_count;
        write_set.dstBinding      = 0;
        write_set.pImageInfo      = &image_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &write_set, 0, NULL);
    }

    //
    dm_memcpy(vulkan_renderer->samplers + vulkan_renderer->sampler_count, &sampler, sizeof(sampler));
    handle->index = vulkan_renderer->sampler_count++;
    handle->descriptor_index = vulkan_renderer->sampler_heap_count++;

    return true;
}

/**************
 * RAYTRACING *
 **************/
bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_raytracing_pipeline pipeline = { 0 };

    VkShaderModule raygen, miss, clossest_hit;

    if(!dm_vulkan_create_shader_module(desc.raygen, &raygen, vulkan_renderer->device.logical)) return false;
    if(!dm_vulkan_create_shader_module(desc.miss, &miss, vulkan_renderer->device.logical)) return false;
    if(!dm_vulkan_create_shader_module(desc.hit_groups[0].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST], &clossest_hit, vulkan_renderer->device.logical)) return false;

    VkPipelineShaderStageCreateInfo raygen_create_info = { 0 };
    raygen_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    raygen_create_info.stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    raygen_create_info.module = raygen;
    raygen_create_info.pName  = "main";

    VkPipelineShaderStageCreateInfo miss_create_info = { 0 };
    miss_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    miss_create_info.stage  = VK_SHADER_STAGE_MISS_BIT_KHR;
    miss_create_info.module = miss;
    miss_create_info.pName  = "main";

    VkPipelineShaderStageCreateInfo hit_create_info = { 0 };
    hit_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    hit_create_info.stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    hit_create_info.module = clossest_hit;
    hit_create_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { raygen_create_info, miss_create_info, hit_create_info };

    VkRayTracingShaderGroupCreateInfoKHR raygen_group = { 0 };
    raygen_group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygen_group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygen_group.generalShader      = 0;
    raygen_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    raygen_group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    raygen_group.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR miss_group = { 0 };
    miss_group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    miss_group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    miss_group.generalShader      = 1;
    miss_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    miss_group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR hit_group = { 0 };
    hit_group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    hit_group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    hit_group.generalShader      = VK_SHADER_UNUSED_KHR;
    hit_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    hit_group.closestHitShader   = 2;
    hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR shader_groups[] = { raygen_group, miss_group, hit_group };

    VkRayTracingPipelineCreateInfoKHR pipeline_create_info = { 0 };
    pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_create_info.stageCount                   = _countof(shader_stages);
    pipeline_create_info.pStages                      = shader_stages;
    pipeline_create_info.groupCount                   = _countof(shader_groups);
    pipeline_create_info.pGroups                      = shader_groups;
    pipeline_create_info.maxPipelineRayRecursionDepth = desc.max_depth;
    pipeline_create_info.layout                       = vulkan_renderer->bindless_pipeline_layout;

    vr = vkCreateRayTracingPipelinesKHR(vulkan_renderer->device.logical, NULL,NULL, 1, &pipeline_create_info, NULL, &pipeline.pipeline);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateRayTracingPipeline failed"); return false; }

    // shader binding table
    VkDeviceSize shader_alignment = vulkan_renderer->device.properties.raytracing_pipeline.shaderGroupBaseAlignment; 
    VkDeviceSize handle_size      = vulkan_renderer->device.properties.raytracing_pipeline.shaderGroupHandleSize;
    size_t handle_size_aligned    = DM_ALIGN_BYTES(handle_size, vulkan_renderer->device.properties.raytracing_pipeline.shaderGroupHandleAlignment);

    VkBufferUsageFlagBits usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        const size_t ray_gen_size = handle_size;
        const size_t miss_size    = handle_size;
        size_t hit_size           = handle_size;

        size_t size = ray_gen_size + miss_size + hit_size;
        size_t aligned_size = DM_ALIGN_BYTES(size, shader_alignment);
        uint8_t* handle_sizes = dm_alloc(size);
        vr = vkGetRayTracingShaderGroupHandlesKHR(vulkan_renderer->device.logical, pipeline.pipeline, 0,3, size, handle_sizes); 

        void* dest = NULL;

        dm_vulkan_buffer* buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];

        VkBufferDeviceAddressInfo buffer_address_info = { 0 };
        buffer_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

        if(!dm_vulkan_create_buffer(DM_ALIGN_BYTES(handle_size_aligned, shader_alignment), usage_flags, VK_SHARING_MODE_EXCLUSIVE, buffer, vulkan_renderer)) return false;
        pipeline.raygen_sbt.index[i]   = vulkan_renderer->buffer_count++;
        pipeline.raygen_sbt.stride     = DM_ALIGN_BYTES(handle_size_aligned, shader_alignment);
        pipeline.raygen_sbt.size       = pipeline.raygen_sbt.stride;
        buffer_address_info.buffer     = buffer->buffer;
        pipeline.raygen_sbt.address[i] = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &buffer_address_info);
        if(!dm_vulkan_copy_memory(buffer->memory, handle_sizes, handle_size, vulkan_renderer)) return false;
        buffer++;

        if(!dm_vulkan_create_buffer(DM_ALIGN_BYTES(handle_size_aligned, shader_alignment), usage_flags, VK_SHARING_MODE_EXCLUSIVE, buffer, vulkan_renderer)) return false;
        pipeline.miss_sbt.index[i]   = vulkan_renderer->buffer_count++;
        pipeline.miss_sbt.size       = DM_ALIGN_BYTES(handle_size_aligned, shader_alignment);
        pipeline.miss_sbt.stride     = handle_size_aligned;
        buffer_address_info.buffer   = buffer->buffer;
        pipeline.miss_sbt.address[i] = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &buffer_address_info);
        if(!dm_vulkan_copy_memory(buffer->memory, handle_sizes + ray_gen_size, handle_size, vulkan_renderer)) return false;
        buffer++;

        if(!dm_vulkan_create_buffer(DM_ALIGN_BYTES(handle_size_aligned, shader_alignment), usage_flags, VK_SHARING_MODE_EXCLUSIVE, buffer, vulkan_renderer)) return false;
        pipeline.hit_group_sbt.index[i]   = vulkan_renderer->buffer_count++;
        pipeline.hit_group_sbt.size       = DM_ALIGN_BYTES(handle_size_aligned, shader_alignment);
        pipeline.hit_group_sbt.stride     = handle_size_aligned;
        buffer_address_info.buffer        = buffer->buffer;
        pipeline.hit_group_sbt.address[i] = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &buffer_address_info);
        if(!dm_vulkan_copy_memory(buffer->memory, handle_sizes + ray_gen_size + miss_size, handle_size, vulkan_renderer)) return false;

        //
        dest = NULL;

        dm_free((void**)&handle_sizes);
    }

    //
    dm_memcpy(vulkan_renderer->rt_pipes + vulkan_renderer->rt_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = vulkan_renderer->rt_pipe_count++;

    vkDestroyShaderModule(vulkan_renderer->device.logical, raygen, NULL);
    vkDestroyShaderModule(vulkan_renderer->device.logical, miss, NULL);
    vkDestroyShaderModule(vulkan_renderer->device.logical, clossest_hit, NULL);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_acceleration_structure(VkAccelerationStructureBuildGeometryInfoKHR build_info, VkBufferUsageFlags usage_flags, uint32_t primitive_count, dm_vulkan_as* as, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = { 0 };
    build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    const uint32_t primitive_counts[] = { primitive_count };

    vkGetAccelerationStructureBuildSizesKHR(vulkan_renderer->device.logical, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, primitive_counts, &build_sizes_info);

    const size_t as_size      = build_sizes_info.accelerationStructureSize;
    const size_t scratch_size = build_sizes_info.buildScratchSize;

    // result buffer
    dm_vulkan_buffer* result_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count];
    dm_vulkan_buffer* scratch_buffer = &vulkan_renderer->buffers[vulkan_renderer->buffer_count+1];

    if(!dm_vulkan_create_buffer(as_size, usage_flags, VK_SHARING_MODE_CONCURRENT, result_buffer, vulkan_renderer)) return false;
    as->result = vulkan_renderer->buffer_count++;

    // acceleration structure
    VkAccelerationStructureCreateInfoKHR as_create_info = { 0 };
    as_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result_buffer->buffer;
    as_create_info.size   = as_size; 
    as_create_info.type   = build_info.type;

    vr = vkCreateAccelerationStructureKHR(vulkan_renderer->device.logical, &as_create_info, NULL, &as->as);
    if(!dm_vulkan_decode_vr(vr) || !as->as)
    {
        DM_LOG_FATAL("vkCreateAccelerationStructureKHR failed");
        return false;
    }

    // scratch buffer
    usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if(!dm_vulkan_create_buffer(scratch_size, usage_flags, VK_SHARING_MODE_CONCURRENT, scratch_buffer, vulkan_renderer)) return false;
    as->scratch = vulkan_renderer->buffer_count++;

    VkBufferDeviceAddressInfo scratch_device_info = { 0 };
    scratch_device_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratch_device_info.buffer = scratch_buffer->buffer;
    
    VkDeviceAddress scratch_device_address = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &scratch_device_info);

    // continue building
    build_info.scratchData.deviceAddress = scratch_device_address;
    build_info.dstAccelerationStructure  = as->as;

    VkAccelerationStructureBuildRangeInfoKHR build_range = { 0 };
    build_range.primitiveCount = primitive_count;

    const VkAccelerationStructureBuildRangeInfoKHR* build_range2 = { &build_range };
    vkCmdBuildAccelerationStructuresKHR(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], 1, &build_info, &build_range2);

    as->result_size  = as_size;
    as->scratch_size = scratch_size;

    return true;
}

bool dm_renderer_backend_create_blas(dm_blas_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_blas blas = { 0 };

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        //VkAccelerationStructureBuildRangeInfoKHR    build_range   = { 0 };
        VkAccelerationStructureGeometryDataKHR      geometry_data = { 0 };

        VkAccelerationStructureGeometryKHR          geometry      = { 0 };
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

        size_t primitive_count = 0;

        switch(desc.flags)
        {
            case DM_BLAS_GEOMETRY_FLAG_OPAQUE:
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            break;

            default:
            DM_LOG_FATAL("Unknown geometry flag");
            return false;
        }

        switch(desc.geometry_type)
        {
            case DM_BLAS_GEOMETRY_TYPE_TRIANGLES:
            {
                dm_vulkan_buffer vb = vulkan_renderer->buffers[vulkan_renderer->vertex_buffers[desc.vertex_buffer.index].device[i]];
                dm_vulkan_buffer ib = vulkan_renderer->buffers[vulkan_renderer->index_buffers[desc.index_buffer.index].device[i]];
                geometry_data.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                geometry.geometryType         = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

                VkBufferDeviceAddressInfo vb_address_info = { 0 };
                vb_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                vb_address_info.buffer = vb.buffer;
                VkDeviceAddress vb_address = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &vb_address_info);

                VkBufferDeviceAddressInfo ib_address_info = { 0 };
                ib_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                ib_address_info.buffer = ib.buffer; 
                VkDeviceAddress ib_address = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &ib_address_info);
                    
                // vertex data
                switch(desc.vertex_type)
                {
                    case DM_BLAS_VERTEX_TYPE_FLOAT_3:
                    geometry_data.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                    break;

                    default:
                    DM_LOG_FATAL("Unsupported vertex format");
                    return false;
                }

                geometry_data.triangles.maxVertex                = desc.vertex_count - 1; 
                geometry_data.triangles.vertexStride             = desc.vertex_stride;
                geometry_data.triangles.vertexData.deviceAddress = vb_address;

                // index data
                primitive_count = desc.index_count / 3; 

                switch(desc.index_type)
                {
                    case DM_INDEX_BUFFER_INDEX_TYPE_UINT32:
                    geometry_data.triangles.indexType = VK_INDEX_TYPE_UINT32;
                    break;

                    default:
                    DM_LOG_FATAL("Unsupported index type");
                    return false;
                }
                geometry_data.triangles.indexData.deviceAddress = ib_address;
            } break;

            default:
            DM_LOG_FATAL("Unsupported geometry type");
            return false;
        }

        geometry.geometry = geometry_data;

        VkAccelerationStructureBuildGeometryInfoKHR build_info    = { 0 };
        build_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.geometryCount = 1;
        build_info.pGeometries   = &geometry; 

        VkBufferUsageFlagBits usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

        if(!dm_vulkan_create_acceleration_structure(build_info, usage_flags, primitive_count, &blas.as[i], vulkan_renderer)) 
        {
            DM_LOG_FATAL("Could not create bottom-level acceleration structure");
            return false;
        }
    }

    //
    dm_memcpy(vulkan_renderer->blas + vulkan_renderer->blas_count, &blas, sizeof(blas));
    handle->index = vulkan_renderer->blas_count++;

    return true;
}

bool dm_renderer_backend_create_tlas(dm_tlas_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;
    
    dm_vulkan_tlas tlas = { 0 };
    tlas.instance_buffer = desc.instance_buffer;

    dm_vulkan_storage_buffer instance_buffer = vulkan_renderer->storage_buffers[desc.instance_buffer.index];

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkBufferDeviceAddressInfo instance_address_info = { 0 };
        instance_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        instance_address_info.buffer = vulkan_renderer->buffers[instance_buffer.device[i]].buffer;

        VkDeviceAddress instance_address = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &instance_address_info);
        
        // top level
        VkAccelerationStructureGeometryDataKHR geometry_data = { 0 };
        geometry_data.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry_data.instances.data.deviceAddress = instance_address;

        VkAccelerationStructureGeometryKHR geometry = { 0 };
        geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry     = geometry_data;
        // TODO: needs to be configurable, hardcoded here
        geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR build_info = { 0 };
        build_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        build_info.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        build_info.pGeometries   = &geometry;
        build_info.geometryCount = 1;

        VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = { 0 };
        build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        
        VkBufferUsageFlagBits usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        if(!dm_vulkan_create_acceleration_structure(build_info, usage_flags, desc.instance_count, &tlas.as[i], vulkan_renderer)) return false;

        // descriptors
        VkWriteDescriptorSetAccelerationStructureKHR write_info = { 0 };
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        write_info.accelerationStructureCount = 1;
        write_info.pAccelerationStructures    = &tlas.as[i].as;

        VkWriteDescriptorSet write_set = { 0 };
        write_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.descriptorCount = 1;
        write_set.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        write_set.dstSet          = vulkan_renderer->resource_bindless_set[i];
        write_set.dstArrayElement = vulkan_renderer->resource_heap_count;
        write_set.dstBinding      = 0;
        write_set.pNext           = &write_info;

        vkUpdateDescriptorSets(vulkan_renderer->device.logical, 1, &write_set, 0, NULL);
    }

    //
    dm_memcpy(vulkan_renderer->tlas + vulkan_renderer->tlas_count, &tlas, sizeof(tlas));
    handle->index = vulkan_renderer->tlas_count++;
    handle->descriptor_index = vulkan_renderer->resource_heap_count++;

    return true;
}

bool dm_renderer_backend_get_blas_gpu_address(dm_resource_handle blas, size_t* address, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_blas as = vulkan_renderer->blas[blas.index];

    VkBufferDeviceAddressInfo address_info = { 0 };
    address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    address_info.buffer = vulkan_renderer->buffers[as.as[vulkan_renderer->current_frame].result].buffer;

    *address = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &address_info);

    return true;
}

/*******************
 * RENDER COMMANDS *
********************/
bool dm_render_command_backend_begin_render_pass(float r, float g, float b, float a, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    VkRenderingAttachmentInfo color_attachment = { 0 };
    VkRenderingAttachmentInfo depth_attachment = { 0 };

    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    color_attachment.clearValue.color.float32[0] = r;
    color_attachment.clearValue.color.float32[1] = g;
    color_attachment.clearValue.color.float32[2] = b;
    color_attachment.clearValue.color.float32[3] = a;
    color_attachment.imageView = vulkan_renderer->swapchain.image_views[vulkan_renderer->image_index];
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    color_attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

    depth_attachment.imageView = vulkan_renderer->depth_stencil.view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    depth_attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.clearValue.depthStencil.depth = 1.f;

    VkRect2D render_area = { 0 };
    render_area.extent.width  = renderer->width;
    render_area.extent.height = renderer->height;

    VkRenderingInfo rendering_info = { 0 };
    rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.pColorAttachments    = &color_attachment;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pDepthAttachment     = &depth_attachment;
    rendering_info.layerCount           = 1;
    rendering_info.renderArea           = render_area;
    
    vkCmdBeginRendering(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], &rendering_info);

    return true;
}

bool dm_render_command_backend_end_render_pass(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    vkCmdEndRendering(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame]);

    return true;
}

bool dm_render_command_backend_bind_raster_pipeline(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame      = vulkan_renderer->current_frame;
    const VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    dm_vulkan_raster_pipeline pipeline = vulkan_renderer->raster_pipes[handle.index];

    pipeline.viewport.y      = (float)renderer->height;
    pipeline.viewport.width  = (float)renderer->width;
    pipeline.viewport.height = -(float)renderer->height;

    pipeline.scissor.extent.width  = renderer->width;
    pipeline.scissor.extent.height = renderer->height;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    vkCmdSetViewport(cmd_buffer, 0,1, &pipeline.viewport);
    vkCmdSetScissor(cmd_buffer, 0,1, &pipeline.scissor);

    vulkan_renderer->bound_pipeline      = handle;
    vulkan_renderer->bound_pipeline_type = DM_PIPELINE_TYPE_RASTER;

    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    dm_vulkan_raytracing_pipeline pipeline = vulkan_renderer->rt_pipes[handle.index];

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.pipeline);

    vulkan_renderer->bound_pipeline      = handle;
    vulkan_renderer->bound_pipeline_type = DM_PIPELINE_TYPE_RAYTRACING;

    return true;
}

bool dm_render_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkCmdPushConstants(cmd_buffer, vulkan_renderer->bindless_pipeline_layout, VK_SHADER_STAGE_ALL, offset, count * sizeof(uint32_t), data);

    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_resource_handle handle, uint8_t slot, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];
    dm_vulkan_vertex_buffer buffer = vulkan_renderer->vertex_buffers[handle.index];

    VkBuffer buffers[]     = { vulkan_renderer->buffers[buffer.device[current_frame]].buffer };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(cmd_buffer, 0, _countof(buffers),buffers, offsets);

    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];
    dm_vulkan_index_buffer buffer = vulkan_renderer->index_buffers[handle.index];

    dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[buffer.device[current_frame]];

    vkCmdBindIndexBuffer(cmd_buffer, device_buffer->buffer, 0, buffer.index_type);

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_copy_buffer(void* data, const size_t size, uint32_t host_index, uint32_t device_index, dm_vulkan_renderer* vulkan_renderer)
{
    VkBufferCopy copy_region = { 0 };
    copy_region.size = size;

    dm_vulkan_buffer* host_buffer   = &vulkan_renderer->buffers[host_index];
    dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[device_index];

    if(!dm_vulkan_copy_memory(host_buffer->memory, data, size, vulkan_renderer)) return false;
    vkCmdCopyBuffer(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], host_buffer->buffer, device_buffer->buffer, 1, &copy_region);

    return true;
}

bool dm_render_command_backend_update_vertex_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_vertex_buffer buffer = vulkan_renderer->vertex_buffers[handle.index];
        
    return dm_vulkan_copy_buffer(data, size, buffer.host[current_frame], buffer.device[current_frame], vulkan_renderer);

    return true;
}

bool dm_render_command_backend_update_index_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_index_buffer buffer = vulkan_renderer->index_buffers[handle.index];
        
    return dm_vulkan_copy_buffer(data, size, buffer.host[current_frame], buffer.device[current_frame], vulkan_renderer);
}

bool dm_render_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
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

bool dm_render_command_backend_update_storage_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_storage_buffer buffer = vulkan_renderer->storage_buffers[handle.index];
        
    dm_vulkan_buffer* host_buffer   = &vulkan_renderer->buffers[buffer.host[current_frame]];
    dm_vulkan_buffer* device_buffer = &vulkan_renderer->buffers[buffer.device[current_frame]];

    if(!dm_vulkan_copy_buffer(data, size, buffer.host[current_frame], buffer.device[current_frame], vulkan_renderer)) return false;

    return true;
}

bool dm_render_command_backend_resize_texture(uint32_t width, uint32_t height, dm_resource_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_tlas(uint32_t instance_count, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_tlas tlas = vulkan_renderer->tlas[handle.index];
    dm_vulkan_storage_buffer instance_buffer = vulkan_renderer->storage_buffers[tlas.instance_buffer.index];
    const uint8_t current_frame = vulkan_renderer->current_frame;

    dm_vulkan_buffer* instance_b = &vulkan_renderer->buffers[instance_buffer.device[current_frame]];
    dm_vulkan_buffer* scratch_buffer = &vulkan_renderer->buffers[tlas.as[current_frame].scratch];

    VkBufferDeviceAddressInfo instance_buffer_address_info = { 0 };
    instance_buffer_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    instance_buffer_address_info.buffer = instance_b->buffer;

    VkAccelerationStructureGeometryDataKHR geometry_data = { 0 };
    geometry_data.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry_data.instances.data.deviceAddress = vkGetBufferDeviceAddressKHR(vulkan_renderer->device.logical, &instance_buffer_address_info); 

    VkAccelerationStructureGeometryKHR geometry = { 0 };
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry     = geometry_data;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkBufferDeviceAddressInfo scratch_buffer_address_info = { 0 };
    scratch_buffer_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratch_buffer_address_info.buffer = scratch_buffer->buffer;

    VkAccelerationStructureBuildGeometryInfoKHR build_info = { 0 };
    build_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    build_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    build_info.dstAccelerationStructure  = tlas.as[current_frame].as;
    build_info.srcAccelerationStructure  = tlas.as[current_frame].as;    
    build_info.scratchData.deviceAddress = vkGetBufferDeviceAddress(vulkan_renderer->device.logical, &scratch_buffer_address_info);
    build_info.pGeometries               = &geometry; 
    build_info.geometryCount             = 1;

    VkAccelerationStructureBuildRangeInfoKHR build_range = { 0 };
    build_range.primitiveCount = instance_count;

    const VkAccelerationStructureBuildRangeInfoKHR* build_range2 = { &build_range };

    vkCmdBuildAccelerationStructuresKHR(vulkan_renderer->device.graphics_queue.buffer[current_frame], 1, &build_info, &build_range2);

    return true;
}

bool dm_render_command_backend_copy_image_to_screen(dm_resource_handle image, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_texture* texture = &vulkan_renderer->textures[image.index];

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer  = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    VkImage source_image  = vulkan_renderer->images[texture->image[current_frame]].image;
    VkImage render_target = vulkan_renderer->swapchain.render_targets[current_frame];

    // transition source image before copy
    VkImageSubresourceRange range = { 0 };
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    // must transition source image and render target
    VkImageMemoryBarrier source_barrier = { 0 };
    source_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    source_barrier.image               = source_image;
    source_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    source_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    source_barrier.srcAccessMask       = 0;
    source_barrier.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    source_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    source_barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    source_barrier.subresourceRange    = range;

    VkImageMemoryBarrier render_target_barrier = { 0 };
    render_target_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    render_target_barrier.image               = render_target;
    render_target_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    render_target_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    render_target_barrier.srcAccessMask       = 0;
    render_target_barrier.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    render_target_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    render_target_barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    render_target_barrier.subresourceRange    = range;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  0,0,NULL,0,NULL, 1, &render_target_barrier);
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  0,0,NULL,0,NULL, 1, &source_barrier);

    // copy
    VkImageCopy image_copy = { 0 };
    image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.srcSubresource.layerCount = 1;
    image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.dstSubresource.layerCount = 1;
    image_copy.extent.width              = renderer->width;
    image_copy.extent.height             = renderer->height;
    image_copy.extent.depth              = 1;
    vkCmdCopyImage(cmd_buffer, source_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, render_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

    // transistion everyone back
    source_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    source_barrier.dstAccessMask = 0;
    source_barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    source_barrier.newLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

    render_target_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    render_target_barrier.dstAccessMask = 0;
    render_target_barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    render_target_barrier.newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  0,0,NULL,0,NULL, 1, &source_barrier);
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  0,0,NULL,0,NULL, 1, &render_target_barrier);

    return true;
}

bool dm_render_command_backend_draw_instanced(uint32_t instance_count, uint32_t instance_offset, uint32_t vertex_count, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    vkCmdDraw(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], vertex_count, instance_count, vertex_offset, instance_offset);

    return true;
}

bool dm_render_command_backend_draw_instanced_indexed(uint32_t instance_count, uint32_t instance_offset, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    vkCmdDrawIndexed(vulkan_renderer->device.graphics_queue.buffer[vulkan_renderer->current_frame], index_count, instance_count, index_offset, vertex_offset, instance_offset);

    return true;
}

bool dm_render_command_backend_dispatch_rays(uint16_t x, uint16_t y, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer  = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    dm_vulkan_raytracing_pipeline pipeline = vulkan_renderer->rt_pipes[handle.index];

    const VkStridedDeviceAddressRegionKHR raygen_sbt = { 
        .deviceAddress=pipeline.raygen_sbt.address[current_frame],
        .stride=pipeline.raygen_sbt.stride, .size=pipeline.raygen_sbt.size
    };

    const VkStridedDeviceAddressRegionKHR miss_sbt = { 
        .deviceAddress=pipeline.miss_sbt.address[current_frame],
        .stride=pipeline.miss_sbt.stride, .size=pipeline.miss_sbt.size
    };

    const VkStridedDeviceAddressRegionKHR hit_sbt = { 
        .deviceAddress=pipeline.hit_group_sbt.address[current_frame],
        .stride=pipeline.hit_group_sbt.stride, .size=pipeline.hit_group_sbt.size
    };

    const VkStridedDeviceAddressRegionKHR callable_sbt = { 0 };
    
    vkCmdTraceRaysKHR(cmd_buffer, &raygen_sbt, &miss_sbt, &hit_sbt, &callable_sbt, x,y,1);

    return true;
}

/**********
* COMPUTE *
***********/
bool dm_compute_backend_create_compute_pipeline(dm_compute_pipeline_desc desc, dm_resource_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_compute_pipeline pipeline = { 0 };

    VkShaderModule cs;
    VkPipelineShaderStageCreateInfo shader_create_info = { 0 };
    // === shader ===
    if(!dm_vulkan_create_shader_module(desc.shader.path, &cs, vulkan_renderer->device.logical)) 
    {
        DM_LOG_ERROR("Could not load vertex shader");
        return false;
    }

    shader_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_create_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_create_info.module = cs;
    shader_create_info.pName  = "main";

    VkComputePipelineCreateInfo create_info = { 0 };
    create_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.layout = vulkan_renderer->bindless_pipeline_layout;
    create_info.stage  = shader_create_info;
#ifdef DM_VULKAN_DESCRIPTOR_BUFFERS
    create_info.flags  = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif

    vr = vkCreateComputePipelines(vulkan_renderer->device.logical, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.pipeline);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateComputePipelines failed");
        return false;
    }

    vkDestroyShaderModule(vulkan_renderer->device.logical, cs, NULL);

    //
    dm_memcpy(vulkan_renderer->compute_pipes + vulkan_renderer->compute_pipe_count, &pipeline, sizeof(pipeline));
    handle->index = vulkan_renderer->compute_pipe_count++;

    return true;
}

bool dm_compute_command_backend_begin_recording(dm_renderer* renderer)
{
    return true;
}

bool dm_compute_command_backend_end_recording(dm_renderer* renderer)
{
    return true;
}

void dm_compute_command_backend_bind_compute_pipeline(dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    dm_vulkan_compute_pipeline pipeline = vulkan_renderer->compute_pipes[handle.index];

    const uint8_t current_frame      = vulkan_renderer->current_frame;
    const VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

    vulkan_renderer->bound_pipeline = handle;
    vulkan_renderer->bound_pipeline_type = DM_PIPELINE_TYPE_COMPUTE;

}

void dm_compute_command_backend_update_constant_buffer(void* data, size_t size, dm_resource_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    dm_vulkan_constant_buffer buffer = vulkan_renderer->constant_buffers[handle.index];

    dm_memcpy(buffer.mapped_memory[current_frame], data, size);
}

void dm_compute_command_backend_set_root_constants(uint8_t slot, uint32_t count, size_t offset, void* data, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    VkCommandBuffer cmd_buffer = vulkan_renderer->device.graphics_queue.buffer[current_frame];

    vkCmdPushConstants(cmd_buffer, vulkan_renderer->bindless_pipeline_layout, VK_SHADER_STAGE_ALL, offset, count * sizeof(uint32_t), data);
}

void dm_compute_command_backend_dispatch(const uint16_t x, const uint16_t y, const uint16_t z, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    VkResult vr;

    const uint8_t current_frame = vulkan_renderer->current_frame;
    
    vkCmdDispatch(vulkan_renderer->device.graphics_queue.buffer[current_frame], x,y,z);
}

// === DEBUG AND DECODE ===
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
        dm_kill(user_data);
        return VK_FALSE;
        
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

    return VK_TRUE;
}
#endif

/***************
 * BOILERPLATE *
****************/
bool dm_vulkan_create_instance(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

    VkResult vr;

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
    app_info.apiVersion         = DM_VULKAN_API_VERSION;

    VkInstanceCreateInfo create_info = { 0 };
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = _countof(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;
#ifdef DM_DEBUG
    create_info.enabledLayerCount       = _countof(required_layers);
    create_info.ppEnabledLayerNames     = required_layers;
#endif

    vr = vkCreateInstance(&create_info, NULL, &vulkan_renderer->instance);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateInstance failed");
        return false;
    }

    volkLoadInstance(vulkan_renderer->instance);

#ifdef DM_DEBUG
    uint32_t message_severity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    message_severity          |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#if 0
    message_severity          |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    message_severity          |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

    uint32_t message_type  = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    message_type          |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    message_type          |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { 0 };
    debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = message_severity;
    debug_create_info.messageType     = message_type;
    debug_create_info.pfnUserCallback = dm_vulkan_debug_callback;
    debug_create_info.pUserData       = context;

    vr = vkCreateDebugUtilsMessengerEXT(vulkan_renderer->instance, &debug_create_info, NULL, &vulkan_renderer->debug_messenger);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("Failed to create Vulkan debug messenger");
        return false;
    }
#endif

    return true;
}

bool dm_vulkan_create_surface(dm_context* context)
{
    VkResult vr;
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;

#ifdef DM_PLATFORM_WIN32
    dm_internal_w32_data* w32_data = context->platform_data.internal_data;

    VkWin32SurfaceCreateInfoKHR create_info = { 0 };
    create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hwnd      = w32_data->hwnd;
    create_info.hinstance = w32_data->h_instance;

    vr = vkCreateWin32SurfaceKHR(vulkan_renderer->instance, &create_info, NULL, &vulkan_renderer->surface);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateWin32SurfaceKHR failed");
        return false;
    }
#endif
    return true;
}

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
bool dm_vulkan_is_device_suitable(VkPhysicalDevice device, const char** device_extensions, const uint8_t extension_count, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    dm_vulkan_device_properties properties = { 0 };
    dm_vulkan_device_features   features   = { 0 };
    VkPhysicalDeviceMemoryProperties memory_properties = { 0 };

    vkGetPhysicalDeviceProperties(device, &properties.properties);
    vkGetPhysicalDeviceFeatures(device, &features.features);
    vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

    if(properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { DM_LOG_ERROR("GPU is not discrete."); return false; }

    properties.properties2.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.properties2.pNext            = &properties.vulkan_1_3;
    properties.vulkan_1_3.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    properties.vulkan_1_3.pNext            = &properties.acceleration_structure;
    properties.acceleration_structure.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    properties.acceleration_structure.pNext = &properties.raytracing_pipeline;
    properties.raytracing_pipeline.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    features.features2.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.features2.pNext              = &features.vulkan_1_3;
    features.vulkan_1_3.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features.vulkan_1_3.pNext              = &features.acceleration_structure;
    features.acceleration_structure.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    features.acceleration_structure.pNext  = &features.raytracing_pipeline;
    features.raytracing_pipeline.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    features.raytracing_pipeline.pNext     = &features.buffer_device_address;
    features.buffer_device_address.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
    features.buffer_device_address.pNext   = &features.mutable_descriptor_type;
    features.mutable_descriptor_type.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;

    vkGetPhysicalDeviceProperties2(device, &properties.properties2);
    vkGetPhysicalDeviceFeatures2(device, &features.features2);

    if(features.acceleration_structure.accelerationStructure==0)  { DM_LOG_ERROR("GPU does not support acceleration structures."); return false; }
    if(features.raytracing_pipeline.rayTracingPipeline==0)        { DM_LOG_ERROR("GPU does not support ray tracing pipelines."); return false; }
    if(features.mutable_descriptor_type.mutableDescriptorType==0) { DM_LOG_ERROR("GPU does not support mutable descriptors."); return false; }

    // === extension support ===
    uint32_t available_extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, NULL);
    VkExtensionProperties* available_properties = dm_alloc(sizeof(VkExtensionProperties) * available_extension_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, available_properties);

#ifdef DM_DEBUG
    DM_LOG_INFO("Device available extensions: ");
    for(uint32_t i=0; i<available_extension_count; i++)
    {
        DM_LOG_INFO("    %s", available_properties[i].extensionName);
    }
#endif

    for(uint32_t i=0; i<extension_count; i++)
    {
        bool found = false;

        DM_LOG_WARN("Searching for device extension: %s", device_extensions[i]);
        for(uint32_t j=0; j<available_extension_count; j++)
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

    // === queue families ===
    uint32_t family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, NULL);
    if(family_count==0) { DM_LOG_ERROR("GPU has no queue families"); return false; }

    VkQueueFamilyProperties* family_properties = dm_alloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, family_properties);

    dm_vulkan_queue_family graphics_queue = { .index=-1 };
    dm_vulkan_queue_family compute_queue = { .index=-1 };
    dm_vulkan_queue_family transfer_queue = { .index=-1 };

    for(uint32_t i=0; i<family_count; i++)
    {
        if(family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkan_renderer->surface, &present_support);
            if(present_support) graphics_queue.index = i;
        }
        else if(family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) transfer_queue.index = i;

        if(family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)       compute_queue.index = i;
    }

    if(graphics_queue.index==-1) { DM_LOG_ERROR("No suitable graphics queue found"); return false; };
    if(compute_queue.index==-1)  { DM_LOG_ERROR("No suitable compute queue found"); return false; };
    if(transfer_queue.index==-1) { DM_LOG_ERROR("No suitable transfer queue found"); return false; };

    //
    dm_free((void**)&family_properties);
    
    // query swapchain support
    dm_vulkan_swapchain_details swapchain_details = dm_vulkan_query_swapchain_support(device, vulkan_renderer->surface);
    if(swapchain_details.present_mode_count==0) { DM_LOG_ERROR("GPU does not support swapchains"); return false; }

    // get everything settled
    vulkan_renderer->device.physical          = device;
    vulkan_renderer->device.properties        = properties;
    vulkan_renderer->device.features          = features;
    vulkan_renderer->device.memory_properties = memory_properties;
    vulkan_renderer->device.graphics_queue    = graphics_queue;
    vulkan_renderer->device.compute_queue     = compute_queue;
    vulkan_renderer->device.transfer_queue    = transfer_queue;

    // have to reset the pointers
    vulkan_renderer->device.properties.properties2.pNext            = &vulkan_renderer->device.properties.vulkan_1_3;
    vulkan_renderer->device.properties.vulkan_1_3.pNext             = &vulkan_renderer->device.properties.acceleration_structure;
    vulkan_renderer->device.properties.acceleration_structure.pNext = &vulkan_renderer->device.properties.raytracing_pipeline;

    vulkan_renderer->device.features.features2.pNext              = &vulkan_renderer->device.features.vulkan_1_3;
    vulkan_renderer->device.features.vulkan_1_3.pNext             = &vulkan_renderer->device.features.acceleration_structure;
    vulkan_renderer->device.features.acceleration_structure.pNext = &vulkan_renderer->device.features.raytracing_pipeline;
    vulkan_renderer->device.features.raytracing_pipeline.pNext    = &vulkan_renderer->device.features.buffer_device_address;
    vulkan_renderer->device.features.buffer_device_address.pNext  = &vulkan_renderer->device.features.mutable_descriptor_type;

    vulkan_renderer->swapchain.details = swapchain_details;

    return true;
}

bool dm_vulkan_create_device(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME
    };

    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &physical_device_count, VK_NULL_HANDLE);
    if(physical_device_count==0) { DM_LOG_FATAL("No physical devices found"); return false; }

    VkPhysicalDevice*           devices    = dm_alloc(sizeof(VkPhysicalDevice) * physical_device_count);
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &physical_device_count, devices);

    vulkan_renderer->device.physical = VK_NULL_HANDLE;

    for(uint32_t i=0; i<physical_device_count; i++)
    {
        if(!dm_vulkan_is_device_suitable(devices[i], device_extensions, _countof(device_extensions), vulkan_renderer)) continue;

        break;
    }

    dm_free((void**)&devices);

    if(vulkan_renderer->device.physical==VK_NULL_HANDLE) { DM_LOG_FATAL("No suitable physical device found"); return false; }

    uint32_t api_version = vulkan_renderer->device.properties.properties.apiVersion;

    DM_LOG_INFO("Vulkan API Version: %u.%u.%u", VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version), VK_API_VERSION_PATCH(api_version));

    // queue families
#define DM_VULKAN_MAX_QUEUE_FAMILIES 3
    VkDeviceQueueCreateInfo queue_create_infos[DM_VULKAN_MAX_QUEUE_FAMILIES] = { 0 };
    
    uint32_t family_indices[DM_VULKAN_MAX_QUEUE_FAMILIES] = { 
        vulkan_renderer->device.graphics_queue.index,
        vulkan_renderer->device.compute_queue.index,
        vulkan_renderer->device.transfer_queue.index
    };

    const float queue_priority = 1.f;

    for(uint8_t i=0; i<DM_VULKAN_MAX_QUEUE_FAMILIES; i++)
    {
        queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = family_indices[i];
        queue_create_infos[i].pQueuePriorities = &queue_priority;
        queue_create_infos[i].queueCount       = 1;
    }

    // === logical device ===
    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.enabledExtensionCount   = _countof(device_extensions);
    device_create_info.pQueueCreateInfos       = queue_create_infos;
    device_create_info.queueCreateInfoCount    = _countof(queue_create_infos);
    device_create_info.pNext                   = &vulkan_renderer->device.features.features2;

    vr = vkCreateDevice(vulkan_renderer->device.physical, &device_create_info, NULL, &vulkan_renderer->device.logical);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateDevice failed");
        return false;
    }

    // === create queues ===
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_queue.index, 0, &vulkan_renderer->device.graphics_queue.queue);
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.compute_queue.index,  0, &vulkan_renderer->device.compute_queue.queue);
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.transfer_queue.index, 0, &vulkan_renderer->device.transfer_queue.queue);

    return true;
}

bool dm_vulkan_create_allocator(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VmaVulkanFunctions functions = { 0 };
    functions.vkAllocateMemory                    = vkAllocateMemory;
    functions.vkBindBufferMemory                  = vkBindBufferMemory;
    functions.vkFreeMemory                        = vkFreeMemory;
    functions.vkGetDeviceProcAddr                 = vkGetDeviceProcAddr;
    functions.vkGetInstanceProcAddr               = vkGetInstanceProcAddr;
    functions.vkMapMemory                         = vkMapMemory;
    functions.vkUnmapMemory                       = vkUnmapMemory;
    functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    functions.vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges;
    functions.vkGetMemoryWin32HandleKHR           = vkGetMemoryWin32HandleKHR;

    functions.vkBindBufferMemory                  = vkBindBufferMemory;
    functions.vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements;
    functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    functions.vkCreateBuffer                      = vkCreateBuffer;
    functions.vkDestroyBuffer                     = vkDestroyBuffer;
    functions.vkCmdCopyBuffer                     = vkCmdCopyBuffer;

    functions.vkBindImageMemory                   = vkBindImageMemory;
    functions.vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements;
    functions.vkGetDeviceImageMemoryRequirements  = vkGetDeviceImageMemoryRequirements;
    functions.vkCreateImage                       = vkCreateImage;
    functions.vkDestroyImage                      = vkDestroyImage;

    VmaAllocatorCreateInfo create_info = { 0 };
    create_info.instance          = vulkan_renderer->instance;
    create_info.physicalDevice    = vulkan_renderer->device.physical;
    create_info.device            = vulkan_renderer->device.logical;
    create_info.pVulkanFunctions  = &functions;
    create_info.flags            |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vr = vmaCreateAllocator(&create_info, &vulkan_renderer->allocator);
    if(dm_vulkan_decode_vr(vr)) return true;

    DM_LOG_FATAL("VMA failed to initialize");
    return false;
}

bool dm_vulkan_create_swapchain(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    vulkan_renderer->swapchain.details = dm_vulkan_query_swapchain_support(vulkan_renderer->device.physical, vulkan_renderer->surface);

    if(vulkan_renderer->swapchain.details.capabilities.currentExtent.width==UINT_MAX || vulkan_renderer->swapchain.details.capabilities.currentExtent.height==UINT_MAX)
    {
        DM_LOG_FATAL("Current swapchain extents are invalid");
        return false;
    }
    
    VkSurfaceFormatKHR surface_format;
    bool found = false;

    for(uint32_t i=0; i<vulkan_renderer->swapchain.details.format_count; i++)
    {
        VkSurfaceFormatKHR format = vulkan_renderer->swapchain.details.formats[i];

        if(format.format==VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            found = true;
            surface_format = format;
            break;
        }
    }

    if(!found) { DM_LOG_FATAL("Desired swapchain format not supported"); return false; }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(uint32_t i=0; i<vulkan_renderer->swapchain.details.present_mode_count; i++)
    {
        VkPresentModeKHR mode = vulkan_renderer->swapchain.details.present_modes[i];

        if(mode==VK_PRESENT_MODE_MAILBOX_KHR) present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    VkExtent2D swapchain_extents = { 0 };
    swapchain_extents.width  = vulkan_renderer->swapchain.details.capabilities.currentExtent.width;
    swapchain_extents.height = vulkan_renderer->swapchain.details.capabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR create_info = { 0 };
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = vulkan_renderer->surface;
    create_info.minImageCount    = DM_VULKAN_MAX_FRAMES_IN_FLIGHT;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = swapchain_extents;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = NULL;

    create_info.preTransform   = vulkan_renderer->swapchain.details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = present_mode;
    create_info.clipped        = VK_TRUE;
    create_info.oldSwapchain   = VK_NULL_HANDLE;

    vr = vkCreateSwapchainKHR(vulkan_renderer->device.logical, &create_info, NULL, &vulkan_renderer->swapchain.swapchain);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateSwapchainKHR failed");
        return false;
    }

    vulkan_renderer->swapchain.format  = surface_format.format;
    vulkan_renderer->swapchain.extents = swapchain_extents;

    return true;
}

bool dm_vulkan_create_render_targets(dm_vulkan_renderer* vulkan_renderer)
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

        vr = vkCreateImageView(vulkan_renderer->device.logical, &create_info, NULL, &vulkan_renderer->swapchain.image_views[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateImageView failed");
            return false;
        }
    }

    return true;
}

bool dm_vulkan_create_depth_stencil_target(const uint32_t width, const uint32_t height, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkImageCreateInfo create_info = { 0 };
    create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.format        = VK_FORMAT_D32_SFLOAT_S8_UINT;
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.extent.width  = width;
    create_info.extent.height = height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocate_create_info = { 0 };
    allocate_create_info.usage = VMA_MEMORY_USAGE_AUTO;

    vr = vmaCreateImage(vulkan_renderer->allocator, &create_info, &allocate_create_info, &vulkan_renderer->depth_stencil.image, &vulkan_renderer->depth_stencil.memory, NULL);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateImage failed");
        DM_LOG_ERROR("Could not create depth stencil target");
        return false;
    }

    VkImageViewCreateInfo view_create_info = { 0 };
    view_create_info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format                      = VK_FORMAT_D32_SFLOAT_S8_UINT;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.layerCount = 1;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_create_info.image                       = vulkan_renderer->depth_stencil.image;

    vr = vkCreateImageView(vulkan_renderer->device.logical, &view_create_info, NULL, &vulkan_renderer->depth_stencil.view);
    if(!dm_vulkan_decode_vr(vr))
    {
        DM_LOG_FATAL("vkCreateImageView failed");
        DM_LOG_ERROR("Could not create depth stencil view");
        return false;
    }

    return true;
}

bool dm_vulkan_create_command_pools_and_buffers(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    uint32_t queue_indices[DM_VULKAN_MAX_QUEUE_FAMILIES] = {
        vulkan_renderer->device.graphics_queue.index,
        vulkan_renderer->device.compute_queue.index,
        vulkan_renderer->device.transfer_queue.index
    };

    VkCommandPool* command_pools[DM_VULKAN_MAX_QUEUE_FAMILIES] = { 
        &vulkan_renderer->device.graphics_queue.pool,
        &vulkan_renderer->device.compute_queue.pool,
        &vulkan_renderer->device.transfer_queue.pool
    };

    VkCommandBuffer* command_buffers[DM_VULKAN_MAX_QUEUE_FAMILIES] = {
        vulkan_renderer->device.graphics_queue.buffer,
        vulkan_renderer->device.compute_queue.buffer,
        vulkan_renderer->device.transfer_queue.buffer
    };

    for(uint8_t i=0; i<DM_VULKAN_MAX_QUEUE_FAMILIES; i++)
    {
        VkCommandPoolCreateInfo pool_create_info = { 0 };
        pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.queueFamilyIndex = queue_indices[i];
        pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        vr = vkCreateCommandPool(vulkan_renderer->device.logical, &pool_create_info, NULL, command_pools[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateCommandPool failed");
            return false;
        }

        VkCommandBufferAllocateInfo buffer_allocate_info = { 0 };
        buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        buffer_allocate_info.commandBufferCount = 1;
        buffer_allocate_info.commandPool        = *command_pools[i];
        buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        for(uint8_t j=0; j<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; j++)
        {
            vr = vkAllocateCommandBuffers(vulkan_renderer->device.logical, &buffer_allocate_info, &command_buffers[i][j]);
            if(!dm_vulkan_decode_vr(vr))
            {
                DM_LOG_FATAL("vkAllocateCommandBuffers failed");
                return false;
            }
        }
    }

    // open graphics and transfer for resource creation
    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(vulkan_renderer->device.graphics_queue.buffer[0], &begin_info);
    vkBeginCommandBuffer(vulkan_renderer->device.transfer_queue.buffer[0], &begin_info);

    return true;
}

bool dm_vulkan_create_synchronization_objects(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkSemaphoreCreateInfo semaphore_create_info = { 0 };
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vr = vkCreateSemaphore(vulkan_renderer->device.logical, &semaphore_create_info, NULL, &vulkan_renderer->wait_semaphores[i]);
        if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateSemaphore failed"); return false;}
        
        vr = vkCreateSemaphore(vulkan_renderer->device.logical, &semaphore_create_info, NULL, &vulkan_renderer->signal_semaphores[i]);
        if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateSemaphore failed"); return false;}

        VkFenceCreateInfo fence_create_info = { 0 };
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vr = vkCreateFence(vulkan_renderer->device.logical, &fence_create_info, NULL, &vulkan_renderer->front_fences[i]);
        if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateFence failed"); return false; }
    }
    
    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
bool dm_vulkan_create_layout(VkDescriptorType* types, uint8_t binding_count, VkDescriptorSetLayout* layout, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkDescriptorSetLayoutBinding bindings[DM_VULKAN_MAX_DESCRIPTOR_TYPES] = { 0 };

    for(uint8_t i=0; i<binding_count; i++)
    {
        bindings[i].binding         = i;
        bindings[i].descriptorType  = types[i];
        bindings[i].descriptorCount = DM_VULKAN_MAX_DESCRIPTORS;
        bindings[i].stageFlags      = VK_SHADER_STAGE_ALL;
    }

    VkDescriptorSetLayoutCreateInfo create_info = { 0 };
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pBindings    = bindings;
    create_info.bindingCount = binding_count;

    vr = vkCreateDescriptorSetLayout(vulkan_renderer->device.logical, &create_info, NULL, layout);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateDescriptorSetLayout failed"); return false; }

    return true;
}
                             
bool dm_vulkan_create_bindless_layout(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    // resource bindless layout
    VkDescriptorType resource_descriptor_types[] = {
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };

    VkMutableDescriptorTypeListEXT mutable_list = { 0 };
    mutable_list.descriptorTypeCount = _countof(resource_descriptor_types);
    mutable_list.pDescriptorTypes    = resource_descriptor_types;

    VkMutableDescriptorTypeCreateInfoEXT mutable_create_info = { 0 };
    mutable_create_info.sType                          = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
    mutable_create_info.mutableDescriptorTypeListCount = 1;
    mutable_create_info.pMutableDescriptorTypeLists    = &mutable_list;

    VkDescriptorSetLayoutBinding mutable_binding = { 0 };
    mutable_binding.binding         = 0;
    mutable_binding.descriptorType  = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
    mutable_binding.descriptorCount = DM_VULKAN_MAX_DESCRIPTORS * _countof(resource_descriptor_types);
    mutable_binding.stageFlags      = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo resource_create_info = { 0 };
    resource_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    resource_create_info.bindingCount = 1;
    resource_create_info.pBindings    = &mutable_binding;
    resource_create_info.pNext        = &mutable_create_info;

    vr = vkCreateDescriptorSetLayout(vulkan_renderer->device.logical, &resource_create_info, NULL, &vulkan_renderer->resource_bindless_layout);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateDescriptorSetLayout failed"); return false; }

    // sampler bindless layout
    VkDescriptorSetLayoutBinding sampler_binding = { 0 };
    sampler_binding.binding         = 0;
    sampler_binding.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_binding.descriptorCount = DM_VULKAN_MAX_DESCRIPTORS;
    sampler_binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampler_create_info.bindingCount = 1;
    sampler_create_info.pBindings    = &sampler_binding;

    vr = vkCreateDescriptorSetLayout(vulkan_renderer->device.logical, &sampler_create_info, NULL, &vulkan_renderer->sampler_bindless_layout);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreateDescriptorSetLayout failed"); return false; }

    return true;
}

bool dm_vulkan_create_bindless_descriptor_pool(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkDescriptorPoolSize resource_pool_size = { 0 };
    resource_pool_size.type = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
    resource_pool_size.descriptorCount = DM_VULKAN_MAX_DESCRIPTORS * 7;

    VkDescriptorPoolCreateInfo resource_create_info = { 0 };
    resource_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    resource_create_info.maxSets       = 1;
    resource_create_info.poolSizeCount = 1;
    resource_create_info.pPoolSizes    = &resource_pool_size;
    resource_create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkDescriptorPoolSize sampler_pool_size = { 0 };
    sampler_pool_size.type            = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_pool_size.descriptorCount = DM_VULKAN_MAX_DESCRIPTORS;

    VkDescriptorPoolCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    sampler_create_info.maxSets       = 1;
    sampler_create_info.poolSizeCount = 1; 
    sampler_create_info.pPoolSizes    = &sampler_pool_size;
    sampler_create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vr = vkCreateDescriptorPool(vulkan_renderer->device.logical, &resource_create_info, NULL, &vulkan_renderer->resource_bindless_pool[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateDescriptorPool failed");
            return false;
        }

        vr = vkCreateDescriptorPool(vulkan_renderer->device.logical, &sampler_create_info, NULL, &vulkan_renderer->sampler_bindless_pool[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkCreateDescriptorPool failed");
            return false;
        }
    }

    return true;
}

bool dm_vulkan_create_bindless_descriptor_sets(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkDescriptorSetLayout layouts[] = { vulkan_renderer->resource_bindless_layout, vulkan_renderer->sampler_bindless_layout };

    for(uint8_t i=0; i<DM_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorSetAllocateInfo resource_info = { 0 };
        resource_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        resource_info.descriptorPool     = vulkan_renderer->resource_bindless_pool[i];
        resource_info.pSetLayouts        = &vulkan_renderer->resource_bindless_layout;
        resource_info.descriptorSetCount = 1;
        
        vr = vkAllocateDescriptorSets(vulkan_renderer->device.logical, &resource_info, &vulkan_renderer->resource_bindless_set[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkAllocateDescriptorSets failed");
            return false;
        }

        VkDescriptorSetAllocateInfo sampler_info = { 0 };
        sampler_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        sampler_info.descriptorPool = vulkan_renderer->sampler_bindless_pool[i];
        sampler_info.pSetLayouts    = &vulkan_renderer->sampler_bindless_layout;
        sampler_info.descriptorSetCount = 1;

        vr = vkAllocateDescriptorSets(vulkan_renderer->device.logical, &sampler_info, &vulkan_renderer->sampler_bindless_set[i]);
        if(!dm_vulkan_decode_vr(vr))
        {
            DM_LOG_FATAL("vkAllocateDescriptorSets failed");
            return false;
        }
    }
        
    return true;
}

bool dm_vulkan_create_bindless_pipeline_layout(dm_vulkan_renderer* vulkan_renderer)
{
    VkResult vr;

    VkDescriptorSetLayout layouts[] = { vulkan_renderer->resource_bindless_layout, vulkan_renderer->sampler_bindless_layout };

    VkPushConstantRange push_constant_range = { 0 };
    push_constant_range.size       = DM_MAX_ROOT_CONSTANTS * sizeof(uint32_t);
    push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
    push_constant_range.offset     = 0;

    VkPipelineLayoutCreateInfo layout_create_info = { 0 };
    layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pSetLayouts            = layouts;
    layout_create_info.setLayoutCount         = _countof(layouts);
    layout_create_info.pPushConstantRanges    = &push_constant_range;
    layout_create_info.pushConstantRangeCount = 1;

    vr = vkCreatePipelineLayout(vulkan_renderer->device.logical, &layout_create_info, NULL, &vulkan_renderer->bindless_pipeline_layout);
    if(!dm_vulkan_decode_vr(vr)) { DM_LOG_FATAL("vkCreatePipelineLayout failed"); return false; }

    return true;
}

#endif
