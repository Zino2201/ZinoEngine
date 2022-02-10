#include "engine/shadersystem/shader.hpp"

namespace ze::shadersystem
{

ShaderInstance::ShaderInstance(ShaderPermutation& in_permutation)
	: permutation(in_permutation), parameter_cache_built(false)
{
	if(permutation.get_state() != ShaderPermutationState::Available)
		permutation.compile();
}

bool ShaderInstance::build_parameters_cache()
{
	if(!parameter_cache_built && permutation.is_available())
	{
		for(const auto& set: permutation.get_sets())
		{
			for(const auto& resource : set)
			{
				if(resource.type == gfx::DescriptorType::UniformBuffer)
				{
					auto result = gfx::get_device()->create_buffer(
						gfx::BufferInfo::make_ubo(resource.size));
					ZE_CHECKF(result.has_value(), std::to_string(result.get_error()));

					auto map_result = gfx::get_device()->map_buffer(result.get_value());
					ZE_CHECKF(map_result.has_value(), std::to_string(map_result.get_error()));

					ubos.emplace_back(
						0,
						resource.binding,
						gfx::UniqueBuffer(result.get_value()),
						map_result.get_value());
				}
			}
		}

		parameter_cache_built = true;
	}

	return parameter_cache_built;
}

bool ShaderInstance::set_ubo_parameter(const std::string& in_name, gfx::BufferHandle in_buffer)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		ubos.emplace_back(parameter_info->set,
			parameter_info->binding,
			in_buffer,
			nullptr);
			return true;
	}

	return false;
}

bool ShaderInstance::set_ssbo_parameter(const std::string& in_name, gfx::BufferHandle in_buffer)
{
	if (const auto* parameter_info = permutation.get_parameter_info(in_name))
	{
		ssbos.emplace_back(parameter_info->set,
			parameter_info->binding,
			in_buffer,
			nullptr);
		return true;
	}

	return false;
}

void ShaderInstance::bind(gfx::CommandListHandle in_handle)
{
	ZE_CHECK(permutation.is_available());

	using namespace gfx;

	get_device()->cmd_bind_pipeline_layout(in_handle, permutation.get_pipeline_layout());

	for (const auto& ubo : ubos)
	{		
		get_device()->cmd_bind_ubo(in_handle,
			ubo.set,
			ubo.binding,
			ubo.buffer.index() == 0 ? std::get<UniqueBuffer>(ubo.buffer).get() : std::get<BufferHandle>(ubo.buffer));
	}

	for (const auto& ssbo : ssbos)
	{
		get_device()->cmd_bind_ssbo(in_handle,
			ssbo.set,
			ssbo.binding,
			ssbo.buffer.index() == 0 ? std::get<UniqueBuffer>(ssbo.buffer).get() : std::get<BufferHandle>(ssbo.buffer));
	}

	for (const auto& [stage, shader] : permutation.get_shader_map())
		get_device()->cmd_bind_shader(in_handle, { stage, Device::get_backend_shader(*shader), "main" });
}

}