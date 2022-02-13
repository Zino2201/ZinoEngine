#include "vulkan_descriptor_manager.hpp"
#include "engine/gfx/pipeline_layout.hpp"

namespace ze::gfx
{

VulkanDescriptorManager::DescriptorIndexPool::DescriptorIndexPool()
	: next_free(0) {}

uint32_t VulkanDescriptorManager::DescriptorIndexPool::allocate()
{
	uint32_t index = std::numeric_limits<uint32_t>::max();
	if (free_indices.empty())
	{
		index = next_free++;
	}
	else
	{
		index = free_indices.front();
		free_indices.pop();
	}

	ZE_CHECK(index != -1);
	return index;
}

void VulkanDescriptorManager::DescriptorIndexPool::free(uint32_t index)
{
	free_indices.push(index);
}

VulkanDescriptorManager::VulkanDescriptorManager(VulkanDevice& in_device)
	: device(in_device)
{
	{
		std::vector<VkDescriptorPoolSize> pool_sizes;
		for (size_t type = 0; type < VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++type)
		{
			pool_sizes.push_back(VkDescriptorPoolSize{ static_cast<VkDescriptorType>(type), 32 });
		}

		VkDescriptorPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		create_info.pNext = nullptr;
		create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
		create_info.maxSets = 1;
		create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		create_info.pPoolSizes = pool_sizes.data();

		VkResult result = vkCreateDescriptorPool(device.get_device(),
			&create_info,
			nullptr,
			&global_descriptor_pool);
		ZE_ASSERT(result == VK_SUCCESS);

	}

	{
		static const std::array bindings = {
			VkDescriptorSetLayoutBinding { srv_storage_buffer_binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				max_descriptors_per_binding, VK_SHADER_STAGE_ALL, nullptr },
			VkDescriptorSetLayoutBinding { uav_storage_buffer_binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				max_descriptors_per_binding, VK_SHADER_STAGE_ALL, nullptr },
			VkDescriptorSetLayoutBinding { srv_texture_2D_binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				max_descriptors_per_binding, VK_SHADER_STAGE_ALL, nullptr },
			VkDescriptorSetLayoutBinding { srv_texture_cube_binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				max_descriptors_per_binding, VK_SHADER_STAGE_ALL, nullptr },
			VkDescriptorSetLayoutBinding { srv_sampler_binding, VK_DESCRIPTOR_TYPE_SAMPLER,
				max_descriptors_per_binding, VK_SHADER_STAGE_ALL, nullptr },
		};

		VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info = {};
		binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

		std::vector<VkDescriptorBindingFlags> binding_flags;
		for (size_t i = 0; i < bindings.size(); ++i)
			binding_flags.emplace_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);

		binding_flags_create_info.bindingCount = static_cast<uint32_t>(binding_flags.size());
		binding_flags_create_info.pBindingFlags = binding_flags.data();

		VkDescriptorSetLayoutCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		create_info.pNext = &binding_flags_create_info;
		create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		create_info.bindingCount = static_cast<uint32_t>(bindings.size());
		create_info.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device.get_device(),
			&create_info,
			nullptr,
			&global_descriptor_set_layout);
		ZE_ASSERT(result == VK_SUCCESS);
	}

	{
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = nullptr;
		alloc_info.descriptorPool = global_descriptor_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &global_descriptor_set_layout;
		VkResult result = vkAllocateDescriptorSets(device.get_device(),
			&alloc_info,
			&global_descriptor_set);
		ZE_ASSERT(result == VK_SUCCESS);
	}
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	vkDestroyDescriptorSetLayout(device.get_device(), global_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(device.get_device(), global_descriptor_pool, nullptr);
}

VulkanDescriptorManager::DescriptorIndexHandle VulkanDescriptorManager::allocate_index(DescriptorType type, bool is_uav)
{
	return DescriptorIndexHandle(type, is_uav, get_pool(type, is_uav).allocate());
}

