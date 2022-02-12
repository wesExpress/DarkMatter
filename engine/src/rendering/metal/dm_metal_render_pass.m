#include "dm_metal_render_pass.h"

#ifdef DM_METAL

#include "dm_metal_shader.h"

#include "core/dm_logger.h"
#include "core/dm_mem.h"

@implementation dm_metal_render_pass

- (id) initWithRenderer: (dm_metal_renderer*)renderer AndPass:(dm_render_pass*)pass
{
    self = [super init];

    if(self)
    {
        // sampler
        MTLSamplerDescriptor* sampler_desc = [MTLSamplerDescriptor new];
        sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
        sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;
        sampler_desc.mipFilter = MTLSamplerMipFilterLinear;
        sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;

        _sampler_state = [renderer.device newSamplerStateWithDescriptor:sampler_desc];
        if(!_sampler_state)
        {
            DM_LOG_FATAL("Could not create metal sampler state!");
            return NULL;
        }

        // shader
        char buffer[512];
        snprintf(buffer, sizeof buffer, "shaders/metal/%s.metallib", pass->shader->name);
        NSString* shader_file = [NSString stringWithUTF8String: buffer];
        pass->shader->internal_shader = [[dm_metal_shader_library alloc] initWithShader: pass->shader AndPath: shader_file AndRenderer: renderer];
        if(!pass->shader->internal_shader) return NULL;

        dm_metal_shader_library* shader_lib = pass->shader->internal_shader;

        // pipeline state
        NSString* vertex_name = [NSString stringWithUTF8String: pass->shader->vertex_desc.path];
        NSString* frag_name = [NSString stringWithUTF8String: pass->shader->pixel_desc.path];

        MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
        pipe_desc.colorAttachments[0].pixelFormat = renderer.view.metal_layer.pixelFormat;
        pipe_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        pipe_desc.vertexFunction = [shader_lib.library newFunctionWithName: vertex_name];
        pipe_desc.fragmentFunction = [shader_lib.library newFunctionWithName: frag_name];

        _pipeline_state = [renderer.device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];
        if(!_pipeline_state)
        {
            DM_LOG_FATAL("Could not create metal pipeline state");
            return NULL;
        }

        // uniform buffer
        size_t buffer_size = 0;
        void* buffer_data = NULL;
        for(uint32_t i=0; i<pass->uniforms->count;i++)
        {
            dm_uniform* uniform = dm_list_at(pass->uniforms, i);

            buffer_data = dm_realloc(buffer_data, buffer_size + uniform->desc.data_size);
            void* dest = (char*)buffer_data + buffer_size;
            dm_memcpy(dest, uniform->data, uniform->desc.data_size);
            buffer_size += uniform->desc.data_size;
        }

        _uniform_buffer = [renderer.device newBufferWithBytes:buffer_data length:buffer_size options:MTLResourceOptionCPUCacheModeDefault];
        if(!_uniform_buffer)
        {
            DM_LOG_FATAL("Could not create uniform buffer!");
            return NULL;
        }

        free(buffer_data);
    }

    return self;
}

- (BOOL) beginPass:(dm_metal_renderer*)renderer
{
    if(renderer.drawable)
    {
        CGSize drawable_size = renderer.view.metal_layer.drawableSize;

        MTLTextureDescriptor* tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                                     width:drawable_size.width
                                                                                                    height:drawable_size.height
                                                                                                 mipmapped:NO];
        tex_desc.usage = MTLTextureUsageRenderTarget;
        tex_desc.storageMode = MTLStorageModePrivate;

        id<MTLTexture> texture = [renderer.device newTextureWithDescriptor:tex_desc];

        renderer.command_buffer = [renderer.command_queue commandBuffer];

        // render pass descriptor
        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(renderer.clear_color.x, renderer.clear_color.y, renderer.clear_color.z, renderer.clear_color.w);

        renderer.command_encoder = [renderer.command_buffer renderCommandEncoderWithDescriptor:passDescriptor];

        // pipeline state
        [renderer.command_encoder setRenderPipelineState:_pipeline_state];

        // sampler
        [renderer.command_encoder setFragmentSamplerState:_sampler_state atIndex:0];
    }
    else
    {
        DM_LOG_ERROR("Drawable was NULL...");
    }

    return YES;
}

- (void) endPass
{
    return;
}

- (BOOL) updateUniforms:(dm_render_pass*)pass
{
    size_t buffer_size = 0;
    void* buffer_data = NULL;
    for(uint32_t i=0; i<pass->uniforms->count;i++)
    {
        dm_uniform* uniform = dm_list_at(pass->uniforms, i);

        buffer_data = dm_realloc(buffer_data, buffer_size + uniform->desc.data_size);
        void* dest = (char*)buffer_data + buffer_size;
        dm_memcpy(dest, uniform->data, uniform->desc.data_size);
        buffer_size += uniform->desc.data_size;
    }

    dm_memcpy([_uniform_buffer contents], buffer_data, buffer_size);

    free(buffer_data);

    return YES;
}

@end

#endif