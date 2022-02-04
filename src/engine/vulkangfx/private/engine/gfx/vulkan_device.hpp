#pragma once

#include "engine/gfx/backend_device.hpp"
#include "Vulkan.hpp"
#include "engine/gfx/vulkan_backend.hpp"
#include <robin_hood.h>
#include "vulkan_descriptor_set.hpp"
#include "engine/containers/sparse_array.hpp"

namespace ze::gfx
{

class VulkanSwapChain;

class VulkanDevice final : public BackendDevice
{
	struct DeviceWrapper
	{
		vkb::Device device;

		explicit DeviceWrapper(vkb::Device&& in_device) : device(in_device) {}
		~DeviceWrapper() { vkb::destroy_device(device); }
	};
	
	/** Small facility to manage VkSurfaceKHRs */
	class SurfaceManager
	{
		struct SurfaceWrapper
		{
			VulkanDevice& device;
			VkSurfaceKHR surface;

			SurfaceWrapper(VulkanDevice& in_device,
				VkSurfaceKHR in_surface) : device(in_device), surface(in_surface) {}

			SurfaceWrapper(SurfaceWrapper&& in_other) noexcept :
				device(in_other.device),
				surface(std::exchange(in_other.surface, VK_NULL_HANDLE)) {}
			
			~SurfaceWrapper()
			{
				vkDestroySurfaceKHR(device.get_backend().get_instance(), surface, nullptr);
			}
		};
	public:
		SurfaceManager(VulkanDevice& in_device) : device(in_device) {}

		Result<VkSurfaceKHR, GfxResult> get_or_create(void* in_os_handle)
		{
			{
				if(auto it = surfaces.find(in_os_handle); it != surfaces.end())
					return it->second.surface;
			}

			auto surface = create_surface(in_os_handle);
			if(!surface)
			{
				logger::error(log_vulkan, "Failed to create Vulkan surface: {}", surface.get_error());
				return make_error(GfxResult::ErrorInitializationFailed);
			}
			
			surfaces.insert({ in_os_handle, SurfaceWrapper(device, surface.get_value()) });
			return make_result(surface.get_value());
		}
	private:
		Result<VkSurfaceKHR, VkResult> create_surface(void* in_os_handle)
		{
#if ZE_PLATFORM(WINDOWS)
			VkWin32SurfaceCreateInfoKHR create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			create_info.hinstance = nullptr;
			create_info.hwnd = static_cast<HWND>(in_os_handle);
			create_info.flags = 0;
			create_info.pNext = nullptr;

			VkSurfaceKHR handle = VK_NULL_HANDLE;
			if(VkResult result = vkCreateWin32SurfaceKHR(device.get_backend().get_instance(),
				&create_info,
				nullptr,
				&handle); result != VK_SUCCESS)
				return make_error(result);

			return make_result(handle);
#endif
		}
	private:
		VulkanDevice& device;
		robin_hood::unordered_map<void*, SurfaceWrapper> surfaces;
	};

	/** Facility to manage framebuffer accouting for lifetimes to destroy unused framebuffers */
	class FramebufferManager
	{
		static constexpr uint8_t framebuffer_expiration_frames = 10;
		
		struct Entry
		{
			/** Frame count since this framebuffer has been polled */
			VulkanDevice& device;
			uint8_t unpolled_frames;
			VkFramebuffer framebuffer;

			Entry(VulkanDevice& in_device, VkFramebuffer in_framebuffer)
				: device(in_device), unpolled_frames(0), framebuffer(in_framebuffer) {}
			~Entry() { vkDestroyFramebuffer(device.get_device(), framebuffer, nullptr); }

			Entry(Entry&& in_other) : device(in_other.device),
				unpolled_frames(std::move(in_other.unpolled_frames)),
				framebuffer(std::exchange(in_other.framebuffer, VK_NULL_HANDLE)) {}
		};

		VulkanDevice& device;
		robin_hood::unordered_map<Framebuffer, Entry> framebuffers;

	public:
		FramebufferManager(VulkanDevice& in_device) : device(in_device) {}
		
