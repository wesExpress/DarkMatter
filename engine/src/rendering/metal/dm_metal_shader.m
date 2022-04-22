#include "dm_metal_shader.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

@implementation dm_metal_shader_library

- (id) createShader:(dm_shader*)shader withPath:(NSString*)path andRenderer:(dm_metal_renderer*)renderer
{
    self = [super init];

    if(self)
    {
        _library = [renderer.device newLibraryWithFile: path error: NULL];
        if(!_library)
        {
            DM_LOG_FATAL("Could not create metal library from file: %s", [path UTF8String]);
            return NULL;
        }

		for (uint32_t i=0; i<shader->num_stages; i++)
		{
			dm_shader_desc stage = shader->stages[i];
			
	        NSString* func_name = [[NSString alloc] initWithUTF8String:stage.source];

			switch (stage.type)
			{
				case DM_SHADER_TYPE_VERTEX:
				{
    	    		_vertex_func = [_library newFunctionWithName:func_name];
				} break;
				case DM_SHADER_TYPE_PIXEL:
				{
        			_fragment_func = [_library newFunctionWithName:func_name];
				} break;
				default:
				DM_LOG_ERROR("Unknown shader type, shouldn't be here...");
				break;
			}
		}

        if(!_vertex_func)
        {
        	DM_LOG_FATAL("Could not create vertex function from shader: %s", shader->pass);
            return NULL;
		}

        if(!_fragment_func)
        {
            DM_LOG_FATAL("Could not create fragment function from shader: %s", shader->pass);
            return NULL;
        }
    }

    return self;
}

@end

#endif