#include "dm.h"

#include <vulkan/vulkan.h>

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
    uint32_t x, y;
    uint32_t width, height;
    float    r, g, b, a;
    float    depth;
    uint32_t stencil;
} dm_vulkan_renderpass_desc;

typedef struct dm_vulkan_renderpass_t
{
    VkRenderPass               renderpass;
    dm_vulkan_renderpass_desc  desc;
    dm_vulkan_renderpass_state state;
} dm_vulkan_renderpass;

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

typedef struct dm_vulkan_framebuffer_desc_t
{
    uint32_t         width, height;
    uint32_t         attachment_count;
} dm_vulkan_framebuffer_desc;

typedef struct dm_vulkan_framebuffer_t
{
    dm_vulkan_framebuffer_desc desc;
    VkFramebuffer              framebuffer;
    VkImageView*               attachments;
    dm_vulkan_renderpass*      renderpass;
} dm_vulkan_framebuffer;

typedef struct dm_vulkan_pipeline_t
{
    VkPipeline       pipeline;
    VkPipelineLayout layout;
} dm_vulkan_pipeline;

#define DM_VULKAN_MAX_SHADER_STAGE_COUNT 2
typedef struct dm_vulkan_shader_t
{
    VkShaderModuleCreateInfo        module_create_infos[DM_VULKAN_MAX_SHADER_STAGE_COUNT];
    VkShaderModule                  modules[DM_VULKAN_MAX_SHADER_STAGE_COUNT];
    VkPipelineShaderStageCreateInfo stage_create_infos[DM_VULKAN_MAX_SHADER_STAGE_COUNT];
    
    uint32_t               stage_count;
} dm_vulkan_shader;

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
    char*                     device_extension_names[DM_VULKAN_DEVICE_MAX_EXTENSIONS];
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

#define DM_VULKAN_MAX_FRAMEBUFFER_COUNT 3
typedef struct dm_vulkan_swapchain_t
{
    uint32_t           max_frames_in_flight, image_count;
    VkSurfaceFormatKHR image_format;
    VkSwapchainKHR     handle;
    dm_vulkan_image    depth;
    VkImage*           images;
    VkImageView*       views;
    
    dm_vulkan_framebuffer framebuffers[DM_VULKAN_MAX_FRAMEBUFFER_COUNT];
    uint32_t              framebuffer_count;
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
    
    VkCommandPool                    graphics_command_pool;
    
    VkFormat depth_format;
    
    dm_vulkan_swapchain_support_info swapchain_support_info;
    
    uint32_t graphics_index, present_index, transfer_index, compute_index;
} dm_vulkan_device;

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

typedef struct dm_vulkan_fence_t
{
    VkFence fence;
    bool    signaled;
} dm_vulkan_fence;

typedef enum dm_vulkan_renderer_flag_t
{
    DM_VULKAN_RENDERER_FLAG_RECREATING_SWAPCHAIN = 1 << 0,
    DM_VULKAN_RENDERER_FLAG_RESIZED              = 1 << 1,
    DM_VULKAN_RENDERER_FLAG_UNKNOWN              = 1 << 2,
} dm_vulkan_renderer_flag;

#define DM_VULKAN_MAX_SEMAPHORE_COUNT 3
#define DM_VULKAN_MAX_FENCE_COUNT     3
typedef struct dm_vulkan_renderer_t
{
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR           surface;
    
    dm_vulkan_device       device;
    dm_vulkan_swapchain    swapchain;
    
    dm_vulkan_command_buffer* command_buffers;
    uint32_t                  command_buffer_count;
    
    VkSemaphore available_semaphores[DM_VULKAN_MAX_SEMAPHORE_COUNT];
    VkSemaphore complete_semaphores[DM_VULKAN_MAX_SEMAPHORE_COUNT];
    uint32_t    available_semaphore_count, complete_semaphore_count;
    
    dm_vulkan_fence  in_flight_fences[DM_VULKAN_MAX_FENCE_COUNT];
    dm_vulkan_fence* images_in_flight[DM_VULKAN_MAX_FENCE_COUNT];
    uint32_t fence_count;
    
    dm_vulkan_renderpass master_renderpass;
    
    uint32_t buffer_count, shader_count, texture_count, pipeline_count;
    uint32_t width, height;
    uint32_t image_index, current_frame;
    
    dm_vulkan_renderer_flag flags;
    
    dm_vulkan_shader      shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_vulkan_pipeline    pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
    
#ifdef DM_DEBUG
    VkDebugUtilsMessengerEXT  debug_messenger;
#endif
} dm_vulkan_renderer;

#define DM_VULKAN_GET_RENDERER dm_vulkan_renderer* vulkan_renderer = renderer->internal_renderer
#define DM_VULKAN_GET_COMMAND_BUFFER dm_vulkan_command_buffer* buffer = &vulkan_renderer->command_buffers[vulkan_renderer->image_index]

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

#define DM_VULKAN_FUNC_CHECK(FUNC_CALL) result = FUNC_CALL
#define DM_VULKAN_FUNC_SUCCESS() result==VK_SUCCESS
#else
#define DM_VULKAN_FUNC_CHECK(FUNC_CALL) FUNC_CALL
#define DM_VULKAN_FUNC_SUCCESS() 1
#endif

bool     dm_vulkan_device_detect_depth_buffer_range(dm_vulkan_device* device);
uint32_t dm_vulkan_device_find_memory_index(uint32_t type_filter, uint32_t property_flags, VkPhysicalDevice device);

extern bool dm_platform_create_vulkan_surface(dm_platform_data* platform_data, VkInstance* instance, VkSurfaceKHR* surface);

