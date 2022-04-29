struct VS_INPUT
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 tex_coords  : TEXCOORD;
    matrix model       : MODEL;
    uint   instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL2;
    float2 tex_coords : TEXCOORD2;
    float3 frag_pos   : POSITION2;
};

cbuffer object_uniform : register(b0)
{
    matrix view_proj;
    float4 light_ambient;
    float4 light_diffuse;
    float4 light_specular;
	float4 object_diffuse;
	float4 object_specular;
    float3 light_pos;
	float shininess;
	float3 view_pos;
};

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.position = mul(float4(input.position, 1.0f), input.model);
    output.position = mul(output.position, view_proj);
    
    output.normal = input.normal;
    output.tex_coords = input.tex_coords;
    output.frag_pos = mul(float4(input.position, 1.0f), input.model).xyz;
 
    return output;
}