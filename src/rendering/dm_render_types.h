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
    dm_buffer_cpu_access cpu_access;
    size_t data_size;
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

// vertex attribs
typedef enum dm_vertex_attrib
{
    DM_VERTEX_ATTRIB_POS,
    DM_VERTEX_ATTRIB_NORM,
    DM_VERTEX_ATTRIB_COLOR,
    DM_VERTEX_ATTRIB_TEX_COORD,
    DM_VERTEX_ATTRIB_UNKNOWN
} dm_vertex_attrib;

typedef struct dm_vertex_3d
{
    dm_vec3 position;
} dm_vertex_3d;

// pipeline stuff
typedef enum dm_cull_mode
{
    DM_CULL_FRONT,
    DM_CULL_BACK,
    DM_CULL_UNKNOWN
} dm_cull_mode;

typedef enum dm_winding
{
    DM_WINDING_FRONT_COUNTER_CLOCK,
    DM_WINDING_FRONT_CLOCK,
    DM_WINDING_BACK_COUNTER_CLOCK,
    DM_WINDING_BACK_CLOCK,
    DM_WINDING_UNKNOWN
} dm_winding;

typedef enum dm_depth_compare_operation
{
    DM_DEPTH_ALWAYS,
    DM_DEPTH_NEVER,
    DM_DEPTH_EQUAL,
    DM_DEPTH_NOTEQUAL,
    DM_DEPTH_LESS,
    DM_DEPTH_LEQUAL,
    DM_DEPTH_GREATER,
    DM_DEPTH_GEQUAL,
    DM_DEPTH_UNKNOWN
} dm_depth_compare_operation;

typedef enum dm_topology
{
    DM_TOPOLOGY_POINT_LIST,
    DM_TOPOLOGY_LINE_LIST,
    DM_TOPOLOGY_LINE_STRIP,
    DM_TOPOLOGY_TRIANGLE_LIST,
    DM_TOPOLOGY_TRIANGLE_STRIP,
    DM_TOPOLOGY_UNKNOWN
} dm_topology;

typedef struct dm_viewport
{
    float x, y;
    float width, height;
    float min_depth, max_depth;
} dm_viewport;

typedef struct dm_render_pipeline
{
    dm_cull_mode cull_mode;
    dm_winding winding;
    dm_depth_compare_operation depth_op;
    dm_topology topology;
    dm_viewport viewport;
    bool wireframe;
    bool depth_test, stencil_test, blend;
} dm_render_pipeline;

#endif