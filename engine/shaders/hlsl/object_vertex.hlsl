struct VS_INPUT
{
    float3 position    : POSITION;
    float2 tex_coords  : TEXCOORD;
    float4 model0       : MODEL;
    float4 model1 : MODEL1;
    float4 model2 : MODEL2;
    float4 model3 : MODEL3;
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
    
    matrix model = float4x4(input.model0, input.model1, input.model2, input.model3);
    
    output.position = mul(float4(input.position, 1.0f), model);
    output.position = mul(output.position, view_proj);
    output.tex_coords = input.tex_coords;
    
    return output;
}