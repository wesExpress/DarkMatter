#include "dm_uniform.h"

#include "core/dm_mem.h"

dm_uniform dm_create_uniform(const char* name, dm_uniform_desc desc, void* data, size_t data_size)
{
    dm_uniform uniform = { 0 };
    
    uniform.name = name;
    uniform.desc = desc;
    uniform.data = dm_alloc(desc.data_size, DM_MEM_RENDERER_UNIFORM);
    dm_memcpy(uniform.data, data, data_size);
    
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
    
    dm_memcpy(uniform->data, data, uniform->desc.data_size);
    
    return true;
}