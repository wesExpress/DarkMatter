struct VS_INPUT
{
    float3 pos   : POSITION;
    float3 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.pos = float4(input.pos, 1.0f);
    output.color = float4(input.color, 1.0f);
    
    return output;
}