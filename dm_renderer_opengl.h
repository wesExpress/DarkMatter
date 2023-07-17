#ifndef DM_RENDERER_OPENGL_H
#define DM_RENDERER_OPENGL_H

#define DM_GLUINT_FAIL UINT_MAX

#if DM_DEBUG
GLenum  glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#define glCheckErrorReturn() if(glCheckError()) return false 
#else
#define glCheckError()
#define glCheckErrorReturn()
#endif

/************
OPENGL_TYPES
**************/
typedef struct dm_opengl_buffer_t
{
	GLuint id;
	GLenum type, usage, data_type;
} dm_opengl_buffer;

typedef struct dm_opengl_texture_t
{
	GLuint id;
    GLuint pbos[2];
    uint32_t pbo_index, pbo_n_index;
	GLint location;
    char name[512];
} dm_opengl_texture;

typedef struct dm_opengl_shader_t
{
    GLuint shader;
    GLuint vao;
} dm_opengl_shader;

typedef struct dm_opengl_pipeline_t
{
	GLenum blend_src, blend_dest;
	GLenum blend_func, depth_func, stencil_func;
    GLenum primitive;
    GLenum min_filter, mag_filter, s_wrap, t_wrap;
    GLenum cull, winding;
    bool blend, depth, stencil, wireframe;
} dm_opengl_pipeline;

typedef struct dm_opengl_framebuffer_t
{
    GLuint fbo;
    GLuint color, depth, stencil;
    bool c_flag,d_flag,s_flag;
} dm_opengl_framebuffer;

