#pragma once

#include "engine/gfx/device.hpp"
#include "engine/shadersystem/shader.hpp"
#include <robin_hood.h>

namespace ze::materialsystem
{

struct MaterialPass
{
	std::string pass;
	std::unique_ptr<shadersystem::ShaderInstance> shader;
};

class Material
{
public:
	Material(shadersystem::ShaderManager& in_shader_manager);

	[[nodiscard]] const MaterialPass* get_pass(const std::string& in_name) const;
private:
	shadersystem::ShaderManager& shader_manager;
	std::vector<std::unique_ptr<MaterialPass>> passes;
};

}