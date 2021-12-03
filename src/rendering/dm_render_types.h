#ifndef __DM_RENDER_TYPES_H__
#define __DM_RENDER_TYPES_H__

#include "dm_math_types.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef dm_vec4 dm_color;
typedef uint16_t dm_handle;

typedef dm_handle dm_buffer_handle;
typedef dm_handle dm_shader_handle;
typedef dm_handle dm_quad_handle;
typedef dm_handle dm_mesh_handle;
typedef dm_handle dm_model_handle;

typedef enum dm_buffer_type
{
	DM_BUFFER_TYPE_VERTEX,
	DM_BUFFER_TYPE_INDEX,
	DM_BUFFER_TYPE_CONSTANT,
	DM_BUFFER_TYPE_UNKNOWN
} dm_buffer_type;

typedef enum dm_buffer_usage
{
    DM_BUFFER_USAGE_DEFAULT,
    DM_BUFFER_USAGE_STATIC,
    DM_BUFFER_USAGE_DYNAMIC,
    DM_BUFFER_USAGE_UNKNOWN
} dm_buffer_usage;

typedef enum dm_buffer_cpu_access
{
    DM_BUFFER_CPU_WRITE,
    DM_BUFFER_CPU_READ
} dm_buffer_cpu_access;

typedef enum dm_buffer_data_type
{
    DM_BUFFER_DATA_SHORT,
    DM_BUFFER_DATA_USHORT,
    DM_BUFFER_DATA_INT,
    DM_BUFFER_DATA_UINT,
    DM_BUFFER_DATA_FLOAT,
    DM_BUFFER_DATA_DOUBLE,
    DM_BUFFER_DATA_UNKNOWN
} dm_buffer_data_type;

#endif