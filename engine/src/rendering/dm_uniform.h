#ifndef __DM_UNIFORM_H__
#define __DM_UNIFORM_H__

#include "dm_render_types.h"

static dm_uniform_desc mat4_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_MATRIX_FLOAT,
    .element_size = sizeof(float),
    .count = 4,
    .data_size = sizeof(float) * 4 * 4
};

static dm_uniform_desc mat3_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_MATRIX_FLOAT,
    .element_size = sizeof(float),
    .count = 3,
    .data_size = sizeof(float) * 3 * 3
};

static dm_uniform_desc mat2_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_MATRIX_FLOAT,
    .element_size = sizeof(float),
    .count = 2,
    .data_size = sizeof(float) * 2 * 2
};

static dm_uniform_desc vec4_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_FLOAT,
    .element_size = sizeof(float),
    .count = 4,
    .data_size = sizeof(float) * 4
};

static dm_uniform_desc vec3_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_FLOAT,
    .element_size = sizeof(float),
    .count = 3,
    .data_size = sizeof(float) * 3
};

static dm_uniform_desc vec2_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_FLOAT,
    .element_size = sizeof(float),
    .count = 2,
    .data_size = sizeof(float) * 2
};

static dm_uniform_desc float_uni_desc = {
    .data_t = DM_UNIFORM_DATA_T_FLOAT,
    .element_size = sizeof(float),
    .count = 1,
    .data_size = sizeof(float)
};

dm_uniform dm_create_uniform(const char* name, dm_uniform_desc desc, void* data, size_t data_size);
void dm_destroy_uniform(dm_uniform* uniform);

bool dm_set_uniform(char* name, void* data, dm_render_pass* render_pass);

#endif