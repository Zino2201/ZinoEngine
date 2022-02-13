#pragma once

#include "vulkan_descriptor_manager.hpp"
#include "engine/gfx/vulkan.hpp"
#include "engine/gfx/buffer.hpp"

namespace ze::gfx
{

class VulkanDevice;

class VulkanBuffer final
{
public:
	VulkanBuffer(VulkanDevice& in_device,
		const VkBuffer& in_buffer,
		const VmaAllocation& in_allocation,
		const VmaAllocationInfo& in_alloc_info,
		VulkanDescriptorManager::DescriptorIndexHandle in_srv_index,
		VulkanDescriptorManager::DescriptorIndexHandle in_uav_index);
	~VulkanBuffer();

	[[nodiscard]] VkBuffer get_buffer() const { return buffer; }
	[[nodiscard]] VmaAllocation get_allocation() const { return allocation; }
	uint32_t get_srv_index() const { return srv_index.index; }
	uint32_t get_uav_index() const { return uav_index.index; }
private:
	VulkanDevice& device;
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo alloc_info;
	VulkanDescriptorManager::DescriptorIndexHandle srv_index;
	VulkanDescriptorManager::DescriptorIndexHandle uav_index;
};
	
}