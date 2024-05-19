#include "dm.h"

#include "platform/dm_platform_mac.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>

#include <stdbool.h>

#define DM_METAL_NUM_FRAMES 3

typedef struct dm_metal_vertex_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        stride, count;
} dm_metal_vertex_buffer;

typedef struct dm_metal_index_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        index_size, count;
} dm_metal_index_buffer;

typedef struct dm_metal_constant_buffer_t
{
    id<MTLBuffer> buffer[DM_METAL_NUM_FRAMES];
    size_t        size;
} dm_metal_constant_buffer;

typedef struct dm_metal_texture_t
{
    id<MTLTexture> texture[DM_METAL_NUM_FRAMES];
    uint32_t       width, height;
} dm_metal_texture;

#ifdef DM_RAYTRACING
typedef struct dm_metal_as_t
{
    id<MTLBuffer> scratch_buffer;
    id<MTLBuffer> result_buffer;
    
    id<MTLAccelerationStructure> as;
    id<MTLAccelerationStructure> compact_as;
} dm_metal_as;

typedef struct dm_metal_blas_t
{
    dm_metal_as as[DM_METAL_NUM_FRAMES];
} dm_metal_blas;

typedef struct dm_metal_tlas_t
{
    dm_metal_as as[DM_METAL_NUM_FRAMES];
    
    id<MTLBuffer> instance_buffer[DM_METAL_NUM_FRAMES];
} dm_metal_tlas;

#define DM_METAL_MAX_BLAS 10
typedef struct dm_metal_acceleration_structure_t
{
    dm_metal_blas blas[DM_METAL_MAX_BLAS];
    uint8_t       blas_count;
    
    dm_metal_tlas tlas;
    
    NSMutableArray* primitive_as[DM_METAL_NUM_FRAMES];
} dm_metal_acceleration_structure;

typedef struct dm_metal_shader_binding_table_t
{
    id<MTLIntersectionFunctionTable> sbt[DM_METAL_NUM_FRAMES];
    
    uint32_t record_count, miss_count, hit_group_count, max_instance_count;
    size_t   record_size;
} dm_metal_shader_binding_table;

typedef struct dm_metal_raytracing_pipeline_t
{
    dm_metal_shader_binding_table sbt;
    
    id<MTLComputePipelineState> state;
    id<MTLLibrary>              library;
} dm_metal_raytracing_pipeline;
#endif

typedef struct dm_metal_pipeline_t
{
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLDepthStencilState>   depth_stencil_state;
    id<MTLSamplerState>        sampler_state;

    id<MTLLibrary>  library;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;

    uint32_t uniform_offset;
    
    MTLCullMode cull_mode;
    MTLWinding  winding;

    MTLPrimitiveType primitive_type;
    bool             wireframe;
} dm_metal_pipeline;

#define DM_METAL_MAX_RESOURCE 1000
typedef struct dm_metal_renderer_t
{
    id<MTLDevice>        device;

    id<MTLCommandQueue>          command_queue;
    
    id<MTLCommandBuffer>         command_buffer;
    id<MTLRenderCommandEncoder>  command_encoder;

    id<MTLCommandBuffer>         compute_command_buffer;
    id<MTLComputeCommandEncoder> compute_command_encoder;

    MTLViewport          active_viewport;

    CAMetalLayer*       swapchain;
    id<MTLTexture>      depth_texture;
    id<CAMetalDrawable> render_target;
    
    dispatch_semaphore_t semaphore;
    
    uint8_t current_frame_index;

    dm_vec4 clear_color;

    dm_render_handle     active_shader;
    dm_compute_handle    active_compute_shader;
    dm_render_handle     active_pipeline;
    MTLPrimitiveType     active_primitive;

    dm_metal_vertex_buffer   vertex_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_index_buffer    index_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_constant_buffer constant_buffers[DM_METAL_MAX_RESOURCE];
    dm_metal_texture         textures[DM_METAL_MAX_RESOURCE];
    dm_metal_pipeline        pipelines[DM_METAL_MAX_RESOURCE];

    uint32_t vb_count, ib_count, cb_count, texture_count, pipeline_count;
    
#ifdef DM_RAYTRACING
    dm_metal_acceleration_structure accel_structs[10];
    dm_metal_raytracing_pipeline    rt_pipes[10];
    
    uint32_t as_count, rt_pipe_count;
#endif
} dm_metal_renderer;

