#include "dm.h"

#include <stdlib.h>

#define VOLK_IMPLEMENTATION
//#define VK_NO_PROTOTYPES
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#include <shaderc/shaderc.h>

#include <assert.h>

#define DM_SWAPCHAIN_MAX_IMAGES 5
#define DM_SWAPCHAIN_FORMAT     VK_FORMAT_B8G8R8A8_SRGB
#define DM_DEPTH_FORMAT         VK_FORMAT_D32_SFLOAT

extern VkSurfaceKHR dm_window_create_vulkan_surface(dm_context *context, VkInstance instance);
extern const char** dm_window_get_vulkan_extensions(u32 *glfw_ext_count);

bool dm_vulkan_decode_vr(VkResult vr);

typedef struct dm_vulkan_gpu_t
{
    VkPhysicalDevice physical;
    VkDevice         device;

    VkQueue gfx_queue;
    u32     gfx_index;

    VkQueue compute_queue;
    u32     compute_index;

    VkPhysicalDeviceFeatures   features;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceProperties2 props2;
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heap_props;
} dm_vulkan_gpu;

typedef struct dm_vulkan_surface_t
{
    VkSurfaceKHR surface;

    VkSurfaceCapabilitiesKHR capabilities;
} dm_vulkan_surface;

typedef struct dm_vulkan_swapchain_image_t
{
    VkImage     image;
    VkImageView view;
    VkSemaphore semaphore;
} dm_vulkan_swapchain_image;

typedef struct dm_vulkan_depth_image_t
{
    VkImage       image;
    VkImageView   view;
    VmaAllocation allocation;
} dm_vulkan_depth_image;

typedef struct dm_vulkan_swapchain_t
{
    VkSwapchainKHR swapchain;
    VkFormat       format, depth_format;

    dm_vulkan_swapchain_image images[DM_SWAPCHAIN_MAX_IMAGES];
    dm_vulkan_depth_image depth_image;

    u16 width, height;
    u32 count, index;
} dm_vulkan_swapchain;

typedef struct dm_vulkan_frame_data_t
{
    VkCommandPool   gfx_pool, compute_pool, blit_pool;
    VkCommandBuffer gfx_cmd,  compute_cmd,  blit_cmd;
    VkSemaphore     semaphore;
} dm_vulkan_frame_data;

typedef struct dm_vulkan_resource_descriptor_heap_t
{
    VkBuffer buffer;
    VmaAllocation allocation;

    size_t buffer_size, image_size;
    size_t image_offset;
    size_t buffer_count, image_count;
    size_t size, count;

    void *start;
} dm_vulkan_resource_descriptor_heap;

typedef struct dm_vulkan_sampler_descriptor_heap_t
{
    VkBuffer buffer;
    VmaAllocation allocation;

    size_t sampler_size, sampler_count;
    size_t size, count, min_size;

    void *start;
} dm_vulkan_sampler_descriptor_heap;

typedef struct dm_vulkan_image_t
{
    VkImage       image;
    VmaAllocation allocation;

    VkFormat format;
    VkDescriptorType type;
    VkImageUsageFlags usage;

    u32 width, height;

    u32 heap_index;

    void *heap_address;
} dm_vulkan_image;

typedef struct dm_vulkan_buffer_t
{
    VkBuffer      host, device;
    VmaAllocation host_alloc, device_alloc;

    size_t size, stride;
    u32    heap_index;

    dm_buffer_type type;
} dm_vulkan_buffer;

typedef struct dm_vulkan_render_target_t
{
    VkImage target;
    VmaAllocation alloc;
    VkImageView target_view;

    bool swapchain, depth;

    u32 heap_index;
    u16 width, height;
    void *heap_address; 
} dm_vulkan_render_target;

typedef struct dm_vulkan_sampler_t
{
    VkSamplerCreateInfo info;
    u32 heap_index;
} dm_vulkan_sampler;

#define DM_VULKAN_MAX_RESOURCES 10
typedef struct dm_vulkan_pipeline_t
{
    VkPipeline pipeline;

    u32 push_indices[DM_FRAMES_IN_FLIGHT][DM_VULKAN_MAX_RESOURCES];
} dm_vulkan_pipeline;

typedef struct dm_vulkan_semaphore_t
{
    VkSemaphore semaphore;
    u64 value;
} dm_vulkan_semaphore;

typedef struct dm_vulkan_renderer_t
{
    VkInstance       instance;
    VmaAllocator     allocator ;

    dm_vulkan_gpu        gpu;
    dm_vulkan_surface    surface;
    dm_vulkan_swapchain  swapchain;
    dm_vulkan_frame_data frame_data[DM_FRAMES_IN_FLIGHT];

    dm_vulkan_resource_descriptor_heap resource_heap;
    dm_vulkan_sampler_descriptor_heap  sampler_heap;

    VkCommandPool single_use_pool;

    u32 frame_index;

    // resources
    dm_vulkan_image images[DM_MAX_TEXTURES * DM_FRAMES_IN_FLIGHT];
    dm_vulkan_buffer buffers[DM_MAX_BUFFERS * DM_FRAMES_IN_FLIGHT]; 
    dm_vulkan_sampler samplers[DM_MAX_SAMPLERS * DM_FRAMES_IN_FLIGHT];
    dm_vulkan_render_target rts[DM_MAX_RENDER_TARGETS * DM_FRAMES_IN_FLIGHT];
    u32 image_count, buffer_count, sampler_count, rt_count;

    dm_vulkan_pipeline pipes[DM_MAX_PIPELINES];
    u32 pipe_count;

    dm_vulkan_semaphore semaphores[1 + DM_MAX_SYNCHRONIZATIONS * DM_FRAMES_IN_FLIGHT]; // 
    u32 semaphore_count;

    dm_resource gfx_semaphore, compute_semaphore;

    dm_pipeline active_pipeline;
} dm_vulkan_renderer;

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data)
{
    switch(severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE("%s", callback_data->pMessage);
            return VK_TRUE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO("%s", callback_data->pMessage);
            return VK_TRUE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARN("%s", callback_data->pMessage);
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR("%s", callback_data->pMessage);
            return VK_FALSE;

        default: return VK_FALSE;
    }
}
#endif

void dm_vulkan_wait_semaphore(dm_vulkan_renderer *renderer, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_SYNCHRONIZATION, "Not a sync resource");

    dm_vulkan_semaphore *semaphore = &renderer->semaphores[handle.index];

    u64 wait_value = semaphore->value;

    VkSemaphoreWaitInfo wait_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount=1,
        .pSemaphores=&semaphore->semaphore,
        .pValues=&wait_value
    };
    vkWaitSemaphores(renderer->gpu.device, &wait_info, UINT64_MAX);
}

void dm_vulkan_signal_semaphore(dm_vulkan_renderer *renderer, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_SYNCHRONIZATION, "Not a sync resource");

    dm_vulkan_semaphore *semaphore= &renderer->semaphores[handle.index];

    VkSemaphoreSignalInfo info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore=semaphore->semaphore,
        .value=++semaphore->value
    };
    vkSignalSemaphore(renderer->gpu.device, &info);
}

bool dm_vulkan_create_buffer(VmaAllocator allocator, dm_vulkan_gpu gpu, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits alloc_flags, VmaMemoryUsage alloc_usage, VkBuffer *buffer, VmaAllocation *allocation, size_t size)
{
    u32 queue_indices[] = {
        gpu.gfx_index,
        gpu.compute_index
    };

    VkBufferCreateInfo buffer_info = {
        .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size=size,
        .sharingMode=VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount=2,
        .pQueueFamilyIndices=queue_indices,
        .usage=usage
    };

    VmaAllocationCreateInfo alloc_info = {
        .flags=alloc_flags,
        .usage=alloc_usage
    };

    if(dm_vulkan_decode_vr(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, buffer, allocation, NULL))) return true;

    LOG_ERROR("vmaCreateBuffer failed");
    return false;
}

u64 dm_vulkan_get_buffer_address(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info = {
        .sType=VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer=buffer
    };

    return vkGetBufferDeviceAddress(device, &info);
}

VkInstance dm_vulkan_create_instance()
{
    VkInstance instance = VK_NULL_HANDLE;

#ifdef DM_DEBUG
    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const u32 layer_count = 1;
#endif

    char extensions[10][512] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#ifdef DM_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };

#ifdef DM_DEBUG
    u32 ext_count = 2;
#else
    u32 ext_count = 1;
#endif

    u32 window_ext_count;
    const char** window_exts = dm_window_get_vulkan_extensions(&window_ext_count);
    for(u32 i=0; i<window_ext_count; i++)
    {
        strcpy(extensions[i+ext_count], window_exts[i]);
    }
    ext_count += window_ext_count;

    const char* ext_ptr[10];
    for(u32 i=0; i<ext_count; i++)
    {
        ext_ptr[i] = extensions[i];
    }
    const char** exts = ext_ptr;

#ifdef DM_DEBUG
    VkDebugUtilsMessageTypeFlagBitsEXT types = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessageSeverityFlagsEXT severity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT dbg_info = {
        .sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageType=types,
        .messageSeverity=severity,
        .pfnUserCallback=dm_vk_debug_callback
    };
#endif

    VkApplicationInfo app_info = {
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion=VK_API_VERSION_1_4,
        .applicationVersion=VK_MAKE_VERSION(0, 1, 0),
        .engineVersion=VK_MAKE_VERSION(0, 1, 0),
        .pEngineName="DarkMatter",
        .pApplicationName="TestApp"
    };

    VkInstanceCreateInfo create_info = {
        .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo=&app_info,
        .enabledExtensionCount=ext_count,
        .ppEnabledExtensionNames=exts,
#ifdef DM_DEBUG
        .enabledLayerCount=layer_count,
        .ppEnabledLayerNames=layers,
        .pNext=&dbg_info
#endif
    };

    if(!dm_vulkan_decode_vr(vkCreateInstance(&create_info, NULL, &instance))) return VK_NULL_HANDLE;
    volkLoadInstance(instance);

    return instance;
}

bool dm_vulkan_check_physical_device(VkPhysicalDevice physical)
{
#ifdef DM_RAY_TRACE
    VkPhysicalDeviceRayQueryFeaturesKHR rq_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR
    };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR as_supported = {
       .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
       .pNext=&rq_supported
    };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipe_supported = {
       .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
       .pNext=&as_supported
    };
#endif
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
#ifdef DM_RAY_TRACE
        .pNext=&rt_pipe_supported
#endif // DM_RAY_TRACE
    };
    VkPhysicalDeviceDescriptorHeapFeaturesEXT heap_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
        .pNext=&untyped_supported
    };
    VkPhysicalDeviceVulkan14Features v14_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext=&heap_supported
    };
    VkPhysicalDeviceVulkan13Features v13_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext=&v14_supported
    };
    VkPhysicalDeviceVulkan12Features v12_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext=&v13_supported
    };
    VkPhysicalDeviceFeatures2 supported_features2 = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext=&v12_supported
    };
    vkGetPhysicalDeviceFeatures2(physical, &supported_features2);

#ifdef DM_RAY_TRACE
    if(!rt_pipe_supported.rayTracingPipeline) { LOG_ERROR("Raytracing pipeline not supported"); return false; }
