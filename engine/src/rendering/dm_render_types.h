#ifndef __DM_RENDER_TYPES_H__
#define __DM_RENDER_TYPES_H__

#include "core/dm_math_types.h"
#include "core/dm_string.h"
#include "structures/dm_list.h"
#include "structures/dm_map.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef dm_vec4 dm_color;

typedef struct dm_vertex
{
    dm_vec3 position;
    dm_vec3 normal;
    dm_vec2 tex_coords;
} dm_vertex;

typedef struct dm_vertex_inst
{
    dm_mat4 model;
    //dm_color diffuse;
    //dm_color specular;
} dm_vertex_inst;

typedef uint32_t             dm_index_t;
typedef dm_vertex            dm_vertex_t;
typedef dm_vertex_inst       dm_vertex_inst_t;

typedef struct dm_inst_data
{
    uint32_t index_offset;
    uint32_t vertex_offset;
    uint32_t index_count;
} dm_inst_data;

typedef struct dm_mesh
{
    uint32_t index_offset;
    uint32_t vertex_offset;
    uint32_t index_count;
    bool is_indexed;
    dm_list* entities;
} dm_mesh;

/***********
    ENUMS
************/

typedef enum dm_comparison
{
    DM_COMPARISON_ALWAYS,
    DM_COMPARISON_NEVER,
    DM_COMPARISON_EQUAL,
    DM_COMPARISON_NOTEQUAL,
    DM_COMPARISON_LESS,
    DM_COMPARISON_LEQUAL,
    DM_COMPARISON_GREATER,
    DM_COMPARISON_GEQUAL,
    DM_COMPARISON_UNKNOWN
} dm_comparison;

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

typedef enum dm_buffer_data_t
{
    DM_BUFFER_DATA_T_BOOL,
    DM_BUFFER_DATA_T_INT,
    DM_BUFFER_DATA_T_UINT,
    DM_BUFFER_DATA_T_FLOAT,
    DM_BUFFER_DATA_T_MATRIX,
    DM_BUFFER_DATA_T_UNKNOWN
} dm_buffer_data_t;

typedef enum dm_uniform_data_t
{
    DM_UNIFORM_DATA_T_BOOL,
    DM_UNIFORM_DATA_T_INT,
    DM_UNIFORM_DATA_T_UINT,
    DM_UNIFORM_DATA_T_FLOAT,
    DM_UNIFORM_DATA_T_VEC2,
    DM_UNIFORM_DATA_T_VEC3,
    DM_UNIFORM_DATA_T_VEC4,
    DM_UNIFORM_DATA_T_MAT4,
    DM_UNIFORM_DATA_T_UNKNOWN
} dm_uniform_data_t;

typedef enum dm_shader_type
{
    DM_SHADER_TYPE_VERTEX,
    DM_SHADER_TYPE_PIXEL,
    DM_SHADER_TYPE_UNKNOWN
} dm_shader_type;

typedef enum dm_texture_format
{
    DM_TEXTURE_FORMAT_RGB,
    DM_TEXTURE_FORMAT_RGBA,
    DM_TEXTURE_FORMAT_UNKNOWN
} dm_texture_format;

typedef enum dm_filter
{
    DM_FILTER_NEAREST,
    DM_FILTER_LINEAR,
    DM_FILTER_NEAREST_MIPMAP_NEAREST,
    DM_FILTER_LIENAR_MIPMAP_NEAREST,
    DM_FILTER_NEAREST_MIPMAP_LINEAR,
    DM_FILTER_LINEAR_MIPMAP_LINEAR,
    DM_FILTER_UNKNOWN
} dm_filter;

typedef enum dm_texture_mode
{
    DM_TEXTURE_MODE_WRAP,
    DM_TEXTURE_MODE_EDGE,
    DM_TEXTURE_MODE_BORDER,
    DM_TEXTURE_MODE_MIRROR_REPEAT,
    DM_TEXTURE_MODE_MIRROR_EDGE,
    DM_TEXTURE_MODE_UNKNOWN
} dm_texture_mode;

typedef enum dm_vertex_data_t
{
    DM_VERTEX_DATA_T_BYTE,
    DM_VERTEX_DATA_T_UBYTE,
    DM_VERTEX_DATA_T_SHORT,
    DM_VERTEX_DATA_T_USHORT,
    DM_VERTEX_DATA_T_INT,
    DM_VERTEX_DATA_T_UINT,
    DM_VERTEX_DATA_T_FLOAT,
    DM_VERTEX_DATA_T_DOUBLE,
    DM_VERTEX_DATA_T_MATRIX_INT,
    DM_VERTEX_DATA_T_MATRIX_FLOAT,
    DM_VERTEX_DATA_T_UNKNOWN
} dm_vertex_data_t;

typedef enum dm_vertex_attrib_class
{
    DM_VERTEX_ATTRIB_CLASS_VERTEX,
    DM_VERTEX_ATTRIB_CLASS_INSTANCE,
    DM_VERTEX_ATTRIB_CLASS_UNKNOWN
} dm_vertex_attrib_class;

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

typedef enum dm_primitive_topology
{
    DM_TOPOLOGY_POINT_LIST,
    DM_TOPOLOGY_LINE_LIST,
    DM_TOPOLOGY_LINE_STRIP,
    DM_TOPOLOGY_TRIANGLE_LIST,
    DM_TOPOLOGY_TRIANGLE_STRIP,
    DM_TOPOLOGY_UNKNOWN
} dm_primitive_topology;

