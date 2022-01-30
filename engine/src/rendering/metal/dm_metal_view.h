#ifndef __DM_METAL_VIEW_H__
#define __DM_METAL_VIEW_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"

@interface dm_metal_view : NSView
    @property (readonly) id<MTLDevice> device;
    @property (readonly) CVDisplayLinkRef display_link;
    @property (readonly) dispatch_source_t display_source;
@end

#endif

#endif