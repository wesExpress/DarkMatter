#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include "rendering/dm_renderer.h"
#include "rendering/dm_image.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"

#include "platform/dm_platform.h"

#include <glad/glad.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define DM_GLUINT_FAIL UINT_MAX

/*
STRUCTS
*/

typedef struct dm_internal_buffer
{
	GLuint id;
	GLenum type, usage, data_type;
} dm_internal_buffer;

typedef struct dm_internal_constant_buffer
{
	GLint location;
	void* data;
} dm_internal_constant_buffer;

typedef struct dm_internal_uniform
{
	GLint location;
} dm_internal_uniform;

typedef struct dm_internal_shader
{
	GLuint id;
} dm_internal_shader;

typedef struct dm_internal_texture
{
	GLuint id;
	GLint location;
} dm_internal_texture;

typedef struct dm_internal_pipeline
{
	GLenum blend_src, blend_dest;
	GLenum blend_func, depth_func, stencil_func;
} dm_internal_pipeline;

typedef struct dm_opengl_render_pass
{
	GLuint vao;
	GLenum cull, winding;
	GLenum primitive;
	GLenum min_filter, mag_filter, s_wrap, t_wrap;
} dm_opengl_render_pass;

typedef enum dm_opengl_uniform
{
	DM_OPENGL_UNI_INT,
	DM_OPENGL_UNI_INT2,
	DM_OPENGL_UNI_INT3,
	DM_OPENGL_UNI_INT4,
	DM_OPENGL_UNI_FLOAT,
	DM_OPENGL_UNI_FLOAT2,
	DM_OPENGL_UNI_FLOAT3,
	DM_OPENGL_UNI_FLOAT4,
	DM_OPENGL_UNI_MAT2,
	DM_OPENGL_UNI_MAT3,
	DM_OPENGL_UNI_MAT4
} dm_opengl_uniform;

#if DM_DEBUG
GLenum glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#define glCheckErrorReturn() if(glCheckError()) return false
#else
#define glCheckError()
#define glCheckErrorReturn()
#endif

/*
OPENGL ENUMS
*/

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

