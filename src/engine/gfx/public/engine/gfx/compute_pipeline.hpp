#pragma once

#include "pipeline.hpp"

namespace ze::gfx
{

struct ComputePipelineCreateInfo
{
	PipelineShaderStage shader_stage;
	BackendDeviceResource pipeline_layout;

	ComputePipelineCreateInfo(const PipelineShaderStage& in_shader_stage,
		const BackendDeviceResource& in_pipeline_layout) :
		shader_stage(in_shader_stage), pipeline_layout(in_pipeline_layout) {}

	bool operator==(const ComputePipelineCreateInfo& in_other) const
	{
		return shader_stage == in_other.shader_stage &&
			pipeline_layout == in_other.pipeline_layout;
	}
};

}

namespace std
{

template<> struct hash<ze::gfx::ComputePipelineCreateInfo>
{
	uint64_t operator()(const ze::gfx::ComputePipelineCreateInfo& in_create_info) const noexcept
	{
		uint64_t hash = 0;

		ze::hash_combine(hash, in_create_info.shader_stage);
		ze::hash_combine(hash, in_create_info.pipeline_layout);
			
		return hash;
	}
};

}