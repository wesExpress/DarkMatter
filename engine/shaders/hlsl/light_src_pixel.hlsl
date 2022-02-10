struct PS_INPUT
{
    float4 position  : SV_Position;
    float3 obj_color : COLOR;
};

float4 p_main(PS_INPUT input) : SV_Target
{
    return float4(input.obj_color, 1);
}