#include "zeshader_compiler.hpp"
#include "engine/shadersystem/shader_declaration.hpp"
#include <stack>

namespace ze::shadersystem
{

bool skip_until(std::istream& stream, char c)
{
	char current_char = static_cast<char>(stream.get());
	while (current_char != c)
	{
		current_char = static_cast<char>(stream.get());
		if (stream.eof())
			return false;
	}

	return true;
}

Result<ShaderDeclaration, std::string> compile_zeshader(std::unique_ptr<std::streambuf>&& in_stream)
{
	ShaderDeclaration declaration;

	std::istream stream(in_stream.get());
	std::string* current_hlsl_stage = &declaration.common_hlsl;
	ShaderPass* current_pass = &declaration.passes.emplace_back();

	enum class BlockType { HLSL, ZESHADER_SHADER, ZESHADER_STAGE, ZESHADER_PASS, ZESHADER_PARAMETERS };
	std::stack<BlockType> blocks;

	while(true)
	{
		char c = static_cast<char>(stream.get());
		if (stream.eof())
			break;

		if (std::isalpha(c))
		{
			std::string word;
			do
			{
				word.push_back(c);
				c = static_cast<char>(stream.get());
			} while (std::isalnum(c));

			if (!blocks.empty() && blocks.top() == BlockType::ZESHADER_PARAMETERS)
			{
				std::string name;
				do
				{
					if(!std::isspace(c))
						name.push_back(c);
					c = static_cast<char>(stream.get());
				} while (std::isalnum(c) || c == '_');

				if(c != ';' && !skip_until(stream, ';'))
					return make_error(std::string("Parameter must finish with a semi-colon."));

				ShaderParameterType parameter_type;
				if (word == "uint")
					parameter_type = ShaderParameterType::Uint;
				else if (word == "uint64_t")
					parameter_type = ShaderParameterType::Uint64;
				else if (word == "float")
					parameter_type = ShaderParameterType::Float;
				else if (word == "float2")
					parameter_type = ShaderParameterType::Float2;
				else if (word == "float3")
					parameter_type = ShaderParameterType::Float3;
				else if (word == "float4")
					parameter_type = ShaderParameterType::Float4;
				else if (word == "float4x4")
					parameter_type = ShaderParameterType::Float4x4;
				else if (word == "Texture2D")
					parameter_type = ShaderParameterType::Texture2D;
				else if (word == "ByteAddressBuffer")
					parameter_type = ShaderParameterType::ByteAddressBuffer;
				else if (word == "RWByteAddressBuffer")
					parameter_type = ShaderParameterType::RWByteAddressBuffer;
					

				declaration.parameters.emplace_back(parameter_type, name);
			}
			else
			{
				if (word == "shader" &&
					declaration.name.empty())
				{
					if (!skip_until(stream, '"'))
						return make_error(std::string("Can't properly parse shader name. Excepted syntax: 'shader \"Name\"'"));

					while (true)
					{
						c = static_cast<char>(stream.get());

						if (c == '"')
							break;

						if (c == '\n')
							return make_error(std::string("Can't properly parse shader name. Excepted syntax: 'shader \"Name\"'"));

						declaration.name.push_back(c);
					}

					if (!skip_until(stream, '{'))
						return make_error(std::string("Shader block never opened."));

					blocks.push(BlockType::ZESHADER_SHADER);
				}
				else if (word == "vertex")
				{
					if (!skip_until(stream, '{'))
						return make_error(std::string("Vertex block never opened."));

					for (const auto& stage : current_pass->stages)
						if (stage.stage == gfx::ShaderStageFlagBits::Compute)
							return make_error(std::string("Only one compute block and no vertex/fragment block must be present per pass."));

					blocks.push(BlockType::ZESHADER_STAGE);
					auto& stage = current_pass->stages.emplace_back(gfx::ShaderStageFlagBits::Vertex);
					current_hlsl_stage = &stage.hlsl;
				}
				else if (word == "fragment")
				{
					if (!skip_until(stream, '{'))
						return make_error(std::string("Fragment block never opened."));

					for (const auto& stage : current_pass->stages)
						if (stage.stage == gfx::ShaderStageFlagBits::Compute)
							return make_error(std::string("Only one compute block and no vertex/fragment block must be present per pass."));


					blocks.push(BlockType::ZESHADER_STAGE);
					auto& stage = current_pass->stages.emplace_back(gfx::ShaderStageFlagBits::Fragment);
					current_hlsl_stage = &stage.hlsl;
				}
				else if (word == "compute")
				{
					if (!skip_until(stream, '{'))
						return make_error(std::string("Compute block never opened."));

					if (!current_pass->stages.empty())
						return make_error(std::string("Only one compute block and no vertex/fragment block must be present per pass."));

					blocks.push(BlockType::ZESHADER_STAGE);
					auto& stage = current_pass->stages.emplace_back(gfx::ShaderStageFlagBits::Compute);
					current_hlsl_stage = &stage.hlsl;
					current_pass->is_compute_pass = true;
				}
				else if (word == "pass")
				{
					if (!skip_until(stream, '"'))
						return make_error(std::string("Invalid pass syntax. 'pass \"Name\"'"));

					current_pass = &declaration.passes.emplace_back();
					current_hlsl_stage = &current_pass->common_hlsl;

					while (true)
					{
						c = static_cast<char>(stream.get());

						if (c == '"')
							break;
						else if (c == '\n')
							return make_error(std::string("Can't properly parse pass name. Excepted syntax: 'pass \"Name\"'"));

						current_pass->name.push_back(c);
					}

					if (!skip_until(stream, '{'))
						return make_error(std::string("Pass block never opened."));

					blocks.push(BlockType::ZESHADER_PASS);
				}
				else if (word == "parameters")
				{
					if (!skip_until(stream, '{'))
						return make_error(std::string("Parameters block never opened."));

					blocks.push(BlockType::ZESHADER_PARAMETERS);
				}
				else
				{
					current_hlsl_stage->append(word);
					current_hlsl_stage->push_back(c);
				}
			}
		}
		else if(c == '{')
		{
			blocks.push(BlockType::HLSL);
			current_hlsl_stage->push_back(c);
		}
		else if(c == '}')
		{
			if (blocks.top() == BlockType::ZESHADER_PASS)
			{
				current_hlsl_stage = &declaration.common_hlsl;
				current_pass = &declaration.passes.front();
			}
			else if(blocks.top() == BlockType::ZESHADER_STAGE)
			{
				current_hlsl_stage = &declaration.passes.back().common_hlsl;
			}
			else if(blocks.top() != BlockType::ZESHADER_PARAMETERS && blocks.top() != BlockType::ZESHADER_SHADER)
			{
				current_hlsl_stage->push_back(c);
			}

			blocks.pop();
		}
		else
		{
			current_hlsl_stage->push_back(c);
		}
	}

	return declaration;
}

}
