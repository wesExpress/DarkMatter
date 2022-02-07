struct VS_INPUT
{
    float3 position    : POSITION;
    float2 tex_coords  : TEXCOORD;
    matrix model       : MODEL;
    uint   instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position   : SV_Position;
    float2 tex_coords : TEXCOORD;
};

cbuffer object_matrix : register(b0)
{
    matrix view_proj;
}

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.position = mul(float4(input.position, 1.0f), input.model);
    output.position = mul(output.position, view_proj);
    output.tex_coords = input.tex_coords;
    
    return output;
}