typedef struct dm_opengl_renderer_t
{
    uint32_t uniform_bindings;
    
    dm_opengl_buffer buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_opengl_shader shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_opengl_texture textures[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_opengl_framebuffer framebuffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_opengl_pipeline pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
    
    GLenum   active_primitive;
    uint32_t buffer_count, shader_count, texture_count, framebuffer_count, pipeline_count;
    uint32_t active_pipeline, active_shader;
} dm_opengl_renderer;

#define DM_OPENGL_GET_RENDERER dm_opengl_renderer* opengl_renderer = renderer->internal_renderer

/**********************
OPENGL_ENUM_CONVERSION
************************/
GLenum dm_buffer_to_opengl_buffer(dm_buffer_type dm_type)
{
    switch (dm_type)
    {
        case DM_BUFFER_TYPE_VERTEX: return GL_ARRAY_BUFFER;
        case DM_BUFFER_TYPE_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
        default:
        DM_LOG_FATAL("Unknown buffer type");
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
        DM_LOG_FATAL("Unknown buffer usage type");
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
        return DM_GLUINT_FAIL;
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

GLenum dm_topology_to_opengl_primitive(dm_primitive_topology topology)
{
    switch (topology)
    {
        case DM_TOPOLOGY_POINT_LIST: return GL_POINT;
        case DM_TOPOLOGY_LINE_LIST: return GL_LINES;
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

/*************
OPENGL_BUFFER
***************/
bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_buffer internal_buffer = { 0 };
    
    internal_buffer.type = dm_buffer_to_opengl_buffer(desc.type);
    if (internal_buffer.type == DM_BUFFER_TYPE_UNKNOWN) return false;
    internal_buffer.usage = dm_usage_to_opengl_draw(desc.usage);
    if (internal_buffer.usage == DM_BUFFER_USAGE_UNKNOWN) return false;
    
    glGenBuffers(1, &internal_buffer.id);
    glCheckErrorReturn();
    
    glBindBuffer(internal_buffer.type, internal_buffer.id);
    glCheckErrorReturn();
    
    glBufferData(internal_buffer.type, desc.buffer_size, data, internal_buffer.usage);
    glCheckErrorReturn();
    
    dm_memcpy(opengl_renderer->buffers + opengl_renderer->buffer_count, &internal_buffer, sizeof(dm_opengl_buffer));
    *handle = opengl_renderer->buffer_count++;
    //glDeleteBuffers(1, &internal_buffer.id);
    return true;
}

void dm_renderer_backend_destroy_buffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->buffer_count) { DM_LOG_ERROR("Trying to delete invalid OpenGL buffer"); return; }
    
    glDeleteBuffers(1, &opengl_renderer->buffers[handle].id);
    glCheckError();
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_buffer internal_uniform = { 0 };
    internal_uniform.type = GL_UNIFORM_BUFFER;
    
    /*
    GLuint block_index = glGetUniformBlockIndex(internal_shader.shader, uniforms[i].name);
    glCheckErrorReturn();
    glUniformBlockBinding(internal_shader.shader, block_index, opengl_renderer.uniform_bindings);
    glCheckErrorReturn();
    */
    
    glGenBuffers(1, &internal_uniform.id);
    glBindBuffer(GL_UNIFORM_BUFFER, internal_uniform.id);
    glCheckErrorReturn();
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
    glCheckErrorReturn();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, renderer->uniform_bindings, internal_uniform.id, 0, size);
    glCheckErrorReturn();
    
    dm_memcpy(opengl_renderer->buffers + opengl_renderer->buffer_count, &internal_uniform, sizeof(dm_opengl_buffer));
    *handle = opengl_renderer->buffer_count++;
    
    renderer->uniform_bindings++;
    
    return true;
}

void dm_renderer_backend_destroy_uniform(dm_render_handle handle, dm_renderer* renderer)
{
    dm_renderer_backend_destroy_buffer(handle, renderer);
}

/**************
OPENGL_TEXTURE
****************/
bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_texture internal_texture = { 0 };
    
    GLenum format;
    switch(num_channels)
    {
        case 3: format = GL_RGB; 
        break;
        case 4: format = GL_RGBA; 
        break;
        
        default: 
        DM_LOG_ERROR("Unknown texture format");
        format = GL_RGBA;
        break;
    }
    
    glGenTextures(1, &internal_texture.id);
    glCheckErrorReturn();
    
    glBindTexture(GL_TEXTURE_2D, internal_texture.id);
    glCheckErrorReturn();
    
    // load data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glCheckErrorReturn();
    glGenerateMipmap(GL_TEXTURE_2D);
    glCheckErrorReturn();
    
    strcpy(internal_texture.name, name);
    
    // pbos
    for(uint32_t i=0; i<2; i++)
    {
        glGenBuffers(1, &internal_texture.pbos[i]);
        glCheckErrorReturn();
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, internal_texture.pbos[i]);
        glCheckErrorReturn();
        glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * sizeof(uint32_t), 0, GL_STATIC_DRAW);
        glCheckErrorReturn();
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    
    internal_texture.pbo_index = 0;
    internal_texture.pbo_n_index = 1;
    
    dm_memcpy(opengl_renderer->textures + opengl_renderer->texture_count, &internal_texture, sizeof(dm_opengl_texture));
    *handle = opengl_renderer->texture_count++;
    
    return true;
}

void dm_renderer_backend_destroy_texture(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->texture_count) { DM_LOG_ERROR("Trying to destroy invalid OpenGL texture"); return; }
    
    glDeleteTextures(1, &opengl_renderer->textures[handle].id);
    glCheckError();
    
    for(uint32_t i=0; i<2; i++)
    {
        glDeleteBuffers(1, &opengl_renderer->textures[handle].pbos[i]);
        glCheckError();
    }
}

