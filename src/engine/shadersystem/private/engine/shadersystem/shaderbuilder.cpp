#include "shaderbuilder.hpp"
#include <sol/sol.hpp>

ZE_DEFINE_LOG_CATEGORY(shaderbuilder);

namespace ze::shadersystem
{

ShaderDeclaration build_lua_shader(const std::string& in_code)
{
	sol::state state;
	state.open_libraries(sol::lib::base);
	state.safe_script_file("assets/shaders/shader_api.lua");

	using namespace shadersystem;

	ShaderDeclaration shader_declaration;
	ShaderStage* current_stage = nullptr;

	/** Setup shaderbuilder table */
	{
		auto table = state["shaderbuilder"].get_or_create<sol::table>();
		table.set_function("new_shader", [&](const std::string& in_name)
		{
			shader_declaration.name = in_name;
		});

		table.set_function("add_parameter", [&](
			std::unordered_map<
				std::string, 
				std::variant<
					std::string, 
					uint32_t, 
					std::vector<std::string>>> in_table)
		{
			const auto& type = std::get<std::string>(in_table["type"]);
			const auto& name = std::get<std::string>(in_table["name"]);

			gfx::ShaderStageFlags stages;

			/** Check for stages override */
			if (in_table.contains("stages"))
			{
				const auto& stages_array = std::get<std::vector<std::string>>(in_table["stages"]);
				for(const auto& stage : stages_array)
				{
					if (stage == "vertex")
						stages |= gfx::ShaderStageFlags(gfx::ShaderStageFlagBits::Vertex);
					else if(stage == "fragment")
						stages |= gfx::ShaderStageFlags(gfx::ShaderStageFlagBits::Fragment);
				}
			}
			else
			{
				stages = gfx::ShaderStageFlags(gfx::ShaderStageFlagBits::Vertex |
					gfx::ShaderStageFlagBits::Fragment);
			}

			shader_declaration.parameters.emplace_back(ShaderParameter::get_type_from_string(type), 
				name,
				stages);
		});


		table.set_function("add_parameter_members", [&](std::vector<std::unordered_map<std::string, std::string>> in_table)
		{
			std::vector<ShaderParameter> members;
			for(auto& member : in_table)
			{
				auto& type = member["type"];
				auto& name = member["name"];
				shader_declaration.parameters.back().members.emplace_back(ShaderParameter::get_type_from_string(type), name);
			}	
		});

		table.set_function("begin_stage", [&](const std::string& in_name)
		{
			current_stage = &shader_declaration.stages.emplace_back();

			if(in_name == "vertex")
			{
				current_stage->stage = gfx::ShaderStageFlagBits::Vertex;
			}
			else if(in_name == "fragment")
			{
				current_stage->stage = gfx::ShaderStageFlagBits::Fragment;
			}
			else
			{
				logger::error(log_shaderbuilder, "Unrecognized stage type {}", in_name);
			}
		});

		table.set_function("add_code", [&](const std::string& in_code)
		{
			current_stage->code = in_code;
		});

		table.set_function("add_input", [&](std::unordered_map<std::string, std::string> in_params)
		{
			const auto& type = in_params["type"];
			const auto& name = in_params["name"];
			current_stage->inputs.emplace_back(ShaderParameter::get_type_from_string(type), name);
		});

		table.set_function("add_output", [&](std::unordered_map<std::string, std::string> in_params)
		{
			const auto& type = in_params["type"];
			const auto& name = in_params["name"];
			current_stage->outputs.emplace_back(ShaderParameter::get_type_from_string(type), name);
		});

		table.set_function("end_stage", [&]()
		{
			current_stage = nullptr;
		});
	}                          

	const auto result = state.safe_script(in_code,"lua shader");
	if (!result.valid())
	{
		const sol::error error = result;
		logger::error(log_shaderbuilder, "Shader building lua error: {}", error.what());
	}

	return shader_declaration;
}

}