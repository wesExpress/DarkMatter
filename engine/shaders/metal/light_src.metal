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
    float3 obj_color;
};

struct Uniforms
{
    float4x4 view_proj;
};

vertex vertex_out vertex_main(
    const device vertex_in* vertices [[buffer(0)]], 
    const device vertex_inst* instance_data [[buffer(1)]], 
    constant Uniforms& uniforms [[buffer(2)]], 
    uint vid[[vertex_id]],
    uint instid[[instance_id]])
{
    vertex_out v_out;

    v_out.position = uniforms.view_proj * instance_data[instid].model * float4(vertices[vid].position, 1);
    
    v_out.obj_color = instance_data[instid].obj_color;

    return v_out;
}

fragment float4 fragment_main(
    vertex_out v_in [[stage_in]])
{
    return float4(v_in.obj_color, 1.0f);
}