/***************
OPENGL_PIPELINE
*****************/
bool dm_renderer_backend_create_pipeline(dm_pipeline_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_pipeline internal_pipeline = { 0 };
    
    internal_pipeline.cull = dm_cull_to_opengl_cull(desc.cull_mode);
    internal_pipeline.winding = dm_wind_top_opengl_wind(desc.winding_order);
    internal_pipeline.primitive = dm_topology_to_opengl_primitive(desc.primitive_topology);
    
    // blending
    if(desc.blend)
    {
        internal_pipeline.blend = true;
        internal_pipeline.blend_func = dm_blend_eq_to_opengl_func(desc.blend_eq);
        internal_pipeline.blend_src = dm_blend_func_to_opengl_func(desc.blend_src_f);
        internal_pipeline.blend_dest = dm_blend_func_to_opengl_func(desc.blend_dest_f);
    }
    
    // depth testing
    if(desc.depth)
    {
        internal_pipeline.depth = true;
        internal_pipeline.depth_func = dm_comp_to_opengl_comp(desc.depth_comp);
    }
    
    // stencil testing
    if(desc.stencil)
    {
        internal_pipeline.stencil = true;
        internal_pipeline.stencil_func = dm_comp_to_opengl_comp(desc.stencil_comp);
    }
    
    // sampler
    internal_pipeline.min_filter = dm_filter_to_opengl_filter(desc.sampler_filter);
    internal_pipeline.mag_filter = dm_filter_to_opengl_filter(desc.sampler_filter);
    internal_pipeline.s_wrap = dm_texture_mode_to_opengl_mode(desc.u_mode);
    internal_pipeline.t_wrap = dm_texture_mode_to_opengl_mode(desc.v_mode);
    
    // misc
    internal_pipeline.wireframe = desc.wireframe;
    
    dm_memcpy(opengl_renderer->pipelines + opengl_renderer->pipeline_count, &internal_pipeline, sizeof(dm_opengl_pipeline));
    *handle = opengl_renderer->pipeline_count++;
    
    return true;
}

void dm_renderer_backend_destroy_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return;
}

/*************
OPENGL_SHADER
***************/
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
    if(!file)
    {
        DM_LOG_FATAL("Could not fopen file: %s", path);
        return DM_GLUINT_FAIL;
    }
    
    // determine size of memory to allocate to the buffer
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* string = dm_alloc(length);
    
    // instead of setting string[length] to nul, we instead
    // find the last character that isn't garbage and set that
    // to nul. otherwise there could be garbage in between and
    // cause a seg fault
    //fread(string, length, 1, file);
    size_t read_count = fread(string, 1, length, file);
    string[read_count-1] = '\0';
    fclose(file);
    
    const char* source = string;
    GLint l = (GLint)length;
    glShaderSource(shader, 1, &source, &l);
    glCheckError();
    glCompileShader(shader);
    glCheckError();
    
    if(!dm_opengl_validate_shader(shader))
    {
        DM_LOG_FATAL("Could not validate shader");
        return DM_GLUINT_FAIL;
    }
    
    dm_free(string);
    
    return shader;
}

