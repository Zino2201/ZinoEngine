#pragma once

#include "engine/gfx/pipeline.hpp"

namespace ze::zesl
{

class Parser;

class Shader
{
public:
	Shader(std::unique_ptr<std::streambuf>&& in_buf);
	~Shader();

	[[nodiscard]] std::vector<std::pair<gfx::ShaderStageFlagBits, std::string>> get_entry_points() const;
	[[nodiscard]] std::string to_hlsl(const gfx::ShaderStageFlagBits in_stage) const;
private:
	std::unique_ptr<Parser> parser;
};

}