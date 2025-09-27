#include <metal_stdlib>
using namespace metal;

struct fragment_in
{
    float4 position[[position]];
    float4 color;
    float2 uv;
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
    texture2d<half> material[[id(2)]];
    sampler s[[id(3)]];
};

fragment float4 fragment_main(const device resource_buffer& resources[[buffer(0)]], fragment_in frag[[stage_in]])
{
    //return frag.color;
    return resources.material.sample(resources.s, frag.uv);
}

