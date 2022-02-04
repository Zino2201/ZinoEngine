#include "engine/materialsystem/material.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze::materialsystem
{

Material::Material(shadersystem::ShaderManager& in_shader_manager,
	const MaterialDeclaration& in_declaration) : shader_manager(in_shader_manager)
{
	if (shadersystem::Shader* default_shader = shader_manager.get_shader(in_declaration.default_shader.shader))
	{
		shadersystem::ShaderPermutationBuilder perm_builder(*default_shader);
		for (const auto& option : in_declaration.default_shader.options)
			perm_builder.add_option(option.name, option.value);

		default_shader_instance = default_shader->instantiate(perm_builder.get_id());
	}
}

shadersystem::ShaderInstance* Material::get_shader_instance(std::string in_pass) const
{
	auto it = passes_shader_instances.find(in_pass);
	if (it != passes_shader_instances.end())
		return it->second.get();

	return default_shader_instance.get();
}

}
