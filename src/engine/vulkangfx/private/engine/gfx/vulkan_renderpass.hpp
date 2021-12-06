#pragma once

#include "vulkan.hpp"

namespace ze::gfx
{

class VulkanRenderPass final
{
public:
	VulkanRenderPass(VulkanDevice& in_device, 
		VkRenderPass in_render_pass) : device(in_device), render_pass(in_render_pass) {}

	VulkanRenderPass(VulkanRenderPass&& in_other) noexcept = delete;
	
	~VulkanRenderPass()
	{
		vkDestroyRenderPass(device.get_device(), render_pass, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkRenderPass get_render_pass() const { return render_pass; }
private:
	VulkanDevice& device;
	VkRenderPass render_pass;
};

inline VkAttachmentLoadOp convert_load_op(const AttachmentLoadOp& in_op)
{
	switch(in_op)
	{
	default:
	case AttachmentLoadOp::DontCare:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case AttachmentLoadOp::Clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case AttachmentLoadOp::Load:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

inline VkAttachmentStoreOp convert_store_op(const AttachmentStoreOp& in_op)
{
	switch(in_op)
	{
	default:
	case AttachmentStoreOp::DontCare:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case AttachmentStoreOp::Store:
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

}