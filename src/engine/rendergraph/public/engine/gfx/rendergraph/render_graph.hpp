#pragma once

#include "engine/gfx/device.hpp"
#include <any>
#include <unordered_set>

/**
 * A render graph implementation inspired by Hans-Kristian Arntzen's work
 */

namespace ze::gfx::rendergraph
{

class RenderGraph;
class RenderPass;
class PhysicalResourceRegistry;

struct AttachmentInfo
{
	Format format;
	uint32_t width;
	uint32_t height;
	bool sampled = false;
};

enum class RenderPassQueueFlagBits
{
	Gfx = 1 << 0
};
ZE_ENABLE_FLAG_ENUMS(RenderPassQueueFlagBits, RenderPassQueueFlags);

enum class ResourceType
{
	Attachment
};

class Resource
{
public:
	static constexpr uint32_t null_resource_idx = std::numeric_limits<uint32_t>::max();

	Resource(const ResourceType in_type, const std::string& in_name, const uint32_t in_index = null_resource_idx)
		: type(in_type), name(in_name), index(in_index), physical_index(Resource::null_resource_idx) {}
	virtual ~Resource() = default;

	void add_read(RenderPass* in_pass) { reads.emplace_back(in_pass); }
	void add_write(RenderPass* in_pass) { writes.emplace_back(in_pass); }
	void add_queue(RenderPassQueueFlagBits in_queue) { queue_flags |= in_queue; }
	void set_physical_index(const uint32_t in_index) { physical_index = in_index; }

	ResourceType get_type() const { return type; }
	const auto& get_name() const { return name; }
	uint32_t get_index() const { return index; }
	uint32_t get_physical_index() const { return physical_index; }
	const auto& get_reads() const { return reads; }
	const auto& get_writes() const { return writes; }
	auto get_queue_flags() const { return queue_flags; }
private:
	ResourceType type;
	std::string name;
	uint32_t index;
	std::vector<RenderPass*> reads;
	std::vector<RenderPass*> writes;
	uint32_t physical_index;
	RenderPassQueueFlags queue_flags;
};

struct ResourceHandle
{
	uint32_t index;

	ResourceHandle(const uint32_t in_index = Resource::null_resource_idx) : index(in_index) {}

	bool is_valid() const { return index != Resource::null_resource_idx; }

	operator uint32_t() const { return index; }
};

class AttachmentResource : public Resource
{
public:
	AttachmentResource(const std::string& in_name, 
		const uint32_t in_index = null_resource_idx) : Resource(ResourceType::Attachment, in_name, in_index) {}

	void add_usage(TextureUsageFlagBits in_bit) { usage_flags |= in_bit; }
	void set_info(const AttachmentInfo& in_info) { info = in_info; }

	auto get_usage_flags() const { return usage_flags; }
	const auto& get_info() const { return info; }
private:
	AttachmentInfo info;
	TextureUsageFlags usage_flags;
};

class RenderPass
{
public:
	RenderPass(RenderGraph& in_graph,
		std::string in_name,
		RenderPassQueueFlagBits in_target_queue);

	virtual void execute(CommandListHandle in_list) = 0;

	ResourceHandle add_attachment_input(const std::string& in_name);
	ResourceHandle add_attachment_input(const ResourceHandle& in_handle);
	void add_color_input(const std::string& in_name);
	ResourceHandle add_color_output(const std::string& in_name, const AttachmentInfo& in_attachment, bool in_force_load = false);
	ResourceHandle set_depth_stencil_input(const std::string& in_name);
	ResourceHandle set_depth_stencil_output(const std::string& in_name, const AttachmentInfo& in_attachment);

	bool is_color_input(const ResourceHandle in_handle) const;
	bool should_load(const ResourceHandle in_handle) const;

	void add_dependency(RenderPass* in_render_pass);
	void add_dependency(RenderPass& in_render_pass) { add_dependency(&in_render_pass); }

	const auto& get_attachment_inputs() const { return attachment_inputs; }
	const auto& get_color_inputs() const { return color_inputs; }
	const auto& get_color_outputs() const { return color_outputs; }
	const auto& get_name() const { return name; }
	const auto get_depth_stencil_input() const { return depth_stencil_input; }
	const auto get_depth_stencil_output() const { return depth_stencil_output; }
	const auto& get_dependencies() const { return dependencies; }
private:
	RenderGraph& graph;
	std::string name;
	RenderPassQueueFlagBits target_queue;
	std::vector<ResourceHandle> attachment_inputs;
	std::vector<ResourceHandle> color_inputs;
	std::vector<ResourceHandle> color_outputs;
	ResourceHandle depth_stencil_input;
	ResourceHandle depth_stencil_output;
	std::unordered_set<RenderPass*> dependencies;
	std::vector<ResourceHandle> force_loads;
};

class RenderPassSimple : public RenderPass
{
public:
	using ExecuteFunc = void(CommandListHandle);

