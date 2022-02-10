#ifndef __DM_METAL_SHADER_H__
#define __DM_METAL_SHADER_H__

#include "core/dm_defines.h"

#ifdef DM_METAL

#include "dm_metal_renderer.h"
#include <stdbool.h>

@interface dm_metal_shader_library : NSObject

@property (nonatomic, strong) id<MTLLibrary> library;
@property (nonatomic, strong) id<MTLFunction> vertex_func;
@property (nonatomic, strong) id<MTLFunction> fragment_func;

- (id)create: (dm_shader*)shader :(NSString*)path :(dm_metal_renderer*)renderer;

@end

#endif

#endif