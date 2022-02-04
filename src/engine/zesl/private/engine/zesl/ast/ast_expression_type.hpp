#pragma once

#include "engine/gfx/texture.hpp"

namespace ze::zesl
{

enum class PrimitiveType
{
	Bool,
	Float,
	Int32,
	Uint32,
	Vector2,
	Vector3,
	Vector4,
};

struct AstExpressionNoType {};

struct AstExpressionIdentifierType
{
	std::string name;
};

struct AstExpressionPrimitiveType
{
	PrimitiveType primitive_type;
};

struct AstExpressionVectorType
{
	PrimitiveType primitive_type;
	size_t component_count;

	AstExpressionVectorType() : component_count(0) {}
};

struct AstExpressionTextureType
{
	gfx::TextureType texture_type;
};

struct AstExpressionSamplerType
{
	PrimitiveType sampled_type;
};

struct AstExpressionStructType
{
	uint64_t id;
};

struct AstExpressionUniformBufferType
{
	AstExpressionIdentifierType structure;
};

using AstExpressionType = std::variant<AstExpressionNoType,
	AstExpressionIdentifierType,
	AstExpressionPrimitiveType,
	AstExpressionTextureType,
	AstExpressionSamplerType,
	AstExpressionVectorType,
	AstExpressionStructType,
	AstExpressionUniformBufferType>;

}