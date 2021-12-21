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

typedef struct dm_buffer_desc
{
    dm_buffer_type type;
    dm_buffer_usage usage;
    dm_buffer_data_type data_type;
    dm_buffer_cpu_access cpu_access;
    size_t elem_size, data_size;
    uint32_t num_v_elements;
} dm_buffer_desc;

typedef enum dm_shader_type
{
    DM_SHADER_TYPE_VERTEX,
    DM_SHADER_TYPE_PIXEL,
    DM_SHADER_TYPE_UNKNOWN
} dm_shader_type;

typedef struct dm_shader_desc
{
    dm_shader_type type;
    const char* path;
} dm_shader_desc;

typedef enum dm_vertex_layout_type
{
    DM_VERTEX_LAYOUT_QUAD,
    DM_VERTEX_LAYOUT_MESH,
    DM_VERTEX_LAYOUT_MESH_COLOR,
    DM_VERTEX_LAYOUT_UNKNOWN
} dm_vertex_layout_type;

typedef struct dm_buffer
{
    dm_buffer_desc desc;
    void* internal_buffer;
} dm_buffer;

typedef struct dm_shader
{
    dm_shader_desc vertex_desc;
    dm_shader_desc pixel_desc;

    void* internal_shader;
} dm_shader;

#endif