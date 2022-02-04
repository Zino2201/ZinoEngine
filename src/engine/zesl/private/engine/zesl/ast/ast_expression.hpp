#pragma once

#include "engine/core.hpp"
#include "engine/gfx/pipeline.hpp"

namespace ze::zesl
{

enum class BinaryType
{
	Add,
	Sub,
	Mul,
	Div,
	Mod,
	Equal,
	NotEqual,
	Or,
	And,
	LessThan,
	LessThanEq,
	GreaterThan,
	GreaterThanEq,
};

enum class ExpressionType
{
	Binary,
	Variable,
	CallFunction,
	CallMethod,
	FunctionDeclaration,
};

class AstExpression
{
public:
	virtual ~AstExpression() = default;
};

class AstExpressionBinary : public AstExpression
{
public:
	AstExpressionBinary(const BinaryType in_type,
		std::unique_ptr<AstExpression>&& in_left,
		std::unique_ptr<AstExpression>&& in_right) : type(in_type),
	left(std::move(in_left)), right(std::move(in_right)) {}

	ExpressionType get_type() const { return ExpressionType::Binary; }
private:
	BinaryType type;
	std::unique_ptr<AstExpression> left;
	std::unique_ptr<AstExpression> right;
};

class AstExpressionVariable : public AstExpression
{
public:
	AstExpressionVariable(uint64_t in_id) : id(in_id) {}

	ExpressionType get_type() const { return ExpressionType::Variable; }
private:
	uint64_t id;
};

class AstExpressionCallFunction : public AstExpression
{
public:
	ExpressionType get_type() const { return ExpressionType::CallFunction; }
private:
	uint64_t function;
	std::vector<std::unique_ptr<AstExpression>> parameters;
};

class AstExpressionCallMethod : public AstExpression
{
public:
	ExpressionType get_type() const { return ExpressionType::CallMethod; }
private:
	std::unique_ptr<AstExpression> object;
	std::string method;
	std::vector<std::unique_ptr<AstExpression>> parameters;
};

class AstExpressionConstantValue : public AstExpression
{
public:
	using Types = std::variant<bool, uint64_t, double>;

	AstExpressionConstantValue(Types in_value) : value(in_value) {}
private:
	Types value;
};

class AstExpressionFunctionDeclaration : public AstExpression
{
public:
	ExpressionType get_type() const { return ExpressionType::FunctionDeclaration; }
private:
	std::string name;
	gfx::ShaderStageFlagBits stage;
	std::unique_ptr<AstExpression> body;
};

class AstExpressionIdentifier : public AstExpression
{
public:
	AstExpressionIdentifier(const std::string& in_name) : name(in_name) {}
private:
	std::string name;
};

class AstExpressionAccessIdentifier : public AstExpression
{
public:
	AstExpressionAccessIdentifier(std::unique_ptr<AstExpression>&& in_expression,
		const std::vector<std::string>& in_identifiers) : expression(std::move(in_expression)), identifiers(in_identifiers) {}
private:
	std::unique_ptr<AstExpression> expression;
	std::vector<std::string> identifiers;
};

enum class AssignType
{
	Normal
};

class AstExpressionAssign : public AstExpression
{
public:
	AstExpressionAssign(std::unique_ptr<AstExpression>&& in_left,
		const AssignType in_type,
		std::unique_ptr<AstExpression>&& in_right) : left(std::move(in_left)),
		type(std::move(in_type)), right(std::move(in_right)) {}
private:
	std::unique_ptr<AstExpression> left;
	AssignType type;
	std::unique_ptr<AstExpression> right;
};

}