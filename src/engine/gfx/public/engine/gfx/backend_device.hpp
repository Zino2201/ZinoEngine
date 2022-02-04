#pragma once

#include "engine/core.hpp"
#include "engine/result.hpp"
#include "device_resource.hpp"
#include "gfx_result.hpp"
#include "buffer.hpp"
#include "swapchain.hpp"
#include "shader.hpp"
#include "render_pass.hpp"
#include "gfx_pipeline.hpp"
#include "compute_pipeline.hpp"
#include "command.hpp"
#include "rect.hpp"
#include "sync.hpp"
#include "sampler.hpp"
#include "pipeline_layout.hpp"

namespace ze::gfx
{

/**
 * Backend implementation of a GPU device
 * This is the low-level version of ze::gfx::Device, it should not be used directly!
 * Every function here must be called at the proper time, it is not the responsibility of the BackendDevice to ensure
 * that the GPU isn't accessing a specific resource !
 * \remark There is no additional runtime parameter checks !
 */
class BackendDevice
{
public:
	BackendDevice() = default; 
	virtual ~BackendDevice() = default;
	
	BackendDevice(const BackendDevice&) = delete;
	BackendDevice(BackendDevice&&) noexcept = default;
	 
	void operator=(const BackendDevice&) = delete;
	BackendDevice& operator=(BackendDevice&&) noexcept = default;

