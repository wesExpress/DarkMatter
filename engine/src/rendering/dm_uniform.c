#include "dm_uniform.h"
#include "core/dm_math_types.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include <stdbool.h>
#include <stdint.h>

dm_uniform dm_create_uniform(char* name, dm_uniform_data_t type, size_t data_size)
{
    dm_uniform uniform = { 0 };
    
    uniform.name = name;
    uniform.type = type;
    uniform.data_size = data_size;
    
    uniform.data = dm_alloc(data_size, DM_MEM_RENDERER_UNIFORM);
    
    return uniform;
}

void dm_destroy_uniform(dm_uniform* uniform)
{
    //dm_free(uniform->data, uniform->desc.data_size, DM_MEM_RENDERER_UNIFORM);
}

bool dm_set_uniform(char* name, void* data, dm_render_pass* render_pass)
{
    dm_uniform* uniform = dm_map_get(render_pass->uniforms, name);
    
    if(!uniform) return false;
    
    dm_memcpy(uniform->data, data, uniform->data_size);
    
    return true;
}