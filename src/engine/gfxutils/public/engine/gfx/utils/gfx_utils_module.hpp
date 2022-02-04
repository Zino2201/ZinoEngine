#pragma once

#include "engine/module/module.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze::gfx
{

class GfxUtilsModule : public Module
{
public:
	GfxUtilsModule();

	void initialize_shaders(shadersystem::ShaderManager& in_shader_system);
private:
	shadersystem::Shader* scatter_upload_shader;
};

}