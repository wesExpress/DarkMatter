#include <metal_stdlib>
using namespace metal;

struct fragment_in
{
    float4 position[[position]];
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

fragment float4 fragment_main(const device resource_buffer& resources[[buffer(0)]], fragment_in frag[[stage_in]])
{
    return frag.color;
}

