#pragma once

#include "engine/gfx/texture.hpp"

namespace ze::gfx
{

class VulkanTextureView final
{
public:
	VulkanTextureView(VulkanDevice& in_device, 
		VkImageView in_image_view) : device(in_device), image_view(in_image_view) {}

	VulkanTextureView(VulkanTextureView&& in_other) noexcept = delete;
	
	~VulkanTextureView()
	{
		vkDestroyImageView(device.get_device(), image_view, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkImageView get_image_view() const { return image_view; }
private:
	VulkanDevice& device;
	VkImageView image_view;
};

inline VkImageViewType convert_view_type(const TextureViewType& in_view_type)
{
	switch(in_view_type)
	{
	default:
	case TextureViewType::Tex1D:
		return VK_IMAGE_VIEW_TYPE_1D;
	case TextureViewType::Tex2D:
		return VK_IMAGE_VIEW_TYPE_2D;
	case TextureViewType::Tex3D:
		return VK_IMAGE_VIEW_TYPE_3D;
	}
}

}