#define DM_METAL_GET_RENDERER dm_metal_renderer* metal_renderer = renderer->internal_renderer

#define DM_METAL_BUFFER_ALIGNMENT 512

#define DM_METAL_ALIGN(VAL, ALIGNMENT) ((VAL + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT

void dm_metal_destroy_vertex_buffer(dm_metal_vertex_buffer* buffer);
void dm_metal_destroy_index_buffer(dm_metal_index_buffer* buffer);
void dm_metal_destroy_constant_buffer(dm_metal_constant_buffer* buffer);
void dm_metal_destroy_texture(dm_metal_texture* texture);
void dm_metal_destroy_pipeline(dm_metal_pipeline* pipeline);

/***********************
 RENDERER BACKEND
*****************************/
bool dm_renderer_backend_init(dm_context* context)
{
    DM_LOG_DEBUG("Initializing Metal backend...");
    
    dm_internal_apple_data* internal_data = context->platform_data.internal_data;
    NSRect frame = [internal_data->content_view getWindowFrame];

    context->renderer.internal_renderer = dm_alloc(sizeof(dm_metal_renderer));
    dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;

    metal_renderer->device = MTLCreateSystemDefaultDevice();
    if(!metal_renderer->device)
    {
        DM_LOG_FATAL("Could not create Metal device");
        return false;
    }

    // swapchain
    metal_renderer->swapchain        = [CAMetalLayer layer];
    metal_renderer->swapchain.device = metal_renderer->device;
    metal_renderer->swapchain.opaque = YES;

    metal_renderer->command_queue = [metal_renderer->device newCommandQueue];
    if(!metal_renderer->command_queue)
    {
        DM_LOG_FATAL("Could not create metal command queue.");
        return false;
    }

    // must set the content view's layer to our metal layer
    [internal_data->content_view setWantsLayer: YES];
    [internal_data->content_view setLayer:metal_renderer->swapchain];
    
    NSSize layer_size = internal_data->content_view.layer.frame.size;
    CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
    metal_renderer->swapchain.contentsScale = scale;
    NSSize drawable_size = NSMakeSize(layer_size.width * scale, layer_size.height * scale);
    metal_renderer->swapchain.drawableSize = drawable_size;
    
    // depth texture
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:drawable_size.width height:drawable_size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage       = MTLTextureUsageRenderTarget;
    
    metal_renderer->depth_texture       = [metal_renderer->device newTextureWithDescriptor:descriptor];
    metal_renderer->depth_texture.label = @"DepthStencil";
    if(!metal_renderer->depth_texture)
    {
        DM_LOG_FATAL("Could not create depth texture");
        return false;
    }
    
    [descriptor release];
    
    // semaphore
    metal_renderer->semaphore = dispatch_semaphore_create(DM_METAL_NUM_FRAMES);
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_metal_renderer* metal_renderer = context->renderer.internal_renderer;
    
    // resource destruction
    for(uint32_t i=0; i<metal_renderer->vb_count; i++)
    {
        dm_metal_destroy_vertex_buffer(&metal_renderer->vertex_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->ib_count; i++)
    {
        dm_metal_destroy_index_buffer(&metal_renderer->index_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->cb_count; i++)
    {
        dm_metal_destroy_constant_buffer(&metal_renderer->constant_buffers[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->texture_count; i++)
    {
        dm_metal_destroy_texture(&metal_renderer->textures[i]);
    }
    
    for(uint32_t i=0; i<metal_renderer->pipeline_count; i++)
    {
        dm_metal_destroy_pipeline(&metal_renderer->pipelines[i]);
    }
    
    // renderer destruction
    [metal_renderer->swapchain release];
    [metal_renderer->depth_texture release];
    [metal_renderer->command_encoder release];
    [metal_renderer->compute_command_encoder release];
    [metal_renderer->command_queue release];
    [metal_renderer->device release];
    
    return true;
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
#if 0
    dispatch_semaphore_wait(metal_renderer->semaphore, DISPATCH_TIME_FOREVER);
    
    [metal_renderer->command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(metal_renderer->semaphore);
    }];
#endif
    
    metal_renderer->current_frame_index = (metal_renderer->current_frame_index + 1) % DM_METAL_NUM_FRAMES;
    
    return true;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    return true;
}

bool dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    return true;
}

/******************************
 RESOURCE CREATION
*****************************/
bool dm_renderer_backend_create_vertex_buffer(const dm_vertex_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_vertex_buffer internal_buffer = { 0 };
    
    internal_buffer.count  = desc.count;
    internal_buffer.stride = desc.stride;
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        if(desc.data)
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:desc.data length:desc.size options:MTLResourceOptionCPUCacheModeDefault];
        }
        else
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithLength:desc.size options:MTLResourceOptionCPUCacheModeDefault];
        }
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal vertex buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->vertex_buffers + metal_renderer->vb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->vb_count++;
    
    return true;
}

