struct VSInput
{
	float2 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR;
};

[[vk::binding(0)]]
cbuffer GlobalData : register(b0, space0)
{
	float2 translate;
	float2 scale;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position * scale + translate, 0.0, 1.0);
	output.texcoord = input.texcoord;
	output.color = input.color;
	return output;
}