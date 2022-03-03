#pragma once

#include "engine/core.hpp"
#include "device_resource.hpp"
#include "swapchain.hpp"
#include "engine/result.hpp"
#include "gfx_result.hpp"
#include "engine/util/simple_pool.hpp"
#include <span>
#include "shader.hpp"
#include "command.hpp"
#include "gfx_pipeline.hpp"
#include "buffer.hpp"
#include "pipeline_layout.hpp"
#include "render_pass.hpp"
#include "sampler.hpp"
#include "sync.hpp"
#include "rect.hpp"
#include "backend_device.hpp"
#include <thread>
#include <robin_hood.h>

namespace ze::gfx
{

ZE_DEFINE_LOG_CATEGORY(gfx_device);

class Backend;
class BackendDevice;
class Device;

namespace detail
{

template<typename T, typename Handle>
struct IsHandleCompatibleWith : std::false_type {};

template<DeviceResourceType Type>
class BackendResourceWrapper
{
	inline static uint64_t resource_unique_idx = 0;

public:
	BackendResourceWrapper(Device& in_device,
		const BackendDeviceResource& in_resource,
		const std::string_view& in_debug_name,
		const uint32_t in_srv_index = -1,
		const uint32_t in_uav_index = -1) : device(in_device),
		resource(in_resource), srv_index(in_srv_index), uav_index(in_uav_index)
	{
#if ZE_BUILD(IS_DEBUG)
		debug_name = in_debug_name;

		if(debug_name.empty())
			debug_name = std::string("Unnamed ") + std::to_string(Type) + " " + std::to_string(resource_unique_idx++);
		device.get_backend_device()->set_resource_name(debug_name, 
			Type,
			in_resource);
#else
		UnusedParameters{ in_debug_name };
#endif
	}

	virtual ~BackendResourceWrapper() = default;

	[[nodiscard]] BackendDeviceResource get_resource() const { return resource; }
	[[nodiscard]] uint32_t get_srv_index() const { return srv_index; }
	[[nodiscard]] uint32_t get_uav_index() const { return uav_index; }
protected:
	Device& device;
	BackendDeviceResource resource;
	uint32_t srv_index;
	uint32_t uav_index;
#if ZE_BUILD(IS_DEBUG)
	std::string debug_name;
#endif
};

class Buffer : public BackendResourceWrapper<DeviceResourceType::Buffer>
{
public:
	Buffer(Device& in_device,
		const BackendDeviceResource& in_buffer,
		const std::string_view& in_debug_name,
		uint32_t in_srv_index,
		uint32_t in_uav_index) : BackendResourceWrapper(in_device, in_buffer, in_debug_name, in_srv_index, in_uav_index) {}
	~Buffer();
};

class Texture : public BackendResourceWrapper<DeviceResourceType::Texture>
{
public:
	Texture(Device& in_device,
		const TextureCreateInfo& in_create_info,
		const bool in_is_swapchain_texture,
		const BackendDeviceResource& in_texture,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_texture, in_debug_name),
		create_info(in_create_info), is_swapchain_texture(in_is_swapchain_texture), has_optimal_layout(true) {}
	~Texture();

	[[nodiscard]] const TextureCreateInfo& get_create_info() { return create_info; }
	[[nodiscard]] bool is_texture_from_swapchain() const { return is_swapchain_texture; }
	[[nodiscard]] TextureLayout get_layout(TextureLayout in_optimal_layout) const { return has_optimal_layout ? in_optimal_layout : TextureLayout::General; }
private:
	TextureCreateInfo create_info;
	bool is_swapchain_texture;
	bool has_optimal_layout;
};

class TextureView : public BackendResourceWrapper<DeviceResourceType::TextureView>
{
public:
	TextureView(Device& in_device,
		Texture& in_texture,
		const TextureViewCreateInfo& in_create_info,
		const BackendDeviceResource& in_texture_view,
		const std::string_view& in_debug_name,
		uint32_t in_srv_index,
		uint32_t in_uav_index) : BackendResourceWrapper(in_device, in_texture_view, in_debug_name, in_srv_index, in_uav_index), texture(in_texture),
		create_info(in_create_info), is_view_from_swapchain(in_texture.is_texture_from_swapchain()) {}
	~TextureView();