bool dm_opengl_create_shader(const char* vertex_src, const char* pixel_src, GLuint* shader)
{
    GLuint vertex_shader = DM_GLUINT_FAIL;
    GLuint frag_shader = DM_GLUINT_FAIL;
    
    vertex_shader = dm_opengl_compile_shader(vertex_src, DM_SHADER_TYPE_VERTEX);
    frag_shader = dm_opengl_compile_shader(pixel_src, DM_SHADER_TYPE_PIXEL);
    
    if(vertex_shader == DM_GLUINT_FAIL) return false;
    if(frag_shader == DM_GLUINT_FAIL) return false;
    
    DM_LOG_DEBUG("Linking shader...");
    GLuint internal_shader = glCreateProgram();
    glAttachShader(internal_shader, vertex_shader);
    glCheckErrorReturn();
    glAttachShader(internal_shader, frag_shader);
    glCheckErrorReturn();
    glLinkProgram(internal_shader);
    glCheckErrorReturn();
    
    if (!dm_opengl_validate_program(internal_shader))
    {
        DM_LOG_FATAL("Failed to validate OpenGL shader");
        return false;
    }
    
    glDetachShader(internal_shader, vertex_shader);
    glCheckErrorReturn();
    glDetachShader(internal_shader, frag_shader);
    glCheckErrorReturn();
    
    glDeleteShader(vertex_shader);
    glCheckErrorReturn();
    glDeleteShader(frag_shader);
    glCheckErrorReturn();
    
    *shader = internal_shader;
    
    return true;
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t num_attribs, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    if(shader_desc.vb_count >2) 
    { 
        DM_LOG_FATAL("OpenGL shader only supports up to 2 buffers: vertex and instance"); 
        return false; 
    }
    
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_buffer vb_buffers[2];
    
    vb_buffers[0] = opengl_renderer->buffers[shader_desc.vb[0]];
    if(shader_desc.vb_count > 1) vb_buffers[1] = opengl_renderer->buffers[shader_desc.vb[1]];
    
    dm_opengl_shader internal_shader = { 0 };
    
    // vao creation
    glGenVertexArrays(1, &internal_shader.vao);
    glCheckErrorReturn();
    
    if(!dm_opengl_create_shader(shader_desc.vertex, shader_desc.pixel, &internal_shader.shader)) return false;
    
    // attribs
    uint32_t attrib_count = 0;
    glBindVertexArray(internal_shader.vao);
    glCheckErrorReturn();
    
    for(uint32_t i=0; i<num_attribs; i++)
    {
        dm_vertex_attrib_desc desc = attrib_descs[i];
        
        switch(desc.attrib_class)
        {
            case DM_VERTEX_ATTRIB_CLASS_VERTEX:
            glBindBuffer(vb_buffers[0].type, vb_buffers[0].id);
            glCheckErrorReturn();
            break;
            case DM_VERTEX_ATTRIB_CLASS_INSTANCE:
            glBindBuffer(vb_buffers[1].type, vb_buffers[1].id);
            glCheckErrorReturn();
            break;
            
            default:
            DM_LOG_FATAL("Unknown vertex attrib class! Shouldn't be here...");
            return false;
        }
        
        GLenum data_t;
        if((desc.data_t==DM_VERTEX_DATA_T_MATRIX_INT) || (desc.data_t==DM_VERTEX_DATA_T_MATRIX_FLOAT))
        {
            if (desc.data_t == DM_VERTEX_DATA_T_MATRIX_INT) data_t = GL_INT;
            else data_t = GL_FLOAT;
            
            for(uint32_t j=0; j<desc.count; j++)
            {
                size_t new_offset;
                size_t type_size = sizeof(float);
                if(data_t==GL_INT) type_size = sizeof(int);
                
                new_offset = j * type_size * desc.count + desc.offset;
                
                glVertexAttribPointer(attrib_count, desc.count, data_t, desc.normalized, desc.stride, (void*)(uintptr_t)new_offset);
                glCheckErrorReturn();
                glEnableVertexAttribArray(attrib_count);
                glCheckErrorReturn();
                if(desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE) 
                {
                    glVertexAttribDivisor(attrib_count, 1);
                    glCheckErrorReturn();
                }
                attrib_count++;
            }
        }
        else
        {
            data_t = dm_vertex_data_t_to_opengl(desc.data_t);
            if(data_t==DM_GLUINT_FAIL) return false;
            
            glVertexAttribPointer(attrib_count, desc.count, data_t, desc.normalized, desc.stride, (void*)(uintptr_t)desc.offset);
            glCheckErrorReturn();
            glEnableVertexAttribArray(attrib_count);
            glCheckErrorReturn();
            if (desc.attrib_class == DM_VERTEX_ATTRIB_CLASS_INSTANCE)
            {
                glVertexAttribDivisor(attrib_count, 1);
                glCheckErrorReturn();
            }
            attrib_count++;
        }
    }
    
    dm_memcpy(opengl_renderer->shaders + opengl_renderer->shader_count, &internal_shader, sizeof(dm_opengl_shader));
    *shader_handle = opengl_renderer->shader_count++;
    
    if(!dm_renderer_backend_create_pipeline(pipe_desc, pipe_handle, renderer)) return false;
    
    //glDeleteVertexArrays(1, &internal_pass.vao);
    //dm_opengl_destroy_shader(internal_pass.shader);
    return true;
}