#endif

    if(!untyped_supported.shaderUntypedPointers) { LOG_ERROR("Untyped shader ptrs not supported"); return false; }
    if(!heap_supported.descriptorHeap)           { LOG_ERROR("Descriptor heaps not supported"); return false; }
    if(!heap_supported.descriptorHeapCaptureReplay) { LOG_ERROR("Descriptor heap replay not supported"); return false; }
    if(!v14_supported.pushDescriptor)    { LOG_ERROR("Push descriptor not supported");     return false; }
    if(!v13_supported.dynamicRendering)  { LOG_ERROR("Dynamic rendering not supported");   return false; }
    if(!v13_supported.synchronization2)  { LOG_ERROR("Synchronization2 not supported");    return false; }
    if(!v12_supported.timelineSemaphore) { LOG_ERROR("Timeline semaphores not supported"); return false; }
    if(!v12_supported.descriptorIndexing) { LOG_ERROR("Descriptor indexing not supported"); return false; }

    return true;
}

VkPhysicalDevice dm_vulkan_create_physical_device(VkInstance instance)
{
    VkPhysicalDevice device = VK_NULL_HANDLE;

    u32 count;
    VkPhysicalDevice devices[10] = { 0 };

    vkEnumeratePhysicalDevices(instance, &count, NULL);
    vkEnumeratePhysicalDevices(instance, &count, devices);

    VkPhysicalDeviceProperties props;

    for(u32 i=0; i<count; i++)
    {
        vkGetPhysicalDeviceProperties(devices[i], &props);

        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            device = devices[i];
            break;
        }
    }
    if(device == VK_NULL_HANDLE)
    {
        LOG_WARN("No dedicated gpu found, looking for integrated gpu...");


        for(u32 i=0; i<count; i++)
        {
            vkGetPhysicalDeviceProperties(devices[i], &props);

            if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                device = devices[i];
                break;
            }
        }
    }

    if(device == VK_NULL_HANDLE) return VK_NULL_HANDLE;

    if(!dm_vulkan_check_physical_device(device))
    {
        LOG_ERROR("Device is not adequate.");
        return VK_NULL_HANDLE;
    }

    LOG_INFO("GPU: %s", props.deviceName);
    LOG_INFO("API: %d.%d.%d", VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion));

    return device;
}

u32 dm_vulkan_find_graphics_queue(VkPhysicalDevice physical, VkSurfaceKHR surface, VkQueueFamilyProperties *props, u32 queue_count)
{
    u32 index = UINT32_MAX;
    bool found = false;
    VkBool32 has_present = VK_FALSE;
    for(u32 i=0; i<queue_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &has_present);
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && has_present)
        {
            index = i;
            found = true;
            break;
        }
    }

    return index;
}

u32 dm_vulkan_find_compute_queue(VkPhysicalDevice physical, VkQueueFamilyProperties *props, u32 queue_count)
{
    u32 index = UINT32_MAX;

    for(u32 i=0; i<queue_count; i++)
    {
        VkQueueFlags flags = props[i].queueFlags;

        if(flags & VK_QUEUE_COMPUTE_BIT && !(flags & VK_QUEUE_GRAPHICS_BIT))
        {
            index = i;
            break;
        }
    }

    return index;
}

VkDevice dm_vulkan_create_device(VkInstance instance, VkPhysicalDevice physical_device, VkSurfaceKHR surface, u32 gfx_index, u32 compute_index)
{
    VkDevice device = VK_NULL_HANDLE;

    // queues
    float priorities[] = { 1.f };

    VkDeviceQueueCreateInfo gfx_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,
        .queueFamilyIndex=gfx_index,
        .pQueuePriorities=priorities
    };
    VkDeviceQueueCreateInfo compute_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,
        .queueFamilyIndex=compute_index,
        .pQueuePriorities=priorities
    };

    VkDeviceQueueCreateInfo queues[] = {
        gfx_info, compute_info
    };

    // features
#ifdef DM_RAY_TRACE
    VkPhysicalDeviceAccelerationStructureFeaturesKHR as_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure=1,
        .descriptorBindingAccelerationStructureUpdateAfterBind=1,
#ifdef DM_DEBUG
        .accelerationStructureCaptureReplay=1,
#endif // DM_DEBUG
    };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipe_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline=1,
        .pNext=&as_features
    };
#endif // DM_RAY_TRACE
       //
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped_ptr = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
        .shaderUntypedPointers=1,
#ifdef DM_RAY_TRACE
        .pNext=&rt_pipe_features
#endif // DM_RAY_TRACE
    };
    VkPhysicalDeviceDescriptorHeapFeaturesEXT heap_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
        .pNext=&untyped_ptr,
        .descriptorHeap=1,
        .descriptorHeapCaptureReplay=1
    };
       //
    VkPhysicalDeviceVulkan14Features v14_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext=&heap_features,
        .pushDescriptor=1,
        .dynamicRenderingLocalRead=1
    };

    VkPhysicalDeviceVulkan13Features v13_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext=&v14_features,
        .dynamicRendering=1,
        .synchronization2=1
    };
    VkPhysicalDeviceVulkan12Features v12_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext=&v13_features,
        .bufferDeviceAddress=1,
        .timelineSemaphore=1,
        .shaderStorageBufferArrayNonUniformIndexing=1,
        .shaderSampledImageArrayNonUniformIndexing=1,
        .shaderStorageImageArrayNonUniformIndexing=1,
        .shaderUniformBufferArrayNonUniformIndexing=1,
        .runtimeDescriptorArray=1,
        .descriptorIndexing=1,
        .scalarBlockLayout=1
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext=&v12_features,
        .features.shaderInt64=1
    };

    const char* extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#ifdef DM_RAY_TRACE
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
#endif // DM_RAY_TRACE
    };
    u32 ext_count = 5;
#ifdef DM_RAY_TRACE
    ext_count += 4;
#endif // DM_RAY_TRACE

#ifdef DM_DEBUG
    VkExtensionProperties ext_props[500] = { 0 };
    u32 ext_prop_count;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_prop_count, NULL);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_prop_count, ext_props);

    LOG_TRACE("%u", ext_prop_count);

    for(u32 i=0; i<ext_count; i++)
    {
        LOG_INFO("Searching for extension %s...", extensions[i]);

        bool found = false;
        for(u32 j=0; j<ext_prop_count; j++)
        {
            if(strcmp(extensions[i], ext_props[j].extensionName)!=0) continue;

            LOG_INFO("Found.");
            found = true;
            break;
        }

        if(!found)
        {
            LOG_FATAL("Extension %s not supported", extensions[i]);
            return VK_NULL_HANDLE;
        }
    }
#endif

    //
    VkDeviceCreateInfo create_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount=2,
        .pQueueCreateInfos=queues,
        .pNext=&features2,
        .enabledExtensionCount=ext_count,
        .ppEnabledExtensionNames=extensions
    };

    if(dm_vulkan_decode_vr(vkCreateDevice(physical_device, &create_info, NULL, &device))) return device;

    LOG_ERROR("vkCreateDevice failed");
    return VK_NULL_HANDLE;
}

dm_vulkan_gpu dm_vulkan_create_gpu(VkInstance instance, dm_vulkan_surface surface)
{
    dm_vulkan_gpu gpu = { 0 };

    VkPhysicalDevice physical = dm_vulkan_create_physical_device(instance);
    if(physical == VK_NULL_HANDLE) { LOG_ERROR("Could not create physical device."); return gpu; }

    u32 queue_count;
    VkQueueFamilyProperties props[10] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, props);

#ifdef DM_DEBUG
    for(u32 i=0; i<queue_count; i++)
    {
        VkQueueFamilyProperties prop = props[i];

        LOG_INFO("Queue %u", i);
        if(prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            LOG_INFO("    GRAPHICS");
        if(prop.queueFlags & VK_QUEUE_COMPUTE_BIT)
            LOG_INFO("    COMPUTE");
        if(prop.queueFlags & VK_QUEUE_TRANSFER_BIT)
            LOG_INFO("    TRANSFER");
    }
#endif

    u32 gfx_index = dm_vulkan_find_graphics_queue(physical, surface.surface, props, queue_count);
    if(gfx_index == UINT32_MAX) { LOG_ERROR("Could not find graphics queue."); return gpu; }
    LOG_DEBUG("Graphics queue index: %u", gfx_index);

    u32 compute_index = dm_vulkan_find_compute_queue(physical, props, queue_count);
    if(compute_index == UINT32_MAX) { LOG_ERROR("Could not find compute queue."); return gpu; }
    LOG_DEBUG("Compute queue index: %u", compute_index);

    VkDevice device = dm_vulkan_create_device(instance, physical, surface.surface, gfx_index, compute_index);
    if(device == VK_NULL_HANDLE) { LOG_ERROR("Could not create device."); return gpu; }

    gpu.physical       = physical;
    gpu.device         = device;
    gpu.gfx_index      = gfx_index;
    gpu.compute_index  = compute_index;

    vkGetPhysicalDeviceFeatures(physical, &gpu.features);
    vkGetPhysicalDeviceProperties(physical, &gpu.properties);

    gpu.heap_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
    gpu.props2.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    gpu.props2.pNext     = &gpu.heap_props;
    vkGetPhysicalDeviceProperties2(physical, &gpu.props2);

    return gpu;
}

VmaAllocator create_vma_allocator(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device)
{
    VmaAllocator allocator = VK_NULL_HANDLE;

    VmaVulkanFunctions functions = { 0 };

    VmaAllocatorCreateInfo info = {
        .instance=instance,
        .physicalDevice=physical_device,
        .device=device,
        .flags=VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .vulkanApiVersion=VK_API_VERSION_1_4,
        .pVulkanFunctions=&functions
    };

    if(!dm_vulkan_decode_vr(vmaImportVulkanFunctionsFromVolk(&info, &functions))) { LOG_ERROR("vmaImportVulkanFunctionsFromVolk failed"); return VK_NULL_HANDLE; }
    LOG_DEBUG("HERE");

    if(dm_vulkan_decode_vr(vmaCreateAllocator(&info, &allocator))) return allocator;

    LOG_ERROR("vmaCreateAllocator failed");
    return VK_NULL_HANDLE;
}

VkSwapchainKHR dm_vulkan_create_vk_swap(dm_vulkan_gpu gpu, dm_vulkan_surface surface)
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    VkSwapchainCreateInfoKHR info = {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface=surface.surface,
        .imageFormat=DM_SWAPCHAIN_FORMAT,
        .imageColorSpace=VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .minImageCount=surface.capabilities.minImageCount,
        .imageExtent.width=surface.capabilities.currentExtent.width,
        .imageExtent.height=surface.capabilities.currentExtent.height,
        .imageArrayLayers=1,
        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=VK_PRESENT_MODE_FIFO_KHR,
    };

    if(!dm_vulkan_decode_vr(vkCreateSwapchainKHR(gpu.device, &info, NULL, &swapchain)))
    { 
        LOG_ERROR("vkCreateSwapchainKHR failed"); 
        return VK_NULL_HANDLE; 
    }

    return swapchain;
}

u32 dm_vulkan_get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkImage* images)
{
    u32 count = UINT32_MAX;

    if(!dm_vulkan_decode_vr(vkGetSwapchainImagesKHR(device, swapchain, &count, NULL)))   return UINT32_MAX;
    if(!dm_vulkan_decode_vr(vkGetSwapchainImagesKHR(device, swapchain, &count, images))) return UINT32_MAX;
    if(count >= DM_SWAPCHAIN_MAX_IMAGES) { LOG_ERROR("Swapchain image count greater than compile time max."); return UINT32_MAX; }

    return count;
}

