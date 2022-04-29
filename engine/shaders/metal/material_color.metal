#include <metal_stdlib>

using namespace metal;

struct vertex_in
{
	packed_float3 position;
	float3 normal;
	packed_float2 tex_coords;
};

struct vertex_inst
{
	float4x4 model;
};

struct vertex_out
{
	float4 position [[position]];
	float3 normal;
	float3 diffuse;
	float3 frag_pos;
	float3 specular;
	float2 tex_coords;
};

struct Uniform
{
    float4x4 view_proj;
	float4 light_ambient;
	float4 light_diffuse;
	float4 light_specular;
	float4 object_diffuse;
	float4 object_specular;
	float3 light_pos;
	float shininess;
	float3 view_pos;
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
	v_out.tex_coords = vertices[vid].tex_coords;
	v_out.normal = vertices[vid].normal;
	v_out.frag_pos = (instance_data[instid].model * float4(vertices[vid].position, 1)).xyz;

	return v_out;
}

fragment float4 fragment_main(vertex_out v_in [[stage_in]], constant Uniform& uniforms [[buffer(0)]])
{
	float3 norm_normal = normalize(v_in.normal);
	float3 light_dir = normalize(uniforms.light_pos - v_in.frag_pos);
	float3 view_dir = normalize(uniforms.view_pos - v_in.frag_pos);
	float3 reflect_dir = reflect(-light_dir, norm_normal);

	float3 ambient = (uniforms.light_ambient * uniforms.object_diffuse).rgb;
	
	float diff = max(dot(norm_normal, light_dir), 0.0f);
	float3 diffuse = (uniforms.light_diffuse * diff * uniforms.object_diffuse).rgb;
	
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), uniforms.shininess);
	float3 specular = (uniforms.light_specular * spec * uniforms.object_specular).rgb;

	return float4((ambient + diffuse + specular), 1.0f);
	//return uniforms.object_diffuse;
}