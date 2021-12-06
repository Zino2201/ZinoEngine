#pragma once

#include "engine/Core.hpp"
#include <span>

namespace ze::gfx
{

struct ShaderCreateInfo
{
	std::span<uint32_t> bytecode;

	explicit ShaderCreateInfo(const std::span<uint32_t>& in_bytecode = {}) : bytecode(in_bytecode) {}
};
	
}