dm_vulkan_swapchain_image dm_vulkan_create_swapchain_image(VkDevice device, VkImage* images, u32 index)
{
    dm_vulkan_swapchain_image image = { 0 };

    VkImageView view = VK_NULL_HANDLE;

    VkImageViewCreateInfo info = { 
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image=images[index],
        .format=DM_SWAPCHAIN_FORMAT,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    if(!dm_vulkan_decode_vr(vkCreateImageView(device, &info, NULL, &view)))
    {
        LOG_ERROR("vkCreateImageView failed");
        return image;
    }

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphore_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    if(!dm_vulkan_decode_vr(vkCreateSemaphore(device, &semaphore_info, NULL, &semaphore)))
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return image;
    }

    image.image     = images[index];
    image.view      = view;
    image.semaphore = semaphore;

    return image;
}

dm_vulkan_depth_image dm_vulkan_create_depth_image(dm_vulkan_gpu gpu, VmaAllocator allocator, dm_vulkan_surface surface)
{
    dm_vulkan_depth_image image = { 0 };

    VkImage       vk_image = VK_NULL_HANDLE;
    VkImageView   vk_view = VK_NULL_HANDLE;
    VmaAllocation vk_alloc = VK_NULL_HANDLE;

    VkImageCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format=DM_DEPTH_FORMAT,
        .imageType=VK_IMAGE_TYPE_2D,
        .usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .extent.width=surface.capabilities.currentExtent.width,
        .extent.height=surface.capabilities.currentExtent.height,
        .extent.depth=1,
        .mipLevels=1,
        .arrayLayers=1
    };

    VmaAllocationCreateInfo alloc_info = {
        .flags=VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage=VMA_MEMORY_USAGE_AUTO
    };

    if(!dm_vulkan_decode_vr(vmaCreateImage(allocator, &info, &alloc_info, &vk_image, &vk_alloc, NULL)))
    {
        LOG_ERROR("vmaCreateImage failed");
        return image;
    }

    VkImageViewCreateInfo depth_view_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image=vk_image,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=DM_DEPTH_FORMAT,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    if(!dm_vulkan_decode_vr(vkCreateImageView(gpu.device, &depth_view_info, NULL, &vk_view)))
    {
        LOG_ERROR("vkCreateImageView failed");
        return image;
    }

    image.image      = vk_image;
    image.view       = vk_view;
    image.allocation = vk_alloc;

    return image;
}

dm_vulkan_swapchain dm_vulkan_create_swapchain(dm_vulkan_gpu gpu, dm_vulkan_surface surface, VmaAllocator allocator)
{
    dm_vulkan_swapchain swapchain = { 0 };

    VkSwapchainKHR vk_swap = dm_vulkan_create_vk_swap(gpu, surface);
    if(vk_swap == VK_NULL_HANDLE) 
    { 
        LOG_ERROR("Could not create vulkan swapchain.");
        return swapchain; 
    }

    VkImage vk_images[DM_SWAPCHAIN_MAX_IMAGES] = { 0 };

    u32 image_count = dm_vulkan_get_swapchain_images(gpu.device, vk_swap, vk_images);
    if(image_count == UINT32_MAX) 
    { 
        LOG_ERROR("Could not get swapchain images.");
        return swapchain; 
    }

    dm_vulkan_swapchain_image images[DM_SWAPCHAIN_MAX_IMAGES] = { 0 };
    for(u32 i=0; i<image_count; i++)
    {
        images[i] = dm_vulkan_create_swapchain_image(gpu.device, vk_images, i);
        if(images[i].image == VK_NULL_HANDLE) 
        { 
            LOG_ERROR("Could not create swapchain image.");
            return swapchain; 
        }
    }

    dm_vulkan_depth_image depth_image = dm_vulkan_create_depth_image(gpu, allocator, surface);
    if(depth_image.image == VK_NULL_HANDLE) 
    { 
        LOG_ERROR("Could not create depth image.");
        return swapchain; 
    }

    // assign
    swapchain.swapchain    = vk_swap;
    swapchain.format       = DM_SWAPCHAIN_FORMAT;
    swapchain.depth_format = DM_DEPTH_FORMAT;
    swapchain.width        = surface.capabilities.currentExtent.width;
    swapchain.height       = surface.capabilities.currentExtent.height;

    for(u32 i=0; i<image_count; i++)
    {
        swapchain.images[i] = images[i];
    }
    swapchain.count     = image_count;

    swapchain.depth_image = depth_image;

    return swapchain;
}

void dm_vulkan_destroy_swapchain(dm_vulkan_swapchain* swapchain, dm_vulkan_gpu gpu, VmaAllocator allocator)
{
    vkDeviceWaitIdle(gpu.device);

    for(u32 i=0; i<swapchain->count; i++)
    {
        vkDestroyImageView(gpu.device, swapchain->images[i].view, NULL);
        vkDestroySemaphore(gpu.device, swapchain->images[i].semaphore, NULL);
    }

    vkDestroySwapchainKHR(gpu.device, swapchain->swapchain, NULL);

    dm_vulkan_depth_image depth_image = swapchain->depth_image;
    vkDestroyImageView(gpu.device, depth_image.view, NULL);
    vmaDestroyImage(allocator, depth_image.image, depth_image.allocation);
}

dm_vulkan_frame_data dm_vulkan_create_frame_data(dm_vulkan_gpu gpu)
{
    dm_vulkan_frame_data data = { 0 };

    VkCommandPool gfx_pool   = VK_NULL_HANDLE;
    VkCommandPool cpte_pool  = VK_NULL_HANDLE;
    VkCommandPool blit_pool  = VK_NULL_HANDLE;
    VkCommandBuffer gfx_cmd  = VK_NULL_HANDLE;
    VkCommandBuffer cp_cmd   = VK_NULL_HANDLE;
    VkCommandBuffer blit_cmd = VK_NULL_HANDLE;
    VkSemaphore semaphore    = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo pool_info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex=gpu.gfx_index
    };

    if(!dm_vulkan_decode_vr(vkCreateCommandPool(gpu.device, &pool_info, NULL, &gfx_pool)))
    {
        LOG_ERROR("vkCreateCommandPool");
        return data;
    }
    if(!dm_vulkan_decode_vr(vkCreateCommandPool(gpu.device, &pool_info, NULL, &blit_pool)))
    {
        LOG_ERROR("vkCreateCommandPool");
        return data;
    }

    VkCommandBufferAllocateInfo cmd_info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool=gfx_pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=1
    };

    if(!dm_vulkan_decode_vr(vkAllocateCommandBuffers(gpu.device, &cmd_info, &gfx_cmd)))
    {
        LOG_ERROR("vkAllocateCommandBuffers failed");
        return data;
    }
    cmd_info.commandPool=blit_pool;
    if(!dm_vulkan_decode_vr(vkAllocateCommandBuffers(gpu.device, &cmd_info, &blit_cmd)))
    {
        LOG_ERROR("vkAllocateCommandBuffers failed");
        return data;
    }

    pool_info.queueFamilyIndex = gpu.compute_index;

    if(!dm_vulkan_decode_vr(vkCreateCommandPool(gpu.device, &pool_info, NULL, &cpte_pool)))
    {
        LOG_ERROR("vkCreateCommandPool");
        return data;
    }

    cmd_info.commandPool = cpte_pool;
    if(!dm_vulkan_decode_vr(vkAllocateCommandBuffers(gpu.device, &cmd_info, &cp_cmd)))
    {
        LOG_ERROR("vkAllocateCommandBuffers failed");
        return data;
    }

    //
    VkSemaphoreCreateInfo semaphore_info = { 
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    if(!dm_vulkan_decode_vr(vkCreateSemaphore(gpu.device, &semaphore_info, NULL, &semaphore)))
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return data;
    }

    // assign
    data.gfx_pool     = gfx_pool;
    data.compute_pool = cpte_pool;
    data.blit_pool    = blit_pool;
    data.gfx_cmd      = gfx_cmd;
    data.compute_cmd  = cp_cmd;
    data.blit_cmd     = blit_cmd;
    data.semaphore    = semaphore;

    return data;
}

VkCommandPool dm_vulkan_create_single_use_pool(dm_vulkan_gpu gpu)
{
    VkCommandPool pool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex=gpu.gfx_index
    };

    if(!dm_vulkan_decode_vr(vkCreateCommandPool(gpu.device, &info, NULL, &pool)))
    {
        LOG_ERROR("vkCreateCommandPool");
        return VK_NULL_HANDLE;
    }

    return pool;
}

dm_vulkan_semaphore dm_vulkan_create_semaphore(dm_vulkan_gpu gpu, u64 value)
{
    dm_vulkan_semaphore sync = { 0 };

    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkSemaphoreTypeCreateInfo type_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue=value
    };
    VkSemaphoreCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=&type_info
    };
    if(!dm_vulkan_decode_vr(vkCreateSemaphore(gpu.device, &info, NULL, &semaphore)))
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return sync;
    }

    sync.semaphore = semaphore;
    sync.value     = value;

    return sync;
}

dm_vulkan_resource_descriptor_heap dm_vulkan_create_resource_heap(VkDevice device, VmaAllocator allocator, dm_vulkan_gpu gpu)
{
    dm_vulkan_resource_descriptor_heap heap = { 0 };

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void *start = NULL;

    size_t size = 0;
    size_t buffer_size, image_offset, image_size;

    buffer_size = DM_ALIGN(gpu.heap_props.bufferDescriptorSize, gpu.heap_props.bufferDescriptorAlignment);
    image_offset = DM_ALIGN((buffer_size * DM_MAX_BUFFERS * DM_FRAMES_IN_FLIGHT), gpu.heap_props.imageDescriptorSize);
    image_size = DM_ALIGN(gpu.heap_props.imageDescriptorSize, gpu.heap_props.imageDescriptorAlignment);
    LOG_DEBUG("Buffer descriptor size: %zu", buffer_size);
    LOG_DEBUG("Buffer descriptor heap alignment: %zu", gpu.heap_props.bufferDescriptorAlignment);
    LOG_DEBUG("Image descriptor size: %zu", image_size);
    LOG_DEBUG("Image descriptor heap alignment: %zu", gpu.heap_props.imageDescriptorAlignment);
    LOG_DEBUG("Image offset: %zu", image_offset);
    LOG_DEBUG("Heap max push data size: %zu", gpu.heap_props.maxPushDataSize);

    size += image_offset;
    size += DM_MAX_TEXTURES * image_size;
    size *= DM_FRAMES_IN_FLIGHT; // all are shoved in here
    size += gpu.heap_props.minResourceHeapReservedRange;
    size = DM_ALIGN(size, gpu.heap_props.resourceHeapAlignment);
    LOG_DEBUG("Resource heap size: %zu", size);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT;
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if(!dm_vulkan_create_buffer(allocator, gpu, usage, VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &buffer, &allocation, size)) return heap;

    if(!dm_vulkan_decode_vr(vmaMapMemory(allocator, allocation, &start)))
    {
        LOG_ERROR("vmaMapMemory failed");
        return heap;
    }

    //
    heap.buffer       = buffer;
    heap.allocation   = allocation;
    heap.start        = start;
    heap.size         = size;
    heap.buffer_size  = buffer_size;
    heap.image_size   = image_size;
    heap.image_offset = image_offset;

    return heap;
}

