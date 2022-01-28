struct VS_INPUT
{
    float3 pos   : POSITION;
    float2 tex_coords : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos   : SV_Position;
    float2 tex_coords : TEXCOORD;
};

cbuffer view_proj : register(b0)
{
    matrix vp;
}

cbuffer model : register(b1)
{
    matrix m;
}

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.pos = mul(mul(float4(input.pos, 1.0f), vp), m);
    output.tex_coords = input.tex_coords;
    
    return output;
}