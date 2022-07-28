#include "core/dm_defines.h"

#ifdef DM_OPENGL

#include "rendering/dm_renderer.h"
#include "rendering/dm_image.h"

#include "core/dm_logger.h"
#include "core/dm_assert.h"
#include "core/dm_mem.h"

#include "structures/dm_slot_list.h"

#include "platform/dm_platform.h"

#include <glad/glad.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define DM_GLUINT_FAIL UINT_MAX

/*******
STRUCTS
*********/

typedef struct dm_opengl_buffer
{
	GLuint id;
	GLenum type, usage, data_type;
} dm_opengl_buffer;

typedef struct dm_opengl_constant_buffer
{
	GLint location;
	void* data;
} dm_opengl_constant_buffer;

typedef struct dm_opengl_uniform
{
	GLint location;
} dm_opengl_uniform;

typedef struct dm_opengl_texture
{
	GLuint id;
	GLint location;
} dm_opengl_texture;

typedef struct dm_opengl_pipeline
{
	GLenum blend_src, blend_dest;
	GLenum blend_func, depth_func, stencil_func;
    GLenum primitive;
    GLenum min_filter, mag_filter, s_wrap, t_wrap;
    GLenum cull, winding;
    bool blend, depth, stencil, wireframe;
} dm_opengl_pipeline;

typedef struct dm_opengl_render_pass
{
    GLuint shader;
	GLuint vao;
    GLuint scene_uni;
    GLuint inst_uni;
} dm_opengl_render_pass;

typedef enum dm_opengl_uniform_dtype
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
} dm_opengl_uniform_dtype;

typedef struct dm_opengl_renderer
{
    dm_opengl_pipeline active_pipeline;
    uint32_t vertex_buffer_index;
    uint32_t index_buffer_index;
    uint32_t instance_buffer_index;
} dm_opengl_renderer;

#if DM_DEBUG
GLenum glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#define glCheckErrorReturn() if(glCheckError()) return false
#else
#define glCheckError()
#define glCheckErrorReturn()
#endif

/*
GLOBALS
*/
dm_opengl_renderer opengl_renderer = { 0 };

dm_slot_list* opengl_render_passes = NULL;
dm_slot_list* opengl_textures = NULL;
dm_slot_list* opengl_buffers = NULL;

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

/******
BUFFER
********/

bool dm_opengl_create_buffer(dm_buffer* buffer, void* data)
{
    dm_opengl_buffer internal_buffer = { 0 };
    
    internal_buffer.type = dm_buffer_to_opengl_buffer(buffer->desc.type);
    if (internal_buffer.type == DM_BUFFER_TYPE_UNKNOWN) { DM_LOG_FATAL("Unknown buffer type!"); return false; }
    internal_buffer.usage = dm_usage_to_opengl_draw(buffer->desc.usage);
    if (internal_buffer.usage == DM_BUFFER_USAGE_UNKNOWN) { DM_LOG_FATAL("Unknown buffer usage!"); return false; }
    
    glGenBuffers(1, &internal_buffer.id);
    glCheckErrorReturn();
    
    glBindBuffer(internal_buffer.type, internal_buffer.id);
    glCheckErrorReturn();
    
    glBufferData(internal_buffer.type, buffer->desc.buffer_size, data, internal_buffer.usage);
    glCheckErrorReturn();
    
    dm_slot_list_insert(opengl_buffers, &internal_buffer, &buffer->internal_index);
    
    if(strcmp(buffer->desc.name, "vertex") == 0) opengl_renderer.vertex_buffer_index = buffer->internal_index;
    else if (strcmp(buffer->desc.name, "index") == 0) opengl_renderer.index_buffer_index = buffer->internal_index;
    else if (strcmp(buffer->desc.name, "instance") == 0) opengl_renderer.instance_buffer_index = buffer->internal_index;
    else
    {
        DM_LOG_FATAL("Unknown buffer!");
        return false;
    }
    
    return true;
}

void dm_opengl_bind_buffer(uint32_t internal_index)
{
    dm_opengl_buffer* internal_buffer = dm_slot_list_at(opengl_buffers, internal_index);
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glCheckError();
}

