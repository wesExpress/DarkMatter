#include <metal_stdlib>

using namespace metal;

struct vertex_in
{
    packed_float3 position;
    packed_float3 normal;
    packed_float2 tex_coords;
};

struct vertex_inst
{
    float4x4 model;
    float3 obj_color;
};

struct vertex_out
{
    float4 position[[position]];
    float3 normal;
    float2 tex_coords;
    float3 obj_color;
    float3 frag_pos;
};

struct Uniforms
{
    float4x4 view_proj;
    float3 light_color;
    float ambient_str;
    float3 light_pos;
    float3 view_pos;
};

constant float spec_str = 0.5f;

vertex vertex_out vertex_main(
    const device vertex_in* vertices [[buffer(0)]], 
    const device vertex_inst* instance_data [[buffer(1)]], 
    constant Uniforms& uniforms [[buffer(2)]], 
    uint vid[[vertex_id]],
    uint instid[[instance_id]])
{
    vertex_out v_out;

    v_out.position = uniforms.view_proj * instance_data[instid].model * float4(vertices[vid].position, 1);
    
    v_out.normal = vertices[vid].normal;
    v_out.tex_coords = vertices[vid].tex_coords;
    v_out.obj_color = instance_data[instid].obj_color;
    v_out.frag_pos = float3(uniforms.view_proj * float4(vertices[vid].position, 1.0f));

    return v_out;
}

fragment float4 fragment_main(
    vertex_out v_in [[stage_in]], 
    constant Uniforms& uniforms [[buffer(0)]],
    texture2d<float> texture1[[texture(0)]], 
    texture2d<float> texture2[[texture(1)]], 
    sampler samp [[sampler(0)]])
{
    //return v_in.position;
    //return mix(texture1.sample(samp, v_in.tex_coords), texture2.sample(samp, v_in.tex_coords), 0.2);
    

    float3 ambient = uniforms.light_color * uniforms.ambient_str;

    float3 norm_normal = normalize(v_in.normal);
    float3 light_dir = normalize(uniforms.light_pos - v_in.frag_pos);

    float diff = max(dot(norm_normal, light_dir), 0.0f);
    float3 diffuse = uniforms.light_color * diff;

    float3 view_dir = normalize(uniforms.view_pos - v_in.frag_pos);
    float3 reflect_dir = reflect(light_dir, norm_normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float3 specular = spec_str * spec * uniforms.light_color;

    return float4((ambient + diffuse + specular) * v_in.obj_color, 1.0f);
}