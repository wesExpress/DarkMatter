struct VS_INPUT
{
	float3 position    	: POSITION;
	float3 normal      	: NORMAL;
	float2 tex_coords  	: TEXCOORD;
	matrix model       	: MODEL;
	float4 object_diffuse  : COLOR;
	float4 object_specular : COLOR1;
	uint   instance_id 	: SV_InstanceID;
};

struct VS_OUTPUT
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

struct inst_data
{
	uint is_light;
	uint has_texture;
	float shininess;
	float padding;
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

cbuffer inst_cb : register(b1)
{
	inst_data inst_data_array[1024];
};

VS_OUTPUT v_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    output.position = mul(float4(input.position, 1.0f), input.model);
    output.position = mul(output.position, view_proj);

	inst_data inst = inst_data_array[input.instance_id];

	if(inst.is_light==0)
	{
		output.normal = input.normal;    
    	output.frag_pos = mul(float4(input.position, 1.0f), input.model).xyz;
    }
	
	if(inst.has_texture==1)
	{
		output.tex_coords = input.tex_coords;
	}
	else
	{
		output.object_diffuse = input.object_diffuse;
		output.object_specular = input.object_specular;
	}

	output.is_light = inst.is_light;
	output.has_texture = inst.has_texture;
	output.shininess = inst.shininess;

    return output;
}