	[[nodiscard]] Texture& get_texture() { return texture; }
	[[nodiscard]] const TextureViewCreateInfo& get_create_info() { return create_info; }
private:
	Texture& texture;
	TextureViewCreateInfo create_info;
	bool is_view_from_swapchain;
};

class Shader : public BackendResourceWrapper<DeviceResourceType::Shader>
{
public:
	Shader(Device& in_device,
		const BackendDeviceResource& in_shader,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_shader, in_debug_name) {}
	~Shader();
};

class Swapchain : public BackendResourceWrapper<DeviceResourceType::Swapchain>
{
	friend class Device;

public:
	Swapchain(Device& in_device,
		const SwapChainCreateInfo& in_create_info,
		const BackendDeviceResource& in_swapchain,
		const std::string_view& in_debug_name);
	~Swapchain();

	void reset_handles();

	[[nodiscard]] TextureHandle get_backbuffer() const { return textures[current_image]; }
	[[nodiscard]] TextureViewHandle get_backbuffer_view() const { return views[current_image]; }
private:
	void destroy();
private:
	SwapChainCreateInfo create_info;
	std::vector<TextureHandle> textures;
	std::vector<TextureViewHandle> views;
	uint32_t current_image;
};

class CommandPool : public BackendResourceWrapper<DeviceResourceType::CommandPool>
{
public:
	CommandPool(Device& in_device,
		const BackendDeviceResource& in_command_pool,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_command_pool, in_debug_name) {}
	~CommandPool();
};

class PipelineLayout : public BackendResourceWrapper<DeviceResourceType::PipelineLayout>
{
public:
	PipelineLayout(Device& in_device,
		const BackendDeviceResource& in_pipeline_layout,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_pipeline_layout, in_debug_name) {}
	~PipelineLayout();
};

class CommandList : public BackendResourceWrapper<DeviceResourceType::CommandList>
{
public:
	CommandList(Device& in_device,
		const BackendDeviceResource& in_list,
		const QueueType& in_type,
		const std::string_view& in_debug_name);
	void reset();
	void prepare_draw();
	
	void set_render_pass(const BackendDeviceResource& in_handle);
	void set_pipeline_layout(const PipelineLayoutHandle& in_handle);
	void set_depth_stencil_state(const PipelineDepthStencilStateCreateInfo& in_state);
	void set_multisampling_state(const PipelineMultisamplingStateCreateInfo& in_info);
	void set_color_blend_state(const PipelineColorBlendStateCreateInfo& in_info);
	void set_vertex_input_state(const PipelineVertexInputStateCreateInfo& in_info);
	void set_input_assembly_state(const PipelineInputAssemblyStateCreateInfo& in_info);
	void set_rasterization_state(const PipelineRasterizationStateCreateInfo& in_info);
	void bind_shader(const PipelineShaderStage& in_stage);
	void push_constants(const ShaderStageFlags in_shader_stage_flags,
		const uint32_t in_offset,
		const uint32_t in_size,
		const void* in_data);

	[[nodiscard]] QueueType get_queue_type() const { return type; }
private:
	void update_pipeline_state();
	void update_descriptors();
private:
	QueueType type;
	PipelineLayoutHandle pipeline_layout;
	BackendDeviceResource render_pass;

	/** Pipeline states */
	PipelineColorBlendStateCreateInfo color_blend_state;
	PipelineDepthStencilStateCreateInfo depth_stencil_state;
	PipelineMultisamplingStateCreateInfo multisampling_state;
	std::vector<PipelineShaderStage> stages;
	PipelineVertexInputStateCreateInfo vertex_input_state;
	PipelineInputAssemblyStateCreateInfo input_assembly_state;
	PipelineRasterizationStateCreateInfo rasterizer_state;
	bool pipeline_state_dirty;
	bool has_compute_stage;
};

class Fence : public BackendResourceWrapper<DeviceResourceType::Fence>
{
public:
	Fence(Device& in_device,
		const BackendDeviceResource& in_fence,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_fence, in_debug_name) {}
	~Fence();
};

class Semaphore : public BackendResourceWrapper<DeviceResourceType::Semaphore>
{
public:
	Semaphore(Device& in_device,
		const BackendDeviceResource& in_semaphore,
		const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_semaphore, in_debug_name) {}
	~Semaphore();
};

class Sampler : public BackendResourceWrapper<DeviceResourceType::Sampler>
{
public:
	Sampler(Device& in_device,
		const BackendDeviceResource& in_sampler,
		const std::string_view& in_debug_name,
		uint32_t in_srv_index) : BackendResourceWrapper(in_device, in_sampler, in_debug_name, in_srv_index) {}
	~Sampler();
};

template<> struct IsHandleCompatibleWith<Buffer, BufferHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Texture, TextureHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<TextureView, TextureViewHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Swapchain, SwapchainHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Shader, ShaderHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<CommandList, CommandListHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<PipelineLayout, PipelineLayoutHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Fence, FenceHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Semaphore, SemaphoreHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Sampler, SamplerHandle> : std::true_type {};

/*
 * Spawn one command pool per thread
 */
class ThreadedCommandPool
{
public:
	struct Pool
	{
		BackendDeviceResource handle;
		std::vector<std::unique_ptr<CommandList>> command_lists;
		QueueType type;
		size_t free_command_list;
		
