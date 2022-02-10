#pragma once

#include "engine/gfx/device.hpp"

namespace ze::shadersystem
{

enum class ShaderParameterType
{
	Uint,
	Uint64,
	Float,
	Float2,
	Float3,
	Float4,
	Float4x4,
	Texture2D,
	Sampler,
	UniformBuffer,
	StorageBuffer,
	ByteAddressBuffer,
	RWByteAddressBuffer,
};

struct ShaderParameter
{
	ShaderParameterType type;
	std::string name;

	ShaderParameter(ShaderParameterType in_type,
		const std::string& in_name) : type(in_type),
		name(in_name) {}

	bool is_stored_in_buffer() const
	{
		switch(type)
		{
		default:
			return false;
		case ShaderParameterType::Uint:
		case ShaderParameterType::Float:
		case ShaderParameterType::Float2:
		case ShaderParameterType::Float3:
		case ShaderParameterType::Float4:
		case ShaderParameterType::Float4x4:
			return true;
		}
	}
};

struct ShaderStage
{
	gfx::ShaderStageFlagBits stage;
	std::string hlsl;

	ShaderStage() : stage(gfx::ShaderStageFlagBits::Vertex) {}
	ShaderStage(const gfx::ShaderStageFlagBits in_stage) : stage(in_stage) {}
};

struct ShaderPass
{
	std::string name;
	std::string common_hlsl;
	std::vector<ShaderStage> stages;
	bool is_compute_pass;

	ShaderPass() : is_compute_pass(false) {}
};

struct ShaderDeclaration
{
	std::string name;
	gfx::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	gfx::PipelineRasterizationStateCreateInfo rasterization_state;
	std::string common_hlsl;
	std::vector<ShaderPass> passes;
	std::vector<ShaderParameter> parameters;
};

}