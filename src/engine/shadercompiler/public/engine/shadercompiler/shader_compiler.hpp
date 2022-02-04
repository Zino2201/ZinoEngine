#pragma once

#include "engine/core.hpp"
#include "engine/result.hpp"
#include "engine/module/module.hpp"
#include "engine/gfx/pipeline.hpp"
#include "engine/gfx/shader_format.hpp"
#include <span>

namespace ze::gfx
{

struct ShaderCompilerInput
{
	std::string name;
	std::span<uint8_t> code;
	ShaderFormat target_format;
	std::string entry_point;
	ShaderStageFlagBits stage;

	ShaderCompilerInput() = default;
};

enum class ShaderReflectionResourceType
{
	UniformBuffer,
	Texture2D,
	Sampler,
};

struct ShaderReflectionResource
{
	std::string name;
	ShaderReflectionResourceType type;
	uint32_t set;
	uint32_t binding;
	uint32_t count;
};

struct ShaderReflectionData
{
	std::vector<ShaderReflectionResource> resources;
};

struct ShaderCompilerOutput
{
	bool failed;
	std::vector<uint8_t> bytecode;
	std::vector<std::string> errors;
	ShaderReflectionData reflection_data;

	ShaderCompilerOutput() : failed(true) {}
};

class ShaderCompiler
{
public:
	ShaderCompiler() = default;
	virtual ~ShaderCompiler() = default;

	ShaderCompiler(const ShaderCompiler&) = delete;
	ShaderCompiler& operator=(const ShaderCompiler&) = delete;

	ShaderCompiler(ShaderCompiler&&) noexcept = default;
	ShaderCompiler& operator=(ShaderCompiler&&) noexcept = default;

	[[nodiscard]] virtual std::string_view get_name() const = 0;
	[[nodiscard]] virtual ShaderCompilerOutput compile_shader(const ShaderCompilerInput& in_input) = 0;
	[[nodiscard]] virtual ShaderLanguage get_shader_language() const = 0;
};

bool register_shader_compiler(ShaderCompiler& in_compiler);
void unregister_shader_compiler(const ShaderCompiler& in_compiler);
ShaderCompiler* get_shader_compiler(ShaderFormat in_format);
ShaderCompilerOutput compile_shader(const ShaderCompilerInput& in_input);

}