		Pool() : handle(null_backend_resource), type(QueueType::Gfx), free_command_list(0) {}
		~Pool();

		void init(const QueueType in_type);

		void reset();
		CommandListHandle allocate_cmd_list();

		Pool(const Pool&) = delete;
		void operator=(const Pool&) = delete;
	};
	
	ThreadedCommandPool(const QueueType in_type) : type(in_type) {}

	CommandListHandle allocate_cmd_list() { return get_pool().allocate_cmd_list(); }
	void reset();
private:
	Pool& get_pool()
	{
		auto it = pools.find(std::this_thread::get_id());
		if(it != pools.end())
			return it->second;

		auto& pool = pools[std::this_thread::get_id()];
		pool.init(type);
		return pool;
	}
private:
	QueueType type;
	robin_hood::unordered_map<std::thread::id, Pool> pools;
};

}

/** Base struct for device resource infos */
template<typename T>
struct DeviceResourceInfo
{
	std::string_view debug_name;
	
	T& set_debug_name(const std::string_view& in_debug_name)
	{
		debug_name = in_debug_name;
		return static_cast<T&>(*this);
	}
};

struct BufferInfo : public DeviceResourceInfo<BufferInfo>
{
	BufferCreateInfo info;
	std::span<const uint8_t> initial_data;

	explicit BufferInfo(const BufferCreateInfo& in_info,
		const std::span<const uint8_t>& in_initial_data = {}) : info(in_info),
		initial_data(in_initial_data) {}
	
	static BufferInfo make_staging(const size_t in_size, const std::span<const uint8_t> in_initial_data = {})
	{
		return BufferInfo(BufferCreateInfo(in_size, 
			MemoryUsage::CpuOnly, 
			BufferUsageFlags()),
			in_initial_data);
	}

	static BufferInfo make_ssbo(const size_t in_size, const std::span<const uint8_t> in_initial_data = {})
	{
		return BufferInfo(BufferCreateInfo(in_size,
			MemoryUsage::GpuOnly,
			BufferUsageFlags(BufferUsageFlagBits::StorageBuffer)),
			in_initial_data);
	}

	static BufferInfo make_ssbo_cpu_visible(const size_t in_size, const std::span<const uint8_t> in_initial_data = {})
	{
		return BufferInfo(BufferCreateInfo(in_size,
			MemoryUsage::CpuToGpu,
			BufferUsageFlags(BufferUsageFlagBits::StorageBuffer)),
			in_initial_data);
	}

	static BufferInfo make_vertex_buffer_cpu_visible(const size_t in_size)
	{
		return BufferInfo(BufferCreateInfo(in_size, 
			MemoryUsage::CpuToGpu, 
			BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)));
	}

	static BufferInfo make_index_buffer_cpu_visible(const size_t in_size)
	{
		return BufferInfo(BufferCreateInfo(in_size, 
			MemoryUsage::CpuToGpu, 
			BufferUsageFlags(BufferUsageFlagBits::IndexBuffer)));
	}

	static BufferInfo make_vertex_buffer(const size_t in_size, const std::span<const uint8_t> in_initial_data)
	{
		return BufferInfo(BufferCreateInfo(in_size,
			MemoryUsage::GpuOnly,
			BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)),
			in_initial_data);
	}

	static BufferInfo make_index_buffer(const size_t in_size, const std::span<const uint8_t> in_initial_data)
	{
		return BufferInfo(BufferCreateInfo(in_size,
			MemoryUsage::GpuOnly,
			BufferUsageFlags(BufferUsageFlagBits::IndexBuffer)),
			in_initial_data);
	}
};

struct TextureInfo : public DeviceResourceInfo<TextureInfo>
{
	TextureCreateInfo info;
	std::vector<std::span<uint8_t>> initial_mip_data;

