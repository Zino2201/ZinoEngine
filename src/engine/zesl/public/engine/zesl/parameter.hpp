#pragma once

#include <string>
#include <cstdint>
#include "engine/gfx/pipeline.hpp"

namespace ze::zesl
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
};

}