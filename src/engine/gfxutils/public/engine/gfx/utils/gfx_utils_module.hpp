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

	static BufferHandle get_quad_vertex_buffer() { return quad_vertex_buffer.get(); }
	static BufferHandle get_quad_index_buffer() { return quad_index_buffer.get(); }
private:
	shadersystem::Shader* scatter_upload_shader;
	inline static UniqueBuffer quad_vertex_buffer;
	inline static UniqueBuffer quad_index_buffer;
};

}