void dm_metal_destroy_vertex_buffer(dm_metal_vertex_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_index_buffer(const dm_index_buffer_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_index_buffer internal_buffer = { 0 };
    
    internal_buffer.count      = desc.count;
    internal_buffer.index_size = desc.size / desc.count;
    
    if(!desc.data)
    {
        DM_LOG_FATAL("Need data to create index buffer");
        return false;
    }
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:desc.data length:desc.size options:MTLResourceOptionCPUCacheModeDefault];
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal vertex buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->index_buffers + metal_renderer->ib_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->ib_count++;
    
    return true;
}

void dm_metal_destroy_index_buffer(dm_metal_index_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_constant_buffer(const void* data, size_t data_size, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_constant_buffer internal_buffer = { 0 };
    
    internal_buffer.size = data_size;
    
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        if(data)
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithBytes:data length:data_size options:MTLResourceOptionCPUCacheModeDefault];
        }
        else
        {
            internal_buffer.buffer[i] = [metal_renderer->device newBufferWithLength:data_size options:MTLResourceOptionCPUCacheModeDefault];
        }
        
        if(internal_buffer.buffer[i]) continue;
        
        DM_LOG_FATAL("Creating Metal constant buffer failed");
        return false;
    }
    
    //
    dm_memcpy(metal_renderer->constant_buffers + metal_renderer->cb_count, &internal_buffer, sizeof(internal_buffer));
    *handle = metal_renderer->cb_count++;
    
    return true;
}

void dm_metal_destroy_constant_buffer(dm_metal_constant_buffer* buffer)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        [buffer->buffer[i] release];
    }
}

