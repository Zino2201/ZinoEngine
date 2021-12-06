#pragma once

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
		const VmaAllocationInfo& in_alloc_info);
	~VulkanBuffer();

	[[nodiscard]] VkBuffer get_buffer() const { return buffer; }
	[[nodiscard]] VmaAllocation get_allocation() const { return allocation; }
private:
	VulkanDevice& device;
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo alloc_info;
};
	
}