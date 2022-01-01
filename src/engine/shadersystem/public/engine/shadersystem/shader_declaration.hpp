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
};

struct ShaderStage
{
	gfx::ShaderStageFlagBits stage;
	std::vector<ShaderParameter> inputs;
	std::vector<ShaderParameter> outputs;
	std::string code;

	ShaderStage() : stage(gfx::ShaderStageFlagBits::Vertex) {}
};

struct ShaderDeclaration
{
	std::string name;
	gfx::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	gfx::PipelineRasterizationStateCreateInfo rasterization_state;
	std::vector<ShaderParameter> parameters;
	std::vector<ShaderStage> stages;
};

}