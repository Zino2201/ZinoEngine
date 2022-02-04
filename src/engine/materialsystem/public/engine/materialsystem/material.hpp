#pragma once

#include "engine/gfx/device.hpp"
#include "engine/shadersystem/shader.hpp"
#include <robin_hood.h>

namespace ze::materialsystem
{

struct MaterialShaderOption
{
	std::string name;
	int32_t value;
};

struct MaterialShaderDeclaration
{
	std::string shader;
	std::vector<MaterialShaderOption> options;
};

struct MaterialDeclaration
{
	struct Pass
	{
		MaterialShaderDeclaration shader;
	};

	MaterialShaderDeclaration default_shader;
	std::vector<Pass> passes;
};

struct MaterialPass
{
	std::string pass;
	shadersystem::Shader* shader;
};

class Material
{
public:
	Material(shadersystem::ShaderManager& in_shader_manager,
		const MaterialDeclaration& in_declaration);

	[[nodiscard]] shadersystem::ShaderInstance* get_shader_instance(std::string in_pass = "") const;
private:
	shadersystem::ShaderManager& shader_manager;
	std::vector<MaterialPass> passes;
	std::unique_ptr<shadersystem::ShaderInstance> default_shader_instance;
	robin_hood::unordered_map<std::string, std::unique_ptr<shadersystem::ShaderInstance>> passes_shader_instances;
};

}