struct PS_INPUT
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
    float2 tex_coords : TEXCOORD;
};

struct PS_OUPUT
{
    float4 color : SV_Target;
};

SamplerState sample_state;
Texture2D uTexture1 : register(t0);
Texture2D uTexture2 : register(t1);

float4 p_main(PS_INPUT input) : SV_Target
{
    PS_OUPUT output;
    
    output.color = input.color;
    
    float4 color1 = uTexture1.Sample(sample_state, input.tex_coords);
    float4 color2 = uTexture2.Sample(sample_state, input.tex_coords);
    
    return float4(lerp(color1.rgb, color2.rgb, 0.2), 1.0);
}