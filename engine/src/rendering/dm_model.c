#include "dm_model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "dm_render_types.h"
#include "dm_renderer_api.h"

#include "core/dm_logger.h"
#include "core/dm_math.h"

#include "structures/dm_list.h"

#include <stdint.h>

typedef struct dm_model_loader
{
    dm_list* vertices;
    dm_list* indices;
    
    dm_list* positions;
    dm_list* normals;
    dm_list* tex_coords;
} dm_model_loader;

typedef enum dm_gltf_vertex_attribute_type
{
    DM_GLTF_VERTEX_ATTRIB_POSITION,
    DM_GLTF_VERTEX_ATTRIB_NORMAL,
    DM_GLTF_VERTEX_ATTRIB_TEX_COORDS,
    DM_GLTF_VERTEX_ATTRIB_UNKNOWN
} dm_gltf_vertex_attribute_type;

typedef struct dm_gltf_vertex_attribute
{
    dm_gltf_vertex_attribute_type type;
    cgltf_component_type c_type;
    uint32_t count;
    uint32_t offset;
    uint32_t stride;
    dm_vec3 min;
    dm_vec3 max;
    cgltf_buffer_view* buffer_view;
} dm_gltf_vertex_attribute;

bool dm_load_gltf(const char* path, bool normalize);
const char* dm_convert_cgltf_error_to_str(cgltf_result code);
dm_gltf_vertex_attribute_type dm_cgltf_attrib_to_dm_attrib(cgltf_attribute_type type);

#define DM_LOAD_ATTRIBUTE(LIST, ELEM_COUNT, ATTR_COUNT, ATTR_OFFSET, BUFFER_OFFSET, BUFFER_DATA, NORMALIZE)\
float* d = (float*)BUFFER_DATA + BUFFER_OFFSET / sizeof(float) + BUFFER_OFFSET / sizeof(float);\
uint32_t runner = 0;\
for(uint32_t t=0; t<ATTR_COUNT; t++)\
{\
switch(ELEM_COUNT)\
{\
case 2:\
{\
dm_vec2 temp = {d[t+runner], d[t+runner+1]};\
if(NORMALIZE)\
{\
dm_vec2 mins = {(BUFFER_DATA)->min[0], (BUFFER_DATA)->min[1]};\
dm_vec2 maxes = {(BUFFER_DATA)->max[0], (BUFFER_DATA)->max[1]};\
dm_vec2 range = dm_vec2_sub_vec2(maxes, mins);\
float scale = DM_MAX(range.x, range.y);\
temp = dm_vec2_scale(temp, 1.0f/scale);\
}\
dm_list_append(LIST, &temp);\
} break;\
case 3:\
{\
dm_vec3 temp = {d[t+runner], d[t+runner+1], d[t+runner+2]};\
if(NORMALIZE)\
{\
dm_vec3 mins = {(BUFFER_DATA)->min[0], (BUFFER_DATA)->min[1], (BUFFER_DATA)->min[2]};\
dm_vec3 maxes = {(BUFFER_DATA)->max[0], (BUFFER_DATA)->max[1], (BUFFER_DATA)->max[2]};\
dm_vec3 range = dm_vec3_sub_vec3(maxes, mins);\
float scale = DM_MAX(DM_MAX(range.x, range.y), range.z);\
temp = dm_vec3_scale(temp, 1.0f/scale);\
}\
dm_list_append(LIST, &temp);\
} break;\
}\
runner += 3;\
}\

dm_model_loader model_loader = {0};

void dm_model_loader_init()
{
    model_loader.vertices = dm_list_create(sizeof(dm_vertex_t), 0);
    model_loader.indices = dm_list_create(sizeof(uint32_t), 0);
    
    model_loader.positions = dm_list_create(sizeof(dm_vec3), 0);
    model_loader.normals = dm_list_create(sizeof(dm_vec3), 0);
    model_loader.tex_coords = dm_list_create(sizeof(dm_vec2), 0);
}

void dm_model_loader_shutdown()
{
    dm_list_destroy(model_loader.tex_coords);
    dm_list_destroy(model_loader.normals);
    dm_list_destroy(model_loader.positions);
    
    dm_list_destroy(model_loader.indices);
    dm_list_destroy(model_loader.vertices);
}

