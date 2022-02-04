#include "engine/zesl/token.hpp"
#include "engine/core.hpp"
#include <istream>
#include <robin_hood.h>
#include <stack>

namespace ze::zesl
{

void PushBackStream::push_back(char c)
{
	char_stack.push(c);
}

char PushBackStream::get_character()
{
	if (!char_stack.empty())
	{
		char c = char_stack.top();
		char_stack.pop();
		return c;
	}

	return static_cast<char>(get());
}

robin_hood::unordered_map<std::string_view, TokenType> token_map
{
	{ "+", TokenType::Add },
	{ "-", TokenType::Sub },
	{ "*", TokenType::Mul },
	{ "/", TokenType::Div },
	{ "==", TokenType::Equal },
	{ "!=", TokenType::NotEqual },
	{ "<", TokenType::LessThan },
	{ "<=", TokenType::LessThanEq },
	{ ">", TokenType::GreaterThan },
	{ ">=", TokenType::GreaterThanEq },
	{ "||", TokenType::Or },
	{ "&&", TokenType::And },

	// todo: modulo
	// todo: bitwise operators

	{ "=", TokenType::Assign },
	{ "(", TokenType::OpenParenthesis },
	{ ")", TokenType::CloseParenthesis },
	{ "{", TokenType::OpenCurly },
	{ "}", TokenType::CloseCurly },
	{ ",", TokenType::Comma },
	{ ":", TokenType::Colon },
	{ ";", TokenType::Semicolon },
	{ "->", TokenType::Arrow },
	{ "[", TokenType::OpenBracket },
	{ "]", TokenType::CloseBracket },
	{ "[[", TokenType::OpenDoubleBracket },
	{ "]]", TokenType::CloseDoubleBracket },
	{ ".", TokenType::Dot },

	{ "if", TokenType::If },
	{ "else", TokenType::Else },
	{ "true", TokenType::True },
	{ "false", TokenType::False },

	{ "let", TokenType::VarDecl },
	{ "fn", TokenType::FuncDecl },
	{ "struct", TokenType::Struct },
	{ "return", TokenType::Return },
};

enum class CharacterType
{
	Space,
	AlphaNumeric,
	Punctuation
};

CharacterType get_character_type(char c)
{
	if (std::isspace(c))
		return CharacterType::Space;

	if (std::isalpha(c) || std::isdigit(c) || c == '_')
		return CharacterType::AlphaNumeric;

	return CharacterType::Punctuation;
}

std::optional<TokenType> get_token_from_word(std::string_view word)
{
	auto it = token_map.find(word);
	if (it != token_map.end())
		return std::make_optional(it->second);

	return std::nullopt;
}

Token tokenize_word(PushBackStream& stream)
{
	std::string word;

	char c = stream.get_character();

	const bool is_number = std::isdigit(c);

	do
	{
		word.push_back(c);
		c = stream.get_character();

		if(c == '.' && word.back() == '.')
		{
			stream.push_back(word.back());
			word.pop_back();
			break;
		}
	}
	while (get_character_type(c) == CharacterType::AlphaNumeric || (is_number && c == '.'));

	stream.push_back(c);

	if(auto token = get_token_from_word(word))
	{
		return { *token };
	}
	else
	{
		if(std::isdigit(word.front()))
		{
			char* end;
			const uint64_t num64 = strtol(word.c_str(), &end, 0);
			if(*end != 0)
			{
				const double numdbl = strtod(word.c_str(), &end);
				return Token{ numdbl };
			}

			return Token{ num64 };
		}
		else
		{
			return { word };
		}
	}
}

std::optional<TokenType> tokenize_operator(PushBackStream& stream)
{
	std::string op;

	char c = stream.get_character();

	do
	{
		op += c;
		c = stream.get_character();

		if (std::ispunct(c))
		{
			std::string predicted_token = op + c;
			if (!get_token_from_word(predicted_token))
			{
				stream.push_back(c);
				break;
			}
		}
		else
		{
			stream.push_back(c);
		}
	} while (std::ispunct(c));

	if (auto token = get_token_from_word(op))
	{
		return { *token };
	}

	return std::nullopt;
}

bool parse_possible_comment(PushBackStream& stream, char c)
{

	if(c == '/')
	{
		char next = stream.get_character();
		if(next == '*' || next == '/')
		{
			const bool multiline_comment = next == '*';
			while(true)
			{
				c = stream.get_character();
				if (stream.eof() || (c == '\n' && !multiline_comment))
					break;

				if(c == '*' || c == '/')
				{
					next = stream.get_character();
					if(next == '/')
						break;
				}
			}

			return true;
		}
		else
		{
			stream.push_back(next);
			return false;
		}
	}

	return false;
}

Token tokenize(PushBackStream& stream)
{
	while (true)
	{
		char c = stream.get_character();

		if (stream.eof())
			return { TokenType::Eof };

		switch(get_character_type(c))
		{
		case CharacterType::AlphaNumeric:
			stream.push_back(c);
			return tokenize_word(stream);
		case CharacterType::Punctuation:
			if (parse_possible_comment(stream, c))
				return TokenType::None;
			stream.push_back(c);
			if (auto token = tokenize_operator(stream))
				return { token.value() };
		case CharacterType::Space:
			continue;
		}
	}
}

}
