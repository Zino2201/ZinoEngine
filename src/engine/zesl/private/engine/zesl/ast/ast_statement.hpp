#pragma once

#include "ast_expression_type.hpp"
#include "attribute.hpp"

namespace ze::zesl
{

class AstStatement
{
public:
	virtual ~AstStatement() = default;
};

class AstStatementMultiple : public AstStatement
{
public:
	AstStatementMultiple() = default;
	AstStatementMultiple(std::vector<std::unique_ptr<AstStatement>>&& in_statements) : statements(std::move(in_statements)) {}

	std::vector<std::unique_ptr<AstStatement>> statements;
};

struct StructDeclaration
{
	struct Member
	{
		std::string name;
		AstExpressionType type;
		std::vector<Attribute> attributes;
	};

	std::string name;
	std::vector<Member> members;
};

class AstStatementStruct : public AstStatement
{
public:
	AstStatementStruct(StructDeclaration&& in_struct_declaration) : struct_declaration(std::move(in_struct_declaration)) {}
private:
	StructDeclaration struct_declaration;
};

struct FunctionDeclaration
{
	struct Parameter
	{
		std::string name;
		AstExpressionType type;
	};

	std::optional<gfx::ShaderStageFlagBits> stage;
	std::string name;
	std::vector<Parameter> parameters;
	std::vector<std::unique_ptr<AstStatement>> statements;
	AstExpressionType return_type;
};

class AstStatementFunction : public AstStatement
{
public:
	AstStatementFunction(FunctionDeclaration&& in_function_declaration) : function_declaration(std::move(in_function_declaration)) {}
private:
	FunctionDeclaration function_declaration;
};

struct VariableDeclaration
{
	std::string name;
	std::unique_ptr<AstExpression> default_expression;
	AstExpressionType type;
};

class AstStatementVariable : public AstStatement
{
public:
	AstStatementVariable(VariableDeclaration&& in_variable_declaration) : variable_declaration(std::move(in_variable_declaration)) {}
private:
	VariableDeclaration variable_declaration;
};

class AstStatementExpression : public AstStatement
{
public:
	AstStatementExpression(std::unique_ptr<AstExpression>&& in_expression) : expression(std::move(in_expression)) {}
private:
	std::unique_ptr<AstExpression> expression;
};

}