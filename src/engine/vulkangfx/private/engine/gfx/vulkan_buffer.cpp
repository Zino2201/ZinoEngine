#include "engine/gfx/vulkan_buffer.hpp"
#include "vulkan_device.hpp"

namespace ze::gfx
{

VulkanBuffer::VulkanBuffer(VulkanDevice& in_device,
	const VkBuffer& in_buffer,
	const VmaAllocation& in_allocation,
	const VmaAllocationInfo& in_alloc_info,
	VulkanDescriptorManager::DescriptorIndexHandle in_srv_index,
	VulkanDescriptorManager::DescriptorIndexHandle in_uav_index) : device(in_device), buffer(in_buffer), allocation(in_allocation),
	alloc_info(in_alloc_info), srv_index(in_srv_index), uav_index(in_uav_index)
{
    UnusedParameters { alloc_info };
}
	
VulkanBuffer::~VulkanBuffer()
{
#if ZE_BUILD(IS_DEBUG)
	if (srv_index)
		device.get_descriptor_manager().update_descriptor(srv_index, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	if (uav_index)
		device.get_descriptor_manager().update_descriptor(srv_index, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

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

	vmaDestroyBuffer(device.get_allocator(), buffer, allocation);
}
	
}
