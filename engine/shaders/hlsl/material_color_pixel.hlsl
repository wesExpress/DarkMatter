struct PS_INPUT
{
    float4 position   : SV_Position;
    float3 normal     : NORMAL2;
    float2 tex_coords : TEXCOORD2;
    float3 frag_pos   : POSITION2;
	float3 diffuse    : COLOR2;
	float3 specular   : COLOR3;
};

cbuffer object_uniform : register(b0)
{
    matrix view_proj;
    float3 light_pos;
    float3 light_ambient;
    float3 light_diffuse;
    float3 light_specular;
	float3 view_pos;
	float shininess;
};

float4 p_main(PS_INPUT input) : SV_Target
{
	float3 norm_normal = normalize(input.normal);
    float3 light_dir = normalize(light_pos - input.frag_pos);
    float3 view_dir = normalize(view_pos - input.frag_pos);
    float3 reflect_dir = reflect(-light_dir, norm_normal);
    
    float3 ambient = light_ambient * input.diffuse;

    float diff = max(dot(norm_normal, light_dir), 0.0f);
    float3 diffuse = light_diffuse * diff * input.diffuse;
    
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    float3 specular = light_specular * spec * input.specular;
    
    return float4((ambient + diffuse + specular), 1.0f);
}