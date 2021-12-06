struct VSInput
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
	float3 world_pos : TEXCOORD1;
    float3x3 tbn : TEXCOORD2;
    float4 shadow_pos : TEXCOORD3;
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

struct RenderNodeGPUInfo
{
    float4x4 world;
    uint first_vertex;
    int vertex_offset;
    uint index_count;
};

//StructuredBuffer<RenderNodeGPUInfo> Instance;

static const float4x4 bias = float4x4(
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0);

VSOutput main(VSInput input/*, VSInstanceInput instance_input*/)
{
	VSOutput output;
	float4x4 WVP = mul(World, mul(View, Proj));
    float4 world_pos = mul(float4(input.position, 1.0), World);

	output.position = mul(float4(input.position, 1.0), WVP);
	output.texcoord = input.texcoord;
	output.normal = input.normal;
    output.world_pos = world_pos.xyz;
	return output;
}