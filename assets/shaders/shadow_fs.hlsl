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

float main(float4 Position : SV_POSITION) : SV_TARGET
{
    return Position.z;
}