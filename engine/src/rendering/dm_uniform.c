#include "dm_uniform.h"
#include "core/dm_math_types.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include <stdbool.h>
#include <stdint.h>

dm_uniform dm_create_uniform(const char* name, dm_uniform_data_t type)
{
    dm_uniform uniform = { 0 };
    
    uniform.name = name;
    uniform.type = type;
    
    size_t uniform_size = 0;
    switch(type)
    {
        case DM_UNIFORM_DATA_T_BOOL:
        {
            uniform_size = sizeof(bool);
        } break;
        case DM_UNIFORM_DATA_T_INT:
        {
            uniform_size = sizeof(int);
        }break;
        case DM_UNIFORM_DATA_T_UINT:
        {
            uniform_size = sizeof(uint32_t);
        } break;
        case DM_UNIFORM_DATA_T_FLOAT:
        {
            uniform_size = sizeof(float);
        } break;
        case DM_UNIFORM_DATA_T_VEC2:
        {
            uniform_size = sizeof(dm_vec2);;
        } break;
        case DM_UNIFORM_DATA_T_VEC3:
        {
            uniform_size = sizeof(dm_vec3);
        } break;
        case DM_UNIFORM_DATA_T_VEC4:
        {
            uniform_size = sizeof(dm_vec4);
        } break;
        case DM_UNIFORM_DATA_T_MAT4_FLOAT:
        case DM_UNIFORM_DATA_T_MAT4_INT:
        {
            uniform_size = sizeof(dm_mat4);
        } break;
        default:
        DM_LOG_ERROR("Shouldn't be here...");
        break;
    }
    
    uniform.data = dm_alloc(uniform_size, DM_MEM_RENDERER_UNIFORM);
    
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
    
    //dm_memcpy(uniform->data, data, uniform->desc.data_size);
    
    return true;
}