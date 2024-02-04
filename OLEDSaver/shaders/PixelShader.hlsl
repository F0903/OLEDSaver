struct V2P
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(V2P input) : SV_TARGET
{
	return input.color;
}