void dm_renderer_delete_buffer_impl(uint32_t internal_index)
{
    dm_opengl_buffer* internal_buffer = dm_slot_list_at(opengl_buffers, internal_index);
    glDeleteBuffers(1, &internal_buffer->id);
    glCheckError();
}

/******
SHADER
********/

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

GLuint dm_opengl_compile_shader(const char* path, dm_shader_type type)
{
    GLenum shader_type = dm_shader_to_opengl_shader(type);
    GLuint shader = glCreateShader(shader_type);
    glCheckError();
    
    DM_LOG_DEBUG("Compiling shader: %s", path);
    
    FILE* file = fopen(path, "r");
    //DM_ASSERT_MSG(file, "Could not fopen file: %s", desc.path);
    if(!file)
    {
        DM_LOG_FATAL("Could not fopen file: %s", path);
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

bool dm_opengl_create_shader(uint32_t pass_index, const char* vertex_src, const char* frg_src)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    GLuint vertex_shader = DM_GLUINT_FAIL;
    GLuint frag_shader = DM_GLUINT_FAIL;
    
    vertex_shader = dm_opengl_compile_shader(vertex_src, DM_SHADER_TYPE_VERTEX);
    frag_shader = dm_opengl_compile_shader(frg_src, DM_SHADER_TYPE_PIXEL);
    
    if(vertex_shader == DM_GLUINT_FAIL) return false;
    if(frag_shader == DM_GLUINT_FAIL) return false;
    
    DM_LOG_DEBUG("Linking shader...");
    internal_pass->shader = glCreateProgram();
    glAttachShader(internal_pass->shader, vertex_shader);
    glCheckErrorReturn();
    glAttachShader(internal_pass->shader, frag_shader);
    glCheckErrorReturn();
    glLinkProgram(internal_pass->shader);
    glCheckErrorReturn();
    
    if (!dm_opengl_validate_program(internal_pass->shader))
    {
        DM_LOG_FATAL("Failed to validate OpenGL shader!");
        return false;
    }
    
    glDetachShader(internal_pass->shader, vertex_shader);
    glCheckErrorReturn();
    glDetachShader(internal_pass->shader, frag_shader);
    glCheckErrorReturn();
    
    glDeleteShader(vertex_shader);
    glCheckErrorReturn();
    glDeleteShader(frag_shader);
    glCheckErrorReturn();
    
    return true;
}

void dm_opengl_delete_shader(uint32_t pass_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    glDeleteProgram(internal_pass->shader);
    glCheckError();
}

void dm_opengl_bind_shader(uint32_t pass_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    glUseProgram(internal_pass->shader);
    glCheckError();
}

/*******
TEXTURE
*********/

bool dm_opengl_create_texture(dm_image* image)
{
	dm_opengl_texture internal_texture = { 0 };
    
	GLenum format = dm_texture_format_to_opengl_format(image->desc.format);
	if (format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
	GLenum internal_format = dm_texture_format_to_opengl_format(image->desc.internal_format);
	if (internal_format == DM_TEXTURE_FORMAT_UNKNOWN) return false;
    
	glGenTextures(1, &internal_texture.id);
	glCheckErrorReturn();
    
	glBindTexture(GL_TEXTURE_2D, internal_texture.id);
	glCheckErrorReturn();
    
	// load data in
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image->desc.width, image->desc.height, 0, format, GL_UNSIGNED_BYTE, image->data);
	glCheckErrorReturn();
	glGenerateMipmap(GL_TEXTURE_2D);
	glCheckErrorReturn();
    
    dm_slot_list_insert(opengl_textures, &internal_texture, &image->internal_index);
    
	return true;
}

void dm_opengl_destroy_texture(uint32_t internal_index)
{
	dm_opengl_texture* internal_texture = dm_slot_list_at(opengl_textures, internal_index);
	glDeleteTextures(1, &internal_texture->id);
	glCheckError();
}

bool dm_opengl_bind_texture(uint32_t internal_index, uint32_t slot)
{
	dm_opengl_texture* internal_texture = dm_slot_list_at(opengl_textures, internal_index);
    
	glActiveTexture(GL_TEXTURE0 + slot);
	glCheckErrorReturn();
	glBindTexture(GL_TEXTURE_2D, internal_texture->id);
	glCheckErrorReturn();
    
	glUniform1i(internal_texture->location, slot);
	glCheckErrorReturn();
    
	return true;
}

/********
PIPELINE
**********/

bool dm_opengl_bind_pipeline()
{
    dm_opengl_pipeline internal_pipe = opengl_renderer.active_pipeline;
    
    // blending
    if (internal_pipe.blend)
    {
        glEnable(GL_BLEND);
        glBlendEquation(internal_pipe.blend_func);
        glCheckErrorReturn();
        glBlendFunc(internal_pipe.blend_src, internal_pipe.blend_dest);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_BLEND);
    }
    
    // depth testing
    if (internal_pipe.depth)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(internal_pipe.depth_func);
        glCheckErrorReturn();
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    
    // stencil testing
    // TODO needs to be fleshed out correctly
    if (internal_pipe.stencil)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
    
    // wireframe
    if (internal_pipe.wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // sampler state
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, internal_pipe.s_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, internal_pipe.t_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, internal_pipe.min_filter);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, internal_pipe.mag_filter);
    glCheckErrorReturn();
    
    // face culling
    glEnable(GL_CULL_FACE);
    glCullFace(internal_pipe.cull);
    glCheckErrorReturn();
    
    //glFrontFace(internal_pipe->winding);
    //glCheckErrorReturn();
    
    return true;
}

