#pragma once

#include "engine/module/module.hpp"
#include "shader_compiler.hpp"
#include <type_traits>

namespace ze::gfx
{

template<typename T>
	requires std::derived_from<T, ShaderCompiler>
class ShaderCompilerModule : public Module
{
public:
	ShaderCompilerModule()
	{
		if(!register_shader_compiler(compiler))
			logger::fatal("Target shader language already registered with another shader compiler");
	}

	~ShaderCompilerModule() override
	{
		unregister_shader_compiler(compiler);
	}

	ShaderCompilerModule(const ShaderCompilerModule&) = delete;
	ShaderCompilerModule& operator=(const ShaderCompilerModule&) = delete;

	ShaderCompilerModule(ShaderCompilerModule&&) noexcept  = default;
	ShaderCompilerModule& operator=(ShaderCompilerModule&&) noexcept = default;

	[[nodiscard]] T& get() const { return compiler; }
private:
	T compiler;
};

}