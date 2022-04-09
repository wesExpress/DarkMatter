struct VS_INPUT
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 tex_coords  : TEXCOORD;
    matrix model       : MODEL;
	float3 diffuse     : COLOR;
	float3 specular    : COLOR1;
    uint   instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL2;
    float2 tex_coords : TEXCOORD2;
    float3 frag_pos   : POSITION2;
	float3 diffuse    : COLOR;
	float3 specular   : COLOR1;
};

cbuffer object_uniform : register(b0)
{
    matrix view_proj;
    float3 light_pos;
    float3 light_ambient;
    float3 light_diffuse;
    float3 light_specular;
	float3 view_pos;
	float shininess;
};

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.position = mul(float4(input.position, 1.0f), input.model);
    output.position = mul(output.position, view_proj);
    
    output.normal = input.normal;
    output.tex_coords = input.tex_coords;
    output.frag_pos = mul(float4(input.position, 1.0f), input.model);
 
	output.diffuse = input.diffuse;
	output.specular = input.specular;
   
    return output;
}