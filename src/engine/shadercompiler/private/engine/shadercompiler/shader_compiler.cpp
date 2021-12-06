#include "engine/shadercompiler/shader_compiler.hpp"
#include <robin_hood.h>

namespace ze::gfx
{

ZE_DEFINE_LOG_CATEGORY(shadercompiler);

robin_hood::unordered_map<ShaderLanguage, ShaderCompiler*> shader_compilers;

bool register_shader_compiler(ShaderCompiler& in_compiler)
{
	auto it = shader_compilers.find(in_compiler.get_shader_language());
	if (it != shader_compilers.end())
		return false;

	shader_compilers.insert({ in_compiler.get_shader_language(), &in_compiler });

	logger::info(log_shadercompiler, "Registered shader compiler {} for shader format {}", 
		in_compiler.get_name(), 
		std::to_string(in_compiler.get_shader_language()));

	return true;
}

void unregister_shader_compiler(const ShaderCompiler& in_compiler)
{
	shader_compilers.erase(in_compiler.get_shader_language());
}

Result<ShaderCompiler*, bool> get_shader_compiler(ShaderFormat in_format)
{
	auto it = shader_compilers.find(in_format.language);
	if (it != shader_compilers.end())
	{
		return make_result((*it).second);
	}

	return make_error(false);
}

ShaderCompilerOutput compile_shader(const ShaderCompilerInput& in_input)
{
	logger::info(log_shadercompiler, "Compiling shader {}", in_input.name);

	if (auto result = get_shader_compiler(in_input.target_format))
		return result.get_value()->compile_shader(in_input);

	return {};
}

}