void VulkanDescriptorManager::free_index(DescriptorIndexHandle in_handle)
{
	return get_pool(in_handle.type, in_handle.is_uav).free(in_handle.index);
}

void VulkanDescriptorManager::update_descriptor(DescriptorIndexHandle in_index, VkBuffer in_buffer, VkDescriptorType in_type)
{
	buffer_updates.push_back({ in_index, in_buffer, in_type });
}

void VulkanDescriptorManager::update_descriptor(DescriptorIndexHandle in_index, VkImageViewType in_type, VkImageView in_view)
{
	texture_updates.push_back({ in_index, in_type, in_view });
}

void VulkanDescriptorManager::update_descriptor(DescriptorIndexHandle in_index, VkSampler in_sampler)
{
	sampler_updates.emplace_back(in_index, in_sampler);
}

void VulkanDescriptorManager::flush_updates()
{
	if (!buffer_updates.empty()
		|| !texture_updates.empty()
		|| !sampler_updates.empty())
	{
		std::vector<VkWriteDescriptorSet> writes;
		std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
		writes.reserve(buffer_updates.size() + texture_updates.size() + sampler_updates.size());
		infos.reserve(writes.capacity());

		for (const auto update : buffer_updates)
		{
			auto& info = infos.emplace_back(VkDescriptorBufferInfo{ update.buffer,
				0,
				VK_WHOLE_SIZE });

			writes.emplace_back(
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				global_descriptor_set,
				update.index.is_uav ? uav_storage_buffer_binding : srv_storage_buffer_binding,
				update.index.index,
				1,
				update.descriptor_type,
				nullptr,
				&std::get<VkDescriptorBufferInfo>(info),
				nullptr);
		}

		for (const auto update : texture_updates)
		{
			auto& info = infos.emplace_back(VkDescriptorImageInfo{ VK_NULL_HANDLE,
				update.texture,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

			uint32_t binding = srv_texture_2D_binding;
			if (update.texture_type == VK_IMAGE_VIEW_TYPE_CUBE)
				binding = srv_texture_cube_binding;

			writes.emplace_back(
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				global_descriptor_set,
				binding,
				update.index.index,
				1,
				VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				&std::get<VkDescriptorImageInfo>(info),
				nullptr,
				nullptr);
		}

		for (const auto update : sampler_updates)
		{
			auto& info = infos.emplace_back(VkDescriptorImageInfo{ update.sampler,
				VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_UNDEFINED });

			writes.emplace_back(
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				global_descriptor_set,
				srv_sampler_binding,
				update.index.index,
				1,
				VK_DESCRIPTOR_TYPE_SAMPLER,
				&std::get<VkDescriptorImageInfo>(info),
				nullptr,
				nullptr);
		}

		vkUpdateDescriptorSets(device.get_device(),
			static_cast<uint32_t>(writes.size()),
			writes.data(),
			0,
			nullptr);

		buffer_updates.clear();
		texture_updates.clear();
		sampler_updates.clear();
	}
}

void VulkanDescriptorManager::bind_descriptors(VkCommandBuffer in_buffer, const VkPipelineBindPoint in_bind_point, VkPipelineLayout in_layout)
{
	vkCmdBindDescriptorSets(in_buffer,
		in_bind_point,
		in_layout,
		0,
		1,
		&global_descriptor_set,
		0,
		nullptr);
}

VulkanDescriptorManager::DescriptorIndexPool& VulkanDescriptorManager::get_pool(DescriptorType type, bool is_uav)
{
	switch (type)
	{
	case DescriptorType::StorageBuffer:
		return is_uav ? uav_storage_buffers : srv_storage_buffers;
	case DescriptorType::Texture2D:
		return is_uav ? uav_textures_2D : srv_textures_2D;
	case DescriptorType::TextureCube:
		return is_uav ? uav_textures_cube: srv_textures_cube;
	case DescriptorType::Sampler:
		ZE_CHECK(!is_uav);
		return srv_samplers;
	}

	ZE_UNREACHABLE();
}

}