dm_vulkan_sampler_descriptor_heap dm_vulkan_create_sampler_descriptor_heap(VkDevice device, VmaAllocator allocator, dm_vulkan_gpu gpu)
{
    dm_vulkan_sampler_descriptor_heap heap = { 0 };

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void *start = NULL;

    size_t size = 0;
    size_t sampler_size;

    sampler_size = DM_ALIGN(gpu.heap_props.samplerDescriptorSize, gpu.heap_props.samplerDescriptorAlignment);

    size += sampler_size * DM_MAX_SAMPLERS;
    size += gpu.heap_props.minSamplerHeapReservedRange;
    size = DM_ALIGN(size, gpu.heap_props.samplerHeapAlignment);
    LOG_DEBUG("Sampler descriptor size: %u", sampler_size);
    LOG_DEBUG("Sampler descriptor heap alignment: %u", gpu.heap_props.samplerHeapAlignment);

    assert(size);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT;
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if(!dm_vulkan_create_buffer(allocator, gpu, usage, VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &buffer, &allocation, size)) return heap;

    if(!dm_vulkan_decode_vr(vmaMapMemory(allocator, allocation, &start)))
    {
        LOG_ERROR("vmaMapMemory failed");
        return heap;
    }

    //
    heap.buffer       = buffer;
    heap.allocation   = allocation;
    heap.start        = start;
    heap.size         = size;
    heap.sampler_size = sampler_size;

    return heap;
}

//
bool dm_renderer_init(dm_context* context)
{
    LOG_INFO("Initializing Vulkan backend...");

    VkInstance instance    = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;

    dm_vulkan_gpu gpu = { 0 };
    dm_vulkan_surface surface = { 0 };
    dm_vulkan_swapchain swapchain = { 0 };
    dm_vulkan_frame_data frame_data[DM_FRAMES_IN_FLIGHT] = { 0 };
    dm_vulkan_resource_descriptor_heap resource_heap = { 0 };
    dm_vulkan_sampler_descriptor_heap  sampler_heap = { 0 };
    dm_vulkan_semaphore gfx_semapore = { 0 };
    dm_vulkan_semaphore compute_semaphore = { 0 };

    VkCommandPool single_use_pool    = VK_NULL_HANDLE;
    
    //
    if(volkInitialize() != VK_SUCCESS) return false;

    instance = dm_vulkan_create_instance();
    if(instance == VK_NULL_HANDLE) return false;

    surface.surface = dm_window_create_vulkan_surface(context, instance); 
    if(surface.surface == VK_NULL_HANDLE) { LOG_ERROR("Could not create Vulkan surface."); return false; }
    gpu = dm_vulkan_create_gpu(instance, surface);
    if(gpu.device == VK_NULL_HANDLE) { LOG_ERROR("Creating Vulkan GPU failed"); return false; }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physical, surface.surface, &surface.capabilities);

    volkLoadDevice(gpu.device);

    vkGetDeviceQueue(gpu.device, gpu.gfx_index, 0, &gpu.gfx_queue);
    vkGetDeviceQueue(gpu.device, gpu.compute_index, 0, &gpu.compute_queue);

    if(!gpu.gfx_queue)     return false;
    if(!gpu.compute_queue) return false;

    allocator = create_vma_allocator(instance, gpu.physical, gpu.device);
    if(allocator == VK_NULL_HANDLE) return false; 

    swapchain = dm_vulkan_create_swapchain(gpu, surface, allocator);
    if(swapchain.swapchain == VK_NULL_HANDLE) { LOG_ERROR("Could not create swapchain."); return false; }

    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        frame_data[i] = dm_vulkan_create_frame_data(gpu);
        if(frame_data[i].gfx_pool==VK_NULL_HANDLE || frame_data[i].compute_pool==VK_NULL_HANDLE)
        {
            LOG_ERROR("Could not create frame data for frame %u", i);
            return false;
        }
    }

    single_use_pool = dm_vulkan_create_single_use_pool(gpu);
    if(single_use_pool == VK_NULL_HANDLE) { LOG_ERROR("Could not create single use pool."); return false; }

    // timeline semaphore
    gfx_semapore = dm_vulkan_create_semaphore(gpu, DM_FRAMES_IN_FLIGHT-1);
    if(gfx_semapore.semaphore == VK_NULL_HANDLE) { LOG_ERROR("Could not create gfx semaphore."); return false; }
    compute_semaphore = dm_vulkan_create_semaphore(gpu, DM_FRAMES_IN_FLIGHT-1);
    if(compute_semaphore.semaphore==VK_NULL_HANDLE) { LOG_ERROR("Could not create compute semaphore"); return false; }

    // resource and smapler heaps
    resource_heap = dm_vulkan_create_resource_heap(gpu.device, allocator, gpu);
    if(resource_heap.buffer == VK_NULL_HANDLE) { LOG_ERROR("Could not create resource descriptor heap"); return false; }
    sampler_heap = dm_vulkan_create_sampler_descriptor_heap(gpu.device, allocator, gpu);
    if(sampler_heap.buffer == VK_NULL_HANDLE) { LOG_ERROR("Could not create sampler descriptor heap"); return false; }

    // assign
    context->renderer.internal_renderer = dm_arena_alloc(&context->arena, sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* renderer = context->renderer.internal_renderer;

    renderer->instance = instance;
    renderer->allocator = allocator;
    renderer->gpu = gpu;
    renderer->surface = surface;
    renderer->swapchain = swapchain;
    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        renderer->frame_data[i] = frame_data[i];
    }
    renderer->single_use_pool = single_use_pool;
    renderer->semaphores[renderer->semaphore_count++] = gfx_semapore;
    renderer->semaphores[renderer->semaphore_count++] = compute_semaphore;
    renderer->gfx_semaphore.index = 0;
    renderer->gfx_semaphore.type = DM_RESOURCE_TYPE_SYNCHRONIZATION;
    renderer->compute_semaphore.index = 1;
    renderer->compute_semaphore.type = DM_RESOURCE_TYPE_SYNCHRONIZATION;
    renderer->resource_heap = resource_heap;
    renderer->sampler_heap = sampler_heap;

    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_surface surface = renderer->surface;

    vkDeviceWaitIdle(gpu.device);

    // resources
    for(u32 i=0; i<renderer->pipe_count; i++)
    {
        vkDestroyPipeline(gpu.device, renderer->pipes[i].pipeline, NULL);
    }
    for(u32 i=0; i<renderer->buffer_count; i++)
    {
        vmaDestroyBuffer(renderer->allocator, renderer->buffers[i].host, renderer->buffers[i].host_alloc);
        vmaDestroyBuffer(renderer->allocator, renderer->buffers[i].device, renderer->buffers[i].device_alloc);
    }
    for(u32 i=0; i<renderer->image_count; i++)
    {
        vmaDestroyImage(renderer->allocator, renderer->images[i].image, renderer->images[i].allocation);
    }
    for(u32 i=0; i<renderer->rt_count; i++)
    {
        vmaDestroyImage(renderer->allocator, renderer->rts[i].target, renderer->rts[i].alloc);
        vkDestroyImageView(renderer->gpu.device, renderer->rts[i].target_view, NULL);
    }
    for(u32 i=0; i<renderer->semaphore_count; i++)
    {
        vkDestroySemaphore(renderer->gpu.device, renderer->semaphores[i].semaphore, NULL);
    }

    vmaUnmapMemory(renderer->allocator, renderer->resource_heap.allocation);
    vmaUnmapMemory(renderer->allocator, renderer->sampler_heap.allocation);
    vmaDestroyBuffer(renderer->allocator, renderer->resource_heap.buffer, renderer->resource_heap.allocation);
    vmaDestroyBuffer(renderer->allocator, renderer->sampler_heap.buffer, renderer->sampler_heap.allocation);

    vkDestroyCommandPool(gpu.device, renderer->single_use_pool, NULL);
    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyCommandPool(gpu.device, renderer->frame_data[i].gfx_pool, NULL);
        vkDestroyCommandPool(gpu.device, renderer->frame_data[i].compute_pool, NULL);
        vkDestroyCommandPool(gpu.device, renderer->frame_data[i].blit_pool, NULL);
        vkDestroySemaphore(gpu.device, renderer->frame_data[i].semaphore, NULL);
    }

    dm_vulkan_destroy_swapchain(&renderer->swapchain, gpu, renderer->allocator);

    vkDestroySurfaceKHR(renderer->instance, surface.surface, NULL);
    vmaDestroyAllocator(renderer->allocator);
    vkDestroyDevice(gpu.device, NULL);
    vkDestroyInstance(renderer->instance, NULL);

    volkFinalize();
}

bool dm_renderer_resize(dm_context *context, u16 width, u16 height)
{
#ifdef DM_DEBUG
    LOG_WARN("Renderer resized: %u %u", width, height);
#endif

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];
    dm_vulkan_surface *surface = &renderer->surface;

    vkDeviceWaitIdle(gpu.device);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physical, surface->surface, &surface->capabilities);

    dm_vulkan_destroy_swapchain(&renderer->swapchain, gpu, renderer->allocator);
    dm_vulkan_swapchain new_swapchain = dm_vulkan_create_swapchain(gpu, renderer->surface, renderer->allocator);
    if(new_swapchain.swapchain == VK_NULL_HANDLE)
    {
        LOG_ERROR("Failed to recreate swapchain");
        return false;
    }

    //
    context->flags |= DM_CONTEXT_FLAG_RENDERER_RESIZED;
    context->renderer.width = width;
    context->renderer.height = height;
    
    renderer->swapchain = new_swapchain;
    renderer->swapchain.width = width;
    renderer->swapchain.height = height;

    return true;
}

size_t dm_renderer_get_internal_size()
{
    return sizeof(dm_vulkan_renderer);
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_swapchain swapchain = renderer->swapchain;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_wait_semaphore(renderer, renderer->gfx_semaphore);

    vkResetCommandPool(gpu.device, frame_data.gfx_pool, 0);

    VkResult vr = vkAcquireNextImageKHR(gpu.device, swapchain.swapchain, UINT64_MAX, frame_data.semaphore, VK_NULL_HANDLE, &swapchain.index);

    if(vr == VK_ERROR_OUT_OF_DATE_KHR)
    {
        context->flags |= DM_CONTEXT_FLAG_RENDERER_RESIZED;
    }
    else if(vr == VK_SUBOPTIMAL_KHR)
    {
        context->flags |= DM_CONTEXT_FLAG_RENDERER_RESIZED;
    }
    else if(vr != VK_SUCCESS)
    {
        dm_vulkan_decode_vr(vr);
        LOG_ERROR("vkAcquireNextImageKHR failed");
        return false;
    }

    dm_vulkan_swapchain_image image = swapchain.images[swapchain.index];

    VkCommandBufferBeginInfo cmd_begin = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(frame_data.gfx_cmd, &cmd_begin);

    // bind resource and sampler heaps
    dm_vulkan_resource_descriptor_heap resource_heap = renderer->resource_heap;
    dm_vulkan_sampler_descriptor_heap  sampler_heap  = renderer->sampler_heap;

    VkBindHeapInfoEXT resource_info = {
        .sType=VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange.size=resource_heap.size,
        .heapRange.address=dm_vulkan_get_buffer_address(renderer->gpu.device, resource_heap.buffer),
        .reservedRangeOffset=resource_heap.size - renderer->gpu.heap_props.minResourceHeapReservedRange,
        .reservedRangeSize=renderer->gpu.heap_props.minResourceHeapReservedRange
    };

    VkBindHeapInfoEXT sampler_info = {
        .sType=VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange.size=sampler_heap.size,
        .heapRange.address=dm_vulkan_get_buffer_address(renderer->gpu.device, sampler_heap.buffer),
        .reservedRangeOffset=sampler_heap.size - renderer->gpu.heap_props.minSamplerHeapReservedRange,
        .reservedRangeSize=renderer->gpu.heap_props.minSamplerHeapReservedRange
    };

    vkCmdBindResourceHeapEXT(frame_data.gfx_cmd, &resource_info);
    vkCmdBindSamplerHeapEXT(frame_data.gfx_cmd, &sampler_info);

    //
    renderer->swapchain = swapchain;
    
    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];
    dm_vulkan_swapchain_image image = renderer->swapchain.images[renderer->swapchain.index];

    dm_vulkan_semaphore *timeline_semaphore = &renderer->semaphores[renderer->gfx_semaphore.index];

    VkImageMemoryBarrier2 present_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask=0,
        .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image=image.image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };
    VkDependencyInfo present_dep_info = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=1,
        .pImageMemoryBarriers=&present_barrier
    };
    vkCmdPipelineBarrier2(frame_data.gfx_cmd, &present_dep_info);

    vkEndCommandBuffer(frame_data.gfx_cmd);

    VkSemaphoreSubmitInfo semaphore_wait_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore=frame_data.semaphore,
        .stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
    };

    VkSemaphoreSubmitInfo signal_semaphores[] = {
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore=image.semaphore,
            .stageMask=VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        },
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore=timeline_semaphore->semaphore,
            .stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .value=++timeline_semaphore->value
        },
    };

    VkCommandBufferSubmitInfo gfx_cmd_submit = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer=frame_data.gfx_cmd
    };

    VkSubmitInfo2 submit = {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount=1,
        .pCommandBufferInfos=&gfx_cmd_submit,
        .waitSemaphoreInfoCount=1,
        .pWaitSemaphoreInfos=&semaphore_wait_info,
        .signalSemaphoreInfoCount=2,
        .pSignalSemaphoreInfos=signal_semaphores
    };
    vkQueueSubmit2(gpu.gfx_queue, 1, &submit, NULL);

    VkPresentInfoKHR present_info = {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount=1,
        .pSwapchains=&renderer->swapchain.swapchain,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&image.semaphore,
        .pImageIndices=&renderer->swapchain.index
    };

    vkQueuePresentKHR(gpu.gfx_queue, &present_info);

    //
    renderer->frame_index++;
    renderer->frame_index %= DM_FRAMES_IN_FLIGHT;
    context->renderer.current_frame = renderer->frame_index;

    renderer->active_pipeline.type = DM_PIPELINE_TYPE_INVALID;

    return true;
}

