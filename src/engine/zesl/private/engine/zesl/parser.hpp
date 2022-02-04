#pragma once

#include "ast/attribute.hpp"
#include "ShaderAST/Shader.hpp"
#include "ShaderAST/Expr/ExprList.hpp"
#include "ShaderAST/Stmt/StmtStructureDecl.hpp"
#include "ShaderAST/Stmt/StmtFunctionDecl.hpp"
#include "engine/zesl/parameter.hpp"

namespace ze::zesl
{

class Parser
{
public:
	Parser(const std::vector<Token>& in_tokens);

	auto& get_root_statement_container() { return root_container; }
	const auto& get_entry_point_map() const { return entry_point_map; }
private:
	Token pop();
	Token peek();
	void backward();
	ast::stmt::StructureDeclPtr parse_struct(std::vector<Attribute>&& in_attributes);
	ast::stmt::FunctionDeclPtr parse_function(std::vector<Attribute>&& in_attributes);
	ast::stmt::ContainerPtr parse_scope();
	ast::stmt::StmtPtr parse_statement();
	ast::stmt::StmtPtr parse_variable_declaration(std::vector<Attribute>&& in_attributes);
	ast::stmt::StmtPtr parse_variable_assignation();
	ast::expr::ExprList parse_parameters();
	ast::expr::ExprPtr parse_expression();
	ast::expr::ExprPtr parse_primary_expression();
	ast::expr::ExprPtr parse_binary_rhs_operation(int32_t in_expression_precedence,
		ast::expr::ExprPtr left);
	std::string parse_identifier();
	std::string parse_identifier_as_name();
	std::string parse_identifier_or_constant_as_string();
	std::vector<Attribute> parse_attributes();
	ast::type::TypePtr parse_type(const Token* in_identifier = nullptr);
	ast::var::VariablePtr register_variable(const std::string& in_name, ast::type::TypePtr in_type);
	void advance(TokenType in_type);
private:
	std::vector<Token> tokens;
	size_t idx;
	ast::type::TypesCache types_cache;
	ast::stmt::ContainerPtr root_container;
	robin_hood::unordered_map<std::string, ast::type::StructPtr> struct_map;
	robin_hood::unordered_map<std::string, ast::var::VariablePtr> variable_map;
	robin_hood::unordered_map<std::string, ast::var::VariablePtr> constant_buffers_map;
	robin_hood::unordered_map<ast::ShaderStage, std::pair<std::string, ast::type::FunctionPtr>> entry_point_map;
	uint32_t variable_id;
};

}