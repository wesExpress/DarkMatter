#include "dm_opengl_enum_conversion.h"

#ifdef DM_OPENGL

#include "core/dm_logger.h"

GLenum dm_buffer_to_opengl_buffer(dm_buffer_type dm_type)
{
    switch (dm_type)
    {
    case DM_BUFFER_TYPE_VERTEX: return GL_ARRAY_BUFFER;
    case DM_BUFFER_TYPE_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
    default:
        DM_LOG_FATAL("Unknown buffer type!");
        return DM_BUFFER_TYPE_UNKNOWN;
    }
}

GLenum dm_usage_to_opengl_draw(dm_buffer_usage dm_usage)
{
    switch (dm_usage)
    {
    case DM_BUFFER_USAGE_DEFAULT:
    case DM_BUFFER_USAGE_STATIC: return GL_STATIC_DRAW;
    case DM_BUFFER_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW;
    default:
        DM_LOG_FATAL("Unknown buffer usage type!");
        return DM_BUFFER_USAGE_UNKNOWN;
    }
}

GLenum dm_shader_to_opengl_shader(dm_shader_type dm_type)
{
    switch (dm_type)
    {
    case DM_SHADER_TYPE_VERTEX: return GL_VERTEX_SHADER;
    case DM_SHADER_TYPE_PIXEL: return GL_FRAGMENT_SHADER;
    default:
        DM_LOG_FATAL("Unknown shader type!");
        return DM_SHADER_TYPE_UNKNOWN;
    }
}

GLenum dm_vertex_data_t_to_opengl(dm_vertex_data_t dm_type)
{
    switch (dm_type)
    {
    case DM_VERTEX_DATA_T_BYTE: return GL_BYTE;
    case DM_VERTEX_DATA_T_UBYTE: return GL_UNSIGNED_BYTE;
    case DM_VERTEX_DATA_T_SHORT: return GL_SHORT;
    case DM_VERTEX_DATA_T_USHORT: return GL_UNSIGNED_SHORT;
    case DM_VERTEX_DATA_T_INT: return GL_INT;
    case DM_VERTEX_DATA_T_UINT: return GL_UNSIGNED_INT;
    case DM_VERTEX_DATA_T_FLOAT: return GL_FLOAT;
    case DM_VERTEX_DATA_T_DOUBLE: return GL_DOUBLE;
    default:
        DM_LOG_FATAL("Unknown vertex data type!");
        return DM_VERTEX_DATA_T_UNKNOWN;
    }
}

GLenum dm_topology_to_opengl_primitive(dm_primitive_topology topology)
{
    switch (topology)
    {
    case DM_TOPOLOGY_POINT_LIST: return GL_POINT;
    case DM_TOPOLOGY_LINE_LIST: return GL_LINE;
    case DM_TOPOLOGY_LINE_STRIP: return GL_LINE_STRIP;
    case DM_TOPOLOGY_TRIANGLE_LIST: return GL_TRIANGLES;
    case DM_TOPOLOGY_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    default:
        DM_LOG_FATAL("Unknown primitive type!");
        return DM_TOPOLOGY_UNKNOWN;
    }
}

GLenum dm_blend_eq_to_opengl_func(dm_blend_equation eq)
{
    switch (eq)
    {
    case DM_BLEND_EQUATION_ADD: return GL_FUNC_ADD;
    case DM_BLEND_EQUATION_SUBTRACT: return GL_FUNC_SUBTRACT;
    case DM_BLEND_EQUATION_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
    case DM_BLEND_EQUATION_MIN: return GL_MIN;
    case DM_BLEND_EQUATION_MAX: return GL_MAX;
    default:
        DM_LOG_FATAL("Unknown blend function!");
        return DM_BLEND_EQUATION_UNKNOWN;
    }
}

