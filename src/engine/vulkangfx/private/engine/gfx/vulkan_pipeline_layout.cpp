#include "engine/gfx/vulkan_pipeline_layout.hpp"
#include "engine/gfx/vulkan_buffer.hpp"
#include "engine/gfx/vulkan_texture.hpp"
#include "engine/gfx/vulkan_texture_view.hpp"

namespace ze::gfx
{

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& in_device, 
	VkPipelineLayout in_pipeline_layout) : device(in_device), pipeline_layout(in_pipeline_layout) {}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	vkDestroyPipelineLayout(device.get_device(), pipeline_layout, nullptr);	
}

}