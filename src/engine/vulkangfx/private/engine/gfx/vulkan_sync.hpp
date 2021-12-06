#pragma once

#include "vulkan.hpp"

namespace ze::gfx
{

class VulkanSemaphore final
{
public:
	VulkanSemaphore(VulkanDevice& in_device, 
		VkSemaphore in_semaphore) : device(in_device), semaphore(in_semaphore) {}

	VulkanSemaphore(VulkanSemaphore&& in_other) noexcept = delete;
	
	~VulkanSemaphore()
	{
		vkDestroySemaphore(device.get_device(), semaphore, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkSemaphore get_semaphore() const { return semaphore; }
private:
	VulkanDevice& device;
	VkSemaphore semaphore;
};

class VulkanFence final
{
public:
	VulkanFence(VulkanDevice& in_device, 
		VkFence in_fence) : device(in_device), fence(in_fence) {}
	
	VulkanFence(VulkanFence&& in_other) noexcept = delete;
	
	~VulkanFence()
	{
		vkDestroyFence(device.get_device(), fence, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkFence get_fence() const { return fence; }
private:
	VulkanDevice& device;
	VkFence fence;
};

}