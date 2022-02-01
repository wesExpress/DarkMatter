#include "dm_metal_shader.h"

#ifdef DM_METAL

#include "core/dm_mem.h"
#include "core/dm_logger.h"

bool dm_metal_create_shader_library(dm_shader* shader, NSString* path, dm_metal_renderer* renderer)
{
    @autoreleasepool
    {
        shader->internal_shader = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
        dm_internal_shader* internal_shader = shader->internal_shader;

        internal_shader->library = [renderer->device newLibraryWithFile:path error:NULL];
        if(!internal_shader->library)
        {
            DM_LOG_FATAL("Could not create metal library from file: %s", path);
            return false;
        }

        NSString* vertex_func_name = [[NSString alloc] initWithUTF8String:shader->vertex_desc.path];
        internal_shader->vertex_func = [internal_shader->library newFunctionWithName:vertex_func_name];
        if(!internal_shader->vertex_func)
        {
            DM_LOG_FATAL("Could not create vertex function with name: %s", shader->vertex_desc.path);
        }

        NSString* fragment_func_name = [[NSString alloc] initWithUTF8String:shader->pixel_desc.path];
        internal_shader->fragment_func = [internal_shader->library newFunctionWithName:fragment_func_name];
        if(!internal_shader->fragment_func)
        {
            DM_LOG_FATAL("Could not create fragment function with name: %s", shader->pixel_desc.path);
        }
    }

    return true;
}

void dm_metal_destroy_shader_library(dm_shader* shader)
{
    @autoreleasepool
    {
        dm_free(shader->internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
    }
}

#endif