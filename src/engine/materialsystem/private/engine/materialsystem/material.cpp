#include "engine/materialsystem/material.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze::materialsystem
{

Material::Material(shadersystem::ShaderManager& in_shader_manager) : shader_manager(in_shader_manager) {}

const MaterialPass* Material::get_pass(const std::string& in_name) const
{
	for(const auto& pass : passes)
	{
		if (pass->pass == in_name)
			return pass.get();
	}

	return nullptr;
}

}
