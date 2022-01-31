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
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
} dm_internal_pipeline;

typedef struct dm_internal_buffer
{
    id<MTLBuffer> buffer;
} dm_internal_buffer;

typedef struct demo_vertex
{
    dm_vec4 position;
    dm_vec4 color;
} demo_vertex;

#endif

#endif