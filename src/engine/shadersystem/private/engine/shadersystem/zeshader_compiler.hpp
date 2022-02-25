#pragma once

#include "engine/result.hpp"
#include "engine/shadersystem/shader_declaration.hpp"

namespace ze::shadersystem
{

/**
 * Compile a zeshader file into a ShaderDeclaration
 */
Result<ShaderDeclaration, std::string> compile_zeshader(std::unique_ptr<std::streambuf>&& in_stream);

}
