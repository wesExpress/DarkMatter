struct PS_INPUT
{
    float4 pos : SV_Position;
};

struct PS_OUPUT
{
    float4 color : SV_Target;
};

PS_OUPUT p_main(PS_INPUT input)
{
    PS_OUPUT output;
    
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    return output;
}