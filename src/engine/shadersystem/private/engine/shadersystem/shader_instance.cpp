#include "engine/shadersystem/shader.hpp"

namespace ze::shadersystem
{

ShaderInstance::ShaderInstance(ShaderPermutation& in_permutation)
	: permutation(in_permutation), push_constant_data({})
{
	if(permutation.get_state() != ShaderPermutationState::Available)
		permutation.compile();
}

bool ShaderInstance::set_parameter(const std::string& in_name, gfx::BufferHandle in_buffer)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		uint32_t index = std::numeric_limits<uint32_t>::max();

		if(parameter_info->is_uav)
			index = gfx::get_device()->get_uav_descriptor_index(in_buffer);
		else
			index = gfx::get_device()->get_srv_descriptor_index(in_buffer);

		memcpy(push_constant_data.data() + parameter_info->offset, &index, sizeof(uint32_t));
	}

	return false;
}

bool ShaderInstance::set_parameter(const std::string& in_name, gfx::TextureViewHandle in_buffer)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		uint32_t index = std::numeric_limits<uint32_t>::max();

		if (parameter_info->is_uav)
			index = gfx::get_device()->get_uav_descriptor_index(in_buffer);
		else
			index = gfx::get_device()->get_srv_descriptor_index(in_buffer);

		memcpy(push_constant_data.data() + parameter_info->offset, &index, sizeof(uint32_t));
	}

	return false;
}

bool ShaderInstance::set_parameter_uav(const std::string& in_name, const std::span<gfx::TextureViewHandle>& in_textures)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		for(size_t i = 0; i < in_textures.size(); ++i)
		{
			const uint32_t index = gfx::get_device()->get_uav_descriptor_index(in_textures[i]);
			memcpy(push_constant_data.data() + parameter_info->offset + (i * sizeof(uint32_t)), &index, sizeof(uint32_t));
		}

		return true;
	}

	return false;
}

bool ShaderInstance::set_parameter_uav(const std::string& in_name, const std::span<gfx::UniqueTextureView>& in_textures)
{	
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		for (size_t i = 0; i < in_textures.size(); ++i)
		{
			const uint32_t index = gfx::get_device()->get_uav_descriptor_index(in_textures[i].get());
			memcpy(push_constant_data.data() + parameter_info->offset + (i * sizeof(uint32_t)), &index, sizeof(uint32_t));
		}

		return true;
	}

	return false;
}

bool ShaderInstance::set_parameter(const std::string& in_name, gfx::SamplerHandle in_buffer)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		const uint32_t index = gfx::get_device()->get_srv_descriptor_index(in_buffer);
		memcpy(push_constant_data.data() + parameter_info->offset, &index, sizeof(uint32_t));
	}

	return false;
}

void ShaderInstance::bind(gfx::CommandListHandle in_handle)
{
	ZE_CHECK(permutation.is_available());

	using namespace gfx;

	get_device()->cmd_bind_pipeline_layout(in_handle, permutation.get_pipeline_layout());

	if(permutation.get_parameters_size() > 0)
		get_device()->cmd_push_constants(in_handle, 
			permutation.get_shader_stage_flags(), 
			0, 
			static_cast<uint32_t>(permutation.get_parameters_size()),
			push_constant_data.data());

	for (const auto& [stage, shader] : permutation.get_shader_map())
		get_device()->cmd_bind_shader(in_handle, { stage, Device::get_backend_shader(*shader), "main" });
}

}