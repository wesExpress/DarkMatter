struct VS_INPUT
{
	float3 position : POSITION;
	
};

struct PS_INPUT
{
	float4 position : SV_Position;
};

PS_INPUT v_main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	
	output.position = float4(input.position, 1);

	return output;
}