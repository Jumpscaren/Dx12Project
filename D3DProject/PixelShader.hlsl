struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 uv : uv;
};

Texture2D colourTexture : register(t0);

SamplerState standardSampler : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
	return colourTexture.Sample(standardSampler, input.uv);
}