/*
BUFFER
*/

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data)
{
    buffer->internal_buffer = dm_alloc(sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
    dm_internal_buffer* internal_buffer = buffer->internal_buffer;
    internal_buffer->type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    if (internal_buffer->type == DM_BUFFER_TYPE_UNKNOWN) return false;
    internal_buffer->usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    if (internal_buffer->usage == DM_BUFFER_USAGE_UNKNOWN) return false;
    
    glGenBuffers(1, &internal_buffer->id);
    glCheckErrorReturn();
    
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckErrorReturn();
    
    glBufferData(internal_buffer->type, buffer->desc.buffer_size, data, internal_buffer->usage);
    glCheckErrorReturn();
    
    return true;
}

void dm_opengl_delete_buffer(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = buffer->internal_buffer;
    glDeleteBuffers(1, &internal_buffer->id);
    glCheckError();
    dm_free(buffer->internal_buffer, sizeof(dm_internal_buffer), DM_MEM_RENDERER_BUFFER);
}

void dm_opengl_bind_buffer(dm_buffer* buffer)
{
    dm_internal_buffer* internal_buffer = buffer->internal_buffer;
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();
}

/*
SHADER
*/

GLuint dm_opengl_compile_shader(dm_shader_desc desc);
bool dm_opengl_validate_shader(GLuint shader);
bool dm_opengl_validate_program(GLuint program);

bool dm_opengl_create_shader(dm_shader* shader)
{
    shader->internal_shader = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
    dm_internal_shader* internal_shader = shader->internal_shader;
    
    GLuint vertex_shader = dm_opengl_compile_shader(shader->vertex_desc);
    if(vertex_shader == DM_GLUINT_FAIL) return false;
    
    GLuint frag_shader = dm_opengl_compile_shader(shader->pixel_desc);
    if(frag_shader == DM_GLUINT_FAIL) return false;
    
    DM_LOG_DEBUG("Linking shader...");
    internal_shader->id = glCreateProgram();
    glAttachShader(internal_shader->id, vertex_shader);
    glCheckErrorReturn();
    glAttachShader(internal_shader->id, frag_shader);
    glCheckErrorReturn();
    glLinkProgram(internal_shader->id);
    glCheckErrorReturn();
    
    if (!dm_opengl_validate_program(internal_shader->id))
    {
        DM_LOG_FATAL("Failed to validate OpenGL shader!");
        return false;
    }
    
    glDetachShader(internal_shader->id, vertex_shader);
    glCheckErrorReturn();
    glDetachShader(internal_shader->id, frag_shader);
    glCheckErrorReturn();
    
    glDeleteShader(vertex_shader);
    glCheckErrorReturn();
    glDeleteShader(frag_shader);
    glCheckErrorReturn();
    
    return true;
}

void dm_opengl_delete_shader(dm_shader* shader)
{
    dm_internal_shader* internal_shader = shader->internal_shader;
    
    glDeleteProgram(internal_shader->id);
    glCheckError();
    dm_free(shader->internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
}

void dm_opengl_bind_shader(dm_shader* shader)
{
    dm_internal_shader* internal_shader = shader->internal_shader;
    
    glUseProgram(internal_shader->id);
    glCheckError();
}

bool dm_opengl_create_uniform(dm_uniform* uniform, dm_shader* shader)
{
    uniform->internal_uniform = dm_alloc(sizeof(dm_internal_uniform), DM_MEM_RENDERER_SHADER);
    
    dm_internal_uniform* internal_uni = uniform->internal_uniform;
    dm_internal_shader* internal_shader = shader->internal_shader;
    
    internal_uni->location = glGetUniformLocation(internal_shader->id, uniform->name);
    if (internal_uni->location == -1)
    {
        DM_LOG_FATAL("Could not find uniform: '%s'", uniform->name);
        return false;
    }
    
    return true;
}

void dm_opengl_destroy_uniform(dm_uniform* uniform)
{
    dm_free(uniform->internal_uniform, sizeof(dm_internal_uniform), DM_MEM_RENDERER_SHADER);
}

bool dm_opengl_bind_uniform(dm_uniform* uniform)
{
    dm_internal_uniform* internal_uniform = uniform->internal_uniform;
    
    switch (uniform->desc.data_t)
    {
        case DM_UNIFORM_DATA_T_INT:
        {
            switch (uniform->desc.count)
            {
                case 1:
                {
                    int data = *(int*)uniform->data;
                    glUniform1i(internal_uniform->location, data);
                    glCheckErrorReturn();
                } break;
                case 2:
                {
                    dm_vec2 data = *(dm_vec2*)uniform->data;
                    glUniform2i(internal_uniform->location, (GLint)data.x, (GLint)data.y);
                    glCheckErrorReturn();
                } break;
                case 3:
                {
                    dm_vec3 data = *(dm_vec3*)uniform->data;
                    glUniform3i(internal_uniform->location, (GLint)data.x, (GLint)data.y, (GLint)data.z);
                    glCheckErrorReturn();
                } break;
                case 4:
                {
                    dm_vec4 data = *(dm_vec4*)uniform->data;
                    glUniform4i(internal_uniform->location, (GLint)data.x, (GLint)data.y, (GLint)data.z, (GLint)data.w);
                    glCheckErrorReturn();
                } break;
                default:
                DM_LOG_FATAL("Trying to upload to uniform with wrong size of ints");
                return false;
            }
        } break;
        
        case DM_UNIFORM_DATA_T_BOOL:
        case DM_UNIFORM_DATA_T_UINT:
        {
            switch (uniform->desc.count)
            {
                case 1:
                {
                    uint32_t data = *(uint32_t*)uniform->data;
                    glUniform1ui(internal_uniform->location, data);
                    glCheckErrorReturn();
                } break;
                case 2:
                {
                    dm_vec2 data = *(dm_vec2*)uniform->data;
                    glUniform2ui(internal_uniform->location, (GLint)data.x, (GLint)data.y);
                    glCheckErrorReturn();
                } break;
                case 3:
                {
                    dm_vec3 data = *(dm_vec3*)uniform->data;
                    glUniform3ui(internal_uniform->location, (GLint)data.x, (GLint)data.y, (GLint)data.z);
                    glCheckErrorReturn();
                } break;
                case 4:
                {
                    dm_vec4 data = *(dm_vec4*)uniform->data;
                    glUniform4ui(internal_uniform->location, (GLint)data.x, (GLint)data.y, (GLint)data.z, (GLint)data.w);
                    glCheckErrorReturn();
                } break;
                default:
                DM_LOG_FATAL("Trying to upload to uniform with wrong size of uints");
                return false;
            }
        } break;
        
        case DM_UNIFORM_DATA_T_FLOAT:
        {
            switch (uniform->desc.count)
            {
                case 1:
                {
                    float data = *(float*)uniform->data;
                    glUniform1f(internal_uniform->location, data);
                    glCheckErrorReturn();
                } break;
                case 2:
                {
                    dm_vec2 data = *(dm_vec2*)uniform->data;
                    glUniform2f(internal_uniform->location, data.x, data.y);
                    glCheckErrorReturn();
                } break;
                case 3:
                {
                    dm_vec3* data = (dm_vec3*)uniform->data;
                    glUniform3f(internal_uniform->location, data->x, data->y, data->z);
                    glCheckErrorReturn();
                } break;
                case 4:
                {
                    dm_vec4 data = *(dm_vec4*)uniform->data;
                    glUniform4f(internal_uniform->location, data.x, data.y, data.z, data.w);
                    glCheckErrorReturn();
                } break;
                default:
                DM_LOG_FATAL("Trying to upload to uniform with wrong size of floats");
                return false;
            }
        } break;
        
        case DM_UNIFORM_DATA_T_MATRIX_INT:
        case DM_UNIFORM_DATA_T_MATRIX_FLOAT:
        {
            switch (uniform->desc.count)
            {
                case 2:
                {
                    dm_mat2 data = *(dm_mat2*)uniform->data;
                    glUniformMatrix2fv(internal_uniform->location, 1, GL_FALSE, &data.m[0]);
                    glCheckErrorReturn();
                } break;
                case 3:
                {
                    dm_mat3 data = *(dm_mat3*)uniform->data;
                    glUniformMatrix3fv(internal_uniform->location, 1, GL_FALSE, &data.m[0]);
                    glCheckErrorReturn();
                } break;
                case 4:
                {
                    dm_mat4 data = *(dm_mat4*)uniform->data;
                    glUniformMatrix4fv(internal_uniform->location, 1, GL_FALSE, &data.m[0]);
                    glCheckErrorReturn();
                } break;
                default:
                DM_LOG_FATAL("Trying to upload to uniform with wrong size of matrices");
                return false;
            }
        } break;
        
        default:
        DM_LOG_FATAL("Unknown constant buffer data type!");
        return false;
    } 
    
    return true;
}

/*
uses dm_shader_desc to compile a glsl shader from file
*/
GLuint dm_opengl_compile_shader(dm_shader_desc desc)
{
    GLenum shader_type = dm_shader_to_opengl_shader(desc.type);
    GLuint shader = glCreateShader(shader_type);
    glCheckError();
    
    DM_LOG_DEBUG("Compiling shader: %s", desc.path);
    
    FILE* file = fopen(desc.path, "r");
    //DM_ASSERT_MSG(file, "Could not fopen file: %s", desc.path);
    if(!file)
    {
        DM_LOG_FATAL("Could not fopen file: %s", desc.path);
        return DM_GLUINT_FAIL;
    }
    
    // determine size of memory to allocate to the buffer
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* string = dm_alloc(length + 1, DM_MEM_STRING);
    DM_ASSERT(string);
    
    // instead of setting string[length] to nul, we instead
    // find the last character that isn't garbage and set that
    // to nul. otherwise there could be garbage in between and
    // cause a seg fault
    //fread(string, length, 1, file);
    size_t read_count = fread(string, 1, length, file);
    string[read_count] = '\0';
    fclose(file);
    DM_ASSERT(string);
    
    const char* source = string;
    GLint l = (GLint)length;
    glShaderSource(shader, 1, &source, &l);
    glCheckError();
    glCompileShader(shader);
    glCheckError();
    
    //DM_ASSERT(dm_opengl_validate_shader(shader));
    if(!dm_opengl_validate_shader(shader))
    {
        DM_LOG_FATAL("Could not validate shader!");
        return DM_GLUINT_FAIL;
    }
    
    dm_free(string, length+1, DM_MEM_STRING);
    
    return shader;
}

bool dm_opengl_validate_shader(GLuint shader)
{
    GLint result = -1;
    int length = -1;
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glCheckErrorReturn();
    
    if (result != GL_TRUE)
    {
        GLchar message[512];
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        glCheckErrorReturn();
        glGetShaderInfoLog(shader, sizeof(message), &length, message);
        glCheckErrorReturn();
        DM_LOG_FATAL("%s", message);
        
        glDeleteShader(shader);
        glCheckErrorReturn();
        
        return false;
    }
    
    return true;
}

bool dm_opengl_validate_program(GLuint program)
{
    GLint result = -1;
    int length = -1;
    
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glCheckErrorReturn();
    
    if (result == GL_FALSE)
    {
        DM_LOG_ERROR("OpenGL Error: %d", glGetError());
        char message[512];
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        glCheckErrorReturn();
        glGetProgramInfoLog(program, length, NULL, message);
        glCheckErrorReturn();
        DM_LOG_FATAL("%s", message);
        
        glDeleteProgram(program);
        glCheckErrorReturn();
        
        return false;
    }
    
    return true;
}

/*
TEXTURE
*/

bool dm_opengl_create_texture(dm_image* image)
{
	image->internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = image->internal_texture;
    
	GLenum format = dm_texture_format_to_opengl_format(image->desc.format);
	if (format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
	GLenum internal_format = dm_texture_format_to_opengl_format(image->desc.internal_format);
	if (internal_format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
    
	glGenTextures(1, &internal_texture->id);
	glCheckErrorReturn();
    
	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();
    
	// load data in
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image->desc.width, image->desc.height, 0, format, GL_UNSIGNED_BYTE, image->data);
	glCheckErrorReturn();
	glGenerateMipmap(GL_TEXTURE_2D);
	glCheckErrorReturn();
    
	return true;
}

void dm_opengl_destroy_texture(dm_image* image)
{
	dm_internal_texture* internal_texture = image->internal_texture;
	glDeleteTextures(1, &internal_texture->id);
	glCheckError();
	dm_free(image->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

bool dm_opengl_bind_texture(dm_image* image, uint32_t slot)
{
	dm_internal_texture* internal_texture = image->internal_texture;
    
	glActiveTexture(GL_TEXTURE0 + slot);
	glCheckErrorReturn();
	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();
    
	glUniform1i(internal_texture->location, slot);
	glCheckErrorReturn();
    
	return true;
}

/*
RENDERPASS
*/

bool dm_opengl_create_render_pass(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    render_pass->internal_render_pass = dm_alloc(sizeof(dm_opengl_render_pass), DM_MEM_RENDER_PASS);
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    // face culling
    internal_pass->cull = dm_cull_to_opengl_cull(render_pass->raster_desc.cull_mode);
    if (internal_pass->cull == DM_CULL_UNKNOWN) return false;
    
    internal_pass->winding = dm_wind_top_opengl_wind(render_pass->raster_desc.winding_order);
    if (internal_pass->winding == DM_WINDING_UNKNOWN) return false;
    
    // primitive
    internal_pass->primitive = dm_topology_to_opengl_primitive(render_pass->raster_desc.primitive_topology);
    if (internal_pass->primitive == DM_TOPOLOGY_UNKNOWN) return false;
    
    // sampler
    internal_pass->min_filter = dm_filter_to_opengl_filter(render_pass->sampler_desc.filter);
    if (internal_pass->min_filter == DM_FILTER_UNKNOWN) return false;
    internal_pass->mag_filter = dm_filter_to_opengl_filter(render_pass->sampler_desc.filter);
    if (internal_pass->mag_filter == DM_FILTER_UNKNOWN) return false;
    internal_pass->s_wrap = dm_texture_mode_to_opengl_mode(render_pass->sampler_desc.u);
    if (internal_pass->s_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    internal_pass->t_wrap = dm_texture_mode_to_opengl_mode(render_pass->sampler_desc.v);
    if (internal_pass->t_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    
    // vertex array object
    glGenVertexArrays(1, &internal_pass->vao);
    glCheckErrorReturn();
    
    // shader
    if (!dm_opengl_create_shader(render_pass->shader)) return false;
    
    // vertex attributes
    uint32_t count = 0;
    glBindVertexArray(internal_pass->vao);
    
    dm_opengl_bind_buffer(pipeline->index_buffer);
    
    for (int i = 0; i < v_layout.num; i++)
    {
        dm_vertex_attrib_desc attrib_desc = v_layout.attributes[i];
        
        switch (attrib_desc.attrib_class)
        {
            case DM_VERTEX_ATTRIB_CLASS_VERTEX:
            {
                dm_opengl_bind_buffer(pipeline->vertex_buffer);
            } break;
            case DM_VERTEX_ATTRIB_CLASS_INSTANCE:
            {
                dm_opengl_bind_buffer(pipeline->inst_buffer);
            } break;
            default: break; 
        }
        
        GLenum data_t;
        if ((attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) || (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) data_t = GL_INT;
            else if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX_FLOAT) data_t = GL_FLOAT;
            else
            {
                DM_LOG_FATAL("Unknown vertex data type!");
                return false;
            }
            
            for (uint32_t j = 0; j < attrib_desc.count; j++)
            {
                size_t new_offset;;
                if (data_t == GL_INT) new_offset = j * (attrib_desc.offset + sizeof(int) * attrib_desc.count);
                else if (data_t == GL_FLOAT) new_offset = j * (attrib_desc.offset + sizeof(float) * attrib_desc.count);
                
                glVertexAttribPointer(count, attrib_desc.count, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)new_offset);
                glCheckErrorReturn();
                glEnableVertexAttribArray(count);
                glCheckErrorReturn();
                if (attrib_desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
                {
                    glVertexAttribDivisor(count, 1);
                    glCheckErrorReturn();
                }
                count++;
            }
        }
        else
        {
            data_t = dm_vertex_data_t_to_opengl(attrib_desc.data_t);
            if (data_t == DM_VERTEX_DATA_T_UNKNOWN) return false;
            
            glVertexAttribPointer(count, attrib_desc.count, data_t, attrib_desc.normalized, attrib_desc.stride, (void*)(uintptr_t)attrib_desc.offset);
            glCheckErrorReturn();
            glEnableVertexAttribArray(count);
            glCheckErrorReturn();
            if (attrib_desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
            {
                glVertexAttribDivisor(count, 1);
                glCheckErrorReturn();
            }
            count++;
        }
    }
    
    glBindVertexArray(0);
    
    // uniforms
    dm_for_map_item(render_pass->uniforms)
    {
        dm_uniform* uniform = item->value;
        
        if (!dm_opengl_create_uniform(uniform, render_pass->shader)) return false;
    }
    
    return true;
}

void dm_opengl_destroy_render_pass(dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    glDeleteVertexArrays(1, &internal_pass->vao);
    glCheckError();
    
    dm_opengl_delete_shader(render_pass->shader);
    
    dm_for_map_item(render_pass->uniforms)
    {
        dm_uniform* uniform = item->value;
        
        dm_opengl_destroy_uniform(uniform);
    }
    
    dm_free(render_pass->internal_render_pass, sizeof(dm_opengl_render_pass), DM_MEM_RENDER_PASS);
}

bool dm_opengl_begin_render_pass(dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    // vertex array
    glBindVertexArray(internal_pass->vao);
    glCheckErrorReturn();
    
    // sampler state
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, internal_pass->s_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, internal_pass->t_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, internal_pass->min_filter);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, internal_pass->mag_filter);
    glCheckErrorReturn();
    
    // face culling
    glEnable(GL_CULL_FACE);
    glCullFace(internal_pass->cull);
    glCheckErrorReturn();
    
    //glFrontFace(internal_pipe->winding);
    //glCheckErrorReturn();
    
    // wireframe
    if (render_pass->wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // shader
    dm_opengl_bind_shader(render_pass->shader);
    
    return true;
}

/*
RENDERER
*/

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_renderer_data* renderer_data)
{
    DM_LOG_DEBUG("Initializing OpenGL render backend...");
    
    if (!dm_platform_init_opengl()) return false;
    
    DM_LOG_INFO("OpenGL Info:");
    DM_LOG_INFO("       Vendor  : %s", glGetString(GL_VENDOR));
    DM_LOG_INFO("       Renderer: %s", glGetString(GL_RENDERER));
    DM_LOG_INFO("       Version : %s", glGetString(GL_VERSION));
    
#ifndef __APPLE__
#if DM_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif
#endif
    
    glViewport(renderer_data->viewport.x, renderer_data->viewport.y, renderer_data->viewport.width, renderer_data->viewport.height);
    
    return true;
}

void dm_renderer_shutdown_impl(dm_renderer_data* renderer_data)
{
    dm_platform_shutdown_opengl();
}

bool dm_renderer_end_frame_impl(dm_renderer_data* renderer_data)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_STENCIL_TEST);
    //glDisable(GL_BLEND);
    
    dm_platform_swap_buffers();
    
    return true;
}

bool dm_renderer_create_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    pipeline->internal_pipeline = dm_alloc(sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
    dm_internal_pipeline* internal_pipe = pipeline->internal_pipeline;
    
    if (pipeline->blend_desc.is_enabled)
    {
        internal_pipe->blend_func = dm_blend_eq_to_opengl_func(pipeline->blend_desc.equation);
        if (internal_pipe->blend_func == DM_BLEND_EQUATION_UNKNOWN) return false;
        
        internal_pipe->blend_src = dm_blend_func_to_opengl_func(pipeline->blend_desc.src);
        if (internal_pipe->blend_src == DM_BLEND_FUNC_UNKNOWN) return false;
        internal_pipe->blend_dest = dm_blend_func_to_opengl_func(pipeline->blend_desc.dest);
        if (internal_pipe->blend_dest == DM_BLEND_FUNC_UNKNOWN) return false;
    }
    
    if (pipeline->depth_desc.is_enabled)
    {
        internal_pipe->depth_func = dm_comp_to_opengl_comp(pipeline->depth_desc.comparison);
        if (internal_pipe->depth_func == DM_COMPARISON_UNKNOWN) return false;
    }
    
    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        internal_pipe->stencil_func = dm_comp_to_opengl_comp(pipeline->stencil_desc.comparison);
        if (internal_pipe->stencil_func == DM_COMPARISON_UNKNOWN) return false;
    }
    
    return true;
}

void dm_renderer_destroy_render_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_opengl_delete_buffer(pipeline->vertex_buffer);
    dm_opengl_delete_buffer(pipeline->index_buffer);
    dm_opengl_delete_buffer(pipeline->inst_buffer);
    
    dm_free(pipeline->internal_pipeline, sizeof(dm_internal_pipeline), DM_MEM_RENDER_PIPELINE);
}

bool dm_renderer_init_pipeline_data_impl(void* vb_data, void* ib_data, dm_render_pipeline* pipeline)
{
    if (!dm_opengl_create_buffer(pipeline->vertex_buffer, vb_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->index_buffer, ib_data)) return false;
    if (!dm_opengl_create_buffer(pipeline->inst_buffer, NULL)) return false;
    
    return true;
}

// render pass stuff

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, dm_vertex_layout v_layout, dm_render_pipeline* pipeline)
{
    return dm_opengl_create_render_pass(render_pass, v_layout, pipeline);
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
    dm_opengl_destroy_render_pass(render_pass);
}

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass)
{
    return dm_opengl_begin_render_pass(render_pass);
}

