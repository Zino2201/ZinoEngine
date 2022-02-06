#pragma once

#include "engine/core.hpp"
#include "engine/flags.hpp"
#include "memory.hpp"
#include "format.hpp"
#include "device_resource.hpp"

namespace ze::gfx
{

enum class TextureAspectFlagBits
{
	Color = 1 << 0,
	Depth = 1 << 1,
	Stencil = 1 << 2
};
ZE_ENABLE_FLAG_ENUMS(TextureAspectFlagBits, TextureAspectFlags);
	
enum class TextureType
{
	Tex1D,
	Tex2D,
	Tex3D,
};

enum class TextureViewType
{
	Tex1D,
	Tex2D,
	Tex3D,
	TexCube,
};

enum class TextureLayout
{
	Undefined,
	General,
	ColorAttachment,
	DepthStencilAttachment,
	DepthReadOnly,
	ShaderReadOnly,
	TransferSrc,
	TransferDst,
	Present,
};
	
enum class TextureUsageFlagBits
{
	ColorAttachment = 1 << 0,
	DepthStencilAttachment = 1 << 1,
	Sampled = 1 << 2,
	TransferSrc = 1 << 3,
	TransferDst = 1 << 4,
	Cube = 1 << 5
};
ZE_ENABLE_FLAG_ENUMS(TextureUsageFlagBits, TextureUsageFlags);
	
enum class SampleCountFlagBits
{
	Count1 = 1 << 0,
	Count2 = 1 << 1,
	Count4 = 1 << 2,
	Count8 = 1 << 3,
	Count16 = 1 << 4,
	Count32 = 1 << 5,
	Count64 = 1 << 6,
};
ZE_ENABLE_FLAG_ENUMS(SampleCountFlagBits, SampleCountFlags);
	
/**
 * Subresource range of a texture
 */
struct TextureSubresourceRange
{
	TextureAspectFlags aspect_flags;
	uint32_t base_mip_level;
	uint32_t level_count;
	uint32_t base_array_layer;
	uint32_t layer_count;

	TextureSubresourceRange() : base_mip_level(0), level_count(0),
		base_array_layer(0), layer_count(0) {}

	TextureSubresourceRange(TextureAspectFlags in_aspect_flags,
		uint32_t in_base_mip_level,
		uint32_t in_level_count,
		uint32_t in_base_array_layer,
		uint32_t in_layer_count) : aspect_flags(in_aspect_flags),
			base_mip_level(in_base_mip_level),
			level_count(in_level_count),
			base_array_layer(in_base_array_layer),
			layer_count(in_layer_count) {}
};

/**
 * Specify a subresource layers
 */
struct TextureSubresourceLayers
{
	TextureAspectFlags aspect_flags;
	uint32_t mip_level;
	uint32_t base_array_layer;
	uint32_t layer_count;

	TextureSubresourceLayers(TextureAspectFlags in_aspect_flags,
		uint32_t in_mip_level,
		uint32_t in_base_array_layer,
		uint32_t in_layer_count) : aspect_flags(in_aspect_flags),
		mip_level(in_mip_level),
		base_array_layer(in_base_array_layer),
		layer_count(in_layer_count) {}
};
	
struct TextureCreateInfo
{
	TextureType type;
	MemoryUsage mem_usage;
	Format format;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mip_levels;
	uint32_t array_layers;
	SampleCountFlagBits sample_count;
	TextureUsageFlags usage_flags;
	
	explicit TextureCreateInfo(TextureType in_type = TextureType::Tex1D,
		MemoryUsage in_mem_usage = MemoryUsage::CpuOnly,
		Format in_format = Format::Undefined,
		uint32_t in_width = 0,
		uint32_t in_height = 0,
		uint32_t in_depth = 0,
		uint32_t in_mip_levels = 0,
		uint32_t in_array_layers = 0,
		SampleCountFlagBits in_sample_count = SampleCountFlagBits::Count1,
		TextureUsageFlags in_usage_flags = TextureUsageFlags()) : type(in_type), mem_usage(in_mem_usage),
		format(in_format), width(in_width), height(in_height), depth(in_depth),
		mip_levels(in_mip_levels), array_layers(in_array_layers), sample_count(in_sample_count),
		usage_flags(in_usage_flags) {}
};

struct TextureViewCreateInfo
{
	BackendDeviceResource texture;
	TextureViewType type;
	Format format;
	TextureSubresourceRange subresource_range;

	TextureViewCreateInfo() : type(TextureViewType::Tex1D),
		format(Format::Undefined), texture(null_backend_resource) {}

	TextureViewCreateInfo(const BackendDeviceResource& in_texture,
		TextureViewType in_type, Format in_format, const TextureSubresourceRange& in_subresource_range) :
		texture(in_texture), type(in_type), format(in_format), subresource_range(in_subresource_range) {}
};

inline TextureAspectFlags format_to_aspect_flags(Format in_format)
{
	switch(in_format)
	{
	case Format::Undefined:
		return TextureAspectFlags();
	case Format::D24UnormS8Uint:
	case Format::D32SfloatS8Uint:
		return TextureAspectFlags(TextureAspectFlagBits::Depth | TextureAspectFlagBits::Stencil);
	case Format::D32Sfloat:
		return TextureAspectFlags(TextureAspectFlagBits::Depth);
	default:
		return TextureAspectFlags(TextureAspectFlagBits::Color);
	}
}
	
}