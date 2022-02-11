struct PS_INPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL2;
    float2 tex_coords : TEXCOORD2;
    float3 obj_color  : COLOR2;
    float3 frag_pos   : POSITION2;
};

cbuffer object_uniform : register(b0)
{
    matrix view_proj;
    float3 light_color;
    float ambient_str;
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
    
    float3 ambient = light_color * ambient_str;
    
    float3 norm_normal = normalize(input.normal);
    float3 light_dir = normalize(light_pos - input.frag_pos);
    
    float diff = max(dot(norm_normal, light_dir), 0.0f);
    float3 diffuse = light_color * diff;
    
    float3 view_dir = normalize(view_pos - input.frag_pos);
    float3 reflect_dir = reflect(light_dir, norm_normal);
    
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float3 specular = spec_str * spec * light_color;
    
    return float4((ambient + diffuse + specular) * input.obj_color, 1.0f);
}