bool dm_renderer_backend_create_pipeline(const dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

void dm_metal_destroy_pipeline(dm_metal_pipeline* pipeline)
{
    
}

bool dm_renderer_backend_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_create_texture(const void* data, uint32_t width, uint32_t height, dm_render_handle* handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_resize_texutre(const void* data, uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

void dm_metal_destroy_texture(dm_metal_texture* texture)
{
    
}

#ifdef DM_RAYTRACING
bool dm_metal_create_as(MTLAccelerationStructureDescriptor* descriptor, dm_metal_as* as, dm_metal_renderer* metal_renderer)
{
    id<MTLCommandBuffer>                       command_buffer  = [metal_renderer->command_queue commandBuffer];
    id<MTLAccelerationStructureCommandEncoder> command_encoder = [command_buffer accelerationStructureCommandEncoder];
    
    MTLAccelerationStructureSizes sizes = [metal_renderer->device accelerationStructureSizesWithDescriptor:descriptor];

    // allocate memory for the acceleration structure
    as->as = [metal_renderer->device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
    if(!as->as)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure");
        return false;
    }
    
    // build the scratch buffer
    as->scratch_buffer = [metal_renderer->device newBufferWithLength:sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
    if(!as->scratch_buffer)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure scratch buffer");
        return false;
    }
    
    as->result_buffer = [metal_renderer->device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];
    if(!as->result_buffer)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure result buffer");
        return false;
    }
    
    // actually build the acceleration structure
    [command_encoder buildAccelerationStructure:as->as
                                     descriptor:descriptor
                                  scratchBuffer:as->scratch_buffer
                            scratchBufferOffset:0];
    if(!as->as)
    {
        DM_LOG_FATAL("Could not create Metal acceleration structure");
        return false;
    }
    
    [command_encoder writeCompactedAccelerationStructureSize:as->as toBuffer:as->result_buffer offset:0];
    
    [command_encoder endEncoding];
    
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    
    // compact acceleration structure
    const uint32_t compacted_size = *(uint32_t *)as->result_buffer.contents;
    
    as->compact_as = [metal_renderer->device newAccelerationStructureWithSize:compacted_size];
    if(!as->compact_as)
    {
        DM_LOG_FATAL("Could not create Metal compact acceleration structure");
        return false;
    }
    
    command_buffer = [metal_renderer->command_queue commandBuffer];

    command_encoder = [command_buffer accelerationStructureCommandEncoder];
    
    [command_encoder copyAndCompactAccelerationStructure:as->as toAccelerationStructure:as->compact_as];
    
    [command_encoder endEncoding];
    [command_buffer commit];
    
    return true;
}

bool dm_metal_create_blas(dm_blas_desc desc, dm_metal_acceleration_structure* internal_as, dm_metal_renderer* metal_renderer)
{
    const uint8_t blas_index = internal_as->blas_count;
    dm_metal_blas* blas = &internal_as->blas[blas_index];
    
    const dm_render_handle vb_handle = desc.vertex_buffer;
    const dm_render_handle ib_handle = desc.index_buffer;
    
    const dm_metal_vertex_buffer* vb = &metal_renderer->vertex_buffers[vb_handle];
    const dm_metal_index_buffer*  ib = NULL;
    if(desc.index_buffer!=DM_RENDER_HANDLE_INVALID) ib = &metal_renderer->index_buffers[ib_handle];
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        internal_as->primitive_as[i] = [[NSMutableArray alloc] init];
        
        MTLPrimitiveAccelerationStructureDescriptor *accel_descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        
        switch(desc.geom_type)
        {
            case DM_BLAS_GEOMETRY_TYPE_TRIANGLES:
            {
                MTLAccelerationStructureTriangleGeometryDescriptor *geom_descriptor = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
                
                geom_descriptor.vertexBuffer = vb->buffer[i];
                geom_descriptor.indexBuffer  = NULL;
                
                geom_descriptor.vertexStride  = vb->stride;
                geom_descriptor.triangleCount = vb->count / 3;
                
                if(desc.index_buffer!=DM_RENDER_HANDLE_INVALID)
                {
                    geom_descriptor.indexBuffer = ib->buffer[i];
                    geom_descriptor.indexType   = MTLIndexTypeUInt32;
                }
                
                accel_descriptor.geometryDescriptors = @[ geom_descriptor ];
            } break;
                
            default:
                DM_LOG_FATAL("Unknown or unsupported Metal gemoetry type for BLAS");
                return false;
        }
        
        if(!dm_metal_create_as(accel_descriptor, &blas->as[i], metal_renderer)) return false;
        
        // add to the primitive blas list
        [internal_as->primitive_as[i] addObject:blas->as[i].compact_as];
        
        [accel_descriptor release];
    }
    
    internal_as->blas_count++;
    
    return true;
}

bool dm_metal_create_tlas(dm_acceleration_structure_desc desc, dm_metal_acceleration_structure* internal_as, dm_metal_renderer* metal_renderer)
{
    dm_metal_tlas* tlas = &internal_as->tlas;
    
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        tlas->instance_buffer[i] = [metal_renderer->device newBufferWithLength:sizeof(MTLAccelerationStructureInstanceDescriptor) * desc.tlas_desc.max_instance_count options:MTLResourceStorageModeManaged];
        
        MTLAccelerationStructureInstanceDescriptor *instance_descriptors = (MTLAccelerationStructureInstanceDescriptor *)tlas->instance_buffer[i].contents;
        
        uint32_t i_offset = 0;
        for(uint32_t j=0; j<desc.tlas_desc.instance_count; j++)
        {
            // associate the instance's mesh with the appropriate blas
            instance_descriptors[j].accelerationStructureIndex = desc.tlas_desc.instance_meshes[j];
            
            instance_descriptors[j].options = MTLAccelerationStructureInstanceOptionOpaque;
            
            instance_descriptors[j].mask = 0xFF;
            
            // InstanceContributionToHitgroupOffset
            instance_descriptors[j].intersectionFunctionTableOffset = i_offset;
            
            i_offset += desc.hit_group_count;
            
            if(!desc.tlas_desc.instance_transforms) continue;
            
            dm_memcpy(instance_descriptors[j].transformationMatrix.columns, desc.tlas_desc.instance_transforms[j], sizeof(float) * 4 * 3);
        }
        
        MTLInstanceAccelerationStructureDescriptor *accel_descriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];

        accel_descriptor.instancedAccelerationStructures = internal_as->primitive_as[i];
        accel_descriptor.instanceCount                   = desc.tlas_desc.max_instance_count;
        accel_descriptor.instanceDescriptorBuffer        = tlas->instance_buffer[i];
        
        if(!dm_metal_create_as(accel_descriptor, &tlas->as[i], metal_renderer)) return false;
        
        [accel_descriptor release];
    }
    
    return true;
}