// resources
VkShaderModule dm_vulkan_create_shader_module(dm_vulkan_gpu gpu, const char *path, const char *entry, shaderc_shader_kind kind)
{
    LOG_INFO("Creating shader module from file %s with entry %s", path, entry);
    VkShaderModule module = VK_NULL_HANDLE;

    size_t file_size;
    void *file_data = dm_read_bytes(path, &file_size);
    if(!file_data) 
    {
        LOG_ERROR("Could not read file: %s", path);
        return VK_NULL_HANDLE;
    }

    shaderc_compiler_t compiler       = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compilation_result_t result;

    shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
    shaderc_compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero);
    shaderc_compile_options_set_generate_debug_info(options);

    result = shaderc_compile_into_spv(compiler, file_data, file_size, kind, path, entry, options);
    if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
    {
        LOG_ERROR("Could not compile shader \'%s\'", path);
        LOG_ERROR("%s", shaderc_result_get_error_message(result));
        return VK_NULL_HANDLE;
    }
    free(file_data);

    assert(shaderc_result_get_length(result) % 4 == 0);

    VkShaderModuleCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize=shaderc_result_get_length(result),
        .pCode=(u32*)shaderc_result_get_bytes(result)
    };

    LOG_DEBUG("Shader size: %u bytes", info.codeSize);

    if(!dm_vulkan_decode_vr(vkCreateShaderModule(gpu.device, &info, NULL, &module)))
    {
        LOG_ERROR("vkCreateShaderModule failed");
        return VK_NULL_HANDLE;
    }

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    return module;
}

VkBlendOp dm_convert_blend_op(dm_blend_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported blend op");
            LOG_WARN("Returning VK_BLEND_OP_ADD");
        case DM_BLEND_OP_ADD: return VK_BLEND_OP_ADD;
        case DM_BLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
        case DM_BLEND_OP_MIN: return VK_BLEND_OP_MIN;
        case DM_BLEND_OP_MAX: return VK_BLEND_OP_MAX;
    }
}

VkBlendFactor dm_convert_blend_factor(dm_blend_factor factor)
{
    switch(factor)
    {
        default:
            LOG_WARN("Unknown/unsupported blend factor");
            LOG_WARN("Returning VK_BLEND_FACTOR_ZERO");
        case DM_BLEND_FACTOR_ZERO:      return VK_BLEND_FACTOR_ZERO;
        case DM_BLEND_FACTOR_ONE:       return VK_BLEND_FACTOR_ONE;
        case DM_BLEND_FACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case DM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }
}

VkFrontFace dm_vulkan_convert_winding(dm_winding_order winding)
{
    switch(winding)
    {
        default:
            LOG_WARN("Unknown/unsupported winding order");
            LOG_WARN("Returning VK_FRONT_FACE_COUNTER_CLOCKWISE");
        case DM_WINDING_COUNTERCLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case DM_WINDING_CLOCKWISE:        return VK_FRONT_FACE_CLOCKWISE;
    }
}

VkPolygonMode dm_vulkan_convert_fill_mode(dm_fill_mode fill)
{
    switch(fill)
    {
        default:
            LOG_WARN("Unknown/unsupported fill mode");
            LOG_WARN("Returning VK_POLYGON_MODE_FILL");
        case DM_FILL_FULL: return VK_POLYGON_MODE_FILL;
        case DM_FILL_LINES: return VK_POLYGON_MODE_LINE;
    }
}

VkCullModeFlagBits dm_vulkan_convert_culling(dm_cull_mode culling)
{
    switch(culling)
    {
        default:
            LOG_WARN("Unknown/unsupported culling");
            LOG_WARN("Returning VK_CULL_MODE_NONE_BIT");
        case DM_CULL_NONE: return VK_CULL_MODE_NONE;
        case DM_CULL_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case DM_CULL_BACK:  return VK_CULL_MODE_BACK_BIT;
    }
}

bool dm_renderer_create_raster_pipeline(dm_context* context, dm_raster_pipe_desc desc, dm_pipeline *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_pipeline pipe = { 0 };

    dm_raster_shader vertex_shader = desc.shaders[DM_RASTER_SHADER_STAGE_VERTEX];
    dm_raster_shader fragment_shader = desc.shaders[DM_RASTER_SHADER_STAGE_FRAGMENT];

    char vertex_path[512];
    sprintf(vertex_path, "%s.glsl", vertex_shader.path);
    char fragment_path[512];
    sprintf(fragment_path, "%s.glsl", fragment_shader.path);

    VkShaderModule vertex_module = dm_vulkan_create_shader_module(renderer->gpu, vertex_path, "main", shaderc_vertex_shader);
    if(vertex_module == VK_NULL_HANDLE)
    {
        LOG_ERROR("Could not create vertex shader module");
        return false;
    }
    VkShaderModule fragment_module = dm_vulkan_create_shader_module(renderer->gpu, fragment_path, "main", shaderc_fragment_shader);
    if(fragment_module == VK_NULL_HANDLE)
    {
        LOG_ERROR("Could not create fragment shader module");
        return false;
    }

    VkPipelineShaderStageCreateInfo vertex_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module=vertex_module,
        .stage=VK_SHADER_STAGE_VERTEX_BIT,
        .pName="main"
    };
    VkPipelineShaderStageCreateInfo fragment_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module=fragment_module,
        .stage=VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName="main"
    };
    VkPipelineShaderStageCreateInfo shader_info[] = {
        vertex_info,
        fragment_info
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineDepthStencilStateCreateInfo depth_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable=VK_TRUE,
        .depthWriteEnable=VK_TRUE,
        .depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL,
        .stencilTestEnable=VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount=1,
        .scissorCount=1
    };

    VkPipelineRasterizationStateCreateInfo raster_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode=dm_vulkan_convert_fill_mode(desc.fill),
        .cullMode=dm_vulkan_convert_culling(desc.culling),
        .frontFace=dm_vulkan_convert_winding(desc.winding),
        .lineWidth=1.f
    };

    VkPipelineMultisampleStateCreateInfo multi_sample_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples=VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState color_attachment_info = {
        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT | 
            VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT,
        .blendEnable=desc.blend,
        .colorBlendOp=desc.blend ? dm_convert_blend_op(desc.color_blend_op) : 0,
        .srcColorBlendFactor=desc.blend ? dm_convert_blend_factor(desc.color_src_factor) : 0,
        .dstColorBlendFactor=desc.blend ? dm_convert_blend_factor(desc.color_dst_factor) : 0,
        .alphaBlendOp=desc.blend ? dm_convert_blend_op(desc.alpha_blend_op) : 0,
        .srcAlphaBlendFactor=desc.blend ? dm_convert_blend_factor(desc.alpha_src_factor) : 0,
        .dstAlphaBlendFactor=desc.blend ? dm_convert_blend_factor(desc.alpha_dst_factor) : 0
    };

    VkPipelineColorBlendStateCreateInfo color_blend_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount=1,
        .pAttachments=&color_attachment_info
    };

    VkDynamicState dynamic_state[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount=2,
        .pDynamicStates=dynamic_state
    };

    VkPipelineCreateFlags2CreateInfo flags2 = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .flags=VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };

    VkPipelineRenderingCreateInfo render_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount=1,
        .pColorAttachmentFormats=&renderer->swapchain.format,
        .depthAttachmentFormat=renderer->swapchain.depth_format,
        .pNext=&flags2
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount=2,
        .pStages=shader_info,
        .pVertexInputState=&vertex_input_info,
        .pInputAssemblyState=&input_assembly_info,
        .pRasterizationState=&raster_info,
        .pMultisampleState=&multi_sample_info,
        .pDepthStencilState=&depth_info,
        .pColorBlendState=&color_blend_info,
        .pDynamicState=&dynamic_state_info,
        .pViewportState=&viewport_info,
        .pNext=&render_info,
    };

    if(!dm_vulkan_decode_vr(vkCreateGraphicsPipelines(renderer->gpu.device, NULL, 1, &pipeline_info, NULL, &pipe.pipeline)))
    {
        LOG_ERROR("vkCreateGraphicsPipelines failed");
        return false;
    }

    //
    vkDestroyShaderModule(renderer->gpu.device, vertex_module, NULL);
    vkDestroyShaderModule(renderer->gpu.device, fragment_module, NULL);

    //
    renderer->pipes[renderer->pipe_count] = pipe;
    handle->index = renderer->pipe_count++;
    handle->type = DM_PIPELINE_TYPE_RASTER;

    return true;
}

VkAttachmentLoadOp dm_vulkan_load_op_convert(dm_render_load_op op)
{
    switch(op)
    {
        case DM_RENDER_LOAD_OP_LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case DM_RENDER_LOAD_OP_CLEAR: 
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case DM_RENDER_LOAD_OP_DONT_CARE: 
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

        default:
            LOG_ERROR("Unknown/unsupported load op");
            return VK_ATTACHMENT_LOAD_OP_NONE_KHR;
    }
}

VkAttachmentStoreOp dm_vulkan_store_op_convert(dm_render_store_op op)
{
    switch(op)
    {
        case DM_RENDER_STORE_OP_STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case DM_RENDER_STORE_OP_DONT_CARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;

        default:
            LOG_ERROR("Unknown/unsupported store op");
            return VK_ATTACHMENT_STORE_OP_NONE;
    }
}

