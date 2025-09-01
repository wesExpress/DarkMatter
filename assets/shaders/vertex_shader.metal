#include <metal_stdlib>
using namespace metal;

struct vertex_in
{
    float4 position;
    float4 color;
};

struct fragment_out
{
    float4 position [[position]];
    float4 color;
};

struct ResourceHeap
{
    texture2d<float> textures[10];

};

vertex fragment_out vertex_main(const device vertex_in* vertices[[buffer(0)]], uint v_id[[vertex_id]], uint inst_id[[instance_id]])
{
    fragment_out frag;

    frag.position = float4(vertices[v_id].position.xyz, 1.f);
    frag.color    = vertices[v_id].color;

    return frag;
}