bool dm_renderer_backend_create_acceleration_structure(dm_acceleration_structure_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_acceleration_structure internal_as = { 0 };
    
    // bottom level acceleration structures
    for(uint32_t i=0; i<desc.blas_count; i++)
    {
        if(!dm_metal_create_blas(desc.blas_descs[i], &internal_as, metal_renderer)) return false;
    }
    
    // top level
    if(!dm_metal_create_tlas(desc, &internal_as, metal_renderer)) return false;
    
    //
    dm_memcpy(metal_renderer->accel_structs + metal_renderer->as_count, &internal_as, sizeof(internal_as));
    *handle = metal_renderer->as_count++;
    
    return true;
}

void dm_metal_destroy_as(dm_metal_as* as)
{
    [as->as release];
    [as->compact_as release];
    [as->result_buffer release];
    [as->scratch_buffer release];
}

void dm_metal_destroy_acceleration_structure(dm_metal_acceleration_structure* as)
{
    for(uint32_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        for(uint32_t j=0; j<as->blas_count; j++)
        {
            dm_metal_destroy_as(&as->blas[j].as[i]);
        }
        
        dm_metal_destroy_as(&as->tlas.as[i]);
        
        [as->primitive_as[i] release];
    }
}

DM_INLINE
bool dm_metal_sbt_load_function(const char* name, NSMutableDictionary <NSString *, id <MTLFunction>> *functions, dm_metal_raytracing_pipeline* internal_pipe)
{
    MTLFunctionConstantValues *constants = [[MTLFunctionConstantValues alloc] init];

    // The first constant is the stride between entries in the resource buffer. The sample
    // uses this stride to allow intersection functions to look up any resources they use.
    [constants setConstantValue:internal_pipe->sbt.record_size type:MTLDataTypeUInt atIndex:0];

    NSError *error;

    // Load the function from the Metal library.
    functions[[NSString stringWithUTF8String:name]] = [internal_pipe->library newFunctionWithName:[NSString stringWithUTF8String:name] constantValues:constants error:&error];

    if(!functions[[NSString stringWithUTF8String:name]])
    {
        DM_LOG_FATAL("Failed to create Metal function: %s", name);
        DM_LOG_FATAL("Error: %s", error);
        return false;
    }
    
    [constants release];
    
    return true;
}

DM_INLINE
void dm_metal_insert_into_sbt(const char* name, uint32_t* index, id<MTLIntersectionFunctionTable> sbt, dm_metal_raytracing_pipeline* internal_pipe, NSMutableDictionary <NSString *, id <MTLFunction>> *functions)
{
    id <MTLFunction> intersection_function = functions[[NSString stringWithUTF8String:name]];
    id <MTLFunctionHandle>          handle = [internal_pipe->state functionHandleWithFunction:intersection_function];
    [sbt setFunction:handle atIndex:(*index)++];
}