bool dm_vulkan_create_image(VmaAllocator allocator, dm_vulkan_gpu gpu, VkImageUsageFlags usage, VkFormat format, u16 width, u16 height, VkImage *image, VmaAllocation *allocation)
{
    u32 queue_indices[] = {
        gpu.gfx_index, gpu.compute_index
    };

    VkImageCreateInfo image_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType=VK_IMAGE_TYPE_2D,
        .format=format,
        .extent.width=width,
        .extent.height=height,
        .extent.depth=1,
        .mipLevels=1,
        .arrayLayers=1,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .usage=usage,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .sharingMode=VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount=2,
        .pQueueFamilyIndices=queue_indices
    };
    VmaAllocationCreateInfo alloc_info = {
        .usage=VMA_MEMORY_USAGE_AUTO,
    };

    if(dm_vulkan_decode_vr(vmaCreateImage(allocator, &image_info, &alloc_info, image, allocation, NULL))) return true;

    LOG_ERROR("vmaCreateImage failed");
    return false;
}

bool dm_renderer_create_render_target(dm_context* context, dm_render_target_desc desc, dm_resource *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_render_target target = { 
        .swapchain=desc.swapchain,
        .depth=desc.depth,
        .width=desc.color_attachment.width,
        .height=desc.color_attachment.height
    };

    if(!desc.swapchain)
    {
        const u16 width = desc.color_attachment.width;
        const u16 height = desc.color_attachment.height;

        VkImageUsageFlags usage = 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT;

        if(!dm_vulkan_create_image(renderer->allocator, renderer->gpu, usage, DM_SWAPCHAIN_FORMAT, width, height, &target.target, &target.alloc)) return false;

        VkImageViewCreateInfo view_info = {
            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType=VK_IMAGE_VIEW_TYPE_2D,
            .image=target.target,
            .format=DM_SWAPCHAIN_FORMAT,
            .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.layerCount=1,
            .subresourceRange.levelCount=1
        };

        if(!dm_vulkan_decode_vr(vkCreateImageView(renderer->gpu.device, &view_info, NULL, &target.target_view))) return false;
    }

    //
    renderer->rts[renderer->rt_count] = target;
    handle->index = renderer->rt_count++;
    handle->type = DM_RESOURCE_TYPE_RENDER_TARGET;

    return true;
}

VkCommandBuffer dm_vulkan_one_time_cmd(VkDevice device, VkCommandPool pool)
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo cmd_alloc = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount=1,
        .commandPool=pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
    if(!dm_vulkan_decode_vr(vkAllocateCommandBuffers(device, &cmd_alloc, &cmd)))
    {
        LOG_ERROR("vkAllocateCommandBuffers failed");
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    return cmd;
}

void dm_vulkan_submit_one_time_cmd(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer cmd)
{
    VkSubmitInfo submit_info = {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount=1,
        .pCommandBuffers=&cmd
    };

    vkEndCommandBuffer(cmd);
    vkQueueSubmit(queue, 1, &submit_info, NULL);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
}

bool dm_vulkan_copy_to_buffer(VmaAllocator allocator, dm_vulkan_buffer buffer, void *data, size_t size, size_t offset)
{
    char* buffer_ptr = NULL;
    if(!dm_vulkan_decode_vr(vmaMapMemory(allocator, buffer.host_alloc, (void**)&buffer_ptr)))
    {
        LOG_ERROR("vmaMapMemory failed");
        buffer_ptr = NULL;
        return false;
    }
    buffer_ptr += offset;
    memcpy(buffer_ptr, data, size);
    vmaUnmapMemory(allocator, buffer.host_alloc);

    buffer_ptr = NULL;

    return true;
}

bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_resource *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    if(renderer->buffer_count >= DM_MAX_BUFFERS)
    {
        LOG_ERROR("Trying to create too many bufers");
        LOG_ERROR("Increase compile time limit");
        return false;
    }

    dm_vulkan_buffer buffer = { .type=desc.type, .stride=desc.stride };

    VkBufferUsageFlags host_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateFlags host_flags = 
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | 
        VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaMemoryUsage host_mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBufferUsageFlagBits device_usage = 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateFlags device_flags = 0;
    VmaMemoryUsage           device_mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;

    switch(desc.type)
    {
        case DM_BUFFER_TYPE_VERTEX:
            device_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case DM_BUFFER_TYPE_INDEX:
            device_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case DM_BUFFER_TYPE_STORAGE:
            break;

        default:
            LOG_ERROR("Unknown/unsupported buffer type");
            return false;
    }

    if(!dm_vulkan_create_buffer(renderer->allocator, renderer->gpu, host_usage, host_flags, host_mem_usage, &buffer.host, &buffer.host_alloc, desc.size)) return false;
    if(!dm_vulkan_create_buffer(renderer->allocator, renderer->gpu, device_usage, device_flags, device_mem_usage, &buffer.device, &buffer.device_alloc, desc.size)) return false;

    // copy over data if needed
    if(desc.data)
    {
        if(!dm_vulkan_copy_to_buffer(renderer->allocator, buffer, desc.data, desc.size, 0)) return false;

        VkCommandBuffer cmd = dm_vulkan_one_time_cmd(renderer->gpu.device, renderer->single_use_pool);

        VkBufferCopy2 region_info = {
            .sType=VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .size=desc.size
        };

        VkCopyBufferInfo2 copy_info = {
            .sType=VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .srcBuffer=buffer.host,
            .dstBuffer=buffer.device,
            .regionCount=1,
            .pRegions=&region_info
        };

        vkCmdCopyBuffer2(cmd, &copy_info);

        dm_vulkan_submit_one_time_cmd(renderer->gpu.device, renderer->gpu.gfx_queue, renderer->single_use_pool, cmd);
    }

    buffer.size = desc.size;

    //
    renderer->buffers[renderer->buffer_count] = buffer;
    handle->type = DM_RESOURCE_TYPE_BUFFER;
    handle->index  = renderer->buffer_count++;

    return true;
}

u64 dm_renderer_get_buffer_address(dm_context *context, dm_resource handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_buffer buffer = renderer->buffers[handle.index];

    return dm_vulkan_get_buffer_address(renderer->gpu.device, buffer.device);
}

void dm_vulkan_copy_buffer_to_image(dm_vulkan_gpu gpu, VkCommandPool pool, VkImage image, VkBuffer buffer, u16 x, u16 y, u16 width, u16 height)
{
    VkCommandBuffer cmd = dm_vulkan_one_time_cmd(gpu.device, pool);

    // transition image to transfer dst
    VkImageMemoryBarrier2 dst_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask=VK_ACCESS_2_NONE,
        .dstStageMask=VK_PIPELINE_STAGE_2_COPY_BIT,
        .dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image=image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };
    VkDependencyInfo dst_dep = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=1,
        .pImageMemoryBarriers=&dst_barrier
    };
    vkCmdPipelineBarrier2(cmd, &dst_dep);

    // copy from buffer to texture
    VkBufferImageCopy image_copy = {
        .imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.layerCount=1,
        .imageExtent.width=width,
        .imageExtent.height=height,
        .imageExtent.depth=1,
        .imageOffset.x=x,
        .imageOffset.y=y,
    };

    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

    // transition to read/sample
    VkImageMemoryBarrier2 post_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        .dstAccessMask=VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image=image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1,
    };
    VkDependencyInfo post_dep = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=1,
        .pImageMemoryBarriers=&post_barrier
    };
    vkCmdPipelineBarrier2(cmd, &post_dep);

    dm_vulkan_submit_one_time_cmd(gpu.device, gpu.gfx_queue, pool, cmd);
}

VkFormat dm_vulkan_convert_format(dm_texture2d_format format)
{
    switch(format)
    {
        default:
            LOG_WARN("Unknown/unsupported format.");
            LOG_WARN("Returning VK_FORMAT_R8G8B8A8_UNORM");
        case DM_TEXTURE2D_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case DM_TEXTURE2D_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case DM_TEXTURE2D_FORMAT_A8_UNORM:       return VK_FORMAT_A8_UNORM;
    }
}

bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_resource *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    if(renderer->image_count >= DM_MAX_TEXTURES)
    {
        LOG_ERROR("Trying to create too many textures");
        LOG_ERROR("Increase compile time limit");
        return false;
    }

    dm_vulkan_image image = { 0 };

    VkImageUsageFlags usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VmaMemoryUsage    alloc_usage = VMA_MEMORY_USAGE_AUTO;

    switch(desc.type)
    {
        case DM_TEXTURE2D_TYPE_COMBINED_SAMPLER:
            image.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case DM_TEXTURE2D_TYPE_SAMPLED:
            image.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case DM_TEXTURE2D_TYPE_STORAGE:
            image.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            break;

        default:
            LOG_ERROR("Unknown/unsupported image usage");
            return false;
    }

    image.format = dm_vulkan_convert_format(desc.format);
    size_t size = desc.width * desc.height;
    switch(image.format)
    {
        default: break;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            size *= 4;
            break;
    }

    if(!dm_vulkan_create_image(renderer->allocator, renderer->gpu, usage, image.format, desc.width, desc.height, &image.image, &image.allocation)) return false;

    image.usage  = usage;

    if(desc.data)
    {
        dm_vulkan_buffer staging_buffer = { .size=size };

        VkBufferUsageFlags buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if(!dm_vulkan_create_buffer(renderer->allocator, renderer->gpu, buffer_usage, 0, VMA_MEMORY_USAGE_CPU_TO_GPU, &staging_buffer.host, &staging_buffer.host_alloc, size)) return false;

        if(!dm_vulkan_copy_to_buffer(renderer->allocator, staging_buffer, desc.data, size, 0)) return false;

        dm_vulkan_copy_buffer_to_image(renderer->gpu, renderer->single_use_pool, image.image, staging_buffer.host, 0,0, desc.width, desc.height);

        vmaDestroyBuffer(renderer->allocator, staging_buffer.host, staging_buffer.host_alloc);
    }

    image.width  = desc.width;
    image.height = desc.height;

    // 
    renderer->images[renderer->image_count] = image;
    handle->type = DM_RESOURCE_TYPE_TEXTURE;
    handle->index  = renderer->image_count++;

    return true;
}

bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_resource *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    if(renderer->sampler_count >= DM_MAX_SAMPLERS)
    {
        LOG_ERROR("Trying to create too many samplers");
        LOG_ERROR("Increase compile time limit");
        return false;
    }

    VkSamplerCreateInfo info = {
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .maxLod       = VK_LOD_CLAMP_NONE,
    };

    renderer->samplers[renderer->sampler_count].info = info;
    handle->type = DM_RESOURCE_TYPE_SAMPLER;
    handle->index = renderer->sampler_count++;

    return true;
}

bool dm_renderer_create_synchronization(dm_context *context, dm_synchronization_desc desc, dm_resource *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_semaphore semaphore = dm_vulkan_create_semaphore(renderer->gpu, desc.value);
    if(semaphore.semaphore == VK_NULL_HANDLE) return false;

    renderer->semaphores[renderer->semaphore_count] = semaphore;
    handle->index = renderer->semaphore_count++;
    handle->type  = DM_RESOURCE_TYPE_SYNCHRONIZATION;

    return true;
}

