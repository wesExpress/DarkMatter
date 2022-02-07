#ifndef __DM_METAL_RENDERER_H__
#define __DM_METAL_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "rendering/dm_renderer.h"
#include "core/math/dm_math.h"

@class dm_metal_view;

typedef struct dm_metal_render_pass
{
    id<CAMetalDrawable> drawable;
    id<MTLCommandBuffer> command_buffer;
    id <MTLRenderCommandEncoder> command_encoder;
} dm_metal_render_pass;

typedef struct dm_metal_renderer
{
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    dm_metal_view* view;
    dm_vec4 clear_color;
} dm_metal_renderer;

typedef struct dm_internal_pipeline
{
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLDepthStencilState> depth_stencil;
    id<MTLSamplerState> sampler_state;
    dm_metal_render_pass* render_pass;
} dm_internal_pipeline;

typedef struct dm_internal_buffer
{
    id<MTLBuffer> buffer;
} dm_internal_buffer;

typedef struct dm_internal_shader
{
    id<MTLLibrary> library;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;
} dm_internal_shader;

typedef struct dm_internal_texture
{
    id<MTLTexture> texture;
} dm_internal_texture;

typedef struct demo_vertex
{
    dm_vec4 position;
    dm_vec4 color;
} demo_vertex;

#endif

#endif