void dm_renderer_backend_destroy_shader(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->shader_count) { DM_LOG_ERROR("Trying to destroy invalid OpenGL shader"); return; }
    
    glDeleteProgram(opengl_renderer->shaders[handle].shader);
    glCheckError();
}

/******************
OPENGL_FRAMEBUFFER
********************/
bool dm_renderer_backend_create_framebuffer(bool color, bool depth, bool stencil, uint32_t width, uint32_t height, dm_render_handle* handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    dm_opengl_framebuffer fbo;
    glGenFramebuffers(1, &fbo.fbo);
    glCheckErrorReturn();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    
    if(color)
    {
        glGenTextures(1, &fbo.color);
        glCheckErrorReturn();
        glBindTexture(GL_TEXTURE_2D, fbo.color);
        glCheckErrorReturn();
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
        glCheckErrorReturn();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.color, 0);
        glCheckErrorReturn();
        
        fbo.c_flag = true;
    }
    
    if(depth)
    {
        glGenTextures(1, &fbo.depth);
        glCheckErrorReturn();
        glBindTexture(GL_TEXTURE_2D, fbo.depth);
        glCheckErrorReturn();
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width,height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
        glCheckErrorReturn();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depth, 0);
        glCheckErrorReturn();
        
        fbo.d_flag = true;
    }
    
    if(stencil)
    {
        glGenTextures(1, &fbo.stencil);
        glCheckErrorReturn();
        glBindTexture(GL_TEXTURE_2D, fbo.stencil);
        glCheckErrorReturn();
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX, width,height, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, NULL);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glCheckErrorReturn();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
        glCheckErrorReturn();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbo.stencil, 0);
        glCheckErrorReturn();
        
        fbo.s_flag = true;
    }
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        DM_LOG_FATAL("Creating OpenGL framebuffer failed");
        return false;
    }
    
    dm_memcpy(opengl_renderer->framebuffers + opengl_renderer->framebuffer_count, &fbo, sizeof(dm_opengl_framebuffer));
    *handle = opengl_renderer->framebuffer_count++;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return true;
}

void dm_renderer_backend_destroy_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->framebuffer_count) { DM_LOG_ERROR("Trying to destroy invalid OpenGL framebuffer"); return; }
    
    glDeleteFramebuffers(1, &opengl_renderer->framebuffers[handle].fbo);
    glCheckError();
    
    if(opengl_renderer->framebuffers[handle].c_flag) glDeleteTextures(1, &opengl_renderer->framebuffers[handle].color);
    if(opengl_renderer->framebuffers[handle].d_flag) glDeleteTextures(1, &opengl_renderer->framebuffers[handle].depth);
    if(opengl_renderer->framebuffers[handle].s_flag) glDeleteTextures(1, &opengl_renderer->framebuffers[handle].stencil);
    
    return;
}

/*************
OPENGL BACKED
***************/
bool dm_renderer_backend_init(dm_context* context)
{
    if(!dm_platform_init_opengl(&context->platform_data))
    {
        DM_LOG_FATAL("Could not initialize OpenGL");
        return false;
    }
    
    context->renderer.internal_renderer = dm_alloc(sizeof(dm_opengl_renderer));
    
    DM_LOG_DEBUG("Initializing OpenGL render backend...");
    
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
    
    return true;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
    dm_opengl_renderer* opengl_renderer = context->renderer.internal_renderer;
    
    // buffers
    for(uint32_t i=0; i<opengl_renderer->buffer_count; i++)
    {
        dm_renderer_backend_destroy_buffer(i, &context->renderer);
    }
    
    // shaders
    for(uint32_t i=0; i<opengl_renderer->shader_count; i++)
    {
        dm_renderer_backend_destroy_shader(i, &context->renderer);
    }
    
    // textures
    for(uint32_t i=0; i<opengl_renderer->texture_count; i++)
    {
        dm_renderer_backend_destroy_texture(i, &context->renderer);
    }
    
    // framebuffers
    for(uint32_t i=0; i<opengl_renderer->framebuffer_count; i++)
    {
        dm_renderer_backend_destroy_framebuffer(i, &context->renderer);
    }
    
    // pipelines
    for(uint32_t i=0; i<opengl_renderer->pipeline_count; i++)
    {
        dm_renderer_backend_destroy_pipeline(i, &context->renderer);
    }
    
    dm_free(context->renderer.internal_renderer);
    dm_platform_shutdown_opengl(&context->platform_data);
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    glEnable(GL_FRAMEBUFFER_SRGB);
    return true;
}