bool dm_vulkan_upload_buffer_to_heap(dm_vulkan_renderer *renderer, dm_resource resource)
{
    dm_vulkan_resource_descriptor_heap heap = renderer->resource_heap;
    dm_vulkan_buffer buffer = renderer->buffers[resource.index];

    VkDeviceAddressRangeKHR address = {
        .address=dm_vulkan_get_buffer_address(renderer->gpu.device, buffer.device),
        .size=buffer.size
    };

    VkResourceDescriptorInfoEXT descriptor = {
        .sType=VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .data.pAddressRange=&address
    };

    size_t buffer_offset = heap.buffer_count * heap.buffer_size;

    VkHostAddressRangeEXT host_range = {
        .address=heap.start + buffer_offset,
        .size=heap.buffer_size
    };

    return dm_vulkan_decode_vr(vkWriteResourceDescriptorsEXT(renderer->gpu.device, 1, &descriptor, &host_range));
}

bool dm_vulkan_upload_image_to_heap(dm_vulkan_renderer *renderer, dm_resource resource)
{
    dm_vulkan_resource_descriptor_heap heap = renderer->resource_heap;
    dm_vulkan_image image = renderer->images[resource.index];
    assert(image.type != VK_DESCRIPTOR_TYPE_SAMPLER);

    VkImageViewCreateInfo image_view = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .image=image.image,
        .format=image.format,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    VkImageDescriptorInfoEXT image_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .pView=&image_view
    };

    VkResourceDescriptorInfoEXT descriptor = {
        .sType=VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type=image.type,
        .data.pImage=&image_info
    };

    size_t image_offset = heap.image_offset + heap.image_count * heap.image_size;

    VkHostAddressRangeEXT host_range = {
        .address=heap.start + image_offset,
        .size=heap.image_size
    };

    return dm_vulkan_decode_vr(vkWriteResourceDescriptorsEXT(renderer->gpu.device, 1, &descriptor, &host_range));
}

bool dm_vulkan_upload_render_target_to_heap(dm_vulkan_renderer *renderer, dm_resource resource)
{
    dm_vulkan_resource_descriptor_heap heap = renderer->resource_heap;
    dm_vulkan_render_target *target = &renderer->rts[resource.index];

    VkImageViewCreateInfo image_view = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .image=target->target,
        .format=DM_SWAPCHAIN_FORMAT,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    VkImageDescriptorInfoEXT image_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .pView=&image_view
    };

    VkResourceDescriptorInfoEXT descriptor = {
        .sType=VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data.pImage=&image_info
    };

    size_t image_offset = heap.image_offset + heap.image_count * heap.image_size;

    VkHostAddressRangeEXT host_range = {
        .address=heap.start + image_offset,
        .size=heap.image_size
    };

    target->heap_address = host_range.address;

    return dm_vulkan_decode_vr(vkWriteResourceDescriptorsEXT(renderer->gpu.device, 1, &descriptor, &host_range));
}

bool dm_vulkan_upload_sampler_to_heap(dm_vulkan_renderer *renderer, dm_resource resource)
{
    dm_vulkan_sampler_descriptor_heap heap = renderer->sampler_heap;

    size_t sampler_offset = heap.sampler_count * heap.sampler_size;
    VkHostAddressRangeEXT host_range = {
        .address=heap.start + sampler_offset,
        .size=heap.sampler_size
    };

    return dm_vulkan_decode_vr(vkWriteSamplerDescriptorsEXT(renderer->gpu.device, 1, &renderer->samplers[resource.index].info, &host_range));
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_resource *resources[], u32 count)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_resource_descriptor_heap *resource_heap = &renderer->resource_heap;
    dm_vulkan_sampler_descriptor_heap  *sampler_heap  = &renderer->sampler_heap;

    const size_t image_offset   = resource_heap->image_offset + resource_heap->image_count * resource_heap->image_size;
    const size_t image_index_offset = image_offset / gpu.heap_props.imageDescriptorSize;

    for(u32 i=0; i<count; i++)
    {
        dm_resource *resource = resources[i];

        switch(resource->type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                if(!dm_vulkan_upload_buffer_to_heap(renderer, *resource)) return false;
                renderer->buffers[resource->index].heap_index = resource_heap->buffer_count++;
                resource_heap->count++;
                break;

            case DM_RESOURCE_TYPE_TEXTURE:
                if(!dm_vulkan_upload_image_to_heap(renderer, *resource)) return false;
                renderer->images[resource->index].heap_index  = image_index_offset; 
                renderer->images[resource->index].heap_index += resource_heap->image_count++;
                resource_heap->count++;
                break;

            case DM_RESOURCE_TYPE_RENDER_TARGET:
                if(!dm_vulkan_upload_render_target_to_heap(renderer, *resource)) return false;
                renderer->rts[resource->index].heap_index  = image_index_offset;
                renderer->rts[resource->index].heap_index += resource_heap->image_count++;
                resource_heap->count++;
                break;

            case DM_RESOURCE_TYPE_SAMPLER:
                if(!dm_vulkan_upload_sampler_to_heap(renderer, *resource)) return false;
                renderer->samplers[resource->index].heap_index = sampler_heap->sampler_count++;
                sampler_heap->count++;
                break;

            case DM_RESOURCE_TYPE_INVALID:
                LOG_ERROR("Invalid resource");
                return false;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                LOG_ERROR("Upload resource to heap failed");
                return false;
        }
    }

    return true;
}

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_resource handle, float r, float g, float b, float a, float d, dm_render_load_op color_load, dm_render_store_op color_store, dm_render_load_op depth_load, dm_render_store_op depth_store)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Invalid render target");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_render_target target = renderer->rts[handle.index];

    VkImage     color_image; 
    VkImageView color_view;
    VkImage     depth_image = renderer->swapchain.depth_image.image;
    VkImageView depth_view  = renderer->swapchain.depth_image.view;

    u16 width, height;

    if(target.swapchain)
    {
        color_image = renderer->swapchain.images[renderer->swapchain.index].image;
        color_view  = renderer->swapchain.images[renderer->swapchain.index].view;

        width = renderer->swapchain.width;
        height = renderer->swapchain.height;
    }
    else
    {
        color_image = target.target;
        color_view  = target.target_view;

        width = target.width;
        height = target.height;
    }

    VkImageMemoryBarrier2 color_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask=0,
        .dstStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image=color_image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount=1,
        .subresourceRange.layerCount=1
    };

    VkImageMemoryBarrier2 depth_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask=VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .dstAccessMask=VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .image=depth_image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.levelCount=1,
        .subresourceRange.layerCount=1
    };

    VkImageMemoryBarrier2 barriers[] = {
        color_barrier,
        depth_barrier
    };

    VkDependencyInfo dep_info = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=2,
        .pImageMemoryBarriers=barriers
    };
    vkCmdPipelineBarrier2(frame_data.gfx_cmd, &dep_info);

    // attachments
    VkRenderingAttachmentInfo color_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .imageView=color_view,
        .loadOp=dm_vulkan_load_op_convert(color_load),
        .storeOp=dm_vulkan_store_op_convert(color_store),
        .clearValue.color.float32[0]=r,
        .clearValue.color.float32[1]=g,
        .clearValue.color.float32[2]=b,
        .clearValue.color.float32[3]=a,
    };

    VkRenderingAttachmentInfo depth_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .imageView=depth_view,
        .loadOp=dm_vulkan_load_op_convert(depth_load),
        .storeOp=dm_vulkan_store_op_convert(depth_store),
        .clearValue.depthStencil.depth=d
    };
    VkRenderingInfo render_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
        .colorAttachmentCount=1,
        .pColorAttachments=&color_info,
        .pDepthAttachment=target.depth ? &depth_info : NULL,
        .layerCount=1,
        .renderArea.extent.width=width,
        .renderArea.extent.height=height
    };
    vkCmdBeginRendering(frame_data.gfx_cmd, &render_info);
}

void dm_render_command_set_viewport(dm_context *context, int x, int y, int w, int h, float d_min, float d_max)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    VkViewport viewport = {
        .x=x, .y=y+h,
        .width=w,
        .height=-h,
        .minDepth=d_min,
        .maxDepth=d_max,
    };

    vkCmdSetViewport(frame_data.gfx_cmd, 0,1, &viewport);
}

void dm_render_command_set_scissor(dm_context *context, int x, int y, int w, int h)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    VkRect2D scissor = {
        .offset.x=x, .offset.y=y,
        .extent.width=w,
        .extent.height=h
    };

    vkCmdSetScissor(frame_data.gfx_cmd, 0,1, &scissor);
}

void dm_render_command_end_rendering(dm_context *context, dm_resource handle)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Invalid render target");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_render_target target = renderer->rts[handle.index];

    vkCmdEndRendering(frame_data.gfx_cmd);

    // if we aren't swapchain, move image to shader read 
    if(target.swapchain) return;
    
    VkImageMemoryBarrier2 color_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image=target.target,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount=1,
        .subresourceRange.layerCount=1
    };

    VkDependencyInfo dep_info = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=1,
        .pImageMemoryBarriers=&color_barrier
    };
    vkCmdPipelineBarrier2(frame_data.gfx_cmd, &dep_info);
}

void dm_render_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    DM_ASSERT(handle.type==DM_PIPELINE_TYPE_RASTER, "Invalid raster pipeline");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_pipeline pipeline = renderer->pipes[handle.index];

    VkPipelineBindPoint bind_point;

    switch(handle.type)
    {
        case DM_PIPELINE_TYPE_RASTER:
            bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
            break;
        case DM_PIPELINE_TYPE_COMPUTE:
            bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
            break;
#ifdef DM_RAY_TRACE
        case DM_PIPELINE_TYPE_RAY_TRACE:
            bind_point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            break;
#endif

        default:
            LOG_ERROR("Unknown/unsupported pipeline type");
            return;
    }

    vkCmdBindPipeline(frame_data.gfx_cmd, bind_point, pipeline.pipeline);

    renderer->active_pipeline = handle;
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_resource handle, size_t offset)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Invalid buffer");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_buffer buffer = renderer->buffers[handle.index];

    DM_ASSERT(buffer.type==DM_BUFFER_TYPE_INDEX, "Not an index buffer");

    VkIndexType index_type;

    switch(buffer.stride)
    {
        default:
        case sizeof(u32): index_type = VK_INDEX_TYPE_UINT32; break;
        case sizeof(u16): index_type = VK_INDEX_TYPE_UINT16; break;
        case sizeof(u8):  index_type = VK_INDEX_TYPE_UINT8;  break;
    }

    vkCmdBindIndexBuffer(frame_data.gfx_cmd, buffer.device, offset, index_type);
}

void dm_render_command_push_resources(dm_context *context, dm_resource *resources, u32 count)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_pipeline.type!=DM_PIPELINE_TYPE_INVALID, "No active pipeline");
    
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    if(sizeof(u32) * count >= renderer->gpu.heap_props.maxPushDataSize)
    {
        LOG_ERROR("Trying to push data of size %u when max is size is", sizeof(u32) * count, renderer->gpu.heap_props.maxPushDataSize);
        return;
    }

    dm_vulkan_pipeline *pipeline = &renderer->pipes[renderer->active_pipeline.index];

    for(u32 i=0; i<count; i++)
    {
        dm_resource resource = resources[i];

        switch(resource.type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                pipeline->push_indices[renderer->frame_index][i] = renderer->buffers[resource.index].heap_index;
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                pipeline->push_indices[renderer->frame_index][i] = renderer->images[resource.index].heap_index;
                break;
            case DM_RESOURCE_TYPE_RENDER_TARGET:
                pipeline->push_indices[renderer->frame_index][i] = renderer->rts[resource.index].heap_index;
                break;
            case DM_RESOURCE_TYPE_SAMPLER:
                pipeline->push_indices[renderer->frame_index][i] = renderer->samplers[resource.index].heap_index;
                break;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                return;
        }
    }

    VkPushDataInfoEXT info = {
        .sType=VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .data.address=&pipeline->push_indices[renderer->frame_index],
        .data.size=sizeof(u32) * count
    };

    vkCmdPushDataEXT(frame_data.gfx_cmd, &info);
}

