#include "engine/gfx/vulkan_pipeline_layout.hpp"
#include "engine/gfx/vulkan_buffer.hpp"
#include "engine/gfx/vulkan_texture.hpp"
#include "engine/gfx/vulkan_texture_view.hpp"

namespace ze::gfx
{

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& in_device, 
	VkPipelineLayout in_pipeline_layout,
	const std::vector<VkDescriptorSetLayout>& in_set_layouts,
	const uint32_t in_descriptor_type_mask) : device(in_device), pipeline_layout(in_pipeline_layout),
	set_layouts(in_set_layouts), descriptor_type_mask(in_descriptor_type_mask)
{
	memset(allocator_indices.data(), 0, sizeof(size_t) * allocator_indices.size());
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	size_t i = 0;
	for(const auto& set_layout : set_layouts)
	{
		vkDestroyDescriptorSetLayout(device.get_device(), set_layout, nullptr);
		device.free_descriptor_set_allocator(allocator_indices[i]);
		i++;
	}

	vkDestroyPipelineLayout(device.get_device(), pipeline_layout, nullptr);	
}

}