bool dm_renderer_backend_end_frame(bool vsync, dm_context* context)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    glDisable(GL_FRAMEBUFFER_SRGB);
    
    dm_platform_swap_buffers(vsync, &context->platform_data);
    
    return true;
}

/**********************
OPENGL RENDER COMMANDS
************************/
void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
    glClearColor(r, g, b, a);
    glCheckError();
    
    GLuint clear_bit = GL_COLOR_BUFFER_BIT;
    
    clear_bit |= GL_DEPTH_BUFFER_BIT;
    
    glClear(clear_bit);
    glCheckError();
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
    glViewport(0, 0, width, height);
    glCheckError();
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->pipeline_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL pipeline"); return false; }
    dm_opengl_pipeline internal_pipe = opengl_renderer->pipelines[handle];
    
    // BLENDING
    if(internal_pipe.blend)
    {
        glEnable(GL_BLEND);
        glBlendEquation(internal_pipe.blend_func);
        glCheckErrorReturn();
        glBlendFunc(internal_pipe.blend_src, internal_pipe.blend_dest);
        glCheckErrorReturn();
    }
    else glDisable(GL_BLEND);
    
    // DEPTH TESTING
    if(internal_pipe.depth)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(internal_pipe.depth_func);
        glCheckErrorReturn();
    }
    else glDisable(GL_DEPTH_TEST);
    
    // STENCIL TESTING
    if(internal_pipe.stencil)
    {
        glEnable(GL_STENCIL_TEST);
        glDepthFunc(internal_pipe.stencil_func);
        glCheckErrorReturn();
    }
    else glDisable(GL_STENCIL_TEST);
    
    // WIREFRAME
    if(internal_pipe.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // CULLING
    if(internal_pipe.cull)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(internal_pipe.cull);
        glCheckErrorReturn();
    }
    else glDisable(GL_CULL_FACE);
    
    // SAMPLER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, internal_pipe.s_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, internal_pipe.t_wrap);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, internal_pipe.min_filter);
    glCheckErrorReturn();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, internal_pipe.mag_filter);
    glCheckErrorReturn();
    
    opengl_renderer->active_primitive = internal_pipe.primitive;
    
    return true;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    GLenum new_topology = dm_topology_to_opengl_primitive(topology);
    
    if(new_topology == DM_TOPOLOGY_UNKNOWN) return false;
    
    opengl_renderer->active_primitive = new_topology;
    
    return true;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->shader_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL shader"); return false; }
    
    glBindVertexArray(opengl_renderer->shaders[handle].vao);
    glCheckErrorReturn();
    glUseProgram(opengl_renderer->shaders[handle].shader);
    glCheckErrorReturn();
    
    opengl_renderer->active_shader = handle;
    
    return true;
}

