struct PS_INPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL;
    float2 tex_coords : TEXCOORD;
    float3 obj_color  : COLOR;
    float3 frag_pos   : POSITION;
};

struct PS_OUPUT
{
    float4 color : SV_Target;
};

cbuffer object_uniform : register(b0)
{
    matrix view_proj;
    float3 global_light;
    float ambient;
    float3 light_pos;
    float3 view_pos;
}

float spec_str = 0.5;

//SamplerState sample_state;
//Texture2D uTexture1 : register(t0);
//Texture2D uTexture2 : register(t1);

float4 p_main(PS_INPUT input) : SV_Target
{
    //float4 color1 = uTexture1.Sample(sample_state, input.tex_coords);
    //float4 color2 = uTexture2.Sample(sample_state, input.tex_coords);
    //
    //return float4(lerp(color1.rgb, color2.rgb, 0.2), 1.0);
    
    float3 norm_normal = normalize(input.normal);
    float3 light_dir = normalize(light_pos - input.frag_pos);
    
    float diffuse = max(dot(norm_normal, light_dir), 0.0f);
    
    float3 view_dir = normalize(view_pos - input.frag_pos);
    float3 reflect_dir = reflect(-light_dir, norm_normal);
    
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float3 specular = spec_str * spec * global_light;
    
    return float4((ambient + diffuse + specular) * global_light, 1.0f);
}