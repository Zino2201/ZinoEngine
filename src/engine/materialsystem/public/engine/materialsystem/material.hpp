#pragma once

#include "engine/shadersystem/shader.hpp"

namespace ze
{

class Material
{
public:
	Material(shadersystem::Shader* in_shader, shadersystem::ShaderPermutationId in_permutation_id);

	shadersystem::Shader* get_shader() const { return shader; }
	const auto& get_shader_permutation_id() const { return shader_permutation_id; }
private:
	shadersystem::Shader* shader;
	shadersystem::ShaderPermutationId shader_permutation_id;
};

}