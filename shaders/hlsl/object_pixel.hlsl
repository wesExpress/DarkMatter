struct PS_INPUT
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

struct PS_OUPUT
{
    float4 color : SV_Target;
};

PS_OUPUT p_main(PS_INPUT input)
{
    PS_OUPUT output;
    
    output.color = input.color;
    
    return output;
}