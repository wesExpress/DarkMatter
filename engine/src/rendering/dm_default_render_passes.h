#include "dm_render_types.h"
#include "dm_vertex_attribs.h"
#include "core/dm_logger.h"

dm_render_pipeline default_pipeline = {
    .blend_desc = {.is_enabled = true, .src = DM_BLEND_FUNC_SRC_ALPHA, .dest = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA},
    .depth_desc = {.is_enabled = true, .comparison = DM_COMPARISON_LESS},
    .stencil_desc = { 0 },
    .raster_desc = {.cull_mode = DM_CULL_BACK, .winding_order = DM_WINDING_COUNTER_CLOCK, .primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST},
    .sampler_desc = {.comparison = DM_COMPARISON_ALWAYS, .filter = DM_FILTER_LINEAR, .u = DM_TEXTURE_MODE_WRAP, .v = DM_TEXTURE_MODE_WRAP, .w = DM_TEXTURE_MODE_WRAP},
    .wireframe = false
};

typedef struct dm_default_pass_scene_cb
{
    dm_mat4 view_proj;
    dm_vec4 light_ambient;
    dm_vec4 light_diffuse;
    dm_vec4 light_specular;
    dm_vec3 light_pos;
#ifdef DM_DIRECTX
    float padding;
#endif
    dm_vec3 view_pos;
#ifdef DM_DIRECTX
    float padding2;
#endif
} dm_default_pass_scene_cb;

typedef struct dm_default_pass_inst_cb
{
    uint32_t is_light;
    uint32_t has_texture;
    float shininess;
#ifdef DM_DIRECTX
    float padding;
#endif
} dm_default_pass_inst_cb;

bool dm_renderer_create_default_pass()
{
    dm_vertex_attrib_desc attribs[] = {
		pos_attrib_desc,
		norm_attrib_desc,
		tex_coord_desc,
		model_attrib_desc,
        diffuse_attrib_desc,
        specular_attrib_desc
	};
    
    dm_vertex_layout layout = {
        .attributes = attribs,
        .num = sizeof(attribs) / sizeof(attribs[0])
    };
    
    dm_shader_desc vertex_stage = { 0 };
    vertex_stage.type = DM_SHADER_TYPE_VERTEX;
#ifdef DM_OPENGL
    vertex_stage.source = "shaders/glsl/light_src_vertex.glsl";
#elif defined DM_DIRECTX
    vertex_stage.source = "shaders/hlsl/new_shader_vertex.fxc";
#elif defined DM_METAL
    vertex_stage.source = "vertex_main";
#endif
    
    dm_shader_desc pixel_stage = { 0 };
    pixel_stage.type = DM_SHADER_TYPE_PIXEL;
#ifdef DM_OPENGL
    pixel_stage.source = "shaders/glsl/light_src_pixel.glsl";
#elif defined DM_DIRECTX
    pixel_stage.source = "shaders/hlsl/new_shader_pixel.fxc";
#elif defined DM_METAL
    pixel_stage.source = "fragment_main";
#endif
    
    dm_shader_desc stages[] = {
        vertex_stage,
        pixel_stage
    };
    
    dm_shader shader = {0};
    shader.stages = stages;
    shader.num_stages = sizeof(stages) / sizeof(stages[0]);
    
    if(!dm_renderer_create_render_pass(shader, layout, sizeof(dm_default_pass_scene_cb), sizeof(dm_default_pass_inst_cb) * 1024,  "default"))
    {
        DM_LOG_FATAL("Could not create default light source pass!");
        return false;
    }
    
    return true;
}