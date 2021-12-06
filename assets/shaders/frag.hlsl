struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
	float3 world_pos : TEXCOORD1;
};

[[vk::binding(0)]]
cbuffer GlobalData : register(b0, space0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float4x4 ShadowView;
    float4x4 ShadowProj;
    float3 CamPos;
    float3 LightPos;
    float3 LightDir;
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
    float3 final_color = texture.Sample(test_sampler, input.texcoord).rgb;
    return float4(final_color, 1.0);
}