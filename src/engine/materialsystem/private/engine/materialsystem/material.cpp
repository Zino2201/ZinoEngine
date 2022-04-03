#include "engine/materialsystem/material.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze
{

Material::Material(shadersystem::Shader* in_shader, shadersystem::ShaderPermutationId in_permutation_id)
	: shader(in_shader), shader_permutation_id(in_permutation_id) {}

}