	template<typename E>
	RenderPassSimple(RenderGraph& in_graph,
		std::string in_name,
		RenderPassQueueFlagBits in_target_queue,
		E in_execute_func) : RenderPass(in_graph, in_name, in_target_queue), execute_func(in_execute_func) {}

	void execute(CommandListHandle in_list) override
	{
		execute_func(in_list);
	}
private:
	std::function<ExecuteFunc> execute_func;
};

template<typename T>
class RenderPassPayloaded : public RenderPass
{
public:
	using ExecuteFunc = void(const T&, CommandListHandle);

	template<typename E>
	RenderPassPayloaded(RenderGraph& in_graph,
		std::string in_name,
		RenderPassQueueFlagBits in_target_queue,
		E in_execute_func) : RenderPass(in_graph, in_name, in_target_queue), execute_func(in_execute_func) {}

	void execute(CommandListHandle in_list) override
	{
		execute_func(data, in_list);
	}

	T& get_data() { return data; }
private:
	T data;
	std::function<ExecuteFunc> execute_func;
};

class RenderGraph
{
	struct PhysicalResource
	{
		std::string name;
		AttachmentInfo texture_info;
		RenderPassQueueFlags queues;
		TextureUsageFlags texture_usage_flags;
	};

	struct Barrier
	{
		ResourceHandle resource;
		TextureLayout src_layout;
		AccessFlags src_access;
		PipelineStageFlags src_stage;

		TextureLayout dst_layout;
		AccessFlags dst_access;
		PipelineStageFlags dst_stage;
	};
public:
	RenderGraph(PhysicalResourceRegistry& in_registry) : physical_resource_registry(in_registry) {}
	~RenderGraph();

	template<typename S, typename E>
	RenderPass& add_gfx_pass(const std::string& in_name,
		S in_setup_func,
		E in_execute_func)
	{
		auto render_pass = std::make_unique<RenderPassSimple>(*this, in_name, RenderPassQueueFlagBits::Gfx, in_execute_func);
		in_setup_func(*render_pass);
		render_passes.emplace_back(std::move(render_pass));
		return *render_passes.back();
	}

	template<typename T, typename S, typename E>
	RenderPass& add_gfx_pass(const std::string& in_name,
		S in_setup_func,
		E in_execute_func)
	{
		auto render_pass = std::make_unique<RenderPassPayloaded<T>>(*this, in_name, RenderPassQueueFlagBits::Gfx, in_execute_func);
		in_setup_func(*render_pass, render_pass->get_data());
		render_passes.emplace_back(std::move(render_pass));
		return *render_passes.back();
	}

	void set_backbuffer_attachment(const std::string& in_name, TextureViewHandle in_backbuffer, uint32_t in_width, uint32_t in_height);
	void compile();
	void execute(CommandListHandle in_list);

	AttachmentResource& get_attachment_resource(const std::string& in_name);
	AttachmentResource& get_attachment_resource(const ResourceHandle& in_idx) { return static_cast<AttachmentResource&>(*resources[in_idx.index]); }

	TextureViewHandle get_handle_from_resource(ResourceHandle in_resource);
	PhysicalResourceRegistry& get_registry() const { return physical_resource_registry; }
	bool depends_on_pass(RenderPass* in_dst, RenderPass* in_src) const
	{
		if (in_dst == in_src)
			return true;

		for(auto& dependency : in_dst->get_dependencies())
		{
			if (depends_on_pass(dependency, in_src))
				return true;
		}

		return false;
	}
private:
	void validate();
	void traverse_pass_dependencies(RenderPass* in_pass, uint32_t stack_depth = 0);
	void add_pass_recursive(RenderPass* in_pass, const std::vector<RenderPass*>& in_written_passes, uint32_t stack_depth);
	void prune_duplicate_passes();
	void order_passes();
	void build_physical_resources();
	void build_barriers();
	std::pair<TextureHandle, TextureViewHandle> get_handles_from_physical_attachment(uint32_t in_physical_resource);
private:
	PhysicalResourceRegistry& physical_resource_registry;
	std::vector<std::unique_ptr<RenderPass>> render_passes;
	std::vector<std::unique_ptr<Resource>> resources;
	std::vector<PhysicalResource> physical_resources;
	robin_hood::unordered_map<std::string, uint32_t> resource_map;
	std::string backbuffer_resource_name;
	TextureViewHandle backbuffer_attachment;
	uint32_t backbuffer_width;
	uint32_t backbuffer_height;
	std::vector<RenderPass*> pass_list;

	struct RenderPassBarriers
	{
		std::vector<Barrier> invalidates;
		std::vector<Barrier> flushs;
	};

	std::vector<RenderPassBarriers> render_pass_barriers;
};

}