	explicit TextureInfo(const TextureCreateInfo& in_info,
		const std::vector<std::span<uint8_t>>& in_initial_mip_data = {}) : info(in_info),
		initial_mip_data(in_initial_mip_data) {}

	static TextureInfo make_immutable_2d(const uint32_t in_width, 
		const uint32_t in_height,
		const Format in_format, 
		const uint32_t in_mip_levels = 1, 
		const TextureUsageFlags in_usage_flags = TextureUsageFlags(TextureUsageFlagBits::Sampled),
		const std::vector<std::span<uint8_t>>& in_initial_datas = {})
	{
		return TextureInfo(TextureCreateInfo(TextureType::Tex2D,
			MemoryUsage::GpuOnly,
			in_format,
			in_width,
			in_height,
			1,
			in_mip_levels,
			1,
			SampleCountFlagBits::Count1,
			in_usage_flags), in_initial_datas);
	}

	static TextureInfo make_immutable_cube(const uint32_t in_width,
		const uint32_t in_height,
		const Format in_format, const uint32_t in_mip_levels = 1,
		const TextureUsageFlags in_usage_flags = TextureUsageFlags(TextureUsageFlagBits::Sampled | gfx::TextureUsageFlagBits::Cube),
		const std::vector<std::span<uint8_t>>& in_initial_datas = {})
	{
		return TextureInfo(TextureCreateInfo(TextureType::Tex2D,
			MemoryUsage::GpuOnly,
			in_format,
			in_width,
			in_height,
			1,
			in_mip_levels,
			6,
			SampleCountFlagBits::Count1,
			in_usage_flags | gfx::TextureUsageFlagBits::Cube), in_initial_datas );
	}

	static TextureInfo make_depth_stencil_attachment(const uint32_t in_width, 
		const uint32_t in_height,
		const Format in_format,
		const TextureUsageFlags in_usage_flags = TextureUsageFlags(TextureUsageFlagBits::DepthStencilAttachment))
	{
		return TextureInfo(TextureCreateInfo(TextureType::Tex2D,
			MemoryUsage::GpuOnly,
			in_format,
			in_width,
			in_height,
			1,
			1,
			1,
			SampleCountFlagBits::Count1,
			in_usage_flags));
	}
};

struct TextureViewInfo : public DeviceResourceInfo<TextureViewInfo>
{
	TextureViewType type;
	TextureHandle texture;
	Format format;
	TextureSubresourceRange subresource_range;

	TextureViewInfo(const TextureViewType in_type,
		const TextureHandle& in_handle,
		const Format in_format,
		const TextureSubresourceRange& in_subresource_range) : type(in_type),
		texture(in_handle),
		format(in_format),
		subresource_range(in_subresource_range) {}

	static TextureViewInfo make_2d(const TextureHandle& in_handle,
		const Format in_format,
		const TextureSubresourceRange& in_subresource_range = TextureSubresourceRange(
			TextureAspectFlags(TextureAspectFlagBits::Color),
			0,1,
			0,
			1))
	{
		return TextureViewInfo(TextureViewType::Tex2D,
			in_handle,
			in_format,
			in_subresource_range);
	}

	static TextureViewInfo make_cube(const TextureHandle& in_handle,
		const Format in_format,
		const TextureSubresourceRange& in_subresource_range = TextureSubresourceRange(
			TextureAspectFlags(TextureAspectFlagBits::Color),
			0, 
			1,
			0,
			6))
	{
		return TextureViewInfo(TextureViewType::TexCube,
			in_handle,
			in_format,
			in_subresource_range);
	}

	static TextureViewInfo make_depth(const TextureHandle& in_handle,
		const Format in_format,
		const TextureSubresourceRange& in_subresource_range = TextureSubresourceRange(
			TextureAspectFlags(TextureAspectFlagBits::Depth),
			0,1,
			0,
			1))
	{
		return TextureViewInfo(TextureViewType::Tex2D,
			in_handle,
			in_format,
			in_subresource_range);
	}
};

struct SwapChainInfo : public DeviceResourceInfo<SwapChainInfo>
{
	SwapChainCreateInfo create_info;

	SwapChainInfo(const SwapChainCreateInfo& in_create_info) : create_info(in_create_info) {}
};

struct ShaderInfo : public DeviceResourceInfo<ShaderInfo>
{
	ShaderCreateInfo create_info;

	ShaderInfo(const ShaderCreateInfo& in_create_info) : create_info(in_create_info) {}

