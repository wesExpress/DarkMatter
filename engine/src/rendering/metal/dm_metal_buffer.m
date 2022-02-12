#include "dm_metal_buffer.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

@implementation dm_metal_buffer

- (id) initWithData: (void*)data AndLength: (size_t)length AndRenderer: (dm_metal_renderer*)renderer
{
    self = [super init];

    if(self)
    {
        _buffer = [renderer.device newBufferWithBytes:data length:length options:MTLResourceOptionCPUCacheModeDefault];
    }

    return self;
}

- (id) initWithLength: (size_t)length AndRenderer: (dm_metal_renderer*)renderer
{
    self = [super init];

    if(self)
    {
        _buffer = [renderer.device newBufferWithLength: length options:MTLResourceOptionCPUCacheModeDefault];
    }

    return self;
}

@end

#endif