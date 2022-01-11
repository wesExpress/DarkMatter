#ifndef __DM_RENDER_TYPES_H__
#define __DM_RENDER_TYPES_H__

#include "dm_math_types.h"
#include "structures/dm_list.h"
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

// pipeline stuff
typedef enum dm_cull_mode
{
    DM_CULL_FRONT,
    DM_CULL_BACK,
    DM_CULL_FRONT_BACK,
    DM_CULL_UNKNOWN
} dm_cull_mode;

typedef enum dm_winding_order
{
    DM_WINDING_CLOCK,
    DM_WINDING_COUNTER_CLOCK,
    DM_WINDING_UNKNOWN
} dm_winding_order;

typedef enum dm_depth_equation
{
    DM_DEPTH_EQUATION_ALWAYS,
    DM_DEPTH_EQUATION_NEVER,
    DM_DEPTH_EQUATION_EQUAL,
    DM_DEPTH_EQUATION_NOTEQUAL,
    DM_DEPTH_EQUATION_LESS,
    DM_DEPTH_EQUATION_LEQUAL,
    DM_DEPTH_EQUATION_GREATER,
    DM_DEPTH_EQUATION_GEQUAL,
    DM_DEPTH_EQUATION_UNKNOWN
} dm_depth_equation;

typedef enum dm_blend_equation
{
    DM_BLEND_EQUATION_ADD,
    DM_BLEND_EQUATION_SUBTRACT,
    DM_BLEND_EQUATION_REVERSE_SUBTRACT,
    DM_BLEND_EQUATION_MIN,
    DM_BLEND_EQUATION_MAX,
    DM_BLEND_EQUATION_UNKNOWN
} dm_blend_equation;

typedef enum dm_blend_func
{
    DM_BLEND_FUNC_ZERO,
    DM_BLEND_FUNC_ONE,
    DM_BLEND_FUNC_SRC_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR,
    DM_BLEND_FUNC_DST_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_DST_COLOR,
    DM_BLEND_FUNC_SRC_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
    DM_BLEND_FUNC_DST_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA,
    DM_BLEND_FUNC_CONST_COLOR,
    DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR,
    DM_BLEND_FUNC_CONST_ALPHA,
    DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA,
    DM_BLEND_FUNC_UNKNOWN
} dm_blend_func;

typedef enum dm_stencil_equation
{
    DM_STENCIL_EQUATION_ALWAYS,
    DM_STENCIL_EQUATION_NEVER,
    DM_STENCIL_EQUATION_EQUAL,
    DM_STENCIL_EQUATION_NOTEQUAL,
    DM_STENCIL_EQUATION_LESS,
    DM_STENCIL_EQUATION_LEQUAL,
    DM_STENCIL_EQUATION_GREATER,
    DM_STENCIL_EQUATION_GEQUAL,
    DM_STENCIL_EQUATION_UNKNOWN
} dm_stencil_equation;

typedef enum dm_primitive_topology
{
    DM_TOPOLOGY_POINT_LIST,
    DM_TOPOLOGY_LINE_LIST,
    DM_TOPOLOGY_LINE_STRIP,
    DM_TOPOLOGY_TRIANGLE_LIST,
    DM_TOPOLOGY_TRIANGLE_STRIP,
    DM_TOPOLOGY_UNKNOWN
} dm_primitive_topology;

typedef struct dm_viewport
{
    float x, y;
    float width, height;
    float min_depth, max_depth;
} dm_viewport;

typedef struct dm_raster_state_desc
{
    dm_cull_mode cull_mode;
    dm_winding_order winding_order;
    dm_primitive_topology primitive_topology;
    dm_shader_handle shader;
} dm_raster_state_desc;

typedef struct dm_blend_state_desc
{
    bool is_enabled;
    dm_blend_equation equation;
    dm_blend_func src, dest;
} dm_blend_state_desc;

typedef struct dm_depth_state_desc
{
    bool is_enabled;
    dm_depth_equation equation;
} dm_depth_state_desc;

typedef struct dm_stencil_state_desc
{
    bool is_enabled;
    dm_stencil_equation equation;
} dm_stencil_state_desc;

typedef struct dm_vertex_attrib_desc
{
    char name[512];
    dm_vertex_attrib attrib;
    size_t stride;
    size_t offset;
} dm_vertex_attrib_desc;

typedef struct dm_vertex_layout
{
    dm_vertex_attrib_desc* attributes;
    size_t size;
    int num;
} dm_vertex_layout;

typedef struct dm_render_pipeline
{
    dm_raster_state_desc raster_desc;
    dm_blend_state_desc blend_desc;
    dm_depth_state_desc depth_desc;
    dm_stencil_state_desc stencil_desc;
    dm_vertex_layout vertex_layout;
    dm_viewport viewport;
    bool wireframe;
    void* interal_pipeline;
} dm_render_pipeline;

// command buffer
typedef enum dm_render_command_type
{
    DM_RENDER_COMMAND_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_END_RENDER_PASS,
    DM_RENDER_COMMAND_SET_VIEWPORT,
    DM_RENDER_COMMAND_CLEAR,
    DM_RENDER_COMMAND_BIND_PIPELINE,
    DM_RENDER_COMMAND_SUBMIT_VERTEX_BUFFER,
    DM_RENDER_COMMAND_SUBMIT_INDEX_BUFFER,
    DM_RENDER_COMMAND_BIND_SHADER,
    DM_RENDER_COMMAND_DRAW_INDEXED,
    DM_RENDER_COMMAND_DRAW_INSTANCED,
    DM_RENDER_COMMAND_UNKNOWN
} dm_render_command_type;

typedef struct dm_render_command
{
    dm_render_command_type command;
    void* data;
} dm_render_command;

typedef struct dm_command_buffer
{
    dm_list(dm_render_command) commands;
} dm_command_buffer;

#endif