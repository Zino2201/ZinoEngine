#pragma once

#include "engine/gfx/device.hpp"
#include "engine/shadersystem/shader.hpp"
#include <robin_hood.h>

namespace ze
{

class Material
{
public:
	Material(shadersystem::Shader* in_shader_permutation);

	[[nodiscard]] shadersystem::Shader* get_shader() const { return shader; }
private:
	shadersystem::Shader* shader;
	//gfx::PipelineMaterialState material_state;
};

}