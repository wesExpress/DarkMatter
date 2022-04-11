#include "dm_model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "dm_render_types.h"
#include "dm_renderer_api.h"

#include "core/dm_logger.h"
#include "core/dm_math.h"

#include "structures/dm_list.h"

typedef struct dm_model_loader
{
    dm_list* vertices;
    dm_list* indices;
    
    dm_list* positions;
    dm_list* normals;
    dm_list* tex_coords;
} dm_model_loader;

bool dm_load_gltf(const char* path);
const char* dm_convert_cgltf_error_to_str(cgltf_result code);

#define DM_LOAD_ATTRIBUTE(LIST, ELEM_COUNT, ATTR_COUNT, ATTR_OFFSET, BUFFER_OFFSET, BUFFER_DATA)\
float* d = (float*)BUFFER_DATA + BUFFER_OFFSET / sizeof(float) + BUFFER_OFFSET / sizeof(float);\
uint32_t runner = 0;\
for(uint32_t t=0; t<ATTR_COUNT; t++)\
{\
switch(ELEM_COUNT)\
{\
case 2:\
{\
dm_vec2 temp = {d[t+runner], d[t+runner+1]};\
dm_list_append(LIST, &temp);\
} break;\
case 3:\
{\
dm_vec3 temp = {d[t+runner], d[t+runner+1], d[t+runner+2]};\
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

bool dm_load_model(const char* path)
{
    return dm_load_gltf(path);
}

bool dm_load_gltf(const char* path)
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
                //DM_LOG_WARN("Attribute: %s", data->meshes[i].primitives[j].attributes[a].name);
                
                cgltf_attribute attribute = primitive.attributes[a];
                
                cgltf_buffer_view* buffer_view = attribute.data->buffer_view;
                uint64_t offset = attribute.data->offset;
                uint64_t count = attribute.data->count;
                uint64_t stride = attribute.data->stride;
                
                switch(attribute.type)
                {
                    case cgltf_attribute_type_position:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.positions, 3, count, offset, buffer_view->offset, buffer_view->buffer->data);
                    } break;
                    case cgltf_attribute_type_normal:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.normals, 3, count, offset, buffer_view->offset, buffer_view->buffer->data);
                    } break;
                    case cgltf_attribute_type_texcoord:
                    {
                        DM_LOAD_ATTRIBUTE(model_loader.tex_coords, 2, count, offset, buffer_view->offset, buffer_view->buffer->data);
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
            
            uint32_t* test = dm_list_at(model_loader.indices, 1558);
            
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