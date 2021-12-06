struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR;
};

[[vk::binding(1)]]
SamplerState texture_sampler : register(t1, space0);

[[vk::binding(2)]]
Texture2D texture : register(t2, space0);

float4 main(VSOutput input) : SV_TARGET
{
	return input.color * texture.Sample(texture_sampler, input.texcoord);

}