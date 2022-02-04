#pragma once

#include "ShaderAST/Expr/Expr.hpp"

namespace ze::zesl
{

struct Attribute
{
	std::string name;
	std::string args;

	Attribute(const std::string& in_name,
		const std::string& in_args) : name(in_name), args(in_args) {}
};

}