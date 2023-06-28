struct PS_INPUT
{
	float4 position : SV_Position;
};

float4 p_main(PS_INPUT input) : SV_Target
{
	return float4(1,1,1,1);
}