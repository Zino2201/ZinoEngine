#pragma once

#include "engine/materialsystem/material.hpp"

namespace ze
{

/**
 * Compile a material from a zematerial file
 */
Result<std::unique_ptr<Material>, std::string> compile_zematerial(shadersystem::ShaderManager& in_manager,
	std::unique_ptr<std::streambuf>&& in_stream);

}