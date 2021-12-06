#pragma once

#include "engine/gfx/sampler.hpp"

namespace ze::gfx
{

inline VkFilter convert_filter(Filter in_filter)
{
	switch(in_filter)
	{
	case Filter::Linear:
		return VK_FILTER_LINEAR;
	case Filter::Nearest:
		return VK_FILTER_NEAREST;
	}

	ZE_UNREACHABLE();
}

inline VkSamplerMipmapMode convert_filter_mipmap_mode(Filter in_filter)
{
	switch(in_filter)
	{
	case Filter::Linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case Filter::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	ZE_UNREACHABLE();
}

inline VkSamplerAddressMode convert_address_mode(SamplerAddressMode in_address_mode)
{
	switch(in_address_mode)
	{
	case SamplerAddressMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case SamplerAddressMode::MirroredRepeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case SamplerAddressMode::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case SamplerAddressMode::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}

	ZE_UNREACHABLE();
}

}