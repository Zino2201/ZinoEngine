#pragma once

#include <variant>
#include <string>
#include <istream>
#include <stack>

namespace ze::zesl
{

enum class TokenType
{
	None,

	Eof,
	Constant,
	Identifier,

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
	Assign,

	If,
	Else,
	True,
	False,
	FuncDecl,
	VarDecl,
	Struct,
	Return,

	Colon,
	Semicolon,
	Comma,
	Arrow,

	OpenParenthesis,
	CloseParenthesis,
	OpenCurly,
	CloseCurly,
	OpenBracket,
	CloseBracket,
	OpenDoubleBracket,
	CloseDoubleBracket,
	OpenSquareBracket,
	CloseSquareBracket,
	Dot
};

class Token
{
public:
	Token(TokenType in_type) : type(in_type) {}
	Token(const std::string& in_identifier) : type(TokenType::Identifier), value(in_identifier) {}
	Token(const uint64_t in_integer_constant) : type(TokenType::Constant), value(in_integer_constant) {}
	Token(const double in_floating_point_constant) : type(TokenType::Constant), value(in_floating_point_constant) {}

	bool is_identifier() const { return value.index() == 1; }
	bool is_integer_constant() const { return value.index() == 2; }
	bool is_floating_point_constant() const { return value.index() == 3; }

	TokenType get_type() const { return type; }
	std::string get_identifier() const { return std::get<1>(value); }
	uint64_t get_integer_constant() const { return std::get<2>(value); }
	double get_floating_point_constant() const { return std::get<3>(value); }
private:
	TokenType type;
	std::variant<std::monostate, std::string, uint64_t, double> value;
};

class PushBackStream : public std::istream
{
public:
	PushBackStream(std::streambuf* buf) : std::istream(buf) {}
	~PushBackStream() { delete rdbuf(); }

	void push_back(char c);
	char get_character();
private:
	std::stack<char> char_stack;
};

Token tokenize(PushBackStream& stream);

}