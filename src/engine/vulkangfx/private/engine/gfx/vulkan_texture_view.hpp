#pragma once

#include "engine/gfx/texture.hpp"

namespace ze::gfx
{

class VulkanTextureView final
{
public:
	VulkanTextureView(VulkanDevice& in_device, 
		VkImageView in_image_view,
		VulkanDescriptorManager::DescriptorIndexHandle in_srv_index,
		VulkanDescriptorManager::DescriptorIndexHandle in_uav_index)
		: device(in_device), image_view(in_image_view), srv_index(in_srv_index), uav_index(in_uav_index) {}

	VulkanTextureView(VulkanTextureView&& in_other) noexcept = delete;
	
	~VulkanTextureView()
	{
#if ZE_BUILD(IS_DEBUG)
		if (srv_index)
			device.get_descriptor_manager().update_descriptor(srv_index, VK_IMAGE_VIEW_TYPE_2D, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);

		if (uav_index)
			device.get_descriptor_manager().update_descriptor(uav_index, VK_IMAGE_VIEW_TYPE_2D, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);

		if (srv_index || uav_index)
			device.get_descriptor_manager().flush_updates();
#endif

		if (srv_index)
		{
			device.get_descriptor_manager().free_index(srv_index);
		}

		if (uav_index)
		{
			device.get_descriptor_manager().free_index(uav_index);
		}

		vkDestroyImageView(device.get_device(), image_view, nullptr);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkImageView get_image_view() const { return image_view; }
	uint32_t get_srv_index() const { return srv_index.index; }
	uint32_t get_uav_index() const { return uav_index.index; }
private:
	VulkanDevice& device;
	VkImageView image_view;
	VulkanDescriptorManager::DescriptorIndexHandle srv_index;
	VulkanDescriptorManager::DescriptorIndexHandle uav_index;
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
	case TextureViewType::TexCube:
		return VK_IMAGE_VIEW_TYPE_CUBE;
	}
}

}