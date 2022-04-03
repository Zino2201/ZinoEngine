#pragma once

#include "vulkan.hpp"
#include <queue>

namespace ze::gfx
{

class VulkanDevice;

class VulkanDescriptorManager
{
	class DescriptorIndexPool
	{
	public:
		DescriptorIndexPool();

		uint32_t allocate();
		void free(uint32_t index);
	private:
		std::queue<uint32_t> free_indices;
		uint32_t next_free;
	};

public:
	enum class DescriptorType
	{
		UniformBuffer,
		StorageBuffer,
		Texture2D,
		TextureCube,
		Sampler,
	};

	struct DescriptorIndexHandle
	{
		DescriptorType type;
		bool is_uav;
		uint32_t index;

		DescriptorIndexHandle() : type(DescriptorType::UniformBuffer), is_uav(false), index(std::numeric_limits<uint32_t>::max()) {}
		DescriptorIndexHandle(DescriptorType in_type,
			bool in_is_uav,
			uint32_t in_index) : type(in_type), is_uav(in_is_uav), index(in_index) {}

		operator bool() const { return index != std::numeric_limits<uint32_t>::max(); }
	};

	VulkanDescriptorManager(VulkanDevice& in_device);
	~VulkanDescriptorManager();

	VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager(VulkanDescriptorManager&&) noexcept = delete;
	VulkanDescriptorManager& operator=(VulkanDescriptorManager&&) noexcept = delete;

	DescriptorIndexHandle allocate_index(DescriptorType type, bool is_uav);
	void free_index(DescriptorIndexHandle in_handle);

	void update_descriptor(DescriptorIndexHandle in_index, VkBuffer in_buffer, VkDescriptorType in_type);
	void update_descriptor(DescriptorIndexHandle in_index, VkImageViewType in_type, VkImageView in_view, VkImageLayout in_image_layout);
	void update_descriptor(DescriptorIndexHandle in_index, VkSampler in_sampler);
	void flush_updates();
	void bind_descriptors(VkCommandBuffer in_buffer, const VkPipelineBindPoint in_bind_point, VkPipelineLayout in_layout);

	const auto& get_global_descriptor_set_layout() const { return global_descriptor_set_layout; }
private:
	DescriptorIndexPool& get_pool(DescriptorType type, bool is_uav);
private:
	VulkanDevice& device;
	VkDescriptorPool global_descriptor_pool;
	VkDescriptorSetLayout global_descriptor_set_layout;
	VkDescriptorSet global_descriptor_set;

	DescriptorIndexPool srv_storage_buffers;
	DescriptorIndexPool uav_storage_buffers;
	DescriptorIndexPool srv_textures_2D;
	DescriptorIndexPool uav_textures_2D;
	DescriptorIndexPool srv_textures_cube;
	DescriptorIndexPool uav_textures_cube;
	DescriptorIndexPool srv_samplers;

	struct BufferUpdate
	{
		DescriptorIndexHandle index;
		VkBuffer buffer;
		VkDescriptorType descriptor_type;
	};

	struct TextureUpdate
	{
		DescriptorIndexHandle index;
		VkImageViewType texture_type;
		VkImageView texture;
		VkImageLayout layout;
	};

	struct SamplerUpdate
	{
		DescriptorIndexHandle index;
		VkSampler sampler;
	};

	std::vector<BufferUpdate> buffer_updates;
	std::vector<TextureUpdate> texture_updates;
	std::vector<SamplerUpdate> sampler_updates;

	/** Dummy objects used as a workaround of Vulkan validation layers bug https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/3729 */
#if ZE_BUILD(IS_DEBUG)
	VkBuffer dummy_buffer;
	VmaAllocation dummy_buffer_allocation;

	VkImage dummy_texture;
	VkImageView dummy_texture_view;
	VmaAllocation dummy_texture_allocation;

	VkSampler dummy_sampler;
#endif
};

}