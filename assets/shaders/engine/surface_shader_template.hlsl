struct Surface
{
	float3 base_color;
	float3 normal;
	float alpha;
};

struct VertexInput
{
	
};

struct VertexOutput
{
	
};

#if ZE_STAGE_VERTEX

VertexOutput main(VertexInput input)
{
	VertexOutput output;

	return output;
}

#elif ZE_STAGE_FRAGMENT

float4 main(VertexOutput input) : SV_TARGET0
{
	return float4(1, 0, 0, 1);
}

#endif