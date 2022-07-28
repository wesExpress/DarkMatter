struct PS_INPUT
{
    float4 position   	 : SV_Position;
    float3 normal     	 : NORMAL1;
    float2 tex_coords 	 : TEXCOORD1;
	float4 object_diffuse  : COLOR2;
	float4 object_specular : COLOR3;
    float3 frag_pos   	 : POSITION1;
	uint   is_light		: LIGHT;
	uint   has_texture	  : TEXTURE;
	float  shininess       : SHININESS;
};

cbuffer scene_cb : register(b0)
{
	matrix view_proj;
	float4 light_ambient;
	float4 light_diffuse;
	float4 light_specular;
	float3 light_pos;
	float padding;
	float3 view_pos;
	float padding2;
};

SamplerState sample_state;
Texture2D diffuse_map : register(t0);
Texture2D specular_map : register(t1);

float4 p_main(PS_INPUT input) : SV_Target
{
	//return float4(1,1,1,1);

	if(input.is_light == 0)
	{
		float3 norm_normal = normalize(input.normal);
		float3 light_dir = normalize(light_pos - input.frag_pos);
		float3 view_dir = normalize(view_pos - input.frag_pos);
		float3 reflect_dir = reflect(-light_dir, norm_normal);

		float3 ambient;
		float3 diffuse;
		float3 specular;

    	float diff = max(dot(norm_normal, light_dir), 0.0f);
		float spec = pow(max(dot(view_dir, reflect_dir), 0.0), input.shininess);

		if(input.has_texture == 0)
		{
    		ambient = (light_ambient * input.object_diffuse).rgb;
	    	diffuse = (light_diffuse * diff * input.object_diffuse).rgb;
	    	specular = (light_specular * spec * input.object_specular).rgb;
		}
		else
		{
			ambient = (light_ambient * diffuse_map.Sample(sample_state, input.tex_coords)).rgb;
	    	diffuse = (light_diffuse * diff * diffuse_map.Sample(sample_state, input.tex_coords)).rgb;
	    	specular = (light_specular * spec * specular_map.Sample(sample_state, input.tex_coords)).rgb;
		}
    
    	return float4((ambient + diffuse + specular), 1.0f);
	}
	else
	{
		return input.object_diffuse;
	}
}