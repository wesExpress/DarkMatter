#include "dm_metal_shader.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

@implementation dm_metal_shader_library

- (id) create: (dm_shader*) shader :(NSString*) path :(dm_metal_renderer*) renderer
{
    self = [super init];

    if(self)
    {
        _library = [renderer.device newLibraryWithFile: path error: NULL];
        if(!_library)
        {
            DM_LOG_FATAL("Could not create metal library from file: %s", path);
            return NULL;
        }

        NSString* vertex_func_name = [[NSString alloc] initWithUTF8String:shader->vertex_desc.path];
        _vertex_func = [_library newFunctionWithName:vertex_func_name];
        if(!_vertex_func)
        {
            DM_LOG_FATAL("Could not create vertex function with name: %s", shader->vertex_desc.path);
            return NULL;
        }

        NSString* fragment_func_name = [[NSString alloc] initWithUTF8String:shader->pixel_desc.path];
        _fragment_func = [_library newFunctionWithName:fragment_func_name];
        if(!_fragment_func)
        {
            DM_LOG_FATAL("Could not create fragment function with name: %s", shader->pixel_desc.path);
            return NULL;
        }
    }

    return self;
}

@end

#endif