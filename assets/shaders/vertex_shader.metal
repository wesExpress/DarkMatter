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

struct camera_data
{
    float4x4 projection;
};

struct resource_buffer 
{
    device camera_data* camera[[id(0)]];
};

vertex fragment_out vertex_main(const device resource_buffer& resources[[buffer(0)]], const device vertex_in* vertices[[buffer(1)]], uint v_id[[vertex_id]], uint inst_id[[instance_id]])
{
    fragment_out frag;

    float4x4 identity = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    frag.position = resources.camera->projection * float4(vertices[v_id].position.xyz, 1.f);
    //frag.position = float4(vertices[v_id].position.xyz, 1.f);
    frag.color    = vertices[v_id].color;

    return frag;
}

