struct PS_INPUT
{
	float4 position : SV_Position;
	float4 color    : COLOR1;
};

float4 p_main(PS_INPUT input) : SV_Target
{
	return input.color;
}