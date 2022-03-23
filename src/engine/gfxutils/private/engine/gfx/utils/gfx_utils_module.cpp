#include "engine/gfx/utils/gfx_utils_module.hpp"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace ze::gfx
{

GfxUtilsModule::GfxUtilsModule()
{
	{
		const std::array positions_uvs =
		{
			glm::vec2(-1, -1),
			glm::vec2(0, 0),

			glm::vec2(1, -1),
			glm::vec2(1, 0),

			glm::vec2(1, 1),
			glm::vec2(1, 1),

			glm::vec2(-1, 1),
			glm::vec2(0, 1),
		};

		const std::array indices =
		{
			0Ui16, 1Ui16, 2Ui16,
			2Ui16, 3Ui16, 0Ui16
		};

		quad_vertex_buffer = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_vertex_buffer(positions_uvs.size() * sizeof(glm::vec4),
				{ reinterpret_cast<const uint8_t*>(positions_uvs.data()), positions_uvs.size() * sizeof(glm::vec4)})).get_value());

		quad_index_buffer = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_index_buffer(indices.size() * sizeof(uint16_t),
				{ reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(uint16_t) })).get_value());
	}
}

void GfxUtilsModule::initialize_shaders(shadersystem::ShaderManager& in_shader_system)
{
	load_required_shaders(in_shader_system, "ScatterUpload");
	load_required_shaders(in_shader_system, "SPDMipmapsGen");
}

void GfxUtilsModule::destroy_resources()
{
	quad_vertex_buffer.reset();
	quad_index_buffer.reset();
}

void GfxUtilsModule::load_required_shaders(shadersystem::ShaderManager& in_shader_system, const std::string& in_name)
{
	auto shader = in_shader_system.get_shader(in_name);
	ZE_ASSERTF(shader,
		"{} shader not found! Can't resume.",
		in_name);

	const auto permutation = shader->get_permutation({});
	permutation->compile();

	/** Wait for all shaders to be compiled */
	while (permutation->is_compiling()) {}

	ZE_ASSERTF(permutation->is_available(),
		"{} shader permutation unavailable! Can't resume.", 
		in_name);
}

ZE_IMPLEMENT_MODULE(GfxUtilsModule, GfxUtils);

}
