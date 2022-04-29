struct PS_INPUT
{
    float4 position : SV_Position;
};

cbuffer object_matrix : register(b0)
{
    matrix view_proj;
	float4 object_diffuse;
}

float4 p_main(PS_INPUT input) : SV_Target
{
    return object_diffuse;
}