void dm_render_command_draw(dm_context *context, u32 index_count, u32 index_offset, u32 instance_count, u32 vertex_offset)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    vkCmdDrawIndexed(frame_data.gfx_cmd, index_count, instance_count, index_offset, vertex_offset, 0);
}

void dm_render_command_update_buffer(dm_context *context, dm_resource handle, void *data, size_t size, size_t offset)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_BUFFER, "Invalid buffer");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_buffer buffer = renderer->buffers[handle.index];

    dm_vulkan_copy_to_buffer(renderer->allocator, buffer, data, size, offset);

    //VkCommandBuffer cmd = dm_vulkan_one_time_cmd(renderer->gpu.device, renderer->single_use_pool);

    VkBufferCopy2 region_info = {
        .sType=VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .size=size,
        .srcOffset=offset,
        .dstOffset=offset,
    };

    VkCopyBufferInfo2 copy_info = {
        .sType=VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .srcBuffer=buffer.host,
        .dstBuffer=buffer.device,
        .regionCount=1,
        .pRegions=&region_info,
    };

    vkCmdCopyBuffer2(frame_data.blit_cmd, &copy_info);

    //dm_vulkan_submit_one_time_cmd(renderer->gpu.device, renderer->gpu.gfx_queue, renderer->single_use_pool, cmd);
}

bool dm_render_command_update_texture(dm_context *context, dm_resource handle, void* data, u16 x, u16 y, u16 width, u16 height)
{
    DM_ASSERT(handle.type==DM_RESOURCE_TYPE_TEXTURE, "Invalid texture");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_vulkan_image *image = &renderer->images[handle.index];

    VkDeviceSize bytes_per_pixel;
    switch(image->format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            bytes_per_pixel = 4;
            break;
        case VK_FORMAT_A8_UNORM:
            bytes_per_pixel = 1;
            break;
        default: return false;
    }
    VkDeviceSize row_size = width * bytes_per_pixel;
    VkDeviceSize upload_size = height * row_size;

    dm_vulkan_buffer staging_buffer = { .size=upload_size};

    VkBufferUsageFlags buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if(!dm_vulkan_create_buffer(renderer->allocator, renderer->gpu, buffer_usage, 0, VMA_MEMORY_USAGE_CPU_TO_GPU, &staging_buffer.host, &staging_buffer.host_alloc, upload_size)) return false;

    if(!dm_vulkan_copy_to_buffer(renderer->allocator, staging_buffer, data, upload_size, 0)) return false;

    dm_vulkan_copy_buffer_to_image(renderer->gpu, renderer->single_use_pool, image->image, staging_buffer.host, x,y, width, height);

    vmaDestroyBuffer(renderer->allocator, staging_buffer.host, staging_buffer.host_alloc);

    return true;
}

bool dm_render_command_resize_render_target(dm_context *context, dm_resource resource, u16 width, u16 height)
{
    DM_ASSERT(resource.type==DM_RESOURCE_TYPE_RENDER_TARGET, "Invalid render target");

    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_render_target *target = &renderer->rts[resource.index];

    vmaDestroyImage(renderer->allocator, target->target, target->alloc);
    vkDestroyImageView(renderer->gpu.device, target->target_view, NULL);

    VkImageUsageFlags usage = 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;

    if(!dm_vulkan_create_image(renderer->allocator, renderer->gpu, usage, DM_SWAPCHAIN_FORMAT, width, height, &target->target, &target->alloc)) return false;

    VkImageViewCreateInfo view_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .image=target->target,
        .format=DM_SWAPCHAIN_FORMAT,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    if(!dm_vulkan_decode_vr(vkCreateImageView(renderer->gpu.device, &view_info, NULL, &target->target_view))) return false;

    VkImageViewCreateInfo heap_view = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image=target->target,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=DM_SWAPCHAIN_FORMAT,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    VkImageDescriptorInfoEXT image_descriptor = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .layout=VK_IMAGE_LAYOUT_GENERAL,
        .pView=&heap_view,
    };

    VkResourceDescriptorInfoEXT resource_descriptor = {
        .sType=VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data.pImage=&image_descriptor
    };

    VkHostAddressRangeEXT host_range = {
        .address=target->heap_address,
        .size=renderer->resource_heap.image_size
    };

    vkWriteResourceDescriptorsEXT(renderer->gpu.device, 1, &resource_descriptor, &host_range);

    target->width = width;
    target->height = height;

    return true;
}

void dm_render_command_signal(dm_context *context, dm_resource handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_signal_semaphore(renderer, handle);
}

void dm_render_command_wait(dm_context *context, dm_resource handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_wait_semaphore(renderer, handle);
}

void dm_render_command_update_begin(dm_context *context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    vkResetCommandPool(renderer->gpu.device, frame_data.blit_pool, 0);

    VkCommandBufferBeginInfo cmd_begin = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(frame_data.blit_cmd, &cmd_begin);
}

void dm_render_command_update_end(dm_context *context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    VkSubmitInfo submit_info = {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount=1,
        .pCommandBuffers=&frame_data.blit_cmd
    };

    vkEndCommandBuffer(frame_data.blit_cmd);
    vkQueueSubmit(renderer->gpu.gfx_queue, 1, &submit_info, NULL);
    vkQueueWaitIdle(renderer->gpu.gfx_queue);
}

/**********
 * COMPUTE
 ***********/
bool dm_renderer_create_compute_pipeline(dm_context *context, dm_compute_pipeline_desc desc, dm_pipeline *handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;

    dm_compute_shader shader = desc.shader;

    dm_vulkan_pipeline pipeline = { 0 };

    char shader_path[512];
    sprintf(shader_path, "%s.glsl", shader.path);

    VkShaderModule module = dm_vulkan_create_shader_module(renderer->gpu, shader_path, "main", shaderc_compute_shader);
    if(!module) return false;

    VkPipelineShaderStageCreateInfo shader_info = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage=VK_SHADER_STAGE_COMPUTE_BIT,
        .pName="main",
        .module=module
    };

    VkPipelineCreateFlags2CreateInfo flags2 = {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .flags=VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT
    };

    VkComputePipelineCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage=shader_info,
        .pNext=&flags2
    };

    if(!dm_vulkan_decode_vr(vkCreateComputePipelines(renderer->gpu.device, NULL, 1, &info, NULL, &pipeline.pipeline)))
    {
        LOG_ERROR("vkCreateComputePipelines failed");
        return false;
    }

    vkDestroyShaderModule(renderer->gpu.device, module, NULL);

    //
    renderer->pipes[renderer->pipe_count] = pipeline;
    handle->type = DM_PIPELINE_TYPE_COMPUTE;
    handle->index  = renderer->pipe_count++;

    return true;
}

void dm_compute_command_begin_recording(dm_context *context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];

    return;
    dm_vulkan_wait_semaphore(renderer, renderer->compute_semaphore);
    vkResetCommandPool(renderer->gpu.device, frame_data->compute_pool, 0);

    VkCommandBufferBeginInfo cmd_begin = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(frame_data->compute_cmd, &cmd_begin);

    VkBindHeapInfoEXT resource_info = {
        .sType=VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange.size=renderer->resource_heap.size,
        .heapRange.address=dm_vulkan_get_buffer_address(renderer->gpu.device, renderer->resource_heap.buffer),
        .reservedRangeOffset=renderer->resource_heap.size - renderer->gpu.heap_props.minResourceHeapReservedRange,
        .reservedRangeSize=renderer->gpu.heap_props.minResourceHeapReservedRange
    };

    vkCmdBindResourceHeapEXT(frame_data->compute_cmd, &resource_info);
}

void dm_compute_command_end_recording(dm_context *context)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data *frame_data = &renderer->frame_data[renderer->frame_index];
    dm_vulkan_semaphore *semaphore = &renderer->semaphores[renderer->compute_semaphore.index];

    return;
    vkEndCommandBuffer(frame_data->compute_cmd);

    VkCommandBufferSubmitInfo compute_cmd_submit = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer=frame_data->compute_cmd
    };

    VkSemaphoreSubmitInfo signal_semaphore = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore=semaphore->semaphore,
        .stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .value=++semaphore->value
    };

    VkSubmitInfo2 submit = {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount=1,
        .pCommandBufferInfos=&compute_cmd_submit,
        .signalSemaphoreInfoCount=1,
        .pSignalSemaphoreInfos=&signal_semaphore
    };
    vkQueueSubmit2(renderer->gpu.compute_queue, 1, &submit, NULL);
}

void dm_compute_command_bind_pipeline(dm_context *context, dm_pipeline handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_pipeline pipeline = renderer->pipes[handle.index];

    vkCmdBindPipeline(frame_data.gfx_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

    renderer->active_pipeline = handle;
}

void dm_compute_command_push_resources(dm_context *context, dm_resource *resources, u32 count)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    DM_ASSERT(renderer->active_pipeline.type==DM_PIPELINE_TYPE_COMPUTE, "Active pipeline is not compute");
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    dm_vulkan_pipeline *pipeline = &renderer->pipes[renderer->active_pipeline.index];

    for(u32 i=0; i<count; i++)
    {
        dm_resource resource = resources[i];

        switch(resource.type)
        {
            case DM_RESOURCE_TYPE_BUFFER:
                pipeline->push_indices[renderer->frame_index][i] = renderer->buffers[resource.index].heap_index;
                break;
            case DM_RESOURCE_TYPE_TEXTURE:
                pipeline->push_indices[renderer->frame_index][i] = renderer->images[resource.index].heap_index;
                break;
            case DM_RESOURCE_TYPE_RENDER_TARGET:
                pipeline->push_indices[renderer->frame_index][i] = renderer->rts[resource.index].heap_index;
                break;

            default:
                LOG_ERROR("Unknown/unsupported resource type");
                return;
        }
    }

    VkPushDataInfoEXT info = {
        .sType=VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .data.address=&pipeline->push_indices[renderer->frame_index],
        .data.size=sizeof(u32) * count
    };

    vkCmdPushDataEXT(frame_data.gfx_cmd, &info);
}

void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    vkCmdDispatch(frame_data.gfx_cmd, x,y,z);
}

void dm_compute_command_signal(dm_context *context, dm_resource handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_signal_semaphore(renderer, handle);
}

void dm_compute_command_wait(dm_context *context, dm_resource handle)
{
    dm_vulkan_renderer *renderer = context->renderer.internal_renderer;
    dm_vulkan_wait_semaphore(renderer, handle);
}

/************
 * DECODE VR
 *************/
bool dm_vulkan_decode_vr(VkResult vr)
{
    if(vr==VK_SUCCESS) return true;

    switch(vr)
    {
        case VK_ERROR_DEVICE_LOST:
            LOG_ERROR("Device lost");
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            LOG_ERROR("Feature not present");
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            LOG_ERROR("Extension not present");
            break;
        case VK_ERROR_VALIDATION_FAILED:
            LOG_ERROR("Validation failed");
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            LOG_ERROR("Layer not present");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            LOG_ERROR("Out of host memory");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            LOG_ERROR("Out of device memory");
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            LOG_ERROR("Surface lost");
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            LOG_ERROR("Too many objects");
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            LOG_ERROR("Initialization failed");
            break;
        default:
        case VK_ERROR_UNKNOWN:
            LOG_ERROR("Unknown");
            break;
    }

    return false;
}
