#include "engine/shadersystem/shader.hpp"
#include "engine/shadercompiler/shader_compiler.hpp"
#include "engine/jobsystem/job.hpp"
#include "engine/jobsystem/job_group.hpp"
#include "tbb/concurrent_hash_map.h"

namespace ze::shadersystem
{

ShaderPermutation::ShaderPermutation(Shader& in_shader, ShaderPermutationId in_id)
	: shader(in_shader), id(in_id), state(ShaderPermutationState::Unavailable)
{

}



void ShaderPermutation::compile()
{
	auto compile_stage = [](const Shader& shader, const ShaderPermutationId& id, const ShaderStage& in_stage)
	{
		gfx::ShaderCompilerInput input;
		input.name = fmt::format("{} (pass {}, options {}, stage {})",
			shader.get_declaration().name,
			id.pass,
			id.options.to_ullong(),
			std::to_string(in_stage.stage));
		input.stage = in_stage.stage;
		input.target_format = shader.get_shader_manager().get_shader_format();
		input.entry_point = "main";

		std::string code = shader.get_declaration().common_hlsl + in_stage.hlsl;
		input.code = { reinterpret_cast<uint8_t*>(code.data()), reinterpret_cast<uint8_t*>(code.data()) + code.size() };

		return compile_shader(input);
	};

	shader_map.clear();

	state = ShaderPermutationState::Compiling;
	root_compilation_job = new_job(
		[&](jobsystem::Job&)
	{
		jobsystem::JobGroup group;
		tbb::concurrent_hash_map<gfx::ShaderStageFlagBits, gfx::ShaderCompilerOutput> outputs;

		for (const auto& pass : shader.get_declaration().passes)
		{
			if (pass.name == id.pass)
			{
				for (const auto stage : pass.stages)
				{
					jobsystem::Job* stage_job = new_child_job(
						[&, stage](jobsystem::Job&)
						{
							outputs.insert({ stage.stage, compile_stage(shader, id, stage) });
						}, root_compilation_job, jobsystem::JobType::Normal, 0.f);
					group.add(stage_job);
				}

				break;
			}
		}

		group.schedule_and_wait();

		for (auto [stage, output] : outputs)
		{
			if (output.failed)
			{
				logger::error(log_shadersystem, "Shader compiling error: {}", output.errors[0]);
				state = ShaderPermutationState::Unavailable;
			}
			else
			{
				auto result = shader.get_shader_manager().get_device().create_shader(
					gfx::ShaderInfo::make({ (uint32_t*)output.bytecode.data(),
						(uint32_t*)output.bytecode.data() + output.bytecode.size() }));

				if (result)
				{
					shader_map[stage] = gfx::UniqueShader(result.get_value());
				}
				else
				{
					logger::error(log_shadersystem, "Failed to create shader {}:",
						std::to_string(result.get_error()));
					state = ShaderPermutationState::Unavailable;
				}
			}
		}

		if (state != ShaderPermutationState::Unavailable)
		{
			shader_stage_flags = {};

			gfx::PushConstantRange push_constant_range(gfx::ShaderStageFlags(), 0, 0);

			for (auto [stage, output] : outputs)
			{
				shader_stage_flags |= stage;
				for (const auto& push_constant : output.reflection_data.push_constants)
				{
					/** Build parameter info map */
					for(const auto& member : push_constant.members)
					{
						for(const auto& parameter : shader.get_declaration().parameters)
						{
							if(parameter.name == member.name)
							{
								parameter_infos.insert({ parameter.name, ParameterInfo { member.offset, member.size, parameter.is_uav() } });
								break;
							}
						}
					}

					push_constant_range.stage |= stage;
					push_constant_range.size = push_constant.size;
					parameters_size = push_constant.size;
				}
			}

			std::vector bindings =
			{
				gfx::DescriptorSetLayoutBinding(gfx::srv_storage_buffer_binding, 
					gfx::DescriptorType::StorageBuffer, gfx::max_descriptors_per_binding, gfx::ShaderStageFlagBits::All),
				gfx::DescriptorSetLayoutBinding(gfx::uav_storage_buffer_binding, 
					gfx::DescriptorType::StorageBuffer, gfx::max_descriptors_per_binding, gfx::ShaderStageFlagBits::All),
				gfx::DescriptorSetLayoutBinding(gfx::srv_texture_2D_binding, 
					gfx::DescriptorType::SampledTexture, gfx::max_descriptors_per_binding, gfx::ShaderStageFlagBits::All),
				gfx::DescriptorSetLayoutBinding(gfx::srv_texture_cube_binding, 
					gfx::DescriptorType::SampledTexture, gfx::max_descriptors_per_binding, gfx::ShaderStageFlagBits::All),
				gfx::DescriptorSetLayoutBinding(gfx::srv_sampler_binding, 
					gfx::DescriptorType::Sampler, gfx::max_descriptors_per_binding, gfx::ShaderStageFlagBits::All),
			};
			
			std::array set_layouts = { gfx::DescriptorSetLayoutCreateInfo(bindings) };
			std::array push_constant_ranges = { push_constant_range };
			auto result = gfx::get_device()->create_pipeline_layout(gfx::PipelineLayoutInfo({ set_layouts, push_constant_ranges }));
			if (result)
				pipeline_layout = gfx::UniquePipelineLayout(result.get_value());
			else
				logger::fatal(log_shadersystem, "Failed to create pipeline layout for shader {} (permutation: {}): {}",
					shader.get_declaration().name,
					id.options.to_ullong(),
					"error");

			state = ShaderPermutationState::Available;
		}
	},
	jobsystem::JobType::Normal, 0.f);

	root_compilation_job->schedule();

}

}