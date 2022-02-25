#pragma once

#include "vulkan.hpp"
#include "vulkan_device.hpp"

namespace ze::gfx
{

class VulkanTexture final
{
public:
	VulkanTexture(VulkanDevice& in_device,
		VkImage in_image,
		VmaAllocation in_allocation,
		VkImageUsageFlags in_flags) : device(in_device), image(in_image), allocation(in_allocation), image_usage_flags(in_flags) {}

	VulkanTexture(VulkanDevice& in_device, 
		VkImage in_image) : device(in_device), image(in_image), allocation(nullptr) {}
	
	VulkanTexture(VulkanTexture&& in_other) noexcept = delete;
	
	~VulkanTexture()
	{
		if(allocation)
			vmaDestroyImage(device.get_allocator(), image, allocation);
	}

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkImage get_texture() const { return image; }
	[[nodiscard]] VmaAllocation get_allocation() const { return allocation; }
	[[nodiscard]] VkImageUsageFlags get_image_usage_flags() const { return image_usage_flags; }
private:
	VulkanDevice& device;
	VkImage image;
	VmaAllocation allocation;
	VkImageUsageFlags image_usage_flags;
};

inline VkSampleCountFlagBits convert_sample_count_bit(const SampleCountFlagBits& in_bit)
{
	switch(in_bit)
	{
	default:
	case SampleCountFlagBits::Count1:
		return VK_SAMPLE_COUNT_1_BIT;
	case SampleCountFlagBits::Count2:
		return VK_SAMPLE_COUNT_2_BIT;
	case SampleCountFlagBits::Count4:
		return VK_SAMPLE_COUNT_4_BIT;
	case SampleCountFlagBits::Count8:
		return VK_SAMPLE_COUNT_8_BIT;
	case SampleCountFlagBits::Count16:
		return VK_SAMPLE_COUNT_16_BIT;
	case SampleCountFlagBits::Count32:
		return VK_SAMPLE_COUNT_32_BIT;
	case SampleCountFlagBits::Count64:
		return VK_SAMPLE_COUNT_64_BIT;
	}
}

inline VkImageLayout convert_texture_layout(const TextureLayout& in_layout)
{
	switch(in_layout)
	{
	default:
	case TextureLayout::Undefined:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	case TextureLayout::General:
		return VK_IMAGE_LAYOUT_GENERAL;
	case TextureLayout::ColorAttachment:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case TextureLayout::DepthStencilAttachment:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case TextureLayout::ShaderReadOnly:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case TextureLayout::DepthStencilReadOnly:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case TextureLayout::TransferSrc:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case TextureLayout::TransferDst:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case TextureLayout::Present:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
}

inline VkImageAspectFlags convert_aspect_flags(const TextureAspectFlags& in_flags)
{
	VkImageAspectFlags flags = 0;

	if(in_flags & TextureAspectFlagBits::Color)
		flags |= VK_IMAGE_ASPECT_COLOR_BIT;
	
	if(in_flags & TextureAspectFlagBits::Depth)
		flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
	
	if(in_flags & TextureAspectFlagBits::Stencil)
		flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
	
	return flags;
}

inline VkImageSubresourceRange convert_subresource_range(const TextureSubresourceRange& in_range)
{
	VkImageSubresourceRange range;
	range.baseMipLevel = in_range.base_mip_level;
	range.baseArrayLayer = in_range.base_array_layer;
	range.levelCount = in_range.level_count;
	range.layerCount = in_range.layer_count;
	range.aspectMask = convert_aspect_flags(in_range.aspect_flags);
	return range;
}

inline VkImageSubresourceLayers convert_subresource_layers(const TextureSubresourceLayers& in_layers)
{
	VkImageSubresourceLayers layers;
	layers.mipLevel = in_layers.mip_level;
	layers.baseArrayLayer = in_layers.base_array_layer;
	layers.layerCount = in_layers.layer_count;
	layers.aspectMask = convert_aspect_flags(in_layers.aspect_flags);
	return layers;
}

inline VkImageType convert_texture_type(const TextureType& in_type)
{
	switch(in_type)
	{
	default:
	case TextureType::Tex1D:
		return VK_IMAGE_TYPE_1D;
	case TextureType::Tex2D:
		return VK_IMAGE_TYPE_2D;
	case TextureType::Tex3D:
		return VK_IMAGE_TYPE_3D;
	}
}

}