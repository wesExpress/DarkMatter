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

struct instance_buffer
{
    float4x4 model;
};

struct resource_buffer 
{
    device camera_data* camera[[id(0)]];
    device instance_buffer* instances[[id(1)]];
};

vertex fragment_out vertex_main(const device resource_buffer& resources[[buffer(0)]], const device vertex_in* vertices[[buffer(1)]], uint v_id[[vertex_id]], uint inst_id[[instance_id]])
{
    fragment_out frag;

    frag.position = resources.camera->projection * resources.instances->model * float4(vertices[v_id].position.xyz, 1.f);
    frag.color    = vertices[v_id].color;

    return frag;
}

