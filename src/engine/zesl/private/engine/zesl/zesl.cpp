#include "engine/zesl/zesl.hpp"
#include "parser.hpp"

namespace ze::zesl
{

Shader::Shader(std::unique_ptr<std::streambuf>&& in_buf)
{
	PushBackStream stream(in_buf.release());
	auto token = tokenize(stream);

	std::vector<Token> tokens;
	tokens.emplace_back(token);
	while (token.get_type() != TokenType::Eof)
	{
		token = tokenize(stream);
		if (token.get_type() != TokenType::None)
			tokens.emplace_back(token);
	}

	parser = std::make_unique<Parser>(tokens);
}

Shader::~Shader() = default;

std::string Shader::to_hlsl(const gfx::ShaderStageFlagBits in_stage) const
{
	ast::ShaderStage ast_stage = ast::ShaderStage::eVertex;
	if (in_stage == gfx::ShaderStageFlagBits::Fragment)
		ast_stage = ast::ShaderStage::eFragment;

	auto container = ast::StmtCloner::submit(parser->get_root_statement_container().get());
	ast::Shader shader(ast_stage);
	shader.addStmt(std::move(container));

	return hlsl::compileHlsl(shader,
		ast::SpecialisationInfo(), 
		hlsl::HlslConfig{ ast_stage, false });
}

std::vector<std::pair<gfx::ShaderStageFlagBits, std::string>> Shader::get_entry_points() const
{
	std::vector<std::pair<gfx::ShaderStageFlagBits, std::string>> entry_points;

	for(const auto [ast_stage, entry_point] : parser->get_entry_point_map())
	{
		gfx::ShaderStageFlagBits stage;

		switch(ast_stage)
		{
		case ast::ShaderStage::eVertex:
			stage = gfx::ShaderStageFlagBits::Vertex;
			break;
		case ast::ShaderStage::eFragment:
			stage = gfx::ShaderStageFlagBits::Fragment;
			break;
		case ast::ShaderStage::eCompute:
			stage = gfx::ShaderStageFlagBits::Compute;
			break;
		}

		ast::type::FunctionPtr p;
		entry_points.emplace_back(stage, entry_point.first);
	}

	return entry_points;
}

}