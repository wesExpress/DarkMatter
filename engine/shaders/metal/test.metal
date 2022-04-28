#include <metal_stdlib>

using namespace metal;

struct Vertex
{
	float4 position [[position]];
};

struct Uniform
{
	float3 blah;
};

vertex Vertex vertex_main(const device Vertex *vertices [[buffer(0)]], constant Uniform& uniform [[buffer(1)]], uint vid [[vertex_id]])
{
	return vertices[vid];
}

fragment float4 fragment_main(Vertex inVertex [[stage_in]], constant Uniform& uniforms [[buffer(0)]])
{
	return float4(1,0,0,0);
}