typedef enum dm_render_command_type
{
    DM_RENDER_COMMAND_BEGIN_RENDER_PASS,
    DM_RENDER_COMMAND_END_RENDER_PASS,
    DM_RENDER_COMMAND_SET_VIEWPORT,
    DM_RENDER_COMMAND_CLEAR,
    DM_RENDER_COMMAND_BIND_PIPELINE,
    DM_RENDER_COMMAND_UPDATE_BUFFER,
    DM_RENDER_COMMAND_BIND_BUFFER,
    DM_RENDER_COMMAND_BIND_TEXTURE,
    DM_RENDER_COMMAND_BIND_UNIFORMS,
    DM_RENDER_COMMAND_DRAW_ARRAYS,
    DM_RENDER_COMMAND_DRAW_INDEXED,
    DM_RENDER_COMMAND_DRAW_INSTANCED,
    DM_RENDER_COMMAND_UNKNOWN
} dm_render_command_type;

/***********
 STRUCTURES
************/

// vertex

typedef struct dm_vertex_attrib_desc
{
    const char* name;
    dm_vertex_data_t data_t;
    dm_vertex_attrib_class attrib_class;
    size_t stride;
    size_t offset;
    size_t count;
    size_t index;
    bool normalized;
} dm_vertex_attrib_desc;

typedef struct dm_vertex_layout
{
    dm_vertex_attrib_desc* attributes;
    int num;
} dm_vertex_layout;

// buffer

typedef struct dm_buffer_desc
{
    dm_buffer_type type;
    dm_buffer_usage usage;
    dm_buffer_cpu_access cpu_access;
    dm_buffer_data_t data_t;
    size_t elem_size;
    size_t buffer_size;
    const char* name;
    uint32_t count;
} dm_buffer_desc;

typedef struct dm_buffer
{
    dm_buffer_desc desc;
    void* internal_buffer;
} dm_buffer;

typedef struct dm_buffer_update_packet
{
    dm_buffer* buffer;
    size_t data_size;
    void* data;
} dm_buffer_update_packet;

typedef struct dm_buffer_bind_packet
{
    dm_buffer* buffer;
    uint32_t slot;
} dm_buffer_bind_packet;

// uniform

typedef struct dm_uniform_desc
{
    dm_uniform_data_t data_t;
    size_t element_size;
    uint32_t count;
    size_t data_size;
} dm_uniform_desc;

typedef struct dm_unifom
{
    dm_uniform_data_t type;
    size_t data_size;
    char* name;
    void* internal_uniform;
    void* data;
} dm_uniform;

// shader

typedef struct dm_shader_desc
{
    dm_shader_type type;
    char* source;
} dm_shader_desc;

/*
typedef struct dm_shader
{
    dm_shader_desc vertex_desc;
    dm_shader_desc pixel_desc;
    char* name;
    void* internal_shader;
} dm_shader;
*/
typedef struct dm_shader
{
    dm_shader_desc* stages;
    uint32_t num_stages;
    char* pass;
    void* internal_shader;
} dm_shader;

// image

typedef struct dm_image_desc
{
    const char* path;
    const char* name;
    int width, height, n_channels;
    bool flip;
    dm_texture_format format;
    dm_texture_format internal_format;
} dm_image_desc;

typedef struct dm_image
{
    dm_image_desc desc;
    void* data;
    void* internal_texture;
} dm_image;

typedef struct dm_sampler_desc
{
    dm_filter filter;
    dm_texture_mode u;
    dm_texture_mode v;
    dm_texture_mode w;
    dm_comparison comparison;
    float min_lod, max_lod;
    dm_vec4 border_color;
} dm_sampler_desc;

// pipeline members

typedef struct dm_viewport
{
    uint32_t x, y;
    uint32_t width, height;
    float min_depth, max_depth;
} dm_viewport;

typedef struct dm_raster_state_desc
{
    dm_cull_mode cull_mode;
    dm_winding_order winding_order;
    dm_primitive_topology primitive_topology;
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
    dm_comparison comparison;
} dm_depth_state_desc;

typedef struct dm_stencil_state_desc
{
    bool is_enabled;
    dm_comparison comparison;
} dm_stencil_state_desc;

typedef struct dm_render_packet
{
    uint32_t index_count;
    uint32_t index_offset;
    uint32_t vertex_offset;
    uint32_t instance_count;
    uint32_t instance_offset;
    const char* tag;
} dm_render_packet;

typedef struct dm_render_command
{
    dm_render_command_type type;
    size_t data_size;
    void* data;
} dm_render_command;

/*
typedef struct dm_render_pipeline
{
    dm_blend_state_desc blend_desc;
    dm_depth_state_desc depth_desc;
    dm_stencil_state_desc stencil_desc;
    dm_render_packet render_packet;
    dm_buffer* vertex_buffer;
    dm_buffer* index_buffer;
    dm_buffer* inst_buffer;
    void* internal_pipeline;
} dm_render_pipeline;

typedef struct dm_render_pass
{
    dm_raster_state_desc raster_desc;
    dm_shader* shader;
    dm_sampler_desc sampler_desc;
    dm_map* uniforms;
    bool wireframe;
    const char* name;
    void* internal_render_pass;
} dm_render_pass;
*/
typedef struct dm_render_pipeline
{
    dm_blend_state_desc blend_desc;
    dm_depth_state_desc depth_desc;
    dm_stencil_state_desc stencil_desc;
    dm_raster_state_desc raster_desc;
    dm_sampler_desc sampler_desc;
    bool wireframe;
    void* internal_pipeline;
} dm_render_pipeline;

typedef struct dm_render_pass
{
    dm_shader shader;
    dm_viewport viewport;
    dm_map* uniforms;
    void* internal_pass;
} dm_render_pass;

#endif