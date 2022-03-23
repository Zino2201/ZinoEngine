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
	void destroy_resources();

	static BufferHandle get_quad_vertex_buffer() { return quad_vertex_buffer.get(); }
	static BufferHandle get_quad_index_buffer() { return quad_index_buffer.get(); }
private:
	void load_required_shaders(shadersystem::ShaderManager& in_shader_system, const std::string& in_name);
private:
	inline static UniqueBuffer quad_vertex_buffer;
	inline static UniqueBuffer quad_index_buffer;
};

}