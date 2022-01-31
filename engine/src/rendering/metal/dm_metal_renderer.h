#ifndef __DM_METAL_RENDERER_H__
#define __DM_METAL_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "rendering/dm_renderer.h"
#include "dm_metal_view.h"

typedef struct dm_metal_renderer
{
    dm_metal_view* metal_view;
} dm_metal_renderer;

#endif

#endif