	static ShaderInfo make(const std::span<uint32_t>& in_bytecode)
	{
		return ShaderInfo(ShaderCreateInfo(in_bytecode));
	}
};

struct FenceInfo : public DeviceResourceInfo<FenceInfo>
{
	FenceCreateInfo create_info;

	FenceInfo() = default;
	FenceInfo(const FenceCreateInfo& in_create_info) : create_info(in_create_info) {}
};

struct SemaphoreInfo : public DeviceResourceInfo<SemaphoreInfo>
{
	SemaphoreCreateInfo create_info;

	SemaphoreInfo() = default;
	SemaphoreInfo(const SemaphoreCreateInfo& in_create_info) : create_info(in_create_info) {}
};

struct PipelineLayoutInfo : public DeviceResourceInfo<PipelineLayoutInfo>
{
	PipelineLayoutCreateInfo create_info;

	PipelineLayoutInfo(const PipelineLayoutCreateInfo& in_create_info) : create_info(in_create_info) {}
};

struct SamplerInfo : public DeviceResourceInfo<SamplerInfo>
{
	SamplerCreateInfo create_info;

	SamplerInfo(const SamplerCreateInfo& in_create_info = {}) : create_info(in_create_info) {}
};

/**
 * Informations about a render pass
 */
struct RenderPassInfo
{
	enum class DepthStencilMode
	{
		ReadOnly,
		ReadWrite
	};
	
	struct Subpass
	{
		std::span<uint32_t> color_attachments;
		std::span<uint32_t> input_attachments;
		std::span<uint32_t> resolve_attachments;
		/** Depth stencil mode for the depth-stencil attachment (determine layout) */
		DepthStencilMode mode;

		Subpass(const std::span<uint32_t>& in_color_attachments,
			const std::span<uint32_t>& in_input_attachments,
			const std::span<uint32_t>& in_resolve_attachments,
			const DepthStencilMode in_mode = DepthStencilMode::ReadWrite) : color_attachments(in_color_attachments),
		input_attachments(in_input_attachments), resolve_attachments(in_resolve_attachments), mode(in_mode) {}
	};
	
	/** Attachments to use */
	std::span<TextureViewHandle> color_attachments;
	std::span<TextureViewHandle> input_attachments;
	TextureViewHandle depth_stencil_attachment;
	bool depth_attachment_read_only;

	/** Clear/load/store flags. To add attachment 2/3 to load e.g: (1 << 2) | (1 << 3) */
	uint32_t clear_attachment_flags;
	uint32_t load_attachment_flags;
	uint32_t store_attachment_flags;

	std::span<ClearValue> clear_values;
	std::span<Subpass> subpasses;

	Rect2D render_area;

	RenderPassInfo() : depth_attachment_read_only(false), clear_attachment_flags(0), load_attachment_flags(0), store_attachment_flags(0) {}
};

/**
 * A GPU device, used to communicate with it
 * The engine currently only supports one active GPU (as using multiple GPU is hard to manage)
 */
class Device final
{
	friend class detail::Swapchain;
	friend class detail::CommandList;
	
	struct Frame
	{
		detail::ThreadedCommandPool gfx_command_pool;
		detail::ThreadedCommandPool compute_command_pool;

		FenceHandle gfx_fence;
		std::vector<FenceHandle> wait_fences;

		/** Expired resources */
		std::vector<BufferHandle> expired_buffers;
		std::vector<TextureHandle> expired_textures;
		std::vector<TextureViewHandle> expired_texture_views;
		std::vector<SwapchainHandle> expired_swapchains;
		std::vector<ShaderHandle> expired_shaders;
		std::vector<PipelineLayoutHandle> expired_pipeline_layouts;
		std::vector<PipelineHandle> expired_pipelines;
		std::vector<FenceHandle> expired_fences;
		std::vector<SemaphoreHandle> expired_semaphores;
		std::vector<SamplerHandle> expired_samplers;

		std::vector<CommandListHandle> gfx_lists;
		std::vector<SemaphoreHandle> gfx_wait_semaphores;
		std::vector<SemaphoreHandle> gfx_signal_semaphores;
		bool gfx_submitted;

		Frame();
		Frame(const Frame&) = delete;
		Frame& operator=(const Frame&) = delete;

