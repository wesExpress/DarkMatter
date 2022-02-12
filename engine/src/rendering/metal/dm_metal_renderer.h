#ifndef __DM_METAL_RENDERER_H__
#define __DM_METAL_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "dm_metal_view.h"
#include "rendering/dm_renderer.h"
#include "core/math/dm_math.h"

@interface dm_metal_renderer : NSObject
    
@property (strong, nonatomic) id<MTLDevice> device;
@property (strong, nonatomic) id<MTLCommandQueue> command_queue;
@property (strong, nonatomic) id<MTLCommandBuffer> command_buffer;
@property (strong, nonatomic) id<MTLRenderCommandEncoder> command_encoder;
@property (strong, nonatomic) id<MTLBuffer> index_buffer;
@property (strong, nonatomic) dm_metal_view* view;
@property (strong, nonatomic) id<CAMetalDrawable> drawable;

@property (nonatomic) dm_vec4 clear_color;

- (id) initWithFrame: (NSRect)frame;

// my functions
- (void) setIndexBuffer: (dm_buffer*)buffer;
- (BOOL) beginFrame;
- (void) endFrame;
- (void) setViewport:(dm_viewport)viewport;
- (void) clearScreen: (dm_color)color;
- (void) drawArrays: (uint32_t)first Count: (uint32_t)count;
- (void) drawIndexed: (uint32_t)num Offset:(uint32_t)offset;
- (void) drawInstanced: (uint32_t)num_indices Offset:(uint32_t)offset NumInstances:(uint32_t)num_insts;

@end

typedef struct dm_internal_texture
{
    id<MTLTexture> texture;
} dm_internal_texture;

#endif

#endif