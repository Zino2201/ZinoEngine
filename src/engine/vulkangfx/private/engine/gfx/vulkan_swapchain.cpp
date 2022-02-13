#include "engine/gfx/vulkan_swapchain.hpp"
#include "engine/gfx/vulkan_device.hpp"
#include "engine/gfx/vulkan_texture.hpp"
#include "engine/gfx/vulkan_texture_view.hpp"

namespace ze::gfx
{

VulkanSwapChain::VulkanSwapChain(VulkanDevice& in_device,
	const vkb::Swapchain& in_swapchain) : device(in_device), swapchain(in_swapchain), current_image(0)
{
	auto image_list = swapchain.get_images();
	auto image_view_list = swapchain.get_image_views();

	for(const auto& image : image_list.value())
	{
		images.push_back(new_resource<VulkanTexture>(device, image));
	}
	
	for(const auto& image_view : image_view_list.value())
	{
		image_views.push_back(new_resource<VulkanTextureView>(device, 
			image_view, 
			VulkanDescriptorManager::DescriptorIndexHandle {}, 
			VulkanDescriptorManager::DescriptorIndexHandle {}));
	}
}

VulkanSwapChain::~VulkanSwapChain()
{
	for(const auto& image : images)
		free_resource<VulkanTexture>(image);

	for(const auto& image_view : image_views)
		free_resource<VulkanTextureView>(image_view);
	
	if(swapchain.swapchain)
		vkb::destroy_swapchain(swapchain);
}

VkResult VulkanSwapChain::acquire_image(VkSemaphore in_signal_semaphore)
{
	return vkAcquireNextImageKHR(device.get_device(),
		swapchain.swapchain,
		std::numeric_limits<uint64_t>::max(),
		in_signal_semaphore,
		VK_NULL_HANDLE,
		&current_image);
}

void VulkanSwapChain::present(const std::span<VkSemaphore>& in_wait_semaphores)
{
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain.swapchain;
	info.waitSemaphoreCount = static_cast<uint32_t>(in_wait_semaphores.size());
	info.pWaitSemaphores = in_wait_semaphores.data();
	info.pResults = nullptr;
	info.pImageIndices = &current_image;
	
	vkQueuePresentKHR(device.get_present_queue(),
		&info);	
}
	
}