bool dm_load_model(const char* path, bool normalize)
{
    return dm_load_gltf(path, normalize);
}

bool dm_load_gltf(const char* path, bool normalize)
{
    cgltf_options options = {0};
    cgltf_data* data = NULL;
    
    DM_LOG_DEBUG("Loading model: %s", path);
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) 
    {
        DM_LOG_FATAL("Could not load model: %s", path);
        DM_LOG_FATAL("Reason: %s", dm_convert_cgltf_error_to_str(result));
        cgltf_free(data);
        return false;
    }
    
    result = cgltf_load_buffers(&options, data, path);
    if(result != cgltf_result_success)
    {
        DM_LOG_FATAL("Could not load buffers for file: %s", path);
        DM_LOG_FATAL("Reason: %s", dm_convert_cgltf_error_to_str(result));
        cgltf_free(data);
        return false;
    }
    
    // per mesh
    for(uint32_t m=0; m<data->meshes_count; m++)
    {
        cgltf_mesh mesh = data->meshes[m];
        
        dm_list_clear(model_loader.vertices, 0);
        dm_list_clear(model_loader.indices, 0);
        dm_list_clear(model_loader.positions, 0);
        dm_list_clear(model_loader.normals, 0);
        dm_list_clear(model_loader.tex_coords, 0);
        
        // per primitive
        for(uint32_t p=0; p<mesh.primitives_count; p++)
        {
            cgltf_primitive primitive = mesh.primitives[p];
            
            switch(primitive.type)
            {
                case cgltf_primitive_type_triangles:
                {
                    dm_gltf_vertex_attribute attribs[10];
                    uint32_t attrib_count = 0;
                    
                    // per attribute
                    for(uint32_t a=0; a<primitive.attributes_count; a++)
                    {
                        cgltf_attribute attribute = primitive.attributes[a];
                        dm_gltf_vertex_attribute_type type = dm_cgltf_attrib_to_dm_attrib(attribute.type);
                        
                        if(type != DM_GLTF_VERTEX_ATTRIB_UNKNOWN)
                        {
                            attribs[attrib_count].type = type;
                            attribs[attrib_count].c_type = attribute.data->component_type;
                            attribs[attrib_count].count = attribute.data->count;
                            attribs[attrib_count].offset = attribute.data->offset;
                            attribs[attrib_count].stride = attribute.data->stride;
                            attribs[attrib_count].buffer_view = attribute.data->buffer_view;
                            
                            attrib_count++;
                        }
                    }
                    
                    // fill in attribute data
                    for(uint32_t a=0; a<attrib_count; a++)
                    {
                        dm_gltf_vertex_attribute attrib = attribs[a];
                        float* buffer = (float*)attrib.buffer_view->buffer->data + attrib.buffer_view->offset;
                        uint32_t num_components = attrib.stride;
                        switch(attrib.c_type)
                        {
                            case cgltf_component_type_r_8:
                            case cgltf_component_type_r_8u:
                            num_components /= sizeof(uint8_t);
                            break;
                            
                            case cgltf_component_type_r_16:
                            case cgltf_component_type_r_16u:
                            num_components /= sizeof(uint16_t);
                            break;
                            
                            case cgltf_component_type_r_32u:
                            case cgltf_component_type_r_32f:
                            num_components /= sizeof(uint32_t);
                            break;
                            
                            default:
                            DM_LOG_ERROR("Shouldn't be here...");
                            break;
                        }
                        
                        for (uint32_t c=0; c<attrib.count; )
                        {
                            switch(a)
                            {
                                case DM_GLTF_VERTEX_ATTRIB_POSITION:
                                {
                                    dm_vec3 pos = {buffer[c], buffer[c+1], buffer[c+2]};
                                    dm_list_append(model_loader.positions, &pos);
                                    
                                    c += 3;
                                } break;
                                case DM_GLTF_VERTEX_ATTRIB_NORMAL:
                                {
                                    dm_vec3 norm = {buffer[c], buffer[c+1], buffer[c+2]};
                                    dm_list_append(model_loader.normals, &norm);
                                    
                                    c += 3;
                                } break;
                                case DM_GLTF_VERTEX_ATTRIB_TEX_COORDS:
                                {
                                    dm_vec2 uv = {buffer[c], buffer[c+1]};
                                    dm_list_append(model_loader.tex_coords, &uv);
                                    
                                    c += 2;
                                } break;
                            }
                        }
                    }
                    
                    // fill in indices
                    if(primitive.indices)
                    {
                        
                    }
                    else
                    {
                        
                    }
                } break;
                default:
                DM_LOG_WARN("Primitive type not supported! Skipping.");
                break;
            } 
        }
    }
    
    return true;
}

