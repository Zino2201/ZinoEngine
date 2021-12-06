#include "vulkan_descriptor_set.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline_layout.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_texture_view.hpp"

namespace ze::gfx
{

static constexpr uint32_t max_descriptor_sets_per_pool = 8;
static constexpr uint32_t default_descriptor_count_per_type = 32;

VulkanDescriptorSetAllocator::VulkanDescriptorSetAllocator(VulkanDevice& in_device,
	VulkanPipelineLayout& in_pipeline_layout,
	VkDescriptorSetLayout in_set_layout) : device(in_device),
	pipeline_layout(in_pipeline_layout), set_layout(in_set_layout)
{
	for(size_t type = 0; type < VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++type)
	{
		if(in_pipeline_layout.get_descriptor_type_mask() & (1 << type))
		{
			pool_sizes.push_back(VkDescriptorPoolSize{ static_cast<VkDescriptorType>(type), default_descriptor_count_per_type });
		}
	}
}

VulkanDescriptorSetAllocator::~VulkanDescriptorSetAllocator()
{
	for(const auto& pool : pools)
		vkDestroyDescriptorPool(device.get_device(), pool, nullptr);
}

VulkanDescriptorSetAllocator::VulkanDescriptorSetAllocator(VulkanDescriptorSetAllocator&& other) noexcept
	: device(other.device),
	pipeline_layout(other.pipeline_layout),
	set_layout(std::move(other.set_layout)),
	pools(std::move(other.pools)),
	free_sets(std::move(other.free_sets)),
	pool_sizes(std::move(other.pool_sizes)),
	hashmap(std::move(other.hashmap)) {}

void VulkanDescriptorSetAllocator::new_frame()
{
	std::vector<uint64_t> hashes;
	hashes.reserve(5);

	for(auto& [hash, node] : hashmap)
	{
		if(node.frame++ >= Node::node_max_unused_lifetime)
		{
			hashes.emplace_back(hash);
			free_sets.emplace(node.set);
		}
	}

	for(const auto& hash : hashes)
		hashmap.erase(hash);
}

VkDescriptorSet VulkanDescriptorSetAllocator::allocate(const std::span<Descriptor, max_bindings>& in_descriptors)
{
	uint64_t hash = 0;
	for(const auto& descriptor : in_descriptors)
		hash_combine(hash, descriptor);

	auto it = hashmap.find(hash);
	if(it != hashmap.end())
	{
		it->second.frame = 0;
		return it->second.set;
	}

	if(free_sets.empty())
		allocate_pool();

	VkDescriptorSet set = free_sets.front();
	free_sets.pop();

	std::vector<VkWriteDescriptorSet> writes;
	writes.reserve(in_descriptors.size());

	std::vector<std::variant<std::monostate, VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
	infos.reserve(in_descriptors.size());

	for(const auto& descriptor : in_descriptors)
	{
		if(descriptor.info.index() == Descriptor::None)
			continue;

		auto& info = infos.emplace_back();

		switch(descriptor.info.index())
		{
		default:
			ZE_UNREACHABLE();
			break;
		case Descriptor::BufferInfo:
		{
			DescriptorBufferInfo buffer = std::get<DescriptorBufferInfo>(descriptor.info);
			info = VkDescriptorBufferInfo { get_resource<VulkanBuffer>(buffer.handle)->get_buffer(),
				buffer.offset,
				buffer.range };
			break;
		}
		case Descriptor::TextureInfo:
		{
			DescriptorTextureInfo texture = std::get<DescriptorTextureInfo>(descriptor.info);
			info = VkDescriptorImageInfo {
				VK_NULL_HANDLE,
				get_resource<VulkanTextureView>(texture.texture_view)->get_image_view(),
				convert_texture_layout(texture.layout) };
			break;
		}
		case Descriptor::SamplerInfo:
		{
			DescriptorSamplerInfo sampler = std::get<DescriptorSamplerInfo>(descriptor.info);
			info = VkDescriptorImageInfo {
				reinterpret_cast<VkSampler>(sampler.sampler),
				VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_UNDEFINED};
			break;
		}
		}

		writes.emplace_back(
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			set,
			descriptor.binding,
			0,
			1,
			convert_descriptor_type(descriptor.type),
			descriptor.info.index() != Descriptor::BufferInfo ? &std::get<VkDescriptorImageInfo>(info) : nullptr,
			descriptor.info.index() == Descriptor::BufferInfo ? &std::get<VkDescriptorBufferInfo>(info) : nullptr,
			nullptr);
	}

	vkUpdateDescriptorSets(device.get_device(),
		static_cast<uint32_t>(writes.size()),
		writes.data(),
		0,
		nullptr);

	logger::verbose(log_vulkan, "Updated descriptor set");

	hashmap.insert({ hash, Node(set)} );

	return set;
}

void VulkanDescriptorSetAllocator::allocate_pool()
{
	/** Allocate pool */
	VkDescriptorPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.maxSets = max_descriptor_sets_per_pool;
	create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	create_info.pPoolSizes = pool_sizes.data();

	VkDescriptorPool pool;
	VkResult result = vkCreateDescriptorPool(device.get_device(),
		&create_info,
		nullptr,
		&pool);
	ZE_ASSERT(result == VK_SUCCESS);

	pools.emplace_back(pool);

	/** Allocate sets */
	std::array<VkDescriptorSet, max_descriptor_sets_per_pool> sets;
	std::array<VkDescriptorSetLayout, max_descriptor_sets_per_pool> layouts;

	for(auto& layout : layouts)
		layout = set_layout;

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = pool;
	alloc_info.descriptorSetCount = max_descriptor_sets_per_pool;
	alloc_info.pSetLayouts = layouts.data();
	result = vkAllocateDescriptorSets(device.get_device(),
		&alloc_info,
		sets.data());
	ZE_ASSERT(result == VK_SUCCESS);

	for(auto set : sets)
		free_sets.push(set);

	logger::verbose(log_vulkan, "Allocated descriptor pool with {} descriptor sets", max_descriptor_sets_per_pool);
}

}
