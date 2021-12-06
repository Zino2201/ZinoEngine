struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
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

float4 main(VSInput input) : SV_POSITION
{
    float4x4 DepthWVP = mul(World, mul(ShadowView, ShadowProj));
    return mul(float4(input.position, 1.0), DepthWVP);
}