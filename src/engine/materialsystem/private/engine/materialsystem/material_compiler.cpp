#include "engine/materialsystem/material_compiler.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze
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

Result<std::unique_ptr<Material>, std::string> compile_zematerial(shadersystem::ShaderManager& in_manager,
	std::unique_ptr<std::streambuf>&& in_stream)
{
	std::istream stream(in_stream.get());

	std::string shader_name;

	while(true)
	{
		char c = static_cast<char>(stream.get());
		if (stream.eof())
			break;

		if(std::isalpha(c))
		{
			std::string word;
			do
			{
				word.push_back(c);
				c = static_cast<char>(stream.get());
			} while (std::isalpha(c));

			if(word == "shader")
			{
				if (!skip_until(stream, '"'))
					return make_error(std::string("Can't properly parse shader name. Excepted syntax: 'shader \"Name\"'"));

				while(true)
				{
					c = static_cast<char>(stream.get());
					if (c == '"')
						break;

					if (c == '"')
						break;
					else if (c == '\n')
						return make_error(std::string("Can't properly parse shader name. Excepted syntax: 'shader \"Name\"'"));

					shader_name.push_back(c);
				}
			}
		}
	}

	if(shadersystem::Shader* shader = in_manager.get_shader(shader_name))
	{
		return make_result(std::make_unique<Material>(shader, shadersystem::ShaderPermutationId{}));
	}

	return make_error(std::string("Shader not found."));
}

}