bool dm_metal_create_shader_binding_table(dm_raytracing_pipeline_desc desc, dm_metal_raytracing_pipeline* internal_pipe, dm_metal_renderer* metal_renderer)
{
    NSMutableDictionary <NSString *, id <MTLFunction>> *functions = [NSMutableDictionary dictionary];
    
    // raygen
    if(!dm_metal_sbt_load_function(desc.raygen, functions, internal_pipe)) return false;
    
    // misses
    for(uint8_t i=0; i<desc.miss_count; i++)
    {
        if(!dm_metal_sbt_load_function(desc.miss[i], functions, internal_pipe)) return false;
    }
    
    // hit groups
    for(uint8_t i=0; i<desc.hit_group_count; i++)
    {
        if(desc.hit_groups[i].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_ANY_HIT)
        {
            if(!dm_metal_sbt_load_function(desc.hit_groups[i].any_hit, functions, internal_pipe)) return false;
        }
        
        if(desc.hit_groups[i].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_CLOSEST_HIT)
        {
            if(!dm_metal_sbt_load_function(desc.hit_groups[i].closest_hit, functions, internal_pipe)) return false;
        }
        
        if(desc.hit_groups[i].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_INTERSECTION)
        {
            if(!dm_metal_sbt_load_function(desc.hit_groups[i].intersection, functions, internal_pipe)) return false;
        }
    }
    
    for(uint8_t i=0; i<DM_METAL_NUM_FRAMES; i++)
    {
        uint32_t index = 0;
        
        MTLIntersectionFunctionTableDescriptor *sbt_descriptor = [[MTLIntersectionFunctionTableDescriptor alloc] init];
        sbt_descriptor.functionCount = 1 + desc.miss_count + desc.hit_group_count * desc.max_instance_count;
        
        internal_pipe->sbt.sbt[i] = [internal_pipe->state newIntersectionFunctionTableWithDescriptor:sbt_descriptor];
        
        // raygen
        dm_metal_insert_into_sbt(desc.raygen, &index, internal_pipe->sbt.sbt[i], internal_pipe, functions);
        
        // miss
        for(uint8_t j=0; j<desc.miss_count; j++)
        {
            dm_metal_insert_into_sbt(desc.miss[j], &index, internal_pipe->sbt.sbt[i], internal_pipe, functions);
        }
        
        // hit groups
        for(uint8_t k=0; k<desc.max_instance_count; k++)
        {
            for(uint8_t j=0; j<desc.hit_group_count; j++)
            {
                if(desc.hit_groups[j].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_ANY_HIT)
                {
                    dm_metal_insert_into_sbt(desc.hit_groups[j].any_hit, &index, internal_pipe->sbt.sbt[i], internal_pipe, functions);
                }
                
                if(desc.hit_groups[j].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_CLOSEST_HIT)
                {
                    dm_metal_insert_into_sbt(desc.hit_groups[j].closest_hit, &index, internal_pipe->sbt.sbt[i], internal_pipe, functions);
                }
                
                if(desc.hit_groups[j].flags && DM_RAYTRACING_PIPELINE_HIT_GROUP_FLAG_INTERSECTION)
                {
                    dm_metal_insert_into_sbt(desc.hit_groups[j].intersection, &index, internal_pipe->sbt.sbt[i], internal_pipe, functions);
                }
            }
        }
        
        //
        [sbt_descriptor release];
    }
    
    //
    [functions release];
    
    return true;
}

bool dm_renderer_backend_create_raytracing_pipeline(dm_raytracing_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_METAL_GET_RENDERER;
    
    dm_metal_raytracing_pipeline internal_pipe = { 0 };
    
    if(!dm_metal_create_shader_binding_table(desc, &internal_pipe, metal_renderer)) return false;
    
    //
    dm_memcpy(metal_renderer->rt_pipes + metal_renderer->rt_pipe_count, &internal_pipe, sizeof(internal_pipe));
    *handle = metal_renderer->rt_pipe_count++;
    
    return true;
}

bool dm_renderer_backend_rt_pipe_global_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_rt_pipe_raygen_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_rt_pipe_miss_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t miss_index, uint32_t slot, dm_render_handle handle, dm_render_handle pipe_handle, dm_renderer* renderer)
{
    return true;
}

bool dm_renderer_backend_rt_pipe_hit_insert_resource(dm_raytracing_pipeline_shader_param_type type, uint32_t hit_group, uint32_t slot, uint32_t instance, dm_render_handle handle, dm_render_handle pipe_handle, dm_renderer* renderer)
{
    return true;
}
#endif

/*****************************
 RENDER COMMANDS
*********************************/
bool dm_render_command_backend_begin_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_end_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_vertex_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_index_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_constant_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_vertex_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_constant_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
    
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    
}

void dm_render_command_backend_draw_instanced(uint32_t index_count, uint32_t vertex_count, uint32_t inst_count, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return true;
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    
}

#ifdef DM_RAYTRACING
bool dm_render_command_backend_update_acceleration_structure_instance(dm_render_handle handle, uint32_t instance_id, void* data, size_t data_size, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_update_acceleration_structure_tlas(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_add_global_param(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle vb_handle, dm_render_handle pipe_handle,  dm_renderer* renderer)
{
    
}

bool dm_render_command_backend_add_raygen_param(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, dm_render_handle vb_handle, dm_render_handle pipe_handle,  dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_add_hit_group_param(dm_raytracing_pipeline_shader_param_type type, uint32_t slot, uint32_t hit_group, uint32_t instance, dm_render_handle vb_handle, dm_render_handle pipe_handle,  dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_bind_raytracing_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_raytracing_pipeline_dispatch_rays(uint32_t width, uint32_t height, dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}

bool dm_render_command_backend_copy_texture_to_screen(dm_render_handle handle, dm_renderer* renderer)
{
    return true;
}
#endif
