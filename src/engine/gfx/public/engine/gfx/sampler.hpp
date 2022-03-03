#pragma once

namespace ze::gfx
{

enum class Filter
{
	Nearest,
	Linear
};

enum class SamplerAddressMode
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder
};

enum class SamplerBorderColor
{
	WhiteIntegerOpaque,
	WhiteFloatOpaque,
};

struct SamplerCreateInfo
{
	Filter min_filter;
	Filter mag_filter;
	Filter mip_map_mode;
	SamplerAddressMode address_mode_u;
	SamplerAddressMode address_mode_v;
	SamplerAddressMode address_mode_w;
	float mip_lod_bias;
	CompareOp compare_op;
	bool enable_anisotropy;
	float max_anisotropy;
	float min_lod;
	float max_lod;
	SamplerBorderColor border_color;

	SamplerCreateInfo(
		const Filter& in_min_filter = Filter::Linear,
		const Filter& in_mag_filter = Filter::Linear,
		const Filter& in_mip_map_mode = Filter::Linear,
		const SamplerAddressMode& in_address_mode_u = SamplerAddressMode::Repeat,
		const SamplerAddressMode& in_address_mode_v = SamplerAddressMode::Repeat,
		const SamplerAddressMode& in_address_mode_w = SamplerAddressMode::Repeat,
		const float in_mip_lod_bias = 0.f,
		const CompareOp& in_compare_op = CompareOp::Never,
		const bool in_enable_aniostropy = false,
		const float in_max_anisotropy = 0.f,
		const float in_min_lod = 0.f,
		const float in_max_lod = std::numeric_limits<float>::max(),
		const SamplerBorderColor in_border_color = SamplerBorderColor::WhiteIntegerOpaque) :
		min_filter(in_min_filter), mag_filter(in_mag_filter),
		mip_map_mode(in_mip_map_mode), address_mode_u(in_address_mode_u),
		address_mode_v(in_address_mode_v), address_mode_w(in_address_mode_w),
		mip_lod_bias(in_mip_lod_bias), compare_op(in_compare_op),
		enable_anisotropy(in_enable_aniostropy), max_anisotropy(in_max_anisotropy),
		min_lod(in_min_lod), max_lod(in_max_lod), border_color(in_border_color) {}
};

}