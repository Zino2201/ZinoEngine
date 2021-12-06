#pragma once

#include "vulkan.hpp"

namespace ze::gfx
{

class VulkanCommandPool final
{
public:
	VulkanCommandPool(VulkanDevice& in_device, 
		VkCommandPool in_command_pool) : device(in_device), command_pool(in_command_pool) {}

	VulkanCommandPool(VulkanCommandPool&& in_other) noexcept = delete;
	
	~VulkanCommandPool()
	{
		vkDestroyCommandPool(device.get_device(), command_pool, nullptr);	
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkCommandPool get_command_pool() const { return command_pool; }
private:
	VulkanDevice& device;
	VkCommandPool command_pool;
};

inline vkb::QueueType convert_queue_type(const QueueType& in_type)
{
	switch(in_type)
	{
	default:
	case QueueType::Gfx:
		return vkb::QueueType::graphics;
	case QueueType::Compute:
		return vkb::QueueType::compute;
	case QueueType::Transfer:
		return vkb::QueueType::transfer;
	case QueueType::Present:
		return vkb::QueueType::present;
	}
}

}