bool dm_load_gltf_old(const char* path, bool normalize)
{
    cgltf_options options = {0};
    cgltf_data* data = NULL;
    
    DM_LOG_DEBUG("Loading model: %s", path);
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) 
    {
        DM_LOG_FATAL("Could not load model: %s", path);
        DM_LOG_FATAL("Reason: %s", dm_convert_cgltf_error_to_str(result));
        cgltf_free(data);
        return false;
    }
    
    result = cgltf_load_buffers(&options, data, path);
    if(result != cgltf_result_success)
    {
        DM_LOG_FATAL("Could not load buffers for file: %s", path);
        DM_LOG_FATAL("Reason: %s", dm_convert_cgltf_error_to_str(result));
        cgltf_free(data);
        return false;
    }
    
    // each mesh
    for(uint32_t i=0; i<data->meshes_count; i++)
    {
        dm_list_clear(model_loader.vertices, 0);
        dm_list_clear(model_loader.indices, 0);
        
        cgltf_mesh mesh = data->meshes[i];
        bool has_indices = true;
        
        // each primitive
        for (uint32_t j=0; j<mesh.primitives_count; j++)
        {
            dm_list_clear(model_loader.positions, 0);
            dm_list_clear(model_loader.normals, 0);
            dm_list_clear(model_loader.tex_coords, 0);
            
            cgltf_primitive primitive = mesh.primitives[j];
            
            // each attribute
            for (uint32_t a=0; a<primitive.attributes_count; a++)
            {
                cgltf_attribute attribute = primitive.attributes[a];
                
                cgltf_buffer_view* buffer_view = attribute.data->buffer_view;
                uint64_t offset = attribute.data->offset;
                uint64_t count = attribute.data->count;
                uint64_t stride = attribute.data->stride;
                cgltf_accessor* buffer_data = buffer_view->buffer->data;
                
                switch(attribute.type)
                {
                    case cgltf_attribute_type_position:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.positions, 3, count, offset, buffer_view->offset, buffer_data, (buffer_data->has_min && buffer_data->has_max));
                    } break;
                    case cgltf_attribute_type_normal:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.normals, 3, count, offset, buffer_view->offset, buffer_data, (buffer_data->has_min && buffer_data->has_max));
                    } break;
                    case cgltf_attribute_type_texcoord:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.tex_coords, 2, count, offset, buffer_view->offset, buffer_data, (buffer_data->has_min && buffer_data->has_max));
                    } break;
                    default:
                    break;
                }
            }
            
            // indices
            cgltf_accessor* acc_indices = primitive.indices;
            
            if(!acc_indices) 
            {
                has_indices = false;
                for(uint32_t t=0; t<model_loader.positions->count; t++)
                {
                    dm_list_append(model_loader.indices, &t);
                }
            }
            else
            {
                uint32_t* inds = (uint32_t*)acc_indices->buffer_view->buffer->data + acc_indices->buffer_view->offset / sizeof(uint32_t) + acc_indices->offset / sizeof(uint32_t);
                for(uint32_t ind=0; ind<acc_indices->count; ind++)
                {
                    
                }
            }
            
            // if normals were missing, calculating flat normals
            if (model_loader.normals->count == 0)
            {
                switch(primitive.type)
                {
                    case cgltf_primitive_type_triangles:
                    {
                        // face normals
                        dm_list* face_normals = dm_list_create(sizeof(dm_vec3), 0);
                        for(uint32_t t=0; t<model_loader.indices->count; )
                        {
                            uint32_t ind1 = *(uint32_t*)dm_list_at(model_loader.indices, t);
                            uint32_t ind2 = *(uint32_t*)dm_list_at(model_loader.indices, t+1);
                            uint32_t ind3 = *(uint32_t*)dm_list_at(model_loader.indices, t+2);
                            
                            dm_vec3* p1 = dm_list_at(model_loader.positions, ind1);
                            dm_vec3* p2 = dm_list_at(model_loader.positions, ind2);
                            dm_vec3* p3 = dm_list_at(model_loader.positions, ind3);
                            
                            dm_vec3 u = dm_vec3_sub_vec3(*p2, *p1);
                            dm_vec3 v = dm_vec3_sub_vec3(*p3, *p1);
                            
                            dm_vec3 normal = dm_vec3_cross(u, v);
                            dm_list_append(face_normals, &normal);
                            
                            t += 3;
                        }
                        
                        // vertex normals
                        for(uint32_t p=0; p<model_loader.positions->count; p++)
                        {
                            uint32_t face = 0;
                            dm_vec3 vertex_normal = {0,0,0};
                            // use indices to generate a face vector
                            for(uint32_t t=0; t<model_loader.indices->count; )
                            {
                                dm_vec3 face_inds = {*(uint32_t*)dm_list_at(model_loader.indices, t), *(uint32_t*)dm_list_at(model_loader.indices, t+1), *(uint32_t*)dm_list_at(model_loader.indices, t+2)};
                                
                                // check if the vertex is present in this face
                                if(p == face_inds.x || p == face_inds.y || p == face_inds.z)
                                {
                                    // add the contribution from this face normal
                                    dm_vec3* face_normal = dm_list_at(face_normals, face);
                                    vertex_normal = dm_vec3_add_vec3(vertex_normal, *face_normal);
                                }
                                
                                face += 1;
                                t += 3;
                            }
                            
                            vertex_normal = dm_vec3_norm(vertex_normal);
                            dm_list_append(model_loader.normals, &vertex_normal);
                        }
                        
                        dm_list_destroy(face_normals);
                    } break;
                    default:
                    {
                        DM_LOG_WARN("Primitive type not handled yet! Setting normals to 0");
                    }
                }
            }
            
            // finally create our vertices and mesh
            for(uint32_t i=0; i<model_loader.positions->count; i++)
            {
                dm_vec3* pos = dm_list_at(model_loader.positions, i);
                dm_vec3* norm = dm_list_at(model_loader.normals, i);
                dm_vec2* t_coords = dm_list_at(model_loader.tex_coords, i);
                
                dm_vertex_t vertex = {0};
                vertex.position = *pos;
                vertex.normal = *norm;
                vertex.tex_coords = *t_coords;
                
                dm_list_append(model_loader.vertices, &vertex);
            }
        }
        
        dm_renderer_api_submit_vertex_data(mesh.name, model_loader.vertices->data, model_loader.indices->data, model_loader.vertices->count, model_loader.indices->count, has_indices);
    }
    
    
    dm_list_clear(model_loader.positions, 0);
    dm_list_clear(model_loader.normals, 0);
    dm_list_clear(model_loader.tex_coords, 0);
    dm_list_clear(model_loader.vertices, 0);
    dm_list_clear(model_loader.indices, 0);
    
    cgltf_free(data);
    
    return true;
}

const char* dm_convert_cgltf_error_to_str(cgltf_result code)
{
    switch(code)
    {
        case cgltf_result_data_too_short: return "data too short";
        case cgltf_result_unknown_format: return "unknown format";
        case cgltf_result_invalid_json: return "invalid json";
        case cgltf_result_invalid_gltf: return "invalid gltf";
        case cgltf_result_invalid_options: return "invalid options";
        case cgltf_result_file_not_found: return "file not found";
        case cgltf_result_io_error: return "io error";
        case cgltf_result_out_of_memory: return "out of memory";
        case cgltf_result_legacy_gltf: return "legacy gltf";
        default: return "unknown error";
    }
}

dm_gltf_vertex_attribute_type dm_cgltf_attrib_to_dm_attrib(cgltf_attribute_type type)
{
    switch(type)
    {
        case cgltf_attribute_type_position: return DM_GLTF_VERTEX_ATTRIB_POSITION;
        case cgltf_attribute_type_normal: return DM_GLTF_VERTEX_ATTRIB_NORMAL;
        case cgltf_attribute_type_texcoord: return DM_GLTF_VERTEX_ATTRIB_TEX_COORDS;
        default: return DM_GLTF_VERTEX_ATTRIB_UNKNOWN;
    }
}