		void new_frame();
		VkFramebuffer get_or_create(VkRenderPass in_render_pass, const Framebuffer& in_framebuffer);
	};
public:
	explicit VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device);
	~VulkanDevice() override;

	void new_frame() override;
	void wait_idle() override
	{
		vkDeviceWaitIdle(get_device());
	}

	void set_resource_name(const std::string_view& in_name, 
		const DeviceResourceType in_type, 
		const BackendDeviceResource in_handle) override;

	Result<BackendDeviceResource, GfxResult> create_buffer(const BufferCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_texture(const TextureCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_texture_view(const TextureViewCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_sampler(const SamplerCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_swap_chain(const SwapChainCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_shader(const ShaderCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_compute_pipeline(const ComputePipelineCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_render_pass(const RenderPassCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_command_pool(const CommandPoolCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_semaphore(const SemaphoreCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_fence(const FenceCreateInfo& in_create_info) override;
	Result<BackendDeviceResource, GfxResult> create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info) override;
	
	void destroy_buffer(const BackendDeviceResource& in_buffer) override;
	void destroy_texture(const BackendDeviceResource& in_texture) override;
	void destroy_texture_view(const BackendDeviceResource& in_texture_view) override;
	void destroy_sampler(const BackendDeviceResource& in_sampler) override;
	void destroy_swap_chain(const BackendDeviceResource& in_swap_chain) override;
	void destroy_shader(const BackendDeviceResource& in_shader) override;
	void destroy_pipeline(const BackendDeviceResource& in_pipeline) override;
	void destroy_render_pass(const BackendDeviceResource& in_render_pass) override;
	void destroy_command_pool(const BackendDeviceResource& in_command_pool) override;
	void destroy_semaphore(const BackendDeviceResource& in_semaphore) override;
	void destroy_fence(const BackendDeviceResource& in_fence) override;
	void destroy_pipeline_layout(const BackendDeviceResource& in_pipeline_layout) override;

	Result<BackendDeviceResource, GfxResult> allocate_descriptor_set(const BackendDeviceResource& in_pipeline_layout,
		const uint32_t in_set,
		const std::span<Descriptor, max_bindings>& in_descriptors) override;

	Result<void*, GfxResult> map_buffer(const BackendDeviceResource& in_buffer) override;
	void unmap_buffer(const BackendDeviceResource& in_buffer) override;

	Result<std::vector<BackendDeviceResource>, GfxResult> allocate_command_lists(const BackendDeviceResource& in_pool, 
		const uint32_t in_count) override;
	void free_command_lists(const BackendDeviceResource& in_pool, const std::vector<BackendDeviceResource>& in_lists) override;
	void reset_command_pool(const BackendDeviceResource& in_pool) override;
	
	void begin_cmd_list(const BackendDeviceResource& in_list) override;
	void cmd_begin_render_pass(const BackendDeviceResource& in_list,
		const BackendDeviceResource& in_render_pass,
		const Framebuffer& in_framebuffer,
		Rect2D in_render_area,
		std::span<ClearValue> in_clear_values) override;
	void cmd_bind_pipeline(const BackendDeviceResource& in_list, 
		const PipelineBindPoint in_bind_point, 
		const BackendDeviceResource& in_pipeline) override;
	void cmd_dispatch(const BackendDeviceResource& in_list, const uint32_t in_x, const uint32_t in_y, const uint32_t in_z) override;
	void cmd_draw(const BackendDeviceResource& in_list,
		const uint32_t in_vertex_count,
		const uint32_t in_instance_count,
		const uint32_t in_first_vertex,
		const uint32_t in_first_instance) override;
	void cmd_draw_indexed(const BackendDeviceResource& in_list,
		const uint32_t in_index_count,
		const uint32_t in_instance_count,
		const uint32_t in_first_index,
		const int32_t in_vertex_offset,
		const uint32_t in_first_instance) override;
	void cmd_end_render_pass(const BackendDeviceResource& in_list) override;
	void cmd_bind_descriptor_sets(const BackendDeviceResource in_list, 
		const BackendDeviceResource in_pipeline_layout, 
		const std::span<BackendDeviceResource> in_descriptor_sets) override;
	void cmd_bind_vertex_buffers(const BackendDeviceResource& in_list, 
		const uint32_t in_first_binding, 
		const std::span<BackendDeviceResource> in_buffers, 
		const std::span<uint64_t> in_offsets) override;
	void cmd_bind_index_buffer(const BackendDeviceResource in_list,
		const BackendDeviceResource in_index_buffer,
		const uint64_t in_offset,
		const IndexType in_index_type) override;
	void cmd_set_viewports(const BackendDeviceResource& in_list, const uint32_t in_first_viewport, const std::span<Viewport>& in_viewports) override;
	void cmd_set_scissors(const BackendDeviceResource& in_list, const uint32_t in_first_scissor, const std::span<Rect2D>& in_scissors) override;

	void cmd_pipeline_barrier(const BackendDeviceResource in_list, const PipelineStageFlags in_src_flags, 
		const PipelineStageFlags in_dst_flags, 
		const std::span<TextureMemoryBarrier>& in_texture_memory_barriers) override;
	void cmd_copy_buffer(const BackendDeviceResource& in_cmd_list, 
		const BackendDeviceResource& in_src_buffer, 
		const BackendDeviceResource& in_dst_buffer, 
		const std::span<BufferCopyRegion>& in_regions) override;
	void cmd_copy_buffer_to_texture(const BackendDeviceResource in_list,
		const BackendDeviceResource in_src_buffer,
		const BackendDeviceResource in_dst_texture,
		const TextureLayout in_dst_layout,
		const std::span<BufferTextureCopyRegion>& in_copy_regions);

	void end_cmd_list(const BackendDeviceResource& in_list) override;

	std::pair<GfxResult, uint32_t> acquire_swapchain_image(const BackendDeviceResource& in_swapchain, 
		const BackendDeviceResource& in_signal_semaphore) override;
	void present(const BackendDeviceResource& in_swapchain,
		const std::span<BackendDeviceResource>& in_wait_semaphores) override;
	const std::vector<BackendDeviceResource>& get_swapchain_backbuffer_views(const BackendDeviceResource& in_swapchain) override;
	const std::vector<BackendDeviceResource>& get_swapchain_backbuffers(const BackendDeviceResource& in_swapchain) override;
	BackendDeviceResource get_swapchain_backbuffer_view(const BackendDeviceResource& in_swapchain) override;
	Format get_swapchain_format(const BackendDeviceResource& in_swapchain) override;
	
	GfxResult wait_for_fences(const std::span<BackendDeviceResource>& in_fences, 
		const bool in_wait_for_all, 
		const uint64_t in_timeout) override;

	void reset_fences(const std::span<BackendDeviceResource>& in_fences) override;
	
	void queue_submit(const QueueType& in_type,
		const std::span<BackendDeviceResource>& in_command_lists,
		const std::span<BackendDeviceResource>& in_wait_semaphores = {},
		const std::span<PipelineStageFlags>& in_wait_pipeline_stages = {},
		const std::span<BackendDeviceResource>& in_signal_semaphores = {},
		const BackendDeviceResource& in_fence = null_backend_resource) override;

	void free_descriptor_set_allocator(const size_t in_idx) { descriptor_set_allocators.remove(in_idx); }

	[[nodiscard]] VulkanBackend& get_backend() const { return backend; }
	[[nodiscard]] VkDevice get_device() const { return device_wrapper.device.device; }
	[[nodiscard]] VkPhysicalDevice get_physical_device() const { return device_wrapper.device.physical_device.physical_device; }
	[[nodiscard]] VmaAllocator get_allocator() const { return allocator; }
	[[nodiscard]] VkQueue get_present_queue() { return device_wrapper.device.get_queue(vkb::QueueType::graphics).value(); }
private:
	VulkanBackend& backend;
	VmaAllocator allocator;
	DeviceWrapper device_wrapper;
	SurfaceManager surface_manager;
	FramebufferManager framebuffer_manager;
	SparseArray<VulkanDescriptorSetAllocator> descriptor_set_allocators;
};
	
}