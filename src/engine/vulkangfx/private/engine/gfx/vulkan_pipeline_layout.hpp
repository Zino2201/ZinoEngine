#pragma once

#include "vulkan.hpp"
#include "engine/gfx/pipeline_layout.hpp"
#include "engine/gfx/device.hpp"
#include <queue>
#include <robin_hood.h>

namespace ze::gfx
{

class VulkanDevice;
class VulkanDescriptorSetAllocator;

class VulkanPipelineLayout final
{
	friend class VulkanDevice;

public:
	VulkanPipelineLayout(VulkanDevice& in_device, 
		VkPipelineLayout in_pipeline_layout,
		const std::vector<VkDescriptorSetLayout>& in_set_layouts,
		const uint32_t in_descriptor_type_mask);
	VulkanPipelineLayout(VulkanPipelineLayout&& in_other) noexcept = delete;
	
	~VulkanPipelineLayout();

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkPipelineLayout get_pipeline_layout() const { return pipeline_layout; }
	[[nodiscard]] uint32_t get_descriptor_type_mask() const { return descriptor_type_mask; }
private:
	void allocate_pool();
private:
	VulkanDevice& device;
	VkPipelineLayout pipeline_layout;
	std::vector<VkDescriptorSetLayout> set_layouts;
	uint32_t descriptor_type_mask;
	std::array<size_t, max_descriptor_sets> allocator_indices;
};

inline VkDescriptorType convert_descriptor_type(const DescriptorType& in_type)
{
	switch(in_type)
	{
	case DescriptorType::UniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescriptorType::InputAttachment:
		return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	case DescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	case DescriptorType::StorageTexture:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::SampledTexture:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	default:
		ZE_UNREACHABLE();
	}
}

}