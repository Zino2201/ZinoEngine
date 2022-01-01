#pragma once

namespace ze::shadersystem
{

/**
 * Compile a lua shader into a Shader declaration structure
 */
ShaderDeclaration build_lua_shader(const std::string& in_code);

}