#pragma once

#include "engine/gfx/device.hpp"

namespace ze::shadersystem
{

enum class ShaderParameterType
{
	Uint,
	Float,
	Float2,
	Float3,
	Float4,
	Float4x4,
	Texture2D,
	Sampler,
	UniformBuffer,
	StorageBuffer
};

struct ShaderParameter
{
	ShaderParameterType type;
	std::string name;
	gfx::ShaderStageFlags stages;
	uint32_t set;
	uint32_t binding;
	std::vector<ShaderParameter> members;

	ShaderParameter(ShaderParameterType in_type,
		const std::string& in_name,
		const gfx::ShaderStageFlags in_stages = gfx::ShaderStageFlags(),
		const uint32_t in_set = 0,
		const uint32_t in_binding = 0,
		const std::vector<ShaderParameter> in_members = {}) : type(in_type),
		name(in_name), stages(in_stages), set(in_set), binding(in_binding), members(in_members) {}

	static ShaderParameterType get_type_from_string(std::string in_string)
	{
		if (in_string == "float")
			return ShaderParameterType::Float;
		else if (in_string == "float2")
			return ShaderParameterType::Float2;
		else if (in_string == "float3")
			return ShaderParameterType::Float3;
		else if (in_string == "float4")
			return ShaderParameterType::Float4;
		else if (in_string == "float4x4")
			return ShaderParameterType::Float4x4;
		else if (in_string == "Texture2D")
			return ShaderParameterType::Texture2D;
		else if (in_string == "Sampler")
			return ShaderParameterType::Sampler;
		else if (in_string == "UniformBuffer")
			return ShaderParameterType::UniformBuffer;
		else if (in_string == "StorageBuffer")
			return ShaderParameterType::StorageBuffer;
		else
			return ShaderParameterType::Float;
	}
	static gfx::DescriptorType convert_shader_parameter_type(ShaderParameterType in_type)
	{
		switch(in_type)
		{
		case ShaderParameterType::UniformBuffer:
			return gfx::DescriptorType::UniformBuffer;
		case ShaderParameterType::Sampler:
			return gfx::DescriptorType::Sampler;
		case ShaderParameterType::Texture2D:
			return gfx::DescriptorType::SampledTexture;
		default:
			ZE_ASSERT(false);
		}

		ZE_UNREACHABLE();
	}
};

struct ShaderStage
{
	gfx::ShaderStageFlagBits stage;
	std::string hlsl;

	ShaderStage() : stage(gfx::ShaderStageFlagBits::Vertex) {}
	ShaderStage(const gfx::ShaderStageFlagBits in_stage) : stage(in_stage) {}
};

struct ShaderDeclaration
{
	std::string name;
	gfx::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	gfx::PipelineRasterizationStateCreateInfo rasterization_state;
	std::string common_hlsl;
	std::vector<ShaderStage> stages;
};

}