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
};

struct vertex_out
{
	float4 position[[position]];
	float3 normal;
	float2 tex_coords;
	float3 frag_pos;
};

struct Uniform
{
    float4x4 view_proj;
	float4 light_ambient;
	float4 light_diffuse;
	float4 light_specular;
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

fragment float4 fragment_main(vertex_out v_in [[stage_in]], texture2d<float> diffuse_map[[texture(0)]], texture2d<float> specular_map[[texture(1)]], constant Uniform& uniforms [[buffer(2)]], sampler samplr [[sampler(0)]])
{
	float3 norm_normal = normalize(v_in.normal);
	float3 light_dir = normalize(uniforms.light_pos - v_in.frag_pos);
	float3 view_dir = normalize(uniforms.view_pos - v_in.frag_pos);
	float3 reflect_dir = reflect(-light_dir, norm_normal);

	float3 ambient = (uniforms.light_ambient * diffuse_map.sample(samplr, v_in.tex_coords)).rgb;
	
	float diff = max(dot(norm_normal, light_dir), 0.0f);
	float3 diffuse = (uniforms.light_diffuse * diff * diffuse_map.sample(samplr, v_in.tex_coords)).rgb;
	
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), uniforms.shininess);
	float3 specular = (uniforms.light_specular * spec * specular_map.sample(samplr, v_in.tex_coords)).rgb;

	return float4((ambient + diffuse + specular), 1.0f);
}