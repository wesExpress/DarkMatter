#include <metal_stdlib>

using namespace metal;

struct vertex_in
{
    packed_float3 position;
    packed_float2 tex_coords;
};

struct vertex_inst
{
    float4x4 model;
};

struct vertex_out
{
    float4 position[[position]];
    float2 tex_coords;
};

struct Uniform
{
    float4x4 view_proj;
};

vertex vertex_out vertex_main(
    const device vertex_in* vertices [[buffer(0)]], 
    const device vertex_inst* instance_data [[buffer(1)]], 
    constant Uniform& uniforms [[buffer(2)]], 
    uint vid[[vertex_id]],
    uint instid[[instance_id]])
{
    vertex_out v_out;

    v_out.position = uniforms.view_proj * instance_data[instid].model * float4(vertices[vid].position, 1);
    v_out.tex_coords = vertices[vid].tex_coords;

    return v_out;
}

fragment float4 fragment_main(vertex_out v_in [[stage_in]], texture2d<float> texture1[[texture(0)]], texture2d<float> texture2[[texture(1)]], sampler samp [[sampler(0)]])
{
    //return v_in.position;
    return mix(texture1.sample(samp, v_in.tex_coords), texture2.sample(samp, v_in.tex_coords), 0.2);
}