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

float4 p_main(PS_INPUT input) : SV_Target
{
    PS_OUPUT output;
    
    output.color = input.color;
    
    return uTexture1.Sample(sample_state, input.tex_coords);
}