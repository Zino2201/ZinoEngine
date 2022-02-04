#include "engine/zesl/parser.hpp"
#include <charconv>
#include "ShaderAST/Type/TypeStruct.hpp"
#include "ShaderAST/Var/Variable.hpp"
#include "ShaderAST/Expr/ExprLiteral.hpp"
#include "ShaderAST/Expr/ExprIdentifier.hpp"
#include "ShaderAST/Expr/ExprBinary.hpp"
#include "ShaderAST/Stmt/StmtSimple.hpp"
#include "ShaderAST/Stmt/StmtContainer.hpp"
#include "ShaderAST/Debug/DebugStmtVisitor.hpp"
#include "CompilerHlsl/compileHlsl.hpp"

namespace ze::zesl
{

const robin_hood::unordered_map<std::string_view, ast::type::Kind> primitive_type_map
{
	{ "bool", ast::type::Kind::eBoolean },
	{ "half", ast::type::Kind::eHalf },
	{ "float", ast::type::Kind::eFloat },
	{ "double", ast::type::Kind::eDouble },
	{ "uint32", ast::type::Kind::eUInt },
	{ "int32", ast::type::Kind::eInt },
};

const robin_hood::unordered_map<TokenType, int32_t> token_precedence_map
{
	{ TokenType::Add, 10 },
	{ TokenType::Sub, 10 },
	{ TokenType::Mul, 20 },
	{ TokenType::Div, 20 },
	{ TokenType::Dot, 100 },
	{ TokenType::Equal, 5 },
	{ TokenType::NotEqual, 5 },
	{ TokenType::LessThan, 5 },
	{ TokenType::LessThanEq, 5 },
	{ TokenType::GreaterThan, 5 },
	{ TokenType::GreaterThanEq, 5 },
	{ TokenType::Or, 150 },
	{ TokenType::And, 150 },
};

int32_t get_token_precedence(const Token& token)
{
	auto it = token_precedence_map.find(token.get_type());
	if (it != token_precedence_map.end())
		return it->second;

	return -1;
}

Parser::Parser(const std::vector<Token>& in_tokens)
	: tokens(in_tokens), idx(0), variable_id(0)
{
	root_container = ast::stmt::makeContainer();

	while (true)
	{
		auto token = pop();
		if (token.get_type() == TokenType::Eof)
			break;

		if (token.get_type() == TokenType::OpenDoubleBracket)
			backward();

		/** Check for any attributes */
		auto attributes = parse_attributes();
		if (!attributes.empty())
		{
			token = pop();
		}

		switch (token.get_type())
		{
		case TokenType::Struct:
			root_container->addStmt(parse_struct(std::move(attributes)));
			break;
		case TokenType::FuncDecl:
			root_container->addStmt(parse_function(std::move(attributes)));
			break;
		case TokenType::VarDecl:
			root_container->addStmt(parse_variable_declaration(std::move(attributes)));
			break;
		}
	}
}

Token Parser::pop()
{
	return tokens[idx++];
}

Token Parser::peek()
{
	return tokens[idx];
}

void Parser::backward()
{
	idx--;
}

ast::stmt::StructureDeclPtr Parser::parse_struct(std::vector<Attribute>&& in_attributes)
{
	const auto name = parse_identifier_as_name();

	ast::var::Flag flag = ast::var::Flag::eNone;
	for(const auto& attribute : in_attributes)
	{
		if(attribute.name == "input")
		{
			flag = ast::var::Flag::eShaderInput;
			break;
		}
		else if (attribute.name == "output")
		{
			flag = ast::var::Flag::eShaderOutput;
			break;
		}
	}

	if(flag != ast::var::Flag::eNone)
	{
		auto struct_type = types_cache.getIOStruct(ast::type::MemoryLayout::eStd430, name, flag);
		struct_map[name] = struct_type;
		advance(TokenType::OpenCurly);
		while (true)
		{
			auto token = peek();
			if (token.get_type() == TokenType::CloseCurly || token.get_type() == TokenType::Eof)
				break;

			ast::Builtin builtin = ast::Builtin::eNone;
			uint32_t location = 0;

			auto attributes = parse_attributes();
			if (!attributes.empty())
			{
				token = peek();

				for (const auto& attribute : attributes)
				{
					if (attribute.name == "builtin")
					{
						if (attribute.args == "position")
							builtin = ast::Builtin::ePosition;
					}
					else if(attribute.name == "location")
					{
						std::from_chars(attribute.args.data(), attribute.args.data() + attribute.args.size(), location);
					}
				}
			}

			if (token.is_identifier())
			{
				const auto name = parse_identifier_as_name();
				advance(TokenType::Colon);
				const auto type = parse_type();

				if (builtin != ast::Builtin::eNone)
					struct_type->declMember(builtin, type->getKind(), ast::type::NotArray);
				else
					struct_type->declMember(name, type, location);
			}
			else
			{
				pop();
			}
		}

		advance(TokenType::CloseCurly);

		return ast::stmt::makeStructureDecl(struct_type);
	}
	else
	{
		auto struct_type = types_cache.getStruct(ast::type::MemoryLayout::eStd430, name);
		struct_map[name] = struct_type;
		advance(TokenType::OpenCurly);
		while (true)
		{
			auto token = peek();
			if (token.get_type() == TokenType::CloseCurly || token.get_type() == TokenType::Eof)
				break;

			ast::Builtin builtin = ast::Builtin::eNone;

			auto attributes = parse_attributes();

			if (token.is_identifier())
			{
				const auto name = parse_identifier_as_name();
				advance(TokenType::Colon);
				const auto type = parse_type();
				struct_type->declMember(name, type);
			}
			else
			{
				pop();
			}
		}

		advance(TokenType::CloseCurly);

		return ast::stmt::makeStructureDecl(struct_type);
	}
}

ast::stmt::FunctionDeclPtr Parser::parse_function(std::vector<Attribute>&& in_attributes)
{
	const auto name = parse_identifier_as_name();
	ast::var::VariableList parameters;

	/** Parse parameters */
	advance(TokenType::OpenParenthesis);
	bool first_parameter = true;
	while(true)
	{
		auto token = peek();
		if (token.get_type() == TokenType::CloseParenthesis)
			break;

		if (!first_parameter)
			advance(TokenType::Comma);
		else
			first_parameter = false;

		const auto parameter_name = parse_identifier_as_name();
		advance(TokenType::Colon);
		const auto parameter_type = parse_type();

		parameters.emplace_back(register_variable(parameter_name, parameter_type));
	}

	advance(TokenType::CloseParenthesis);
	advance(TokenType::Arrow);

	ast::type::TypePtr return_type = parse_type();

	const auto function_type = types_cache.getFunction(return_type, parameters);
	ast::stmt::FunctionFlag flag = ast::stmt::FunctionFlag::eNone;
	for (const auto& attribute : in_attributes)
	{
		if (attribute.name == "entry")
		{
			if (attribute.args == "vertex")
			{
				entry_point_map[ast::ShaderStage::eVertex] = { name, function_type };
			}
			else if(attribute.args == "fragment")
			{
				entry_point_map[ast::ShaderStage::eFragment] = { name, function_type };
			}
		}
	}

	auto function = ast::stmt::makeFunctionDecl(function_type, name);

	function->addStmt(parse_scope());

	return function;
}

ast::stmt::ContainerPtr Parser::parse_scope()
{
	ast::stmt::ContainerPtr statements = ast::stmt::makeContainer();
	advance(TokenType::OpenCurly);

	while(true)
	{
		if (peek().get_type() == TokenType::CloseCurly)
			break;

		statements->addStmt(parse_statement());
	}

	pop();
	return statements;
}

ast::stmt::StmtPtr Parser::parse_statement()
{
	const auto token = peek();
	switch (token.get_type())
	{
	case TokenType::VarDecl:
	{
		pop();
		auto variable_declaration = parse_variable_declaration({});
		advance(TokenType::Semicolon);
		return variable_declaration;
	}
	case TokenType::Return:
	{
		pop();
		auto declaration = ast::stmt::makeReturn(parse_expression());
		advance(TokenType::Semicolon);
		return declaration;
	}
	case TokenType::Identifier:
	{
		auto variable_declaration = parse_variable_assignation();
		advance(TokenType::Semicolon);
		return variable_declaration;
	}
	}

	ZE_UNREACHABLE();

	return nullptr;
}

ast::stmt::StmtPtr Parser::parse_variable_declaration(std::vector<Attribute>&& in_attributes)
{
	const auto name = parse_identifier_as_name();
	ast::type::TypePtr type;

	if (peek().get_type() == TokenType::Colon)
	{
		advance(TokenType::Colon);

		uint32_t dst_binding = 0;

		bool is_parameter = false;
		for (const auto attribute : in_attributes)
		{
			if (attribute.name == "parameter")
			{
				is_parameter = true;
				std::from_chars(&attribute.args.front(), &attribute.args.back() + 1, dst_binding);
				break;
			}
		}

		if(is_parameter && peek().is_identifier())
		{
			auto identifier = parse_identifier();
			if(identifier.find("UniformBuffer") != std::string::npos)
			{
				advance(TokenType::LessThan);
				auto contained_type = parse_type();
				advance(TokenType::GreaterThan);

				auto cbuffer = ast::stmt::makeConstantBufferDecl(
					fmt::format("ConstantBuffer_{}", name),
					ast::type::MemoryLayout::eStd430,
					dst_binding,
					0);
				cbuffer->add(ast::stmt::makeVariableDecl(register_variable(name,
					contained_type)));
				advance(TokenType::Semicolon);
				return cbuffer;
			}

			if (identifier.find("Texture") != std::string::npos)
			{
				int dimension = 0;
				std::from_chars(identifier.data() + (identifier.size() - 2),
					identifier.data() + (identifier.size() - 1),
					dimension);

				advance(TokenType::Semicolon);

				ast::type::ImageConfiguration image_config;
				image_config.accessKind = ast::type::AccessKind::eRead;
				image_config.isSampled = ast::type::Trinary::eTrue;
				image_config.dimension = ast::type::ImageDim::e2D;
				return ast::stmt::makeSampledImageDecl(
					register_variable(name, types_cache.getSampledImage(image_config)),
					dst_binding,
					0);
			}

			if (identifier.find("Sampler") != std::string::npos)
			{
				advance(TokenType::Semicolon);

				return ast::stmt::makeSamplerDecl(
					register_variable(name, types_cache.getSampler()),
					dst_binding,
					0);
			}
		}

		type = parse_type();
	}

	auto container = ast::stmt::makeContainer();

	if (peek().get_type() == TokenType::Assign)
	{
		advance(TokenType::Assign);
		auto expr = parse_expression();

		/** Deduce type from init expression */
		if(!type && expr && expr->isConstant())
		{
			auto* literal = static_cast<ast::expr::Literal*>(expr.get());
			type = literal->getType();
		}

		ZE_CHECK(type);

		return ast::stmt::makeSimple(ast::expr::makeInit(ast::expr::makeIdentifier(types_cache,
			register_variable(name, type)),
			std::move(expr)));
	}
	else
	{
		return ast::stmt::makeVariableDecl(register_variable(name, type));
	}
}

ast::stmt::StmtPtr Parser::parse_variable_assignation()
{
	ast::expr::AssignPtr assign_expr;
	auto left = parse_expression();
	const auto op_token = pop();
	auto right = parse_expression();

	if (op_token.get_type() == TokenType::Assign)
	{
		const auto type = left->getType();
		assign_expr = ast::expr::makeAssign(type,
			std::move(left),
			std::move(right));
	}

	return ast::stmt::makeSimple(std::move(assign_expr));
}

ast::expr::ExprList Parser::parse_parameters()
{
	ast::expr::ExprList list;

	advance(TokenType::OpenParenthesis);
	bool first = true;
	while(peek().get_type() != TokenType::CloseParenthesis)
	{
		if (!first)
			advance(TokenType::Comma);

		first = false;
		list.emplace_back(parse_expression());
	}
	advance(TokenType::CloseParenthesis);

	return list;
}

ast::expr::ExprPtr Parser::parse_expression()
{
	return parse_binary_rhs_operation(0, parse_primary_expression());
}

ast::expr::ExprPtr Parser::parse_primary_expression()
{
	const auto token = pop();
	if(token.is_integer_constant())
	{
		return ast::expr::makeLiteral(types_cache, token.get_integer_constant());
	}
	else if (token.is_floating_point_constant())
	{
		return ast::expr::makeLiteral(types_cache, token.get_floating_point_constant());
	}
	else if(token.is_identifier())
	{
		const auto type = parse_type(&token);
		if(type)
		{
			ast::expr::ExprList parameters = parse_parameters();
			return ast::expr::makeCompositeConstruct(ast::expr::CompositeType::eVec4, 
				ast::type::Kind::eFloat, 
				std::move(parameters));
		}

		return ast::expr::makeIdentifier(types_cache, variable_map[token.get_identifier()]);
	}

	return nullptr;
}

ast::expr::ExprPtr Parser::parse_binary_rhs_operation(int32_t in_expression_precedence, ast::expr::ExprPtr left)
{
	while(true)
	{
		auto token = peek();
		const int32_t token_precedence = get_token_precedence(token);
		if (token_precedence < in_expression_precedence)
			return left;

		/** Parse dot or subscript operator */
		bool has_parsed_dot = false;
		while(token.get_type() == TokenType::Dot ||
			token.get_type() == TokenType::OpenSquareBracket)
		{
			has_parsed_dot = true;

			if(token.get_type() == TokenType::Dot)
			{
				std::vector<std::string> identifiers;

				do
				{
					pop();
					identifiers.emplace_back(parse_identifier_as_name());
				} while (peek().get_type() == TokenType::Dot);

				auto* struct_var = static_cast<ast::expr::Identifier*>(left.get());
				auto* struct_type = static_cast<ast::type::Struct*>(struct_var->getVariable()->getType().get());

				if(!identifiers.empty() && identifiers.front() == "Sample")
				{
					if(peek().get_type() == TokenType::OpenParenthesis)
					{
						auto parameters = parse_parameters();
						parameters.insert(parameters.begin(), std::move(left));
						left = ast::expr::makeTextureAccessCall(types_cache.getVec4F(),
							ast::expr::TextureAccess::eTexture2DF,
							std::move(parameters));
					}
				}
				else
				{
					for (const auto& identifier : identifiers)
					{
						uint32_t idx = struct_type->findMember(identifier);
						if (idx == ast::type::Struct::NotFound)
						{
							if (identifier == "position")
								idx = struct_type->findMember(ast::Builtin::ePosition);
						}

						left = ast::expr::makeMbrSelect(std::move(left), idx, 0);
					}
				}
			}
			else
			{
				// TODO: Array subscript
				ZE_UNREACHABLE();
			}

			token = peek();
		}

		if (has_parsed_dot)
			continue;

		pop();
		auto right = parse_primary_expression();

		const auto next_op = peek();
		const int32_t next_token_precedence = get_token_precedence(next_op);
		if (token_precedence < next_token_precedence)
			right = parse_binary_rhs_operation(token_precedence + 1, std::move(right));

		ast::type::TypePtr type = left->getType();
		switch(token.get_type())
		{
		case TokenType::Add:
			left = ast::expr::makeAdd(type, std::move(left), std::move(right));
			break;
		case TokenType::Sub:
			left = ast::expr::makeMinus(type, std::move(left), std::move(right));
			break;
		case TokenType::Mul:
			left = ast::expr::makeTimes(type, std::move(left), std::move(right));
			break;
		case TokenType::Div:
			left = ast::expr::makeDivide(type, std::move(left), std::move(right));
			break;
		default:
			ZE_UNREACHABLE();
		}
	}
}

std::string Parser::parse_identifier()
{
	const auto token = pop();
	if (!token.is_identifier())
		return "";

	return token.get_identifier();
}

std::string Parser::parse_identifier_as_name()
{
	const auto token = pop();
	if (!token.is_identifier())
		return "";

	return token.get_identifier();
}

std::string Parser::parse_identifier_or_constant_as_string()
{
	const auto token = pop();
	if (token.is_integer_constant())
		return std::to_string(token.get_integer_constant());

	if (token.is_floating_point_constant())
		return std::to_string(token.get_floating_point_constant());

	return token.get_identifier();
}

std::vector<Attribute> Parser::parse_attributes()
{
	std::vector<Attribute> attributes;

	auto token = peek();
	while (token.get_type() == TokenType::OpenDoubleBracket)
	{
		token = pop();

		std::string name = parse_identifier_as_name();
		std::string args;

		if (peek().get_type() == TokenType::OpenParenthesis)
		{
			token = pop();
			args = parse_identifier_or_constant_as_string();
			advance(TokenType::CloseParenthesis);
		}

		advance(TokenType::CloseDoubleBracket);
		attributes.emplace_back(name, args);

		if (peek().get_type() != TokenType::OpenDoubleBracket)
			break;
	}

	return attributes;
}

ast::type::TypePtr Parser::parse_type(const Token* in_identifier)
{
	const auto identifier = in_identifier ? in_identifier->get_identifier() : parse_identifier();

	{
		auto it = primitive_type_map.find(identifier);
		if (it != primitive_type_map.end())
			return types_cache.getBasicType(it->second);
	}


	if (peek().get_type() == TokenType::LessThan)
	{
		pop();

		auto contained_type = parse_type();

		if(identifier.find("Vector") != std::string::npos)
		{
			int length = 0;
			std::from_chars(identifier.data() + (identifier.size() - 1), 
				identifier.data() + (identifier.size()), 
				length);

			advance(TokenType::GreaterThan);
			return types_cache.getVector(contained_type->getKind(), length);
		}

		advance(TokenType::GreaterThan);
	}

	return struct_map[identifier];
}

ast::var::VariablePtr Parser::register_variable(const std::string& in_name, ast::type::TypePtr in_type)
{
	auto variable = ast::var::makeVariable({ ++variable_id, in_name }, in_type);
	variable_map.insert({ in_name, variable });
	return variable;
}

void Parser::advance(TokenType in_type)
{
	const auto token = pop();
	ZE_CHECK(token.get_type() == in_type);
}

}