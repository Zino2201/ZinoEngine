#pragma once

#include "vulkan.hpp"

namespace ze::gfx
{

class VulkanCommandList final
{
public:
	VulkanCommandList(VulkanDevice& in_device, 
		VkCommandBuffer in_buffer) : device(in_device), buffer(in_buffer) {}

	VulkanCommandList(VulkanCommandList&& in_other) noexcept = delete;

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkCommandBuffer get_command_buffer() const { return buffer; }
private:
	VulkanDevice& device;
	VkCommandBuffer buffer;
};

}