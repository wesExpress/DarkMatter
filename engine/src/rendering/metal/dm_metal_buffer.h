#ifndef __DM_METAL_BUFFER_H__
#define __DM_METAL_BUFFER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"

@interface dm_metal_buffer : NSObject

@property (strong, nonatomic) id<MTLBuffer> buffer;

- (id) initWithData: (void*)data AndLength: (size_t)length AndRenderer: (dm_metal_renderer*)renderer;
- (id) initWithLength: (size_t)length AndRenderer: (dm_metal_renderer*)renderer;

@end

#endif

#endif