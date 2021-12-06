#pragma once

namespace ze::gfx
{

/**
 * A shader model is a set of GPU features
 * This enum uses the D3D shader model format as it is the most used
 */
enum class ShaderModel
{
	/** Tessellation, wave ops ... */
	SM6_0,

	/** Ray tracing, mesh/amplifications shaders */
	SM6_5,
};

enum class ShaderLanguage
{
	VK_SPIRV,
	DXIL,
	MSL
};

struct ShaderFormat
{
	ShaderModel model;
	ShaderLanguage language;

	ShaderFormat() = default;
	ShaderFormat(ShaderModel in_model, ShaderLanguage in_language) : model(in_model), language(in_language) {}
};
	
}

namespace std
{

inline std::string to_string(const ze::gfx::ShaderModel& in_shader_model)
{
	switch(in_shader_model)
	{
	case ze::gfx::ShaderModel::SM6_0:
		return "SM6_0";
	case ze::gfx::ShaderModel::SM6_5:
		return "SM6_5";
	}
	
	return "";
}

inline std::string to_string(const ze::gfx::ShaderLanguage& in_language)
{
	switch(in_language)
	{
	case ze::gfx::ShaderLanguage::VK_SPIRV:
		return "VK_SPIRV";
	case ze::gfx::ShaderLanguage::DXIL:
		return "DXIL";
	case ze::gfx::ShaderLanguage::MSL:
		return "MSL";
	}

	return "";
}

}