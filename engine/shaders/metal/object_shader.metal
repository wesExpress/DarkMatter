#include <metal_stdlib>

using namespace metal;

struct Vertex
{
    float4 position [[position]];
    float2 tex_coords;
};

struct Uniform
{
    float4x4 mvp;
};

vertex Vertex vertex_main(const device Vertex* vertices [[buffer(0)]], constant Uniform* uniforms [[buffer(1)]], uint vid[[vertex_id]])
{
    Vertex v_out;

    v_out.position = uniforms->mvp * vertices[vid].position;
    v_out.tex_coords = vertices[vid].tex_coords;

    return v_out;
}

fragment float4 fragment_main(Vertex v_in [[stage_in]], texture2d<float> texture1[[texture(0)]], texture2d<float> texture2[[texture(1)]], sampler samp [[sampler(0)]])
{
    return texture1.sample(samp, v_in.tex_coords);
}