struct VS_INPUT
{
    float3 position   : POSITION;
    float2 tex_coords : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position   : SV_Position;
    float2 tex_coords : TEXCOORD;
};

cbuffer object_matrix : register(b0)
{
    matrix mvp;
}

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.tex_coords = input.tex_coords;
    
    return output;
}