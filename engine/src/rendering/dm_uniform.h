#ifndef __DM_UNIFORM_H__
#define __DM_UNIFORM_H__

#include "dm_render_types.h"

dm_uniform dm_create_uniform(char* name, dm_uniform_data_t type, size_t data_size);
void dm_destroy_uniform(dm_uniform* uniform);

bool dm_set_uniform(char* name, void* data,  dm_render_pass* render_pass);

#endif