struct VS_INPUT
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 tex_coords  : TEXCOORD;
    matrix model       : MODEL;
    float3 color       : COLOR;
    uint   instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL;
    float2 tex_coords : TEXCOORD;
    float3 obj_color  : COLOR;
    float3 frag_pos   : POSITION;
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