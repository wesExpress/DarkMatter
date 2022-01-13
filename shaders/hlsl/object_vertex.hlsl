struct VS_INPUT
{
    float3 pos : POSITION0;
};

struct PS_INPUT
{
    float4 pos : SV_Position;
};

PS_INPUT v_main(VS_INPUT input)
{
    PS_INPUT output;
    
    output.pos = float4(input.pos, 1.0f);
    
    return output;
}