/****************
ENUM CONVERSIONS
******************/
VkAttachmentLoadOp dm_load_op_to_vulkan_load_op(dm_load_operation load_op)
{
    switch(load_op)
    {
        case DM_LOAD_OPERATION_LOAD:      return VK_ATTACHMENT_LOAD_OP_LOAD;
        case DM_LOAD_OPERATION_CLEAR:     return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case DM_LOAD_OPERATION_DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        
        default:
        DM_LOG_ERROR("Unknown load operation, shouldn't be here...");
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

VkAttachmentStoreOp dm_store_op_to_vulkan_store_op(dm_store_operation store_op)
{
    switch(store_op)
    {
        case DM_STORE_OPERATION_STORE:     return VK_ATTACHMENT_STORE_OP_STORE;
        case DM_STORE_OPERATION_DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        default:
        DM_LOG_ERROR("Unknown store operation, shouldn't be here...");
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

VkCullModeFlagBits dm_cull_mode_to_vulkan_cull_mode(dm_cull_mode cull_mode)
{
    switch(cull_mode)
    {
        case DM_CULL_FRONT:      return VK_CULL_MODE_FRONT_BIT;
        case DM_CULL_BACK:       return VK_CULL_MODE_BACK_BIT;
        case DM_CULL_FRONT_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        
        default:
        DM_LOG_ERROR("Unknown cull mode, shouldn't be here...");
        return VK_CULL_MODE_FRONT_BIT;
    }
}

VkFrontFace dm_winding_to_vulkan_front_face(dm_winding_order winding)
{
    switch(winding)
    {
        case DM_WINDING_CLOCK:         return VK_FRONT_FACE_CLOCKWISE;
        case DM_WINDING_COUNTER_CLOCK: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        
        default:
        DM_LOG_ERROR("Unknown winding order, shouldn't be here...");
        return VK_FRONT_FACE_CLOCKWISE;
    }
}

VkCompareOp dm_compare_to_vulkan_compare(dm_comparison comp)
{
    switch(comp)
    {
        case DM_COMPARISON_ALWAYS:   return VK_COMPARE_OP_ALWAYS;
        case DM_COMPARISON_NEVER:    return VK_COMPARE_OP_NEVER;
        case DM_COMPARISON_EQUAL:    return VK_COMPARE_OP_EQUAL;
        case DM_COMPARISON_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case DM_COMPARISON_LESS:     return VK_COMPARE_OP_LESS;
        case DM_COMPARISON_LEQUAL:   return VK_COMPARE_OP_LESS_OR_EQUAL;
        case DM_COMPARISON_GREATER:  return VK_COMPARE_OP_GREATER;
        case DM_COMPARISON_GEQUAL:   return VK_COMPARE_OP_GREATER_OR_EQUAL;
        
        default:
        DM_LOG_ERROR("Unknown comparison operation, shouldn't be here...");
        return VK_COMPARE_OP_LESS;
    }
}

VkBlendFactor dm_blend_to_vulkan_blend(dm_blend_func func)
{
    switch(func)
    {
        case DM_BLEND_FUNC_ZERO:                  return VK_BLEND_FACTOR_ZERO;
        case DM_BLEND_FUNC_ONE:                   return VK_BLEND_FACTOR_ONE;
        case DM_BLEND_FUNC_SRC_COLOR:             return VK_BLEND_FACTOR_SRC_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case DM_BLEND_FUNC_DST_COLOR:             return VK_BLEND_FACTOR_DST_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR:   return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case DM_BLEND_FUNC_SRC_ALPHA:             return VK_BLEND_FACTOR_SRC_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case DM_BLEND_FUNC_DST_ALPHA:             return VK_BLEND_FACTOR_DST_ALPHA;
        case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA:   return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case DM_BLEND_FUNC_CONST_COLOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        
        default:
        DM_LOG_ERROR("Unknown blend function, shouldn't be here...");
        return VK_BLEND_FACTOR_ZERO;
    }
}

VkBlendOp dm_blend_eq_to_vulkan(dm_blend_equation eq)
{
    switch(eq)
    {
        case DM_BLEND_EQUATION_ADD: return VK_BLEND_OP_ADD;
        case DM_BLEND_EQUATION_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
        case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case DM_BLEND_EQUATION_MIN: return VK_BLEND_OP_MIN;
        case DM_BLEND_EQUATION_MAX: return VK_BLEND_OP_MAX;
        
        default:
        DM_LOG_ERROR("Unknown blend equation, shouldn't be here...");
        return VK_BLEND_OP_ADD;
    }
}

VkVertexInputRate dm_vertex_class_to_vulkan_vertex_input(dm_vertex_attrib_class v_class)
{
    switch(v_class)
    {
        case DM_VERTEX_ATTRIB_CLASS_VERTEX:   return VK_VERTEX_INPUT_RATE_VERTEX;
        case DM_VERTEX_ATTRIB_CLASS_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
        
        default:
        DM_LOG_ERROR("Unknown vertex attrib class, shouldn't be here...");
        return VK_VERTEX_INPUT_RATE_VERTEX;
    }
}

VkPrimitiveTopology dm_primitive_to_vulkan_primitive(dm_primitive_topology primitive)
{
    switch(primitive)
    {
        case DM_TOPOLOGY_POINT_LIST:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case DM_TOPOLOGY_LINE_LIST:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case DM_TOPOLOGY_LINE_STRIP:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case DM_TOPOLOGY_TRIANGLE_LIST:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case DM_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        
        default:
        DM_LOG_ERROR("Unknown primitive topology, shouldn't be here...");
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

VkFormat dm_vertex_data_t_to_vulkan_format(dm_vertex_attrib_desc desc)
{
    switch(desc.data_t)
    {
        case DM_VERTEX_DATA_T_BYTE:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R8_SINT;
                case 2: return VK_FORMAT_R8G8_SINT;
                case 4: return VK_FORMAT_R8G8B8A8_SINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_UBYTE:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R8_UINT;
                case 2: return VK_FORMAT_R8G8_UINT;
                case 4: return VK_FORMAT_R8G8B8A8_UINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_SHORT:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R16_SINT;
                case 2: return VK_FORMAT_R16G16_SINT;
                case 4: return VK_FORMAT_R16G16B16A16_SINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_USHORT:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R16_UINT;
                case 2: return VK_FORMAT_R16G16_UINT;
                case 4: return VK_FORMAT_R16G16B16A16_UINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_INT:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R32_SINT;
                case 2: return VK_FORMAT_R32G32_SINT;
                case 3: return VK_FORMAT_R32G32B32_SINT;
                case 4: return VK_FORMAT_R32G32B32A32_SINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_UINT:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
            }
        } break;
        
        case DM_VERTEX_DATA_T_FLOAT:
        case DM_VERTEX_DATA_T_DOUBLE:
        {
            switch(desc.count)
            {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        } break;
        
        default:
        DM_LOG_ERROR("Unknown vertex data t, shouldn't be here...");
        return VK_FORMAT_UNDEFINED;
    }

    return VK_FORMAT_UNDEFINED;
}

/*********************
VULKAN COMMAND BUFFER
***********************/
bool dm_vulkan_command_buffer_allocate(VkCommandPool pool, dm_vulkan_command_buffer* buffer, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif

    VkCommandBufferAllocateInfo allocate_info = { 0 };
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool  = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    
    DM_VULKAN_FUNC_CHECK(vkAllocateCommandBuffers(vulkan_renderer->device.logical, &allocate_info, &buffer->buffer));
    if(DM_VULKAN_FUNC_SUCCESS()) return true;
    
    DM_LOG_FATAL("Could not allocate Vulkan command buffer");
    return false;
}

void dm_vulkan_command_buffer_free(VkCommandPool pool, dm_vulkan_command_buffer* buffer, dm_vulkan_renderer* vulkan_renderer)
{
    vkFreeCommandBuffers(vulkan_renderer->device.logical, pool, 1, &buffer->buffer);
    buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

bool dm_vulkan_command_buffer_begin(bool single_use, bool renderpass_continue, bool simultaneous_use, dm_vulkan_command_buffer* buffer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if(single_use)          begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if(renderpass_continue) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    if(simultaneous_use)    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    DM_VULKAN_FUNC_CHECK(vkBeginCommandBuffer(buffer->buffer, &begin_info));
    if(DM_VULKAN_FUNC_SUCCESS()) 
    {
        buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_RECORDING;
        return true;
    }
    
    DM_LOG_FATAL("Begin command buffer failed");
    return false;
}

bool dm_vulkan_command_buffer_end(dm_vulkan_command_buffer* buffer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif

    DM_VULKAN_FUNC_CHECK(vkEndCommandBuffer(buffer->buffer));
    if(DM_VULKAN_FUNC_SUCCESS())
    {
        buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_RECORDING_ENDED;
        return true;
    }
    
    DM_LOG_FATAL("End command buffer failed");
    return false;
}

void dm_vulkan_command_buffer_update_submitted(dm_vulkan_command_buffer* buffer)
{
    buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_SUBMITTED;
}

void dm_vulkan_command_buffer_reset(dm_vulkan_command_buffer* buffer)
{
    buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_READY;
}

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
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    VkImageViewCreateInfo create_info = { 0 };
    create_info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image                       = image->handle;
    create_info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format                      = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.layerCount = 1;
    
    DM_VULKAN_FUNC_CHECK(vkCreateImageView(device, &create_info, allocator, &image->view));
    if(DM_VULKAN_FUNC_SUCCESS()) return true;
    
    DM_LOG_FATAL("Creating Vulkan image view failed");
    return false;
}

bool dm_vulkan_create_image(dm_vulkan_image_desc desc, dm_vulkan_image* image, dm_vulkan_device* device, VkAllocationCallbacks* allocator)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    image->desc = desc;
    
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
    if(!DM_VULKAN_FUNC_SUCCESS()) 
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
    if(!DM_VULKAN_FUNC_SUCCESS()) 
    {
        DM_LOG_FATAL("vkAllocateMemory failed");
        return false;
    }
    
    DM_VULKAN_FUNC_CHECK(vkBindImageMemory(device->logical, image->handle, image->memory, 0));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
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

/******************
VULKAN FRAMEBUFFER
********************/
bool dm_vulkan_create_framebuffer(dm_vulkan_framebuffer_desc desc, VkImageView* attachments, dm_vulkan_framebuffer* framebuffer, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    framebuffer->attachments = dm_alloc(sizeof(VkImageView) * desc.attachment_count);
    dm_memcpy(framebuffer->attachments, attachments, sizeof(VkImageView) * desc.attachment_count);
    
    framebuffer->desc = desc;
    
    VkFramebufferCreateInfo create_info = { 0 };
    create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass      = vulkan_renderer->master_renderpass.renderpass;
    create_info.attachmentCount = desc.attachment_count;
    create_info.pAttachments    = framebuffer->attachments;
    create_info.width           = desc.width;
    create_info.height          = desc.height;
    create_info.layers          = 1;
    
    DM_VULKAN_FUNC_CHECK(vkCreateFramebuffer(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &framebuffer->framebuffer));
    if(DM_VULKAN_FUNC_SUCCESS()) return true;

    DM_LOG_FATAL("Could not create Vulkan framebuffer");
    return false;
}

void dm_vulkan_destroy_framebuffer(dm_vulkan_framebuffer* framebuffer, dm_vulkan_renderer* vulkan_renderer)
{
    vkDestroyFramebuffer(vulkan_renderer->device.logical, framebuffer->framebuffer, vulkan_renderer->allocator);
    dm_free(framebuffer->attachments);
}

/***************
VULKAN PIPELINE
*****************/
bool dm_vulkan_create_pipeline(dm_pipeline_desc desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, VkViewport viewport, VkRect2D scissor, dm_vulkan_shader* shader, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    dm_vulkan_pipeline internal_pipe = { 0 };
    
    // viewport
    VkPipelineViewportStateCreateInfo viewport_create_info = { 0 };
    viewport_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports    = &viewport;
    viewport_create_info.scissorCount  = 1;
    viewport_create_info.pScissors     = &scissor;
    
    // rasterizer
    VkPipelineRasterizationStateCreateInfo raster_create_info = { 0 };
    raster_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_create_info.depthClampEnable        = VK_FALSE;
    raster_create_info.rasterizerDiscardEnable = VK_FALSE;
    raster_create_info.polygonMode             = desc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    raster_create_info.lineWidth               = 1;
    raster_create_info.cullMode                = dm_cull_mode_to_vulkan_cull_mode(desc.cull_mode);
    raster_create_info.frontFace               = dm_winding_to_vulkan_front_face(desc.winding_order);
    raster_create_info.depthBiasEnable         = VK_FALSE;
    
    // multisampling
    VkPipelineMultisampleStateCreateInfo multi_create_info = { 0 };
    multi_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_create_info.sampleShadingEnable   = VK_FALSE;
    multi_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multi_create_info.minSampleShading      = 1;
    multi_create_info.alphaToCoverageEnable = VK_FALSE;
    multi_create_info.alphaToOneEnable      = VK_FALSE;
    
    // depth stencil testing
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = { 0 };
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    if(desc.depth)
    {
        depth_stencil_info.depthTestEnable  = VK_TRUE;
        depth_stencil_info.depthWriteEnable = VK_TRUE;
        depth_stencil_info.depthCompareOp   = dm_compare_to_vulkan_compare(desc.depth_comp);
    }
    else depth_stencil_info.depthTestEnable = VK_FALSE;
    if(desc.stencil) depth_stencil_info.stencilTestEnable = VK_TRUE;
    else depth_stencil_info.stencilTestEnable = VK_FALSE;
    
    // color blending
    VkPipelineColorBlendAttachmentState color_blend_state = { 0 };
    if(desc.blend)
    {
        color_blend_state.blendEnable = VK_TRUE;
        color_blend_state.srcColorBlendFactor = dm_blend_to_vulkan_blend(desc.blend_src_f);
        color_blend_state.dstColorBlendFactor = dm_blend_to_vulkan_blend(desc.blend_dest_f);
        color_blend_state.colorBlendOp = dm_blend_eq_to_vulkan(desc.blend_eq);
        color_blend_state.srcAlphaBlendFactor = dm_blend_to_vulkan_blend(desc.blend_src_f);
        color_blend_state.dstAlphaBlendFactor = dm_blend_to_vulkan_blend(desc.blend_dest_f);
        color_blend_state.alphaBlendOp = dm_blend_eq_to_vulkan(desc.blend_eq);
    }
    else color_blend_state.blendEnable = VK_FALSE;
    
    color_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo color_blend_info = { 0 };
    color_blend_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOp         = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments    = &color_blend_state;
    
    // dynamic state
#define DM_VULKAN_DYNAMIC_STATE_COUNT 3
    static VkDynamicState dynamic_states[DM_VULKAN_DYNAMIC_STATE_COUNT] = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    
    VkPipelineDynamicStateCreateInfo dynamic_create_info = { 0 };
    dynamic_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_create_info.pDynamicStates = dynamic_states;
    
    // vertex inputs
#define DM_VULKAN_MAX_VERTEX_INPUTS 2
    VkVertexInputBindingDescription vertex_bind_desc[DM_VULKAN_MAX_VERTEX_INPUTS] = { 0 };
    uint32_t vertex_binding_count = 0;
    for(uint32_t i=0; i<attrib_count; i++)
    {
        if(vertex_bind_desc[0].stride != 0 && vertex_bind_desc[1].stride != 0) break;
        if(vertex_binding_count>0 && attrib_descs[i].stride == vertex_bind_desc[vertex_binding_count-1].stride) continue;
        
        vertex_bind_desc[vertex_binding_count].binding   = i;
        vertex_bind_desc[vertex_binding_count].stride    = attrib_descs[i].stride;
        vertex_bind_desc[vertex_binding_count].inputRate = dm_vertex_class_to_vulkan_vertex_input(attrib_descs[i].attrib_class);
        
        vertex_binding_count++;
    }
    
    // vertex attributes
#define DM_VULKAN_MAX_ATTRIB_COUNT 20
    VkVertexInputAttributeDescription attribute_descs[DM_VULKAN_MAX_ATTRIB_COUNT];
    uint32_t count = 0;
    for(uint32_t i=0; i<attrib_count; i++)
    {
        dm_vertex_attrib_desc attrib = attrib_descs[i];
        if(attrib.data_t==DM_VERTEX_DATA_T_MATRIX_INT || attrib.data_t==DM_VERTEX_DATA_T_MATRIX_FLOAT)
        {
            for(uint32_t j=0; j<attrib.count; j++)
            {
                dm_vertex_attrib_desc sub_desc = attrib;
                if(attrib.data_t==DM_VERTEX_DATA_T_MATRIX_INT) sub_desc.data_t = DM_VERTEX_DATA_T_INT;
                else                                           sub_desc.data_t = DM_VERTEX_DATA_T_FLOAT;
                
                sub_desc.offset = sub_desc.offset + sizeof(float) * j;
                
                attribute_descs[count].binding  = sub_desc.attrib_class==DM_VERTEX_ATTRIB_CLASS_VERTEX ? 0 : 1;
                attribute_descs[count].location = count;
                attribute_descs[count].format   = dm_vertex_data_t_to_vulkan_format(sub_desc);
                attribute_descs[count].offset   = sub_desc.offset;
                
                count++;
            }
        }
        else
        {
            attribute_descs[count].binding  = attrib.attrib_class==DM_VERTEX_ATTRIB_CLASS_VERTEX ? 0 : 1;
            attribute_descs[count].location = count;
            attribute_descs[count].format   = dm_vertex_data_t_to_vulkan_format(attrib);
            attribute_descs[count].offset   = attrib.offset;
            
            count++;
        }
    }
    
    VkPipelineVertexInputStateCreateInfo vertex_input_info = { 0 };
    vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount   = vertex_binding_count;
    vertex_input_info.pVertexBindingDescriptions      = vertex_bind_desc;
    vertex_input_info.vertexAttributeDescriptionCount = count;
    vertex_input_info.pVertexAttributeDescriptions    = attribute_descs;
    
    
    
    // input assembly
    VkPipelineInputAssemblyStateCreateInfo input_info = { 0 };
    input_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_info.topology               = dm_primitive_to_vulkan_primitive(desc.primitive_topology);
    input_info.primitiveRestartEnable = VK_FALSE;
    
    // layout
    VkPipelineLayoutCreateInfo layout_info = { 0 };
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    DM_VULKAN_FUNC_CHECK(vkCreatePipelineLayout(vulkan_renderer->device.logical, &layout_info, vulkan_renderer->allocator, &internal_pipe.layout));
    if(!DM_VULKAN_FUNC_SUCCESS()) 
    {
        DM_LOG_FATAL("Could not create Vulkan pipeline layout");
        return false;
    }
    
    // finally pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = { 0 };
    pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = shader->stage_count;
    pipeline_info.pStages             = shader->stage_create_infos;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_info;
    
    pipeline_info.pViewportState      = &viewport_create_info;
    pipeline_info.pRasterizationState = &raster_create_info;
    pipeline_info.pMultisampleState   = &multi_create_info;
    pipeline_info.pDepthStencilState  = &depth_stencil_info;
    pipeline_info.pColorBlendState    = &color_blend_info;
    pipeline_info.pDynamicState       = &dynamic_create_info;
    
    pipeline_info.layout              = internal_pipe.layout;
    
    pipeline_info.renderPass          = vulkan_renderer->master_renderpass.renderpass;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex   = -1;
    
    DM_VULKAN_FUNC_CHECK(vkCreateGraphicsPipelines(vulkan_renderer->device.logical, VK_NULL_HANDLE, 1, &pipeline_info, vulkan_renderer->allocator, &internal_pipe.pipeline));
    if(DM_VULKAN_FUNC_SUCCESS()) 
    {
        vulkan_renderer->pipelines[vulkan_renderer->pipeline_count] = internal_pipe;
        *handle = vulkan_renderer->pipeline_count++;
        
        return true;
    }
    
    DM_LOG_FATAL("Could not create Vulkan pipeline");
    return false;
}

void dm_vulkan_destroy_pipeline(dm_render_handle handle, dm_vulkan_renderer* vulkan_renderer)
{
    if(handle > vulkan_renderer->pipeline_count) { DM_LOG_ERROR("Trying to destroy invalid Vulkan pipeline"); return; }
    
    dm_vulkan_pipeline* internal_pipe = &vulkan_renderer->pipelines[handle];
    vkDestroyPipeline(vulkan_renderer->device.logical, internal_pipe->pipeline, vulkan_renderer->allocator);
    vkDestroyPipelineLayout(vulkan_renderer->device.logical, internal_pipe->layout, vulkan_renderer->allocator);
}

/*****************
VULKAN RENDERPASS
*******************/
bool dm_vulkan_create_renderpass(dm_renderpass_desc desc, dm_vulkan_renderpass_desc vulkan_desc, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    vulkan_renderer->master_renderpass.desc = vulkan_desc;
    
    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
    VkAttachmentDescription attachment_descs[2];
    
    VkAttachmentDescription color_attachment = { 0 };
    color_attachment.format         = vulkan_renderer->swapchain.image_format.format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = dm_load_op_to_vulkan_load_op(desc.color_load_op);
    color_attachment.storeOp        = dm_store_op_to_vulkan_store_op(desc.color_store_op);
    color_attachment.stencilLoadOp  = dm_load_op_to_vulkan_load_op(desc.color_stencil_load_op);
    color_attachment.stencilStoreOp = dm_store_op_to_vulkan_store_op(desc.color_stencil_store_op);
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    attachment_descs[0] = color_attachment;
    
    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkAttachmentDescription depth_attachment = { 0 };
    depth_attachment.format         = vulkan_renderer->device.depth_format;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = dm_load_op_to_vulkan_load_op(desc.depth_load_op);
    depth_attachment.storeOp        = dm_store_op_to_vulkan_store_op(desc.depth_store_op);
    depth_attachment.stencilLoadOp  = dm_load_op_to_vulkan_load_op(desc.depth_stencil_load_op);
    depth_attachment.stencilStoreOp = dm_store_op_to_vulkan_store_op(desc.depth_stencil_store_op);
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    attachment_descs[1] = depth_attachment;
    
    VkAttachmentReference depth_attachment_reference = { 0 };
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    subpass.pDepthStencilAttachment = &depth_attachment_reference;
    
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    // create
    VkRenderPassCreateInfo create_info = { 0 };
    create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 2;
    create_info.pAttachments    = attachment_descs;
    create_info.subpassCount    = 1;
    create_info.pSubpasses      = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies   = &dependency;
    
    DM_VULKAN_FUNC_CHECK(vkCreateRenderPass(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->master_renderpass.renderpass));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    return true;
}

void dm_vulkan_destroy_renderpass(dm_vulkan_renderpass* renderpass, dm_vulkan_renderer* vulkan_renderer)
{
    vkDestroyRenderPass(vulkan_renderer->device.logical, renderpass->renderpass, vulkan_renderer->allocator);
}

void dm_vulkan_begin_renderpass(dm_vulkan_renderpass* renderpass, VkFramebuffer framebuffer, dm_vulkan_command_buffer* command_buffer, dm_vulkan_renderer* vulkan_renderer)
{
    VkRenderPassBeginInfo begin_info = { 0 };
    begin_info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass               = renderpass->renderpass;
    begin_info.framebuffer              = framebuffer;
    begin_info.renderArea.offset.x      = renderpass->desc.x;
    begin_info.renderArea.offset.y      = renderpass->desc.y;
    begin_info.renderArea.extent.width  = renderpass->desc.width;
    begin_info.renderArea.extent.height = renderpass->desc.height;
    
    VkClearValue clear_values[2] = { 0 };
    clear_values[0].color.float32[0]     = renderpass->desc.r;
    clear_values[0].color.float32[1]     = renderpass->desc.g;
    clear_values[0].color.float32[2]     = renderpass->desc.b;
    clear_values[0].color.float32[3]     = renderpass->desc.a;
    clear_values[1].depthStencil.depth   = renderpass->desc.depth;
    clear_values[1].depthStencil.stencil = renderpass->desc.stencil;
    
    begin_info.clearValueCount = 2;
    begin_info.pClearValues = clear_values;
    
    vkCmdBeginRenderPass(command_buffer->buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_IN_RENDERPASS;
}

void dm_vulkan_end_renderpass(dm_vulkan_renderpass* renderpass, dm_vulkan_command_buffer* command_buffer, dm_vulkan_renderer* vulkan_renderer)
{
    vkCmdEndRenderPass(command_buffer->buffer);
    command_buffer->state = DM_VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}

/*************
VULKAN SHADER
***************/
bool dm_vulkan_create_shader_module(const char* shader_file, VkShaderStageFlagBits shader_stage, uint32_t stage_index, dm_vulkan_shader* shader, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    shader->module_create_infos[stage_index].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    
    size_t file_size;
    uint32_t* file_bytes = dm_read_bytes(shader_file, "rb", &file_size);
    if(!file_bytes) return false;
    
    shader->module_create_infos[stage_index].codeSize = file_size;
    shader->module_create_infos[stage_index].pCode = file_bytes;
    
    DM_VULKAN_FUNC_CHECK(vkCreateShaderModule(vulkan_renderer->device.logical, &shader->module_create_infos[stage_index], vulkan_renderer->allocator, &shader->modules[stage_index]));
    
    dm_free(file_bytes); 
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    shader->stage_create_infos[stage_index].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader->stage_create_infos[stage_index].stage  = shader_stage;
    shader->stage_create_infos[stage_index].module = shader->modules[stage_index];
    shader->stage_create_infos[stage_index].pName  = "main";
    
    return true;
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    
    dm_vulkan_shader internal_shader = { 0 };
    if(!dm_vulkan_create_shader_module(shader_desc.vertex, VK_SHADER_STAGE_VERTEX_BIT, 0, &internal_shader, vulkan_renderer)) return false;
    if(!dm_vulkan_create_shader_module(shader_desc.pixel, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &internal_shader, vulkan_renderer)) return false;
    
    internal_shader.stage_count = 2;
    
    vulkan_renderer->shaders[vulkan_renderer->shader_count]= internal_shader;
    *shader_handle = vulkan_renderer->shader_count++;
    
    // viewport
    VkViewport viewport = { 0 };
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = (float)vulkan_renderer->width;
    viewport.height   = (float)vulkan_renderer->height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    
    // scissor
    VkRect2D scissor = { 0 };
    scissor.extent.width  = vulkan_renderer->width;
    scissor.extent.height = vulkan_renderer->height;
    
    // create pipeline
    if(!dm_vulkan_create_pipeline(pipe_desc, attrib_descs, attrib_count, viewport, scissor, &internal_shader, pipe_handle, renderer)) return false;
    
    return true;
}

void dm_renderer_backend_destroy_shader(dm_render_handle handle, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    
    if(handle >= vulkan_renderer->shader_count) { DM_LOG_FATAL("Trying to destroy invalid Vulkan shader"); return; }
    
    dm_vulkan_shader internal_shader = vulkan_renderer->shaders[handle];
    for(uint32_t i=0; i<internal_shader.stage_count; i++)
    {
        vkDestroyShaderModule(vulkan_renderer->device.logical, internal_shader.modules[i], vulkan_renderer->allocator);
    }
}

/****************
VULKAN SWAPCHAIN
******************/
bool dm_vulkan_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, dm_vulkan_swapchain_support_info* support_info)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &support_info->capabilities));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support_info->format_count, 0));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    if(support_info->format_count)
    {
        if(!support_info->formats) support_info->formats = dm_alloc(sizeof(VkSurfaceFormatKHR) * support_info->format_count);
        DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support_info->format_count, support_info->formats));
        if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    }
    
    DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support_info->present_mode_count, 0));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    if(support_info->present_mode_count)
    {
        if(!support_info->present_modes) support_info->present_modes = dm_alloc(sizeof(VkPresentModeKHR) * support_info->present_mode_count);
        DM_VULKAN_FUNC_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support_info->present_mode_count, support_info->present_modes));
        if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    }
    
    return true;
}