		void free_resources();
		void reset()
		{
			expired_buffers.clear();
			expired_textures.clear();
			expired_texture_views.clear();
			expired_swapchains.clear();
			expired_shaders.clear();
			expired_pipeline_layouts.clear();
			expired_pipelines.clear();
			expired_semaphores.clear();
			expired_samplers.clear();

			gfx_command_pool.reset();

			gfx_lists.clear();
			gfx_wait_semaphores.clear();
			gfx_signal_semaphores.clear();
			gfx_submitted = false;

			compute_command_pool.reset();

			wait_fences.clear();
		}

		void destroy()
		{
			expired_fences.emplace_back(gfx_fence);
		}
	};
public:
	static constexpr size_t max_frames_in_flight = 1;

	Device(Backend& in_backend, std::unique_ptr<BackendDevice>&& in_backend_device);
	~Device();
	
	Device(const Device&) = delete;	 
	void operator=(const Device&) = delete;
	
	void wait_idle();

	void new_frame();
	void end_frame();
	void submit(CommandListHandle in_cmd_list, 
		const std::span<SemaphoreHandle>& in_wait_semaphores = {},
		const std::span<SemaphoreHandle>& in_signal_semaphores = {});
	
	[[nodiscard]] Result<BufferHandle, GfxResult> create_buffer(BufferInfo in_create_info);
	[[nodiscard]] Result<TextureHandle, GfxResult> create_texture(TextureInfo in_create_info);
	[[nodiscard]] Result<TextureViewHandle, GfxResult> create_texture_view(TextureViewInfo in_create_info);
	[[nodiscard]] Result<SwapchainHandle, GfxResult> create_swapchain(const SwapChainInfo& in_create_info);
	[[nodiscard]] Result<SemaphoreHandle, GfxResult> create_semaphore(const SemaphoreInfo& in_create_info);
	[[nodiscard]] Result<FenceHandle, GfxResult> create_fence(const FenceInfo& in_create_info);
	[[nodiscard]] Result<ShaderHandle, GfxResult> create_shader(const ShaderInfo& in_create_info);
	[[nodiscard]] Result<PipelineLayoutHandle, GfxResult> create_pipeline_layout(const PipelineLayoutInfo& in_create_info);
	[[nodiscard]] Result<SamplerHandle, GfxResult> create_sampler(const SamplerInfo& in_create_info);

	void destroy_buffer(const BufferHandle& in_buffer);
	void destroy_texture(const TextureHandle& in_texture);
	void destroy_texture_view(const TextureViewHandle& in_texture_view);
	void destroy_sampler(const SamplerHandle& in_sampler);
	void destroy_swapchain(const SwapchainHandle& in_swapchain);
	void destroy_shader(const ShaderHandle& in_shader);
	void destroy_pipeline_layout(const PipelineLayoutHandle& in_pipeline_layout);
	void destroy_fence(const FenceHandle& in_fence);
	void destroy_semaphore(const SemaphoreHandle& in_semaphore);

	Result<void*, GfxResult> map_buffer(const BufferHandle& in_handle);
	void unmap_buffer(const BufferHandle& in_handle);

	[[nodiscard]] CommandListHandle allocate_cmd_list(const QueueType& in_type);

	void wait_for_fences(const std::span<FenceHandle>& in_fences, 
		const bool in_wait_for_all = true, 
		const uint64_t in_timeout = std::numeric_limits<uint64_t>::max());
	void reset_fences(const std::span<FenceHandle>& in_fences);

	uint32_t get_srv_descriptor_index(const BufferHandle& in_buffer);
	uint32_t get_uav_descriptor_index(const BufferHandle& in_buffer);
	uint32_t get_srv_descriptor_index(const TextureViewHandle& in_texture_view);
	uint32_t get_uav_descriptor_index(const TextureViewHandle& in_texture_view);
	uint32_t get_srv_descriptor_index(const SamplerHandle& in_sampler);