bool dm_opengl_bind_buffer(GLenum type, GLuint id)
{
    glBindBuffer(type, id);
    glCheckErrorReturn();
    
    return true;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL buffer"); return false; }
    
    return dm_opengl_bind_buffer(opengl_renderer->buffers[handle].type, opengl_renderer->buffers[handle].id);
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->buffer_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL uniform"); return false; }
    
    return dm_opengl_bind_buffer(opengl_renderer->buffers[handle].type, opengl_renderer->buffers[handle].id);
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid OpenGL buffer"); return false; }
    
    glBindBuffer(opengl_renderer->buffers[handle].type, opengl_renderer->buffers[handle].id);
    glCheckErrorReturn();
    glBufferSubData(opengl_renderer->buffers[handle].type, offset, data_size, data);
    glCheckErrorReturn();
    
    return true;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->buffer_count) { DM_LOG_FATAL("Trying to update invalid OpenGL uniform"); return false; }
    
    glBindBuffer(GL_UNIFORM_BUFFER, opengl_renderer->buffers[handle].id);
    glCheckErrorReturn();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, data_size, data);
    glCheckErrorReturn();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckErrorReturn();
    
    return true;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->texture_count) { DM_LOG_FATAL("Trying to update invalid OpenGL texture"); return false; }
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glCheckErrorReturn();
    glBindTexture(GL_TEXTURE_2D, opengl_renderer->textures[handle].id);
    glCheckErrorReturn();
    
    /*
    GLint location = -1;
    location = glGetUniformLocation(opengl_renderer->shaders[opengl_renderer->active_shader].shader, internal_texture->name);
    
    if(location < 0)
    {
        DM_LOG_FATAL("Could not find texture: %s", internal_texture->name);
        return false;
    }
    
    glUniform1i(location, slot);
    glCheckErrorReturn();
    */
    
    return true;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->texture_count) { DM_LOG_FATAL("Trying to update invalid OpenGL texture"); return false; }
    
    dm_opengl_texture internal_texture = opengl_renderer->textures[handle];
    
    internal_texture.pbo_index = (internal_texture.pbo_index + 1) % 2;
    internal_texture.pbo_n_index = (internal_texture.pbo_n_index + 1) % 2;
    
    glBindTexture(GL_TEXTURE_2D, internal_texture.id);
    glCheckErrorReturn();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, internal_texture.pbos[internal_texture.pbo_index]);
    glCheckErrorReturn();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glCheckErrorReturn();
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, internal_texture.pbos[internal_texture.pbo_n_index]);
    glCheckErrorReturn();
    glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, 0, GL_STREAM_DRAW);
    glCheckErrorReturn();
    
    void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(ptr)
    {
        dm_memcpy(ptr, data, data_size);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glCheckErrorReturn();
    }
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glCheckErrorReturn();
    
    return true;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClearColor(1,1,1,1);
    //glClear(GL_COLOR_BUFFER_BIT);
    
    return true;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->framebuffer_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL framebuffer"); return false; }
    
    dm_opengl_framebuffer fbo = opengl_renderer->framebuffers[handle];
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    
    if(fbo.color) glClear(GL_COLOR_BUFFER_BIT);
    if(fbo.depth) glClear(GL_DEPTH_BUFFER_BIT);
    if(fbo.stencil) glClear(GL_STENCIL_BUFFER_BIT);
    
    return true;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    if(handle > opengl_renderer->framebuffer_count) { DM_LOG_FATAL("Trying to bind invalid OpenGL framebuffer texture"); return false; }
    
    glBindTexture(GL_TEXTURE_2D, opengl_renderer->framebuffers[handle].color);
    glCheckErrorReturn();
    
    return true;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    glDrawArrays(opengl_renderer->active_primitive, start, count);
    glCheckError();
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    glDrawElements(opengl_renderer->active_primitive, num_indices, GL_UNSIGNED_INT, (void*)(index_offset * sizeof(GLuint)));
    glCheckError();
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
    DM_OPENGL_GET_RENDERER;
    
    glDrawElementsInstanced(opengl_renderer->active_primitive, num_indices, GL_UNSIGNED_INT, (void*)(index_offset * sizeof(GLuint)), num_insts);
    glCheckError();
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
    if(wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/************
OPENGL DEBUG
**************/
#ifdef DM_DEBUG
GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode = GL_NO_ERROR;
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
            case GL_CONTEXT_LOST:                  error = "CONTEXT_LOST"; break;
            default:                               error = "NO_ERROR"; break;
        }
        DM_LOG_ERROR("%s | %s (%d)", error, file, line);
    }
    return errorCode;
}
#endif

#endif //DM_RENDERER_OPENGL_H
