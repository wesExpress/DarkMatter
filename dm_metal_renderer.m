#include "dm.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/NSWindow.h>

typedef struct dm_metal_render_target_t
{
    id<MTLCommandEncoder> encoder;

    MTLLoadAction color_load_op, depth_load_op;
    MTLStoreAction color_store_op, depth_store_op;
} dm_metal_render_target;

typedef struct dm_metal_renderer_t
{
    id<MTLDevice> device;
    CAMetalLayer *swapchain;

    id<CAMetalDrawable> drawable;

    id<MTLCommandQueue> queue;
    id<MTLCommandBuffer> cmd;

    dm_metal_render_target rts[DM_MAX_TEXTURES];
    u32 rt_count;

} dm_metal_renderer;

extern void *dm_window_get_native_window(dm_context *context);

bool dm_renderer_init(dm_context* context)
{
    LOG_INFO("Initializing metal backend...");

    dm_metal_renderer *renderer = dm_arena_alloc(&context->arena, sizeof(dm_metal_renderer), &context->renderer.offset);
    if(!renderer) return false;

    renderer->device = MTLCreateSystemDefaultDevice();

    renderer->swapchain = [CAMetalLayer layer];
    renderer->swapchain.device = renderer->device;
    renderer->swapchain.opaque = YES;

    NSWindow *window = dm_window_get_native_window(context);
    window.contentView.layer = renderer->swapchain;
    window.contentView.wantsLayer = YES;

    renderer->queue = [renderer->device newCommandQueue];

    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    [renderer->queue release];
    [renderer->swapchain release];
    [renderer->device release];
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    renderer->drawable = [renderer->swapchain nextDrawable];
    if(!renderer->drawable)
    {
        LOG_ERROR("nextDrawable failed");
        return false;
    }

    renderer->cmd = [renderer->queue commandBuffer];

    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    [renderer->cmd presentDrawable:renderer->drawable];
    [renderer->cmd commit];
    
    return true;
}

bool dm_renderer_resize(dm_context *context, u16 width, u16 height)
{
    return true;
}

size_t dm_renderer_get_internal_size()
{
    return sizeof(dm_metal_renderer);
}

bool dm_renderer_create_raster_pipeline(dm_context *context, dm_raster_pipe_desc desc, dm_handle *handle)
{
    return true;
}

MTLLoadAction dm_metal_convert_load(dm_render_attachment_load_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported load action");
            LOG_WARN("Returning MTLLoadActionLoad");
        case DM_RENDER_ATTACHMENT_LOAD_OP_LOAD:
            return MTLLoadActionLoad;
        case DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR:
            return MTLLoadActionClear;
        case DM_RENDER_ATTACHMENT_LOAD_OP_DONT_CARE:
            return MTLLoadActionDontCare;

    }
}

MTLStoreAction dm_metal_convert_store(dm_render_attachment_store_op op)
{
    switch(op)
    {
        default:
            LOG_WARN("Unknown/unsupported store action");
            LOG_WARN("Returning MTLStoreActionStore");
        case DM_RENDER_ATTACHMENT_STORE_OP_STORE:
            return MTLStoreActionStore;
        case DM_RENDER_ATTACHMENT_STORE_OP_DONT_CARE:
            return MTLStoreActionDontCare;
    }
}

bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_handle *handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_metal_render_target render_target = { 
        .color_load_op=dm_metal_convert_load(desc.color_attachment.load_op),
        .color_store_op=dm_metal_convert_store(desc.color_attachment.store_op),
        .depth_load_op=dm_metal_convert_load(desc.depth_attachment.load_op),
        .depth_store_op=dm_metal_convert_store(desc.depth_attachment.store_op)
    };

    //
    renderer->rts[renderer->rt_count] = render_target;
    handle->r_type = DM_RESOURCE_TYPE_RENDER_TARGET;
    handle->index = renderer->rt_count++;

    return true;
}

bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_handle *handle)
{
    return true;
}

bool dm_renderer_create_texture(dm_context *context, dm_texture2d_desc desc, dm_handle *handle)
{
    return true;
}

bool dm_renderer_create_sampler(dm_context *context, dm_sampler_desc desc, dm_handle *handle)
{
    return true;
}

bool dm_renderer_upload_resources_to_heap(dm_context *context, dm_handle *resources[], u32 count)
{
    return true;
}

bool dm_renderer_upload_samplers_to_heap(dm_context *context, dm_handle *samplers[], u32 count)
{
    return true;
}

u64 dm_renderer_get_buffer_address(dm_context *context, dm_handle handle)
{
    return 0;
}

bool dm_renderer_create_compute_pipeline(dm_context *context, dm_handle *handle)
{
    return true;
}

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_handle handle, float r, float g, float b, float a, float d)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_render_target *target = &renderer->rts[handle.index];

    MTLClearColor clear = MTLClearColorMake(r, g, b, a);

    MTLRenderPassDescriptor *desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.colorAttachments[0].clearColor = clear;
    desc.colorAttachments[0].loadAction = target->color_load_op;
    desc.colorAttachments[0].storeAction = target->color_store_op;
    desc.colorAttachments[0].texture = renderer->drawable.texture;
    desc.depthAttachment.clearDepth = d;
    desc.depthAttachment.loadAction = target->depth_load_op;
    desc.depthAttachment.storeAction = target->depth_store_op;

    target->encoder = [renderer->cmd renderCommandEncoderWithDescriptor:desc];
}

void dm_render_command_end_rendering(dm_context *context, dm_handle handle)
{
    dm_metal_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);
    dm_metal_render_target *target = &renderer->rts[handle.index];

    [target->encoder endEncoding];
}

void dm_render_command_bind_pipeline(dm_context *context, dm_handle handle)
{
}

void dm_render_command_bind_index_buffer(dm_context *context, dm_handle handle, size_t offset)
{
}

void dm_render_command_push_constants(dm_context *context, dm_handle handle)
{
}

void dm_render_command_push_data(dm_context *context, void *data, size_t size)
{
}

void dm_render_command_draw(dm_context *context, u32 index_count, u32 instance_count)
{
}

void dm_render_command_update_buffer(dm_context *context, dm_handle handle, void *data, size_t size)
{
}

void dm_render_command_copy_buffer(dm_context *context, dm_handle src, dm_handle dst)
{
}

bool dm_render_command_update_texture(dm_context *context, dm_handle handle, void* data, size_t size, u16 width, u16 height)
{
    return true;
}

void dm_render_command_copy_texture(dm_context *context, dm_handle src, dm_handle dst)
{
}

// compute commands
void dm_compute_command_push_data(dm_context *context, void *data, size_t size)
{
}

void dm_compute_command_bind_pipeline(dm_context *context, dm_handle handle)
{
}

void dm_compute_command_dispatch(dm_context *context, u16 x, u16 y, u16 z)
{
}