void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass)
{
    
}

bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline)
{
    dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->internal_pipeline;
    
    // blending
    if (pipeline->blend_desc.is_enabled)
    {
        glEnable(GL_BLEND);
        glBlendEquation(internal_pipe->blend_func);
        glCheckErrorReturn();
        glBlendFunc(internal_pipe->blend_src, internal_pipe->blend_dest);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_BLEND);
    }
    
    // depth testing
    if (pipeline->depth_desc.is_enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(internal_pipe->depth_func);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    
    // stencil testing
    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
    
    return true;
}

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t size)
{
    switch(buffer->desc.type)
    {
        case DM_BUFFER_TYPE_VERTEX:
        {
            dm_internal_buffer* internal_buffer = buffer->internal_buffer;
            glBindBuffer(internal_buffer->type, internal_buffer->id);
            glBufferSubData(internal_buffer->type, 0, size, data);
        } break;
        case DM_BUFFER_TYPE_CONSTANT:
        {
            dm_internal_constant_buffer* internal_cb = buffer->internal_buffer;
            dm_memcpy(internal_cb->data, data, size);
        } break;
        default:
        DM_LOG_ERROR("Haven't implemented this buffer update type!");
        return false;
    }
    
    return true;
}

bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot)
{
    switch (buffer->desc.type)
    {
        case DM_BUFFER_TYPE_VERTEX: 
        {
            dm_opengl_bind_buffer(buffer);
        } break;
        default:
        {
            DM_LOG_ERROR("Haven't implemented this bind buffer type yet!");
            return false;
        }
    }
    
    return true;
}

