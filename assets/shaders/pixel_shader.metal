#include <metal_stdlib>
using namespace metal;

struct fragment_in
{
    float4 position[[position]];
    float4 color;
};

fragment float4 fragment_main(fragment_in frag[[stage_in]])
{
    return frag.color;
}

