struct VS_INPUT
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 tex_coords  : TEXCOORD;
    matrix model       : MODEL;
    float3 obj_color   : COLOR;
    uint   instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position  : SV_Position;
    float3 obj_color : COLOR;
};

cbuffer object_matrix : register(b0)
{
    matrix view_proj;
}

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), input.model);
    output.position = mul(output.position, view_proj);
    output.obj_color = input.obj_color;
    
    return output;
}