	virtual void new_frame() = 0;
	virtual void wait_idle() = 0;
	virtual void set_resource_name(const std::string_view& in_name, 
		const DeviceResourceType in_type, 
		const BackendDeviceResource in_handle) = 0;

	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_buffer(const BufferCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_texture(const TextureCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_texture_view(const TextureViewCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_sampler(const SamplerCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_swap_chain(const SwapChainCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_shader(const ShaderCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_compute_pipeline(const ComputePipelineCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_render_pass(const RenderPassCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_command_pool(const CommandPoolCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_semaphore(const SemaphoreCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_fence(const FenceCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info) = 0;
	
	virtual void destroy_buffer(const BackendDeviceResource& in_swap_chain) = 0;
	virtual void destroy_texture(const BackendDeviceResource& in_texture) = 0;
	virtual void destroy_texture_view(const BackendDeviceResource& in_texture_view) = 0;
	virtual void destroy_sampler(const BackendDeviceResource& in_sampler) = 0;
	virtual void destroy_swap_chain(const BackendDeviceResource& in_swap_chain) = 0;
	virtual void destroy_shader(const BackendDeviceResource& in_shader) = 0;
	virtual void destroy_pipeline(const BackendDeviceResource& in_pipeline) = 0;
	virtual void destroy_render_pass(const BackendDeviceResource& in_render_pass) = 0;
	virtual void destroy_command_pool(const BackendDeviceResource& in_command_pool) = 0;
	virtual void destroy_semaphore(const BackendDeviceResource& in_semaphore) = 0;
	virtual void destroy_fence(const BackendDeviceResource& in_fence) = 0;
	virtual void destroy_pipeline_layout(const BackendDeviceResource& in_pipeline_layout) = 0;

	/** Buffer */
	[[nodiscard]] virtual Result<void*, GfxResult> map_buffer(const BackendDeviceResource& in_buffer) = 0;
	virtual void unmap_buffer(const BackendDeviceResource& in_buffer) = 0;

	/** Command pool */
	[[nodiscard]] virtual Result<std::vector<BackendDeviceResource>, GfxResult> allocate_command_lists(const BackendDeviceResource& in_pool, 
		const uint32_t in_count) = 0;
	virtual void free_command_lists(const BackendDeviceResource& in_pool, const std::vector<BackendDeviceResource>& in_lists) = 0;
	virtual void reset_command_pool(const BackendDeviceResource& in_pool) = 0;
	
	/** Swapchain */
	[[nodiscard]] virtual std::pair<GfxResult, uint32_t> acquire_swapchain_image(const BackendDeviceResource& in_swapchain,
		const BackendDeviceResource& in_signal_semaphore) = 0;
	virtual void present(const BackendDeviceResource& in_swapchain,
		const std::span<BackendDeviceResource>& in_wait_semaphores = {}) = 0;
	[[nodiscard]] virtual BackendDeviceResource get_swapchain_backbuffer_view(const BackendDeviceResource& in_swapchain) = 0;
	[[nodiscard]] virtual const std::vector<BackendDeviceResource>& get_swapchain_backbuffers(const BackendDeviceResource& in_swapchain) = 0;
	[[nodiscard]] virtual const std::vector<BackendDeviceResource>& get_swapchain_backbuffer_views(const BackendDeviceResource& in_swapchain) = 0;
	[[nodiscard]] virtual Format get_swapchain_format(const BackendDeviceResource& in_swapchain) = 0;

	/** Pipeline layout */
	[[nodiscard]] virtual Result<BackendDeviceResource, GfxResult> allocate_descriptor_set(const BackendDeviceResource& in_pipeline_layout,
		const uint32_t in_set,
		const std::span<Descriptor, max_bindings>& in_descriptors) = 0;

	/** Commands */
	virtual void begin_cmd_list(const BackendDeviceResource& in_list) = 0;
	virtual void cmd_begin_render_pass(const BackendDeviceResource& in_list,
		const BackendDeviceResource& in_render_pass,
		const Framebuffer& in_framebuffer,
		Rect2D in_render_area,
		std::span<ClearValue> in_clear_values) = 0;
	virtual void cmd_bind_pipeline(const BackendDeviceResource& in_list,
		const PipelineBindPoint in_bind_point,
		const BackendDeviceResource& in_pipeline) = 0;
	virtual void cmd_dispatch(const BackendDeviceResource& in_list,
		const uint32_t in_x,
		const uint32_t in_y,
		const uint32_t in_z) = 0;
	virtual void cmd_draw(const BackendDeviceResource& in_list,
		const uint32_t in_vertex_count,
		const uint32_t in_instance_count,
		const uint32_t in_first_vertex,
		const uint32_t in_first_instance) = 0;
	virtual void cmd_draw_indexed(const BackendDeviceResource& in_list,
		const uint32_t in_index_count,
		const uint32_t in_instance_count,
		const uint32_t in_first_index,
		const int32_t in_vertex_offset,
		const uint32_t in_first_instance) = 0;
	virtual void cmd_end_render_pass(const BackendDeviceResource& in_list) = 0;

	virtual void cmd_bind_descriptor_sets(const BackendDeviceResource in_list,
		const BackendDeviceResource in_pipeline_layout,
		const std::span<BackendDeviceResource> in_descriptor_sets) = 0;
	virtual void cmd_bind_vertex_buffers(const BackendDeviceResource& in_list,
		const uint32_t in_first_binding,
		const std::span<BackendDeviceResource> in_buffers,
		const std::span<uint64_t> in_offsets) = 0;
	virtual void cmd_bind_index_buffer(const BackendDeviceResource in_list,
		const BackendDeviceResource in_index_buffer,
		const uint64_t in_offset,
		const IndexType in_index_type) = 0;
	virtual void cmd_set_viewports(const BackendDeviceResource& in_list,
		const uint32_t in_first_viewport,
		const std::span<Viewport>& in_viewports) = 0;
	virtual void cmd_set_scissors(const BackendDeviceResource& in_list,
		const uint32_t in_first_scissor,
		const std::span<Rect2D>& in_scissors) = 0;

	virtual void cmd_pipeline_barrier(const BackendDeviceResource in_list,
		const PipelineStageFlags in_src_flags,
		const PipelineStageFlags in_dst_flags,
		const std::span<TextureMemoryBarrier>& in_texture_memory_barriers) = 0;

	/** Transfer commands */
	virtual void cmd_copy_buffer(const BackendDeviceResource& in_cmd_list,
		const BackendDeviceResource& in_src_buffer,
		const BackendDeviceResource& in_dst_buffer,
		const std::span<BufferCopyRegion>& in_regions) = 0;

	virtual void cmd_copy_buffer_to_texture(const BackendDeviceResource in_list,
		const BackendDeviceResource in_src_buffer,
		const BackendDeviceResource in_dst_texture,
		const TextureLayout in_dst_layout,
		const std::span<BufferTextureCopyRegion>& in_copy_regions) = 0;

	virtual void end_cmd_list(const BackendDeviceResource& in_list) = 0;

	/** Fence */

	/**
	 * Wait for a list of fences
	 * \param in_fences List of fences
	 * \param in_wait_for_all Wait for all fences ?
	 * \param in_timeout Timeout to wait (in nanoseconds)
	 */
	virtual GfxResult wait_for_fences(const std::span<BackendDeviceResource>& in_fences,
		const bool in_wait_for_all = true,
		const uint64_t in_timeout = std::numeric_limits<uint64_t>::max()) = 0;

	virtual void reset_fences(const std::span<BackendDeviceResource>& in_fences) = 0;

	/** Queue */

	/**
	 * Submit command lists to a queue
	 * \param in_type Targetted queue
	 * \param in_command_lists Lists to execute
	 * \param in_wait_semaphores Semaphores to wait from
	 * \param in_wait_pipeline_stages Pipeline stage to wait for the semaphore
	 * \param in_signal_semaphores Semaphores to signal when all lists are executed
	 * \param in_fence Fence to signal when work is done
	 */
	virtual void queue_submit(const QueueType& in_type,
		const std::span<BackendDeviceResource>& in_command_lists,
		const std::span<BackendDeviceResource>& in_wait_semaphores = {},
		const std::span<PipelineStageFlags>& in_wait_pipeline_stages = {},
		const std::span<BackendDeviceResource>& in_signal_semaphores = {},
		const BackendDeviceResource& in_fence = null_backend_resource) = 0;
};
	
}