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
	auto compile_stage = [&](const ShaderStage& in_stage)
	{
		gfx::ShaderCompilerInput input;
		input.name = fmt::format("{} (permutation {}, stage {})",
			shader.get_declaration().name,
			id.to_ullong(),
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

		for (const auto& stage : shader.get_declaration().stages)
		{
			jobsystem::Job* stage_job = new_child_job(
				[&, stage](jobsystem::Job&)
				{
					outputs.insert({ stage.stage, compile_stage(stage) });
				}, root_compilation_job, jobsystem::JobType::Normal, 0.f);
			group.add(stage_job);
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
			std::vector<gfx::DescriptorSetLayoutBinding> bindings;

			for (auto [stage, output] : outputs)
			{
				for (const auto& resource : output.reflection_data.resources)
				{
					gfx::DescriptorType type = gfx::DescriptorType::UniformBuffer;
					switch(resource.type)
					{
					case gfx::ShaderReflectionResourceType::Texture2D:
						type = gfx::DescriptorType::SampledTexture;
						break;
					case gfx::ShaderReflectionResourceType::Sampler:
						type = gfx::DescriptorType::Sampler;
						break;
					}

					bool found_existing = false;
					for (auto& binding : bindings)
					{
						if (binding.binding == resource.binding)
						{
							binding.stage |= gfx::ShaderStageFlags(stage);
							found_existing = true;
						}
					}

					if(!found_existing)
						bindings.emplace_back(
							resource.binding,
							type,
							1,
							gfx::ShaderStageFlags(stage));
				}
			}

			std::array set_layouts = { gfx::DescriptorSetLayoutCreateInfo(bindings) };
			auto result = gfx::get_device()->create_pipeline_layout(gfx::PipelineLayoutInfo({ set_layouts, {} }));
			if (result)
				pipeline_layout = gfx::UniquePipelineLayout(result.get_value());
			else
				logger::fatal(log_shadersystem, "Failed to create pipeline layout for shader {} (permutation: {}): {}",
					shader.get_declaration().name,
					id.to_ullong(),
					"error");

			state = ShaderPermutationState::Available;
		}
	},
	jobsystem::JobType::Normal, 0.f);

	root_compilation_job->schedule();

}

}