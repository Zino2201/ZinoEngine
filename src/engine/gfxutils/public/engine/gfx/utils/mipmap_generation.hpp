#pragma once

#include "engine/core.hpp"
#include "engine/gfx/device.hpp"
#include "engine/shadersystem/shader.hpp"

namespace ze::gfx
{

void generate_mipmaps(shadersystem::ShaderManager& in_shader_manager, 
	TextureHandle in_texture,
	Format in_format,
	uint32_t in_width, 
	uint32_t in_height, 
	uint32_t in_layer_count);

}