bool dm_opengl_create_render_pipeline(dm_render_pipeline* pipeline, dm_opengl_pipeline* internal_pipe)
{ 
    // face culling
    internal_pipe->cull = dm_cull_to_opengl_cull(pipeline->raster_desc.cull_mode);
    if (internal_pipe->cull == DM_CULL_UNKNOWN) return false;
    
    internal_pipe->winding = dm_wind_top_opengl_wind(pipeline->raster_desc.winding_order);
    if (internal_pipe->winding == DM_WINDING_UNKNOWN) return false;
    
    // primitive
    internal_pipe->primitive = dm_topology_to_opengl_primitive(pipeline->raster_desc.primitive_topology);
    if (internal_pipe->primitive == DM_TOPOLOGY_UNKNOWN) return false;
    
    // sampler
    internal_pipe->min_filter = dm_filter_to_opengl_filter(pipeline->sampler_desc.filter);
    if (internal_pipe->min_filter == DM_FILTER_UNKNOWN) return false;
    internal_pipe->mag_filter = dm_filter_to_opengl_filter(pipeline->sampler_desc.filter);
    if (internal_pipe->mag_filter == DM_FILTER_UNKNOWN) return false;
    internal_pipe->s_wrap = dm_texture_mode_to_opengl_mode(pipeline->sampler_desc.u);
    if (internal_pipe->s_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    internal_pipe->t_wrap = dm_texture_mode_to_opengl_mode(pipeline->sampler_desc.v);
    if (internal_pipe->t_wrap == DM_TEXTURE_MODE_UNKNOWN) return false;
    
    if (pipeline->blend_desc.is_enabled)
    {
        internal_pipe->blend_func = dm_blend_eq_to_opengl_func(pipeline->blend_desc.equation);
        if (internal_pipe->blend_func == DM_BLEND_EQUATION_UNKNOWN) return false;
        
        internal_pipe->blend_src = dm_blend_func_to_opengl_func(pipeline->blend_desc.src);
        if (internal_pipe->blend_src == DM_BLEND_FUNC_UNKNOWN) return false;
        internal_pipe->blend_dest = dm_blend_func_to_opengl_func(pipeline->blend_desc.dest);
        if (internal_pipe->blend_dest == DM_BLEND_FUNC_UNKNOWN) return false;
        
        internal_pipe->blend = true;
    }
    
    if (pipeline->depth_desc.is_enabled)
    {
        internal_pipe->depth_func = dm_comp_to_opengl_comp(pipeline->depth_desc.comparison);
        if (internal_pipe->depth_func == DM_COMPARISON_UNKNOWN) return false;
        
        internal_pipe->depth = true;
    }
    
    // TODO needs to be fleshed out correctly
    if (pipeline->stencil_desc.is_enabled)
    {
        internal_pipe->stencil_func = dm_comp_to_opengl_comp(pipeline->stencil_desc.comparison);
        if (internal_pipe->stencil_func == DM_COMPARISON_UNKNOWN) return false;
        
        internal_pipe->stencil = true;
    }
    
    internal_pipe->wireframe = pipeline->wireframe;
    
    return true;
}

/**********
RENDERPASS
************/

bool dm_opengl_create_render_pass(dm_render_pass* render_pass, const char* vertex_src, const char* pixel_src, dm_vertex_layout layout, size_t scene_cb_size, size_t inst_cb_size)
{
    dm_opengl_render_pass internal_pass = { 0 };
    
    // vertex array object
    glGenVertexArrays(1, &internal_pass.vao);
    glCheckErrorReturn();
    
    // shader
    if (!dm_opengl_create_shader(render_pass->internal_index, vertex_src, pixel_src)) return false;
    
    // vertex attributes
    uint32_t count = 0;
    glBindVertexArray(internal_pass.vao);
    
    // uniforms
    glGenBuffers(1, &internal_pass.scene_uni);
    glBindBuffer(GL_UNIFORM_BUFFER, internal_pass.scene_uni);
    glCheckErrorReturn();
    glBufferData(GL_UNIFORM_BUFFER, scene_cb_size, NULL, GL_STATIC_DRAW);
    glCheckErrorReturn();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckErrorReturn();
    
    glGenBuffers(1, &internal_pass.inst_uni);
    glBindBuffer(GL_UNIFORM_BUFFER, internal_pass.inst_uni);
    glCheckErrorReturn();
    glBufferData(GL_UNIFORM_BUFFER, inst_cb_size, NULL, GL_STATIC_DRAW);
    glCheckErrorReturn();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckErrorReturn();
    
    for (int i = 0; i < layout.num; i++)
    {
        dm_vertex_attrib_desc attrib_desc = layout.attributes[i];
        
        switch (attrib_desc.attrib_class)
        {
            case DM_VERTEX_ATTRIB_CLASS_VERTEX:
            {
                dm_opengl_bind_buffer(opengl_renderer.vertex_buffer_index);
            } break;
            case DM_VERTEX_ATTRIB_CLASS_INSTANCE:
            {
                dm_opengl_bind_buffer(opengl_renderer.instance_buffer_index);
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
    
    dm_slot_list_insert(opengl_render_passes, &internal_pass, &render_pass->internal_index);
    
    return true;
}

void dm_opengl_destroy_render_pass(uint32_t pass_index, uint32_t shader_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    glDeleteVertexArrays(1, &internal_pass->vao);
    glCheckError();
    
    dm_opengl_delete_shader(shader_index);
}

bool dm_opengl_begin_render_pass(uint32_t pass_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    // vertex array
    glBindVertexArray(internal_pass->vao);
    glCheckErrorReturn();
    
    // shader
    dm_opengl_bind_shader(pass_index);
    
    return true;
}

bool dm_renderer_create_render_pass_impl(dm_render_pass* render_pass, const char* vertex_src, const char* pixel_src, dm_vertex_layout layout, size_t scene_cb_size, size_t object_cb_size)
{
    return dm_opengl_create_render_pass(render_pass, vertex_src, pixel_src, layout, scene_cb_size, object_cb_size);
}

void dm_renderer_destroy_render_pass_impl(dm_render_pass* render_pass)
{
    dm_opengl_destroy_render_pass(render_pass->internal_index, render_pass->shader.internal_index);
}

/********
RENDERER
**********/

bool dm_renderer_init_impl(dm_platform_data* platform_data, dm_render_pipeline* pipeline)
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
    
    if(!dm_opengl_create_render_pipeline(pipeline, &opengl_renderer.active_pipeline)) return false;
    
    opengl_render_passes = dm_slot_list_create(sizeof(dm_opengl_render_pass), 0);
    opengl_textures = dm_slot_list_create(sizeof(dm_opengl_texture), 0);
    opengl_buffers = dm_slot_list_create(sizeof(dm_opengl_buffer), 0);
    
    return true;
}

void dm_renderer_shutdown_impl()
{
    dm_slot_list_destroy(opengl_render_passes);
    dm_slot_list_destroy(opengl_textures);
    dm_slot_list_destroy(opengl_buffers);
    
    dm_platform_shutdown_opengl();
}

bool dm_renderer_begin_frame_impl()
{
    return dm_opengl_bind_pipeline();
}

bool dm_renderer_end_frame_impl()
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

bool dm_renderer_init_buffer_data_impl(dm_buffer* buffer, void* data)
{
    return dm_opengl_create_buffer(buffer, data);
}

/********
COMMANDS
**********/

bool dm_renderer_begin_renderpass_impl(uint32_t pass_index)
{
    return dm_opengl_begin_render_pass(pass_index);
}

void dm_renderer_end_renderpass_impl(uint32_t pass_index)
{
    
}

bool dm_renderer_update_buffer_impl(uint32_t buffer_index, void* data, size_t size)
{
    dm_opengl_buffer* internal_buffer = dm_slot_list_at(opengl_buffers, buffer_index);
    glBindBuffer(internal_buffer->type, internal_buffer->id);
    glBufferSubData(internal_buffer->type, 0, size, data);
    
    return true;
}

bool dm_renderer_bind_buffer_impl(uint32_t buffer_index, uint32_t slot)
{
    dm_opengl_buffer* internal_buffer = dm_slot_list_at(opengl_buffers, buffer_index);
    dm_opengl_bind_buffer(buffer->internal_index);
    
    return true;
}

bool dm_create_texture_impl(dm_image* image)
{
    return dm_opengl_create_texture(image);
}

void dm_destroy_texture_impl(uint32_t texture_index)
{
    dm_opengl_destroy_texture(texture_index);
}

void dm_renderer_bind_scene_cb_impl(uint32_t slot, uint32_t pass_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    uint32_t uni_index = glGetUniformBlockIndex(internal_pass->shader, "scene_cb");
    glUniformBlockBinding(internal_pass->shader, uni_index, slot);
}

void dm_renderer_bind_inst_cb_impl(uint32_t slot, uint32_t pass_index)
{
    dm_opengl_render_pass* internal_pass = dm_slot_list_at(opengl_render_passes, pass_index);
    
    uint32_t uni_index = glGetUniformBlockIndex(internal_pass->shader, "inst_cb");
    glUniformBlockBinding(internal_pass->shader, uni_index, slot);
}

bool dm_renderer_update_scene_cb_impl(void* data, size_t data_size, uint32_t pass_index)
{
    return true;
}

bool dm_renderer_update_inst_cb_impl(void* data, size_t data_size, uint32_t pass_index)
{
    return true;
}

bool dm_renderer_bind_texture_impl(uint32_t texture_index, uint32_t slot, uint32_t pass_index)
{
    return dm_opengl_bind_texture(texture_index, slot);
}

void dm_renderer_set_viewport_impl(dm_viewport viewport)
{
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glCheckError();
}

void dm_renderer_clear_impl(dm_color clear_color)
{
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glCheckError();
    
    GLuint clear_bit = GL_COLOR_BUFFER_BIT;
    
    clear_bit |= GL_DEPTH_BUFFER_BIT;
    
    glClear(clear_bit);
    glCheckError();
}

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, uint32_t pass_index)
{
    glDrawArrays(opengl_renderer.active_pipeline.primitive, start, count);
    glCheckError();
}

void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, uint32_t pass_index)
{
    glDrawElements(opengl_renderer.active_pipeline.primitive, num_indices, GL_UNSIGNED_INT, index_offset);
    glCheckError();
}

void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, uint32_t pass_index)
{
    glDrawElementsInstanced(opengl_renderer.active_pipeline.primitive, num_indices, GL_UNSIGNED_INT, 0, num_insts);
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