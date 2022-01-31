#include <metal_stdlib>

using namespace metal;

struct Vertex
{
    float4 position [[position]];
    float4 color;
};

struct Uniform
{
    float4x4 mvp;
};

vertex Vertex vertex_main(const device Vertex* vertices [[buffer(0)]], constant Uniform* uniforms [[buffer(1)]], uint vid[[vertex_id]])
{
    Vertex v_out;

    v_out.position = uniforms->mvp * vertices[vid].position;
    v_out.color = vertices[vid].color;

    return vertices[vid];
}

fragment half4 fragment_main(Vertex v_in [[stage_in]])
{
    return half4(v_in.color);
}