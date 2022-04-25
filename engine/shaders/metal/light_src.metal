#include <metal_stdlib>

using namespace metal;

struct vertex_in
{
	float3 position;
	float3 normal;
	float2 tex_coords;
};

struct vertex_inst
{
	float4x4 model;
	float3 diffuse;
};

struct vertex_out
{
	float4 position[[position]];
	float3 diffuse;
};

struct Uniform
{
    float4x4 view_proj;
};

vertex vertex_out vertex_main(
	const device vertex_in* vertices[[buffer(0)]],
	const device vertex_inst* instance_data [[buffer(1)]],
	constant Uniform& uniforms [[buffer(2)]],
	uint vid[[vertex_id]],
	uint instid[[instance_id]]
)
{
	vertex_out v_out;

	v_out.position = uniforms.view_proj * instance_data[instid].model * float4(vertices[vid].position, 1);
	v_out.diffuse = instance_data[instid].diffuse;

	return v_out;
}

fragment float4 fragment_main(vertex_out v_in [[stage_in]], constant Uniform& uniforms [[buffer(2)]])
{
	return float4(v_in.diffuse, 1.0f);
}