bool dm_create_texture_impl(dm_image* image)
{
    return dm_opengl_create_texture(image);
}

bool dm_renderer_bind_uniform_impl(dm_uniform* uniform)
{
    return dm_opengl_bind_uniform(uniform);
}

void dm_destroy_texture_impl(dm_image* image)
{
    dm_opengl_destroy_texture(image);
}

bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot)
{
    return dm_opengl_bind_texture(image, slot);
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
{
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glCheckError();
}

void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline)
{
    glClearColor(clear_color->x, clear_color->y, clear_color->z, clear_color->w);
    glCheckError();
    
    glClear(GL_COLOR_BUFFER_BIT);
    if(pipeline->depth_desc.is_enabled) glClear(GL_DEPTH_BUFFER_BIT);
    glCheckError();
}

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    glDrawArrays(internal_pass->primitive, start, count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    glDrawElements(internal_pass->primitive, num_indices, GL_UNSIGNED_INT, index_offset);
    glCheckError();
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass)
{
    dm_opengl_render_pass* internal_pass = render_pass->internal_render_pass;
    
    glDrawElementsInstanced(internal_pass->primitive, num_indices, GL_UNSIGNED_INT, 0, num_insts);
    glCheckError();
}

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char* error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        DM_LOG_ERROR("%s | %s (%d)", error, file, line);
    }
    return errorCode;
}

#endif