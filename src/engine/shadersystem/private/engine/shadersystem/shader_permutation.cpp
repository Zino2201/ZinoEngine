#include "engine/shadersystem/shader.hpp"
#include "engine/shadercompiler/shader_compiler.hpp"
#include "engine/jobsystem/job.hpp"
#include "engine/jobsystem/job_group.hpp"
#include "tbb/concurrent_hash_map.h"

namespace ze::shadersystem
{

ShaderPermutation::ShaderPermutation(Shader& in_shader, ShaderPermutationId in_id)
	: shader(in_shader), id(in_id), state(ShaderPermutationState::Unavailable) {}

void ShaderPermutation::compile()
{
	/** Setup input/output structure for each stage */
	// TODO: recode all
	struct Data
	{
		robin_hood::unordered_map<gfx::ShaderStageFlagBits, std::string> inputs;
		robin_hood::unordered_map<gfx::ShaderStageFlagBits, std::string> outputs;
		robin_hood::unordered_map<gfx::ShaderStageFlagBits, std::string> parameters;
		robin_hood::unordered_map<gfx::ShaderStageFlagBits, std::string> main_functions;
		std::vector<ShaderParameter> vertex_outputs;
	};

	std::shared_ptr<Data> shared_data = std::make_shared<Data>();
	for(const auto& stage : shader.get_declaration().stages)
	{
		auto convert_type_to_hlsl_type = [](const ShaderParameterType in_type) -> std::string
		{
			switch (in_type)
			{
			default:
			case ShaderParameterType::Float:
				return "float";
			case ShaderParameterType::Float2:
				return "float2";
			case ShaderParameterType::Float3:
				return "float3";
			case ShaderParameterType::Float4:
				return "float4";
			case ShaderParameterType::Texture2D:
				return "Texture2D";
			case ShaderParameterType::Sampler:
				return "SamplerState";
			case ShaderParameterType::Uint:
				return "uint";
			case ShaderParameterType::UniformBuffer:
				return "cbuffer";
			case ShaderParameterType::StorageBuffer:
				return "TODOTODO";
			}
		};
		auto format_structure = [&](const std::vector<ShaderParameter>& parameters, const std::string& in_name, bool is_output) -> std::string
		{
			std::string str = fmt::format(
				"struct {} {{\n",
				in_name);

			size_t texcoord_idx = 0;
			for (const auto& parameter : parameters)
			{
				std::string semantic = parameter.name == "position" ? "POSITION" : fmt::format("TEXCOORD{}", texcoord_idx++);
				if ((is_output || stage.stage == gfx::ShaderStageFlagBits::Fragment) && semantic == "POSITION")
					semantic = "SV_POSITION";

				str += fmt::format("{} {} : {};\n",
					convert_type_to_hlsl_type(parameter.type),
					parameter.name,
					semantic);
			}

			str += "};\n";

			return str;
		};

		if (stage.stage == gfx::ShaderStageFlagBits::Vertex)
			shared_data->vertex_outputs = stage.outputs;

		const std::string input_structure_name = fmt::format("{}ShaderInput", std::to_string(stage.stage));
		const std::string output_structure_name = stage.stage == gfx::ShaderStageFlagBits::Fragment ? 
			"float4" : fmt::format("{}ShaderOutput", std::to_string(stage.stage));

		/** For fragment shader take vertex shader inputs and return float4 */
		if (stage.stage == gfx::ShaderStageFlagBits::Fragment)
		{
			shared_data->inputs[stage.stage] = format_structure(shared_data->vertex_outputs, input_structure_name, false);
		}
		else
		{
			shared_data->inputs[stage.stage] = format_structure(stage.inputs, input_structure_name, false);
			shared_data->outputs[stage.stage] = format_structure(stage.outputs, output_structure_name, true);
		}

		for (const auto& parameter : shader.get_declaration().parameters)
		{
			if (parameter.stages & stage.stage)
			{
				shared_data->parameters[stage.stage] += fmt::format("[[vk::binding({})]]\n", parameter.binding);
				if (parameter.type == ShaderParameterType::UniformBuffer ||
					parameter.type == ShaderParameterType::StorageBuffer)
				{
					shared_data->parameters[stage.stage] += fmt::format("{} {} {{\n",
						convert_type_to_hlsl_type(parameter.type),
						parameter.name);

					for(const auto& member : parameter.members)
					{
						shared_data->parameters[stage.stage] += fmt::format("{} {};\n",
							convert_type_to_hlsl_type(member.type),
							member.name);
					}

					shared_data->parameters[stage.stage] += "};\n";
				}
				else
				{
					shared_data->parameters[stage.stage] += fmt::format("{} {};\n",
						convert_type_to_hlsl_type(parameter.type),
						parameter.name);
				}
			}
		}

		if (stage.stage == gfx::ShaderStageFlagBits::Fragment)
		{
			shared_data->main_functions[stage.stage] = fmt::format(
				"{} main({} input) : SV_TARGET {{\n"
				"{} output;\n"
				"{}\n"
				"return output;\n"
				"}}",
				output_structure_name,
				input_structure_name,
				output_structure_name,
				stage.code);
		}
		else
		{
			shared_data->main_functions[stage.stage] = fmt::format(
				"{} main({} input) {{\n"
				"{} output;\n"
				"{}\n"
				"return output;\n"
				"}}",
				output_structure_name,
				input_structure_name,
				output_structure_name,
				stage.code);
		}
	}

	auto compile_stage = [&, shared_data](gfx::ShaderStageFlagBits stage)
	{
		gfx::ShaderCompilerInput input;
		input.name = fmt::format("{} (permutation {}, stage {})", 
			shader.get_declaration().name, 
			id.to_ullong(),
			std::to_string(stage));
		input.stage = stage;
		input.target_format = shader.get_shader_manager().get_shader_format();
		input.entry_point = "main";

		std::string code = fmt::format(
			"{}"
			"{}"
			"{}"
			"{}",
			shared_data->inputs[stage],
			shared_data->outputs[stage],
			shared_data->parameters[stage],
			shared_data->main_functions[stage]);

		input.code = { reinterpret_cast<uint8_t*>(code.data()), reinterpret_cast<uint8_t*>(code.data()) + code.size() };

		return compile_shader(input);
	};

	shader_map.clear();

	root_compilation_job = new_job(
		[&, shared_data = std::move(shared_data)](jobsystem::Job&)
		{
			jobsystem::JobGroup group;
			tbb::concurrent_hash_map<gfx::ShaderStageFlagBits, gfx::ShaderCompilerOutput> outputs;

			for (const auto& stage : shader.get_declaration().stages)
			{
				jobsystem::Job* stage_job = new_child_job(
					[&, stage](jobsystem::Job&)
					{
						outputs.insert({ stage.stage, compile_stage(stage.stage) });
					}, root_compilation_job, jobsystem::JobType::Normal, 0.f);
				group.add(stage_job);
			}

			group.schedule_and_wait();

			for (auto [stage, output] : outputs)
			{
				if(output.failed)
				{
					logger::error(log_shadersystem, "Shader compiling error: {}", output.errors[0]);
				}
				else
				{
					auto result = shader.get_shader_manager().get_device().create_shader(
						gfx::ShaderInfo::make({ (uint32_t*)output.bytecode.data(),
							(uint32_t*)output.bytecode.data() + output.bytecode.size() }));

					if (result)
					{
						shader_map[stage] = gfx::UniqueShader(result.get_value());
						if (shader_map.size() == shader.get_declaration().stages.size())
							state = ShaderPermutationState::Available;
					}
					else
					{
						logger::error(log_shadersystem, "Failed to create shader {}:",
							std::to_string(result.get_error()));
					}
				}
			}
		},
		jobsystem::JobType::Normal, 0.f);

	root_compilation_job->schedule();
}

}