	/** Commands */
	void cmd_begin_region(const CommandListHandle& in_list, const std::string_view& in_name, const glm::vec4& in_color);
	void cmd_end_region(const CommandListHandle& in_list);
	void cmd_begin_render_pass(const CommandListHandle& in_cmd_list,
		const RenderPassInfo& in_info);
	void cmd_draw(const CommandListHandle& in_cmd_list,
		const uint32_t in_vertex_count, 
		const uint32_t in_instance_count,
		const uint32_t in_first_vertex,
		const uint32_t in_first_index);
	void cmd_draw_indexed(const CommandListHandle& in_list, 
		const uint32_t in_index_count, 
		const uint32_t in_instance_count, 
		const uint32_t in_first_index, 
		const int32_t in_vertex_offset, 
		const uint32_t in_first_instance);
	void cmd_dispatch(const CommandListHandle& in_list,
		const uint32_t in_x,
		const uint32_t in_y,
		const uint32_t in_z);
	void cmd_end_render_pass(const CommandListHandle& in_cmd_list);
	void cmd_bind_vertex_buffer(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_buffer,
		const uint64_t in_offset);
	void cmd_bind_vertex_buffers(const CommandListHandle& in_cmd_list,
		const std::span<BufferHandle>& in_buffers,
		const std::span<uint64_t>& in_offsets);
	void cmd_bind_index_buffer(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_buffer,
		const uint64_t in_offset,
		const IndexType in_index_type);
	void cmd_copy_buffer(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_src_buffer,
		const BufferHandle& in_dst_buffer,
		const std::span<BufferCopyRegion>& in_regions);
	void cmd_copy_buffer_to_texture(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_src_buffer,
		const TextureHandle& in_dst_texture,
		const TextureLayout in_dst_layout,
		const std::span<BufferTextureCopyRegion>& in_regions);
	void cmd_texture_barrier(const CommandListHandle& in_cmd_list,
		const TextureHandle& in_texture,
		const PipelineStageFlags in_src_flags,
		const TextureLayout in_src_layout,
		const AccessFlags in_src_access_flags,
		const PipelineStageFlags in_dst_flags,
		const TextureLayout in_dst_layout,
		const AccessFlags in_dst_access_flags);

	/** Pipeline management */
	void cmd_bind_pipeline_layout(const CommandListHandle& in_cmd_list, const PipelineLayoutHandle& in_handle);
	void cmd_set_depth_stencil_state(const CommandListHandle& in_cmd_list, const PipelineDepthStencilStateCreateInfo& in_state);
	void cmd_set_multisampling_state(const CommandListHandle& in_cmd_list, const PipelineMultisamplingStateCreateInfo& in_info);
	void cmd_set_color_blend_state(const CommandListHandle& in_cmd_list, const PipelineColorBlendStateCreateInfo& in_info);
	void cmd_set_vertex_input_state(const CommandListHandle& in_cmd_list, const PipelineVertexInputStateCreateInfo& in_info);
	void cmd_set_input_assembly_state(const CommandListHandle& in_cmd_list, const PipelineInputAssemblyStateCreateInfo& in_info);
	void cmd_set_rasterization_state(const CommandListHandle& in_cmd_list, const PipelineRasterizationStateCreateInfo& in_info);
	void cmd_bind_shader(const CommandListHandle& in_cmd_list, const PipelineShaderStage& in_stage);
	void cmd_push_constants(const CommandListHandle& in_cmd_list,
		const ShaderStageFlags in_shader_stage_flags,
		const uint32_t in_offset,
		const uint32_t in_size,
		const void* in_data);

	/** Dynamic states */
	void cmd_set_scissor(const CommandListHandle& in_cmd_list, const Rect2D& in_scissor);

	/** Swapchain */
	GfxResult acquire_swapchain_texture(const SwapchainHandle& in_swapchain,
		const SemaphoreHandle& in_signal_semaphore = SemaphoreHandle());
	void present(const SwapchainHandle& in_swapchain,
		const std::span<SemaphoreHandle>& in_wait_semaphores = {});
	TextureViewHandle get_swapchain_backbuffer_view(const SwapchainHandle& in_swapchain) const;
	BackendDeviceResource get_swapchain_backend_handle(const SwapchainHandle& in_swapchain) const;

	template<typename T>
	[[nodiscard]] static T* cast_handle(const auto& in_handle)
		requires detail::IsHandleCompatibleWith<T, std::decay_t<decltype(in_handle)>>::value
	{
		return in_handle ? reinterpret_cast<T*>(in_handle.get_handle()) : nullptr;		
	}

	template<typename Handle>
	[[nodiscard]] static Handle cast_resource_ptr(auto* in_resource)
		requires detail::IsHandleCompatibleWith<std::remove_pointer_t<decltype(in_resource)>, Handle>::value
	{
		return Handle(reinterpret_cast<uint64_t>(in_resource));
	}

	static BackendDeviceResource get_backend_shader(const ShaderHandle& in_handle)
	{
		return cast_handle<detail::Shader>(in_handle)->get_resource();
	}

