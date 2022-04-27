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

bool dm_renderer_create_material_pass()
{
    dm_vertex_attrib_desc attribs[] = {
        pos_attrib_desc,
        norm_attrib_desc,
        tex_coord_desc,
        model_attrib_desc
    };
    
    dm_vertex_layout layout = {
        .attributes = attribs,
        .num = sizeof(attribs) / sizeof(attribs[0])
    };
    
    dm_uniform vp = dm_create_uniform("view_proj", DM_UNIFORM_DATA_T_MAT4, sizeof(dm_mat4));
    dm_uniform shiny = dm_create_uniform("shininess", DM_UNIFORM_DATA_T_FLOAT, sizeof(float));
    dm_uniform light_pos = dm_create_uniform("light_pos", DM_UNIFORM_DATA_T_VEC3, sizeof(dm_vec3));
    dm_uniform light_ambient = dm_create_uniform("light_ambient", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform light_diffuse = dm_create_uniform("light_diffuse", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform light_specular = dm_create_uniform("light_specular", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform view_pos = dm_create_uniform("view_pos", DM_UNIFORM_DATA_T_VEC3, sizeof(dm_vec3));
    
    dm_shader_desc vertex_stage = { 0 };
    vertex_stage.type = DM_SHADER_TYPE_VERTEX;
#ifdef DM_OPENGL
    vertex_stage.source = "shaders/glsl/material_vertex.glsl";
#elif defined DM_DIRECTX
    vertex_stage.source = "shaders/hlsl/material_vertex.fxc";
#elif defined DM_METAL
    vertex_stage.source = "vertex_main";
#endif
    
    dm_shader_desc pixel_stage = { 0 };
    pixel_stage.type = DM_SHADER_TYPE_PIXEL;
#ifdef DM_OPENGL
    pixel_stage.source = "shaders/glsl/material_pixel.glsl";
#elif defined DM_DIRECTX
    pixel_stage.source = "shaders/hlsl/material_pixel.fxc";
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
    
    dm_uniform uniforms[] = {
        vp, light_ambient, light_diffuse, light_specular, light_pos, shiny, view_pos
    };
    
    if(!dm_renderer_create_render_pass(shader, layout, uniforms, sizeof(uniforms) / sizeof(dm_uniform),
                                       "material"))
    {
        DM_LOG_FATAL("Could not create default material pass!");
        return false;
    }
    
    return true;
}

bool dm_renderer_create_material_color_pass()
{
    dm_vertex_attrib_desc attribs[] = {
		pos_attrib_desc,
		norm_attrib_desc,
		tex_coord_desc,
		model_attrib_desc
	};
    
    dm_vertex_layout layout = {
        .attributes = attribs,
        .num = sizeof(attribs) / sizeof(attribs[0])
    };
    
    dm_uniform vp = dm_create_uniform("view_proj", DM_UNIFORM_DATA_T_MAT4, sizeof(dm_mat4));
    dm_uniform shiny = dm_create_uniform("shininess", DM_UNIFORM_DATA_T_FLOAT, sizeof(float));
    dm_uniform light_pos = dm_create_uniform("light_pos", DM_UNIFORM_DATA_T_VEC3, sizeof(dm_vec3));
    dm_uniform light_ambient = dm_create_uniform("light_ambient", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform light_diffuse = dm_create_uniform("light_diffuse", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform light_specular = dm_create_uniform("light_specular", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform view_pos = dm_create_uniform("view_pos", DM_UNIFORM_DATA_T_VEC3, sizeof(dm_vec3));
    dm_uniform obj_diffuse = dm_create_uniform("object_diffuse", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    dm_uniform obj_specular = dm_create_uniform("object_specular", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    
    dm_shader_desc vertex_stage = { 0 };
    vertex_stage.type = DM_SHADER_TYPE_VERTEX;
#ifdef DM_OPENGL
    vertex_stage.source = "shaders/glsl/material_color_vertex.glsl";
#elif defined DM_DIRECTX
    vertex_stage.source = "shaders/hlsl/material_color_vertex.fxc";
#elif defined DM_METAL
    vertex_stage.source = "vertex_main";
#endif
    
    dm_shader_desc pixel_stage = { 0 };
    pixel_stage.type = DM_SHADER_TYPE_PIXEL;
#ifdef DM_OPENGL
    pixel_stage.source = "shaders/glsl/material_color_pixel.glsl";
#elif defined DM_DIRECTX
    pixel_stage.source = "shaders/hlsl/material_color_pixel.fxc";
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
    
    dm_uniform uniforms[] = {
        vp, light_ambient, light_diffuse, light_specular, light_pos, shiny, obj_diffuse, obj_specular, view_pos
    };
    
    if(!dm_renderer_create_render_pass(shader, layout, uniforms, sizeof(uniforms) / sizeof(dm_uniform),  "material_color"))
    {
        DM_LOG_FATAL("Could not create default material color pass!");
        return false;
    }
    
    return true;
}

bool dm_renderer_create_light_src_pass()
{
    dm_vertex_attrib_desc attribs[] = {
		pos_attrib_desc,
		norm_attrib_desc,
		tex_coord_desc,
		model_attrib_desc,
	};
    
    dm_vertex_layout layout = {
        .attributes = attribs,
        .num = sizeof(attribs) / sizeof(attribs[0])
    };
    
    dm_uniform vp = dm_create_uniform("view_proj", DM_UNIFORM_DATA_T_MAT4, sizeof(dm_mat4));
    dm_uniform obj_diffuse = dm_create_uniform("object_diffuse", DM_UNIFORM_DATA_T_VEC4, sizeof(dm_vec4));
    
    dm_shader_desc vertex_stage = { 0 };
    vertex_stage.type = DM_SHADER_TYPE_VERTEX;
#ifdef DM_OPENGL
    vertex_stage.source = "shaders/glsl/light_src_vertex.glsl";
#elif defined DM_DIRECTX
    vertex_stage.source = "shaders/hlsl/light_src_vertex.fxc";
#elif defined DM_METAL
    vertex_stage.source = "vertex_main";
#endif
    
    dm_shader_desc pixel_stage = { 0 };
    pixel_stage.type = DM_SHADER_TYPE_PIXEL;
#ifdef DM_OPENGL
    pixel_stage.source = "shaders/glsl/light_src_pixel.glsl";
#elif defined DM_DIRECTX
    pixel_stage.source = "shaders/hlsl/light_src_pixel.fxc";
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
    
    dm_uniform uniforms[] = { vp, obj_diffuse };
    
    if(!dm_renderer_create_render_pass(shader, layout, uniforms, sizeof(uniforms) / sizeof(dm_uniform),  "light_src"))
    {
        DM_LOG_FATAL("Could not create default light source pass!");
        return false;
    }
    
    return true;
}