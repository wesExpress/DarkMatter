#include "dm_opengl_enum_conversion.h"

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

GLenum dm_data_to_opengl_data(dm_buffer_data_type dm_data)
{
    switch (dm_data)
    {
    case DM_BUFFER_DATA_FLOAT: return GL_FLOAT;
    case DM_BUFFER_DATA_INT: return GL_INT;
    default:
        DM_LOG_FATAL("Unknown buffer data type!");
        return DM_BUFFER_DATA_UNKNOWN;
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

GLenum dm_depth_eq_to_opengl_func(dm_depth_equation eq)
{
    switch (eq)
    {
    case DM_DEPTH_EQUATION_ALWAYS: return GL_ALWAYS;
    case DM_DEPTH_EQUATION_NEVER: return GL_NEVER;
    case DM_DEPTH_EQUATION_EQUAL: return GL_EQUAL;
    case DM_DEPTH_EQUATION_NOTEQUAL: return GL_NOTEQUAL;
    case DM_DEPTH_EQUATION_LESS: return GL_LESS;
    case DM_DEPTH_EQUATION_LEQUAL: return GL_LEQUAL;
    case DM_DEPTH_EQUATION_GREATER: return GL_GREATER;
    case DM_DEPTH_EQUATION_GEQUAL: return GL_GEQUAL;
    default:
        DM_LOG_FATAL("Unknown depth function!");
        return DM_DEPTH_EQUATION_UNKNOWN;
    }
}

GLenum dm_stencil_eq_to_opengl_func(dm_stencil_equation eq)
{
    switch (eq)
    {
    case DM_STENCIL_EQUATION_ALWAYS: return GL_ALWAYS;
    case DM_STENCIL_EQUATION_NEVER: return GL_NEVER;
    case DM_STENCIL_EQUATION_EQUAL: return GL_EQUAL;
    case DM_STENCIL_EQUATION_NOTEQUAL: return GL_NOTEQUAL;
    case DM_STENCIL_EQUATION_LESS: return GL_LESS;
    case DM_STENCIL_EQUATION_LEQUAL: return GL_LEQUAL;
    case DM_STENCIL_EQUATION_GREATER: return GL_GREATER;
    case DM_STENCIL_EQUATION_GEQUAL: return GL_GEQUAL;
    default:
        DM_LOG_FATAL("Unknown stencil function!");
        return DM_DEPTH_EQUATION_UNKNOWN;
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