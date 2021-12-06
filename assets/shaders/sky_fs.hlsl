struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

[[vk::binding(0)]]
cbuffer GlobalData : register(b0, space0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CamPos;
    float Time;
}

[[vk::binding(1)]]
SamplerState test_sampler;

[[vk::binding(2)]]
Texture2D texture;

[[vk::binding(3)]]
Texture2D normal_map;

float4 main(VSOutput input) : SV_TARGET
{
    return texture.Sample(test_sampler, input.texcoord);
}