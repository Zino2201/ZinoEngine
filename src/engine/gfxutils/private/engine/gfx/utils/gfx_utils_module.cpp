#include "engine/gfx/utils/gfx_utils_module.hpp"

namespace ze::gfx
{

GfxUtilsModule::GfxUtilsModule() : scatter_upload_shader(nullptr) {}

void GfxUtilsModule::initialize_shaders(shadersystem::ShaderManager& in_shader_system)
{
	scatter_upload_shader = in_shader_system.get_shader("ScatterUpload");
	ZE_ASSERTF(scatter_upload_shader,
		"Scatter upload shader unavailable! Can't resume.");

	const auto scatter_upload_shader_permutation = scatter_upload_shader->get_permutation({});
	scatter_upload_shader_permutation->compile();

	/** Wait for all shaders to be compiled */
	while(scatter_upload_shader_permutation->is_compiling()) {}

	ZE_ASSERTF(scatter_upload_shader && scatter_upload_shader_permutation->is_available(), 
		"Scatter upload shader unavailable! Can't resume.");
}

ZE_IMPLEMENT_MODULE(GfxUtilsModule, GfxUtils);

}