	[[nodiscard]] BackendDevice* get_backend_device() const { return backend_device.get(); }
	size_t get_current_frame_idx() const { return current_frame; }
private:
	void submit_queue(const QueueType& in_type);
	BackendDeviceResource get_or_create_render_pass(const RenderPassCreateInfo& in_create_info);
	BackendDeviceResource get_or_create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info);
	BackendDeviceResource get_or_create_compute_pipeline(const ComputePipelineCreateInfo& in_create_info);
	
	[[nodiscard]] Frame& get_current_frame() { return *frames[current_frame]; }
private:
	Backend& backend;
	std::unique_ptr<BackendDevice> backend_device;
	size_t current_frame;
	std::vector<std::unique_ptr<Frame>> frames;

	robin_hood::unordered_map<RenderPassCreateInfo, BackendDeviceResource> render_passes;
	robin_hood::unordered_map<GfxPipelineCreateInfo, BackendDeviceResource> gfx_pipelines;
	robin_hood::unordered_map<ComputePipelineCreateInfo, BackendDeviceResource> compute_pipelines;
	
	/** Resources pools */
	ThreadSafeSimplePool<detail::Buffer> buffers;
	ThreadSafeSimplePool<detail::Texture> textures;
	ThreadSafeSimplePool<detail::TextureView> texture_views;
	ThreadSafeSimplePool<detail::Shader> shaders;
	ThreadSafeSimplePool<detail::Swapchain> swapchains;
	ThreadSafeSimplePool<detail::PipelineLayout> pipeline_layouts;
	ThreadSafeSimplePool<detail::Fence> fences;
	ThreadSafeSimplePool<detail::Semaphore> semaphores;
	ThreadSafeSimplePool<detail::Sampler> samplers;
};
	
/**
 * Get the currently used device
 */
Device* get_device();

/**
 * Smart device resources
 */

namespace detail
{
/**
 * std::unique_ptr-like for DeviceResource
 */
template<DeviceResourceType Type, typename Deleter>
struct UniqueDeviceResource
{
	using HandleType = DeviceResource<Type>;

	UniqueDeviceResource() {}
	explicit UniqueDeviceResource(HandleType in_handle) { reset(in_handle); }
	~UniqueDeviceResource() { destroy(); }

	UniqueDeviceResource(const UniqueDeviceResource&) = delete;
	void operator=(const UniqueDeviceResource&) = delete;

	UniqueDeviceResource(UniqueDeviceResource&& other) noexcept
	{
		reset(std::exchange(other.handle, HandleType()));
	}

	UniqueDeviceResource& operator=(UniqueDeviceResource&& other) noexcept
	{
		reset(std::exchange(other.handle, HandleType()));
		return *this;
	}

	[[nodiscard]] HandleType free()
	{
		HandleType freed_handle = handle;
		handle = HandleType();
		return freed_handle;
	}

	void reset(const HandleType& in_new_handle = HandleType())
	{
		destroy();
		handle = in_new_handle;
	}

	[[nodiscard]] HandleType get() const
	{
		ZE_CHECK(handle)
		return handle;
	}

	[[nodiscard]] HandleType operator*() const
	{
		return handle;
	}

	[[nodiscard]] operator bool() const
	{
		return is_valid();
	}

	[[nodiscard]] bool is_valid() const
	{
		return static_cast<bool>(handle);
	}
private:
	void destroy()
	{
		if(handle)
		{
			Deleter()(handle);
			handle = HandleType();
		}
	}
private:
	HandleType handle;
	[[no_unique_address]] Deleter deleter;
};

/**
 * Deleters
 */
#define ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(UpperCaseName, LowerCaseName, Type) \
	namespace detail \
	{ \
	struct SmartDeleter_##UpperCaseName\
	{ \
		void operator()(Type in_handle) const \
		{ \
			get_device()->destroy_##LowerCaseName(in_handle); \
		} \
	}; \
	} \
	using Unique##UpperCaseName = detail::UniqueDeviceResource<DeviceResourceType::UpperCaseName, detail::SmartDeleter_##UpperCaseName>;

}

ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Buffer, buffer, BufferHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Texture, texture, TextureHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(TextureView, texture_view, TextureViewHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(PipelineLayout, pipeline_layout, PipelineLayoutHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Shader, shader, ShaderHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Swapchain, swapchain, SwapchainHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Semaphore, semaphore, SemaphoreHandle);
ZE_GFX_DECLARE_SMART_DEVICE_RESOURCE(Sampler, sampler, SamplerHandle);

}