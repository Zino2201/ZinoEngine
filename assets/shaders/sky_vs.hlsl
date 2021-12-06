struct VSInput
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

struct VSInstanceInput
{
    uint node_index : TEXCOORD1;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
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

VSOutput main(VSInput input, VSInstanceInput inst_input)
{
	VSOutput output;
	float4x4 WVP = mul(World, mul(View, Proj));
	output.position = mul(float4(input.position, 1.0), WVP);
	output.texcoord = input.texcoord;
	return output;
}