GLenum dm_blend_func_to_opengl_func(dm_blend_func func)
{
    switch (func)
    {
    case DM_BLEND_FUNC_ZERO: return GL_ZERO;
    case DM_BLEND_FUNC_ONE: return GL_ONE;
    case DM_BLEND_FUNC_SRC_COLOR: return GL_SRC_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
    case DM_BLEND_FUNC_DST_COLOR: return GL_DST_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
    case DM_BLEND_FUNC_SRC_ALPHA: return GL_SRC_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    case DM_BLEND_FUNC_DST_ALPHA: return GL_DST_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
    case DM_BLEND_FUNC_CONST_COLOR: return GL_CONSTANT_COLOR;
    case DM_BLEND_FUNC_ONE_MINUS_CONST_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
    case DM_BLEND_FUNC_CONST_ALPHA: return GL_CONSTANT_ALPHA;
    case DM_BLEND_FUNC_ONE_MINUS_CONST_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
    default:
        DM_LOG_FATAL("Unknown blend function!");
        return DM_BLEND_FUNC_UNKNOWN;
    }
}

GLenum dm_comp_to_opengl_comp(dm_comparison dm_comp)
{
    switch (dm_comp)
    {
    case DM_COMPARISON_ALWAYS: return GL_ALWAYS;
    case DM_COMPARISON_NEVER: return GL_NEVER;
    case DM_COMPARISON_EQUAL: return GL_EQUAL;
    case DM_COMPARISON_NOTEQUAL: return GL_NOTEQUAL;
    case DM_COMPARISON_LESS: return GL_LESS;
    case DM_COMPARISON_LEQUAL: return GL_LEQUAL;
    case DM_COMPARISON_GREATER: return GL_GREATER;
    case DM_COMPARISON_GEQUAL: return GL_GEQUAL;
    default:
        DM_LOG_FATAL("Unknown depth function!");
        return DM_COMPARISON_UNKNOWN;
    }
}

GLenum dm_cull_to_opengl_cull(dm_cull_mode cull)
{
    switch (cull)
    {
    case DM_CULL_FRONT: return GL_FRONT;
    case DM_CULL_BACK: return GL_BACK;
    case DM_CULL_FRONT_BACK: return GL_FRONT_AND_BACK;
    default:
        DM_LOG_FATAL("Unknown culling mode!");
        return DM_CULL_UNKNOWN;
    }
}

GLenum dm_wind_top_opengl_wind(dm_winding_order winding)
{
    switch (winding)
    {
    case DM_WINDING_CLOCK: return GL_CW;
    case DM_WINDING_COUNTER_CLOCK: return GL_CCW;
    default:
        DM_LOG_FATAL("Unkown winding order!");
        return DM_WINDING_UNKNOWN;
    }
}

GLenum dm_texture_format_to_opengl_format(dm_texture_format dm_format)
{
    switch (dm_format)
    {
    case DM_TEXTURE_FORMAT_RGB: return GL_RGB;
    case DM_TEXTURE_FORMAT_RGBA: return GL_RGBA;
    default:
        DM_LOG_FATAL("Unknown texture format!");
        return DM_TEXTURE_FORMAT_UNKNOWN;
    }
}

GLenum dm_filter_to_opengl_filter(dm_filter filter)
{
    switch (filter)
    {
    case DM_FILTER_LINEAR: return GL_LINEAR;
    case DM_FILTER_NEAREST: return GL_NEAREST;
    case DM_FILTER_LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
    case DM_FILTER_LIENAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
    case DM_FILTER_NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
    case DM_FILTER_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
    default:
        DM_LOG_FATAL("Unknown texture filter function!");
        return DM_FILTER_UNKNOWN;
    }
}

GLenum dm_texture_mode_to_opengl_mode(dm_texture_mode dm_mode)
{
    switch (dm_mode)
    {
    case DM_TEXTURE_MODE_WRAP: return GL_REPEAT;
    case DM_TEXTURE_MODE_EDGE: return GL_CLAMP_TO_EDGE;
    case DM_TEXTURE_MODE_BORDER: return GL_CLAMP_TO_BORDER;
    case DM_TEXTURE_MODE_MIRROR_REPEAT: return GL_MIRRORED_REPEAT;
    case DM_TEXTURE_MODE_MIRROR_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
    default:
        DM_LOG_FATAL("Unknwon clamping function!");
        return DM_TEXTURE_MODE_UNKNOWN;
    }
}

#endif