bool dm_vulkan_internal_create_swapchain(uint32_t width, uint32_t height, dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    VkExtent2D swapchain_extent = { width, height };
    
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
    
    swapchain->max_frames_in_flight = image_count - 1;
    
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
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    if(!swapchain->images) swapchain->images = dm_alloc(sizeof(VkImage) * swapchain->image_count);
    if(!swapchain->views)  swapchain->views  = dm_alloc(sizeof(VkImageView) * swapchain->image_count);
    
    DM_VULKAN_FUNC_CHECK(vkGetSwapchainImagesKHR(vulkan_renderer->device.logical, swapchain->handle, &swapchain->image_count, swapchain->images));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
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
        if(!DM_VULKAN_FUNC_SUCCESS()) return false;
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

void dm_vulkan_destroy_swapchain(dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    dm_vulkan_destroy_image(&swapchain->depth, vulkan_renderer->device.logical, vulkan_renderer->allocator);
    
    for(uint32_t i=0; i<swapchain->image_count; i++)
    {
        vkDestroyImageView(vulkan_renderer->device.logical, swapchain->views[i], vulkan_renderer->allocator);
    }
    
    vkDestroySwapchainKHR(vulkan_renderer->device.logical, swapchain->handle, vulkan_renderer->allocator);
    
    if(swapchain->images) dm_free(swapchain->images);
    if(swapchain->views)  dm_free(swapchain->views);

    swapchain->images = NULL;
    swapchain->views = NULL;
}

bool dm_vulkan_create_swapchain(dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    return dm_vulkan_internal_create_swapchain(vulkan_renderer->width, vulkan_renderer->height, swapchain, vulkan_renderer);
}

bool dm_vulkan_recreate_swapchain(uint32_t width, uint32_t height, dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    dm_vulkan_destroy_swapchain(swapchain, vulkan_renderer);
    return dm_vulkan_internal_create_swapchain(width, height, swapchain, vulkan_renderer) ;
}


bool dm_vulkan_swapchain_next_image_index(uint32_t timeout_ms, VkSemaphore image_available_semaphore, VkFence fence, uint32_t* image_index, dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    VkResult result = vkAcquireNextImageKHR(vulkan_renderer->device.logical, swapchain->handle, timeout_ms, image_available_semaphore, fence, image_index);
    
    switch(result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        dm_vulkan_recreate_swapchain(vulkan_renderer->width, vulkan_renderer->height, swapchain, vulkan_renderer);
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

bool dm_vulkan_swapchain_present(VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t image_index, dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    VkPresentInfoKHR present_info   = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &render_complete_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain->handle;
    present_info.pImageIndices      = &image_index;
    
    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    switch(result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
        dm_vulkan_recreate_swapchain(vulkan_renderer->width, vulkan_renderer->height, swapchain, vulkan_renderer);
        break;
        
        case VK_SUCCESS:
        break;
        
        default:
        DM_LOG_FATAL("Failed to present swap chain image");
        return false;
    }
    
    vulkan_renderer->current_frame = (vulkan_renderer->current_frame+1) % vulkan_renderer->swapchain.max_frames_in_flight;
    
    return true;
}

bool dm_vulkan_swapchain_regenerate_framebuffers(dm_vulkan_swapchain* swapchain, dm_vulkan_renderer* vulkan_renderer)
{
    for(uint32_t i=0; i<swapchain->image_count; i++)
    {
#define DM_VULKAN_SWAPCHAIN_ATTACHMENT_COUNT 2
        VkImageView attachments[DM_VULKAN_SWAPCHAIN_ATTACHMENT_COUNT] = {
            swapchain->views[i],
            swapchain->depth.view
        };
        
        dm_vulkan_framebuffer_desc desc = { 0 };
        desc.width            = vulkan_renderer->width;
        desc.height           = vulkan_renderer->height;
        desc.attachment_count = 2;
        
        dm_vulkan_create_framebuffer(desc, attachments, &swapchain->framebuffers[i], vulkan_renderer);
    }
    
    return true;
}

/*************
VULKAN DEVICE
***************/
bool dm_vulkan_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface, dm_vulkan_device* device, dm_vulkan_physical_device_reqs* reqs, dm_vulkan_physical_device_queue_family_info* queue_info, dm_vulkan_swapchain_support_info* swapchain_support_info)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
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
        if(!DM_VULKAN_FUNC_SUCCESS()) 
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
        if(!DM_VULKAN_FUNC_SUCCESS()) return false;
        if(!available_count) return false;
        
        VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * available_count);
        DM_VULKAN_FUNC_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0, &available_count, available_extensions));
        if(!DM_VULKAN_FUNC_SUCCESS()) return false;
        
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
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    vulkan_renderer->device.physical = VK_NULL_HANDLE;
    
    uint32_t device_count;
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, NULL);
    if(!device_count) { DM_LOG_FATAL("No GPUs with Vulkan support"); return false; }
    
    VkPhysicalDevice* physical_devices = dm_alloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(vulkan_renderer->instance, &device_count, physical_devices);
    
    dm_vulkan_physical_device_reqs reqs = { 0 };
    reqs.flags = DM_VULKAN_PHYSICAL_DEVICE_FLAG_GRAPHICS | DM_VULKAN_PHYSICAL_DEVICE_FLAG_PRESENT | DM_VULKAN_PHYSICAL_DEVICE_FLAG_COMPUTE  | DM_VULKAN_PHYSICAL_DEVICE_FLAG_TRANSFER | DM_VULKAN_PHYSICAL_DEVICE_FLAG_SAMPLER_ANISOP | DM_VULKAN_PHYSICAL_DEVICE_FLAG_DISCRETE_GPU;
    
    reqs.device_extension_names[reqs.extension_count] = dm_alloc(strlen(VK_KHR_SWAPCHAIN_EXTENSION_NAME)+1);
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
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_index, 0, &vulkan_renderer->device.graphics_queue); 
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.present_index, 0, &vulkan_renderer->device.present_queue); 
    vkGetDeviceQueue(vulkan_renderer->device.logical, vulkan_renderer->device.transfer_index, 0, &vulkan_renderer->device.transfer_queue); 
    
    // command pool
    VkCommandPoolCreateInfo pool_create_info = { 0 };
    pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = vulkan_renderer->device.graphics_index;
    pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    DM_VULKAN_FUNC_CHECK(vkCreateCommandPool(vulkan_renderer->device.logical, &pool_create_info, vulkan_renderer->allocator, &vulkan_renderer->device.graphics_command_pool));
    if(!DM_VULKAN_FUNC_SUCCESS())
    {
        DM_LOG_FATAL("Could not create command pool");
        return false;
    }
    
    return true;
}

bool dm_vulkan_device_detect_depth_buffer_range(dm_vulkan_device* device)
{
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
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    // extensions
#ifdef DM_DEBUG
    uint32_t extension_count;
    // available
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    VkExtensionProperties* available_extensions = dm_alloc(sizeof(VkExtensionProperties) * extension_count);
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, available_extensions));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    DM_LOG_INFO("Available Vulkan extensions:");
    for(uint32_t i=0; i<extension_count; i++)
    {
        DM_LOG_INFO("     %s", available_extensions[i].extensionName);
    }
    
    dm_free(available_extensions);
#endif
    
    // required
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
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    VkLayerProperties* available_layers = dm_alloc(sizeof(VkLayerProperties) * available_count);
    DM_VULKAN_FUNC_CHECK(vkEnumerateInstanceLayerProperties(&available_count, available_layers));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
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
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
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

/************
VULKAN FENCE
**************/
bool dm_vulkan_create_fence(bool signaled, dm_vulkan_fence* fence, dm_vulkan_renderer* vulkan_renderer)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    fence->signaled = signaled;
    
    VkFenceCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(signaled) create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    DM_VULKAN_FUNC_CHECK(vkCreateFence(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &fence->fence));
    if(DM_VULKAN_FUNC_SUCCESS()) return true; 
    
    DM_LOG_FATAL("Could not create Vulkan fence");
    return false;
}

void dm_vulkan_destroy_fence(dm_vulkan_fence* fence, dm_vulkan_renderer* vulkan_renderer)
{
    vkDestroyFence(vulkan_renderer->device.logical, fence->fence, vulkan_renderer->allocator);
}

bool dm_vulkan_fence_wait(size_t timeout_ms, dm_vulkan_fence* fence, dm_vulkan_renderer* vulkan_renderer)
{
    if(fence->signaled) return true;
    
    VkResult result = vkWaitForFences(vulkan_renderer->device.logical, 1, &fence->fence, true, timeout_ms);
    switch(result)
    {
        case VK_SUCCESS:
        fence->signaled = true;
        return true;
        
        case VK_TIMEOUT:
        DM_LOG_WARN("vkWaitForFence - time out");
        break;
        
        case VK_ERROR_DEVICE_LOST:
        DM_LOG_ERROR("vkWaitForFence - device lost");
        break;
        
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        DM_LOG_ERROR("vkWaitForFence - out of host memory");
        break;
        
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        DM_LOG_ERROR("vkWaitForFence - out of device memory");
        break;
        
        default:
        DM_LOG_ERROR("vkWaitForFence - unknown error");
        break;
    }
    
    return false;
}

bool dm_vulkan_fence_reset(dm_vulkan_fence* fence, dm_vulkan_renderer* vulkan_renderer)
{
    if(!fence->signaled) return true;
    
    fence->signaled = false;
#ifdef DM_DEBUG
    VkResult result;
#endif
    DM_VULKAN_FUNC_CHECK(vkResetFences(vulkan_renderer->device.logical, 1, &fence->fence));
    if(DM_VULKAN_FUNC_SUCCESS()) return true;
    
    DM_LOG_FATAL("vkResetFences failed");
    return false;
}

/**************
VULKAN BACKEND
****************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Vulkan backend...");
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_vulkan_renderer));
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    
    vulkan_renderer->width = context->platform_data.window_data.width;
    vulkan_renderer->height = context->platform_data.window_data.height;
    
    if(!dm_vulkan_create_instance(&context->platform_data, vulkan_renderer)) return false;
    if(!dm_platform_create_vulkan_surface(&context->platform_data, &vulkan_renderer->instance, &vulkan_renderer->surface)) return false;
    if(!dm_vulkan_create_device(vulkan_renderer)) return false;
    if(!dm_vulkan_create_swapchain(&vulkan_renderer->swapchain, vulkan_renderer)) return false;
    
    // renderpass
    dm_renderpass_desc renderpass_desc = { 0 };
    renderpass_desc.color_load_op          = DM_LOAD_OPERATION_CLEAR;
    renderpass_desc.color_store_op         = DM_STORE_OPERATION_STORE;
    renderpass_desc.color_stencil_load_op  = DM_LOAD_OPERATION_DONT_CARE;
    renderpass_desc.color_stencil_store_op = DM_STORE_OPERATION_DONT_CARE;
    renderpass_desc.depth_load_op          = DM_LOAD_OPERATION_CLEAR;
    renderpass_desc.depth_store_op         = DM_STORE_OPERATION_STORE;
    renderpass_desc.depth_stencil_load_op  = DM_LOAD_OPERATION_DONT_CARE;
    renderpass_desc.depth_stencil_store_op = DM_STORE_OPERATION_DONT_CARE;
    
    renderpass_desc.flags |= DM_RENDERPASS_FLAG_COLOR | DM_RENDERPASS_FLAG_DEPTH;
    
    dm_vulkan_renderpass_desc vulkan_desc = { 0 };
    vulkan_desc.width  = context->platform_data.window_data.width;
    vulkan_desc.height = context->platform_data.window_data.height;
    vulkan_desc.r      = 0.2f;
    vulkan_desc.a      = 1.0f;
    
    if(!dm_vulkan_create_renderpass(renderpass_desc, vulkan_desc, vulkan_renderer)) return false;
    
    // swapchain framebuffers
    dm_vulkan_swapchain_regenerate_framebuffers(&vulkan_renderer->swapchain, vulkan_renderer);
    
    // command buffers
    if(!vulkan_renderer->command_buffers) vulkan_renderer->command_buffers = dm_alloc(sizeof(dm_vulkan_command_buffer) * vulkan_renderer->swapchain.image_count);
    
    for(uint32_t i=0; i<vulkan_renderer->swapchain.image_count; i++)
    {
        dm_vulkan_command_buffer_allocate(vulkan_renderer->device.graphics_command_pool, &vulkan_renderer->command_buffers[i], vulkan_renderer);
    }
    
    // semaphores
    vulkan_renderer->available_semaphore_count = vulkan_renderer->swapchain.max_frames_in_flight;
    vulkan_renderer->complete_semaphore_count = vulkan_renderer->swapchain.max_frames_in_flight;
    vulkan_renderer->fence_count = vulkan_renderer->swapchain.max_frames_in_flight;
    for(uint32_t i=0; i<vulkan_renderer->swapchain.max_frames_in_flight; i++)
    {
        VkSemaphoreCreateInfo create_info = { 0 };
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->available_semaphores[i]);
        vkCreateSemaphore(vulkan_renderer->device.logical, &create_info, vulkan_renderer->allocator, &vulkan_renderer->complete_semaphores[i]);
        
        if(!dm_vulkan_create_fence(true, &vulkan_renderer->in_flight_fences[i], vulkan_renderer)) return false;
    }
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    
    vkDeviceWaitIdle(vulkan_renderer->device.logical);
    
    // resources
    uint32_t i = 0;
    for(i=0; i<vulkan_renderer->shader_count; i++)
    {
        dm_renderer_backend_destroy_shader(i, &context->renderer);
    }
    
    for(i=0; i<vulkan_renderer->pipeline_count; i++)
    {
        dm_vulkan_destroy_pipeline(i, vulkan_renderer);
    }
    
    // fences
    for(i=0; i<vulkan_renderer->fence_count; i++)
    {
        dm_vulkan_destroy_fence(&vulkan_renderer->in_flight_fences[i], vulkan_renderer);
    }
    
    // semaphores
    for(i=0; i<vulkan_renderer->available_semaphore_count; i++)
    {
        vkDestroySemaphore(vulkan_renderer->device.logical, vulkan_renderer->available_semaphores[i], vulkan_renderer->allocator);
    }
    
    for(i=0; i<vulkan_renderer->complete_semaphore_count; i++)
    {
        vkDestroySemaphore(vulkan_renderer->device.logical, vulkan_renderer->complete_semaphores[i], vulkan_renderer->allocator);
    }
    
    // framebuffers
    for(i=0; i<vulkan_renderer->swapchain.image_count; i++)
    {
        dm_vulkan_destroy_framebuffer(&vulkan_renderer->swapchain.framebuffers[i], vulkan_renderer);
    }
    
    // command buffers
    for(i=0; i<vulkan_renderer->swapchain.image_count; i++)
    {
        dm_vulkan_command_buffer_free(vulkan_renderer->device.graphics_command_pool, &vulkan_renderer->command_buffers[i], vulkan_renderer);
    }
    dm_free(vulkan_renderer->command_buffers);
    
    dm_vulkan_destroy_renderpass(&vulkan_renderer->master_renderpass, vulkan_renderer);
    
    // swapchain
    dm_vulkan_destroy_swapchain(&vulkan_renderer->swapchain, vulkan_renderer);
    
    if(vulkan_renderer->device.swapchain_support_info.formats) dm_free(vulkan_renderer->device.swapchain_support_info.formats);
    if(vulkan_renderer->device.swapchain_support_info.present_modes) dm_free(vulkan_renderer->device.swapchain_support_info.present_modes);
    
    vkDestroyCommandPool(vulkan_renderer->device.logical, vulkan_renderer->device.graphics_command_pool, vulkan_renderer->allocator);
    vkDestroyDevice(vulkan_renderer->device.logical, vulkan_renderer->allocator);
    
    // surface
    vkDestroySurfaceKHR(vulkan_renderer->instance, vulkan_renderer->surface, 0);
    
    // live objects
#ifdef DM_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkDestroyDebugUtilsMessengerEXT");
    func(vulkan_renderer->instance, vulkan_renderer->debug_messenger, vulkan_renderer->allocator);
#endif
    
    // instance
    vkDestroyInstance(vulkan_renderer->instance, NULL);
    
    dm_free(context->renderer.internal_renderer);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    DM_VULKAN_GET_COMMAND_BUFFER;
    
    // quick boot out
    if(vulkan_renderer->flags & DM_VULKAN_RENDERER_FLAG_RECREATING_SWAPCHAIN) return false;
    
    // resized
    if(vulkan_renderer->flags & DM_VULKAN_RENDERER_FLAG_RESIZED)
    {
        vulkan_renderer->flags &= ~DM_VULKAN_RENDERER_FLAG_RESIZED;
        
        vkDeviceWaitIdle(vulkan_renderer->device.logical);
        
        dm_vulkan_query_swapchain_support(vulkan_renderer->device.physical, vulkan_renderer->surface, &vulkan_renderer->device.swapchain_support_info);
        dm_vulkan_device_detect_depth_buffer_range(&vulkan_renderer->device);
        
        if(!dm_vulkan_recreate_swapchain(vulkan_renderer->width, vulkan_renderer->height, &vulkan_renderer->swapchain, vulkan_renderer)) return false;
        
        for(uint32_t i=0; i<vulkan_renderer->swapchain.image_count; i++)
        {
            dm_vulkan_command_buffer_free(vulkan_renderer->device.graphics_command_pool, &vulkan_renderer->command_buffers[i], vulkan_renderer);
            dm_vulkan_destroy_framebuffer(&vulkan_renderer->swapchain.framebuffers[i], vulkan_renderer);
        }
        
        vulkan_renderer->master_renderpass.desc.x = 0;
        vulkan_renderer->master_renderpass.desc.y = 0;
        vulkan_renderer->master_renderpass.desc.width  = vulkan_renderer->width;
        vulkan_renderer->master_renderpass.desc.height = vulkan_renderer->height;
        
        dm_vulkan_swapchain_regenerate_framebuffers(&vulkan_renderer->swapchain, vulkan_renderer);
        
        for(uint32_t i=0; i<vulkan_renderer->swapchain.image_count; i++)
        {
            dm_vulkan_command_buffer_allocate(vulkan_renderer->device.graphics_command_pool, &vulkan_renderer->command_buffers[i], vulkan_renderer);
        }
        
        vulkan_renderer->flags &= ~DM_VULKAN_RENDERER_FLAG_RESIZED;
    }
    
    if(!dm_vulkan_fence_wait(UINT64_MAX, &vulkan_renderer->in_flight_fences[vulkan_renderer->current_frame], vulkan_renderer)) return false;
    
    if(!dm_vulkan_swapchain_next_image_index(UINT32_MAX, vulkan_renderer->available_semaphores[vulkan_renderer->current_frame], 0, &vulkan_renderer->image_index, &vulkan_renderer->swapchain, vulkan_renderer)) return false;
    
    dm_vulkan_command_buffer_reset(buffer);
    dm_vulkan_command_buffer_begin(false, false, false, buffer);
    
    VkFramebuffer framebuffer = vulkan_renderer->swapchain.framebuffers[vulkan_renderer->image_index].framebuffer;
    dm_vulkan_begin_renderpass(&vulkan_renderer->master_renderpass, framebuffer, buffer, vulkan_renderer);
    
    return true;
}

bool dm_renderer_backend_end_frame(bool vsync, dm_context* context)
{
#ifdef DM_DEBUG
    VkResult result;
#endif
    
    dm_vulkan_renderer* vulkan_renderer = context->renderer.internal_renderer;
    DM_VULKAN_GET_COMMAND_BUFFER;
    
    dm_vulkan_end_renderpass(&vulkan_renderer->master_renderpass, buffer, vulkan_renderer);
    if(!dm_vulkan_command_buffer_end(buffer)) return false;
    
    if(vulkan_renderer->images_in_flight[vulkan_renderer->image_index] != VK_NULL_HANDLE)
    {
        dm_vulkan_fence_wait(UINT64_MAX, vulkan_renderer->images_in_flight[vulkan_renderer->image_index], vulkan_renderer);
    }
    
    vulkan_renderer->images_in_flight[vulkan_renderer->image_index] = &vulkan_renderer->in_flight_fences[vulkan_renderer->current_frame];
    dm_vulkan_fence_reset(&vulkan_renderer->in_flight_fences[vulkan_renderer->current_frame], vulkan_renderer);
    
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &buffer->buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &vulkan_renderer->complete_semaphores[vulkan_renderer->current_frame];
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &vulkan_renderer->available_semaphores[vulkan_renderer->current_frame];
    
    VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = flags;
    
    DM_VULKAN_FUNC_CHECK(vkQueueSubmit(vulkan_renderer->device.graphics_queue, 1, &submit_info, vulkan_renderer->in_flight_fences[vulkan_renderer->current_frame].fence));
    if(!DM_VULKAN_FUNC_SUCCESS()) return false;
    
    dm_vulkan_command_buffer_update_submitted(buffer);
    
    return dm_vulkan_swapchain_present(vulkan_renderer->device.graphics_queue, vulkan_renderer->device.present_queue, vulkan_renderer->complete_semaphores[vulkan_renderer->current_frame], vulkan_renderer->image_index, &vulkan_renderer->swapchain, vulkan_renderer);
}

void dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    
    vulkan_renderer->width  = width;
    vulkan_renderer->height = height;
    
    vulkan_renderer->flags |= DM_VULKAN_RENDERER_FLAG_RESIZED;
}

/********
COMMANDS
**********/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    DM_VULKAN_GET_RENDERER;
    DM_VULKAN_GET_COMMAND_BUFFER;
    
    VkViewport viewport = { 0 };
    viewport.y        = 0;
    viewport.width    = (float)width;
    viewport.height   = (float)height;
    viewport.maxDepth = 1;
    
    VkRect2D scissor = { 0 };
    scissor.extent.width = width;
    scissor.extent.height = height;
    
    
    vkCmdSetViewport(buffer->buffer, 0, 1, &viewport);
    vkCmdSetScissor(buffer->buffer, 0, 1, &scissor);
    
    vulkan_renderer->master_renderpass.desc.width  = width;
    vulkan_renderer->master_renderpass.desc.height = height;
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

        default:
        DM_LOG_ERROR("Shouldn't be here");
        break;
    }
    
    return VK_FALSE;
}
#endif
