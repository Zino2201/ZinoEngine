#pragma once

#include "engine/gfx/sampler.hpp"

namespace ze::gfx
{

class VulkanSampler final
{
public:
	VulkanSampler(VulkanDevice& in_device,
		VkSampler in_sampler,
		VulkanDescriptorManager::DescriptorIndexHandle in_srv_index)
		: device(in_device), sampler(in_sampler), srv_index(in_srv_index) {}

	~VulkanSampler()
	{
		if (srv_index)
			device.get_descriptor_manager().free_index(srv_index);

		vkDestroySampler(device.get_device(), sampler, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkSampler get_sampler() const { return sampler; }
	uint32_t get_srv_index() const { return srv_index.index; }
private:
	VulkanDevice& device;
	VkSampler sampler;
	VulkanDescriptorManager::DescriptorIndexHandle srv_index;
};

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