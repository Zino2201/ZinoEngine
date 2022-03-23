#include "engine/gfx/rendergraph/render_graph.hpp"
#include "engine/gfx/rendergraph/resource_registry.hpp"

namespace ze::gfx::rendergraph
{

RenderGraph::~RenderGraph() = default;

AttachmentResource& RenderGraph::get_attachment_resource(const std::string& in_name)
{
	auto it = resource_map.find(in_name);
	if(it != resource_map.end())
	{
		ZE_CHECK(resources[it->second]->get_type() == ResourceType::Attachment);
		return static_cast<AttachmentResource&>(*resources[it->second]);
	}

	const uint32_t index = static_cast<uint32_t>(resources.size());
	auto& resource = resources.emplace_back(new AttachmentResource(in_name, index));
	resource_map[in_name] = index;
	return static_cast<AttachmentResource&>(*resource);
}

TextureViewHandle RenderGraph::get_handle_from_resource(ResourceHandle in_resource)
{
	const auto& resource = resources[in_resource];
	return get_handles_from_physical_attachment(resource->get_physical_index()).second;
}

void RenderGraph::set_backbuffer_attachment(const std::string& in_name, TextureViewHandle in_backbuffer, uint32_t in_width, uint32_t in_height)
{
	backbuffer_resource_name = in_name;
	backbuffer_attachment = in_backbuffer;
	backbuffer_width = in_width;
	backbuffer_height = in_height;
}

void RenderGraph::validate()
{
	ZE_CHECK(!backbuffer_resource_name.empty());
	ZE_CHECK(resource_map.contains(backbuffer_resource_name));
}

void RenderGraph::compile()
{
#if ZE_BUILD(IS_DEBUG)
	validate();
#endif

	Resource& backbuffer = *resources[resource_map[backbuffer_resource_name]];

	/** Start from render passes that write to the backbuffer and traverse the passes dependencies */
	for (auto& pass : backbuffer.get_writes())
	{
		pass_list.emplace_back(pass);
	}

	/** Traverse each pass dependencies recursively to get a (reversed) unordered list of referenced render passes */
	{
		const auto pass_list_copy = pass_list;
		for(auto& pass : pass_list_copy)
			traverse_pass_dependencies(pass);
	}

	/** Reverse, prune & order all render passes */
	std::ranges::reverse(pass_list);
	prune_duplicate_passes();
	order_passes();

	/** Build all physical resources, this will also alias any virtual resources if possible */
	build_physical_resources();

	build_barriers();
}

void RenderGraph::traverse_pass_dependencies(RenderPass* in_pass, uint32_t stack_depth)
{
	if(in_pass->get_depth_stencil_input() != Resource::null_resource_idx)
	{
		const auto& resource = *resources[in_pass->get_depth_stencil_input()];
		add_pass_recursive(in_pass, resource.get_writes(), stack_depth);
	}

	for(auto input : in_pass->get_attachment_inputs())
	{
		const auto& resource = *resources[input];
		if(std::ranges::find(in_pass->get_color_outputs(), input) == in_pass->get_color_outputs().end())
			add_pass_recursive(in_pass, resource.get_writes(), stack_depth);
	}

	for(auto input : in_pass->get_color_inputs())
	{
		const auto& resource = *resources[input];
		add_pass_recursive(in_pass, resource.get_writes(), stack_depth);
	}
}

void RenderGraph::add_pass_recursive(RenderPass* in_pass,
	const std::vector<RenderPass*>& in_written_passes, 
	uint32_t stack_depth)
{
	ZE_CHECK(stack_depth < render_passes.size());

	for (auto& dep : in_written_passes)
		if (dep != in_pass)
			in_pass->add_dependency(dep);

	for(auto pass : in_written_passes)
	{
		ZE_CHECK(pass != in_pass);

		pass_list.emplace_back(pass);
		traverse_pass_dependencies(pass, stack_depth + 1);
	}
}

void RenderGraph::prune_duplicate_passes()
{
	robin_hood::unordered_set<RenderPass*> handles;

	auto [begin, end] = std::ranges::remove_if(pass_list,
		[&](RenderPass* in_pass)
		{
			if (handles.contains(in_pass))
				return true;

			handles.insert(in_pass);
			return false;
		});

	pass_list.erase(begin, end);
}

void RenderGraph::order_passes()
{
	std::vector<RenderPass*> passes = std::move(pass_list);

	auto schedule = [&](const size_t in_index)
	{
		pass_list.emplace_back(passes[in_index]);
		std::ranges::move(passes.begin() + in_index + 1,
			passes.end(),
			passes.begin() + in_index);
		passes.pop_back();
	};

	schedule(0);
	while(!passes.empty())
	{
		size_t pass_to_schedule = 0;

		for(size_t i = 0; i < passes.size(); ++i)
		{
			bool candidate = true;

			for(size_t j = 0; j < i; ++j)
			{
				if(depends_on_pass(passes[i], passes[j]))
				{
					candidate = false;
					break;
				}
			}

			if (!candidate)
				continue;

			pass_to_schedule = i;
		}

		schedule(pass_to_schedule);
	}
}

void RenderGraph::build_physical_resources()
{
	uint32_t current_physical_index = 0;

	for(auto pass : pass_list)
	{
		for(auto output_handle : pass->get_color_outputs())
		{
			auto& output = static_cast<AttachmentResource&>(*resources[output_handle]);
			if(output.get_physical_index() != Resource::null_resource_idx)
			{
				physical_resources[output.get_physical_index()].queues |= output.get_queue_flags();
				physical_resources[output.get_physical_index()].texture_usage_flags |= output.get_usage_flags();
			}
			else
			{
				output.set_physical_index(current_physical_index++);
				auto& phys_resource = physical_resources.emplace_back();
				phys_resource.name = output.get_name();
				phys_resource.texture_info = output.get_info();
				phys_resource.queues = output.get_queue_flags();
				phys_resource.texture_usage_flags = output.get_usage_flags();
			}
		}

		if(pass->get_depth_stencil_output() != Resource::null_resource_idx)
		{
			auto& depth_stencil = static_cast<AttachmentResource&>(*resources[pass->get_depth_stencil_output()]);
			depth_stencil.set_physical_index(current_physical_index++);
			auto& phys_resource = physical_resources.emplace_back();
			phys_resource.name = depth_stencil.get_name();
			phys_resource.texture_info = depth_stencil.get_info();
			phys_resource.queues = depth_stencil.get_queue_flags();
			phys_resource.texture_usage_flags = depth_stencil.get_usage_flags();
		}
	}
}

void RenderGraph::build_barriers()
{
	struct ResourceState
	{
		TextureLayout src_layout;
		TextureLayout dst_layout;
		AccessFlags invalidate_access;
		PipelineStageFlags invalidate_stages;

		ResourceState() :
			src_layout(TextureLayout::Undefined),
			dst_layout(TextureLayout::Undefined) {}
	};

	std::vector<ResourceState> states;
	states.reserve(physical_resources.size());

	for(auto pass : pass_list)
	{
		states.clear();
		states.resize(physical_resources.size());

		RenderPassBarriers barriers;

		for(auto input : pass->get_attachment_inputs())
		{
			auto& resource = static_cast<AttachmentResource&>(*resources[input]);
			if (resource.get_usage_flags() & TextureUsageFlagBits::DepthStencilAttachment)
				continue;

			barriers.flushs.emplace_back(input,
				TextureLayout::ColorAttachment,
				AccessFlagBits::ColorAttachmentWrite,
				PipelineStageFlagBits::ColorAttachmentOutput,
				TextureLayout::ColorAttachment,
				AccessFlagBits::InputAttachmentRead,
				PipelineStageFlagBits::FragmentShader);
		}

		for (auto output : pass->get_color_outputs())
		{
			barriers.flushs.emplace_back(output,
				TextureLayout::Undefined,
				AccessFlags(),
				PipelineStageFlagBits::FragmentShader,
				TextureLayout::ColorAttachment,
				AccessFlagBits::ColorAttachmentWrite,
				PipelineStageFlagBits::ColorAttachmentOutput);
		}

		render_pass_barriers.emplace_back(std::move(barriers));
	}
}

void RenderGraph::execute(CommandListHandle in_list)
{
	get_device()->cmd_begin_region(in_list, "Render Graph", { 0.5f, 0.35f, 0.75f, 1 });

	for(size_t i = 0; i < pass_list.size(); ++i)
	{
		auto pass = pass_list[i];

		for(const auto& barrier : render_pass_barriers[i].flushs)
		{
			const auto& resource = resources[barrier.resource];
			if (resource->get_name() == backbuffer_resource_name)
				continue;

			get_device()->cmd_texture_barrier(in_list,
				get_handles_from_physical_attachment(resources[barrier.resource]->get_physical_index()).first,
				barrier.src_stage,
				barrier.src_layout,
				barrier.src_access,
				barrier.dst_stage,
				barrier.dst_layout,
				barrier.dst_access);
		}

		/** Setup render pass */
		RenderPassInfo info;
		std::vector<RenderPassInfo::Subpass> subpasses;
		std::vector<ClearValue> clear_values;

		std::vector<TextureViewHandle> color_attachments;
		std::vector<uint32_t> color_attachments_refs;
		std::vector<uint32_t> input_attachments_refs;

		for(const auto attachment : pass->get_attachment_inputs())
		{
			const auto& resource = resources[attachment];
			const auto& physical_resource = physical_resources[resource->get_physical_index()];
			if (physical_resource.texture_usage_flags & TextureUsageFlagBits::DepthStencilAttachment)
				continue;

			if (resource->get_name() == "backbuffer")
			{
				color_attachments.emplace_back(backbuffer_attachment);
			}
			else
			{
				color_attachments.emplace_back(get_handles_from_physical_attachment(resource->get_physical_index()).second);
			}

			clear_values.emplace_back(ClearColorValue({ 0, 0, 0, 1 }));
			input_attachments_refs.emplace_back(color_attachments.size() - 1);
		}

		for(const auto attachment : pass->get_color_outputs())
		{
			const auto& resource = resources[attachment];
			const auto& physical_resource = physical_resources[resource->get_physical_index()];
			const uint32_t width = physical_resource.texture_info.width == 0 ? backbuffer_width : physical_resource.texture_info.width;
			const uint32_t height = physical_resource.texture_info.height == 0 ? backbuffer_height : physical_resource.texture_info.height;

			if(resource->get_name() == "backbuffer")
			{
				color_attachments.emplace_back(backbuffer_attachment);
			}
			else
			{
				color_attachments.emplace_back(get_handles_from_physical_attachment(resource->get_physical_index()).second);
			}

			color_attachments_refs.emplace_back(color_attachments.size() - 1);
			info.render_area.width = std::max<uint32_t>(info.render_area.width, width);
			info.render_area.height = std::max<uint32_t>(info.render_area.height, height);

			if (pass->should_load(attachment))
			{
				info.load_attachment_flags |= 1 << (color_attachments.size() - 1);
			}
			else
			{
				info.clear_attachment_flags |= 1 << (color_attachments.size() - 1);
				clear_values.emplace_back(ClearColorValue({ 0, 0, 0, 1 }));
			}

			info.store_attachment_flags |= 1 << (color_attachments.size() - 1);
		}

		RenderPassInfo::DepthStencilMode depth_stencil_mode = RenderPassInfo::DepthStencilMode::ReadOnly;
		if (pass->get_depth_stencil_output() != Resource::null_resource_idx)
		{
			clear_values.emplace_back(ClearDepthStencilValue(0.f, 0));
			depth_stencil_mode = RenderPassInfo::DepthStencilMode::ReadWrite;
		}

		/** Create subpasses */
		subpasses.push_back(RenderPassInfo::Subpass{ color_attachments_refs,
			input_attachments_refs,
			{},
			depth_stencil_mode });

		info.color_attachments = color_attachments;
		info.subpasses = subpasses;
		info.clear_values = clear_values;

		if (pass->get_depth_stencil_input() != Resource::null_resource_idx)
		{
			const auto& resource = resources[pass->get_depth_stencil_input()];
			info.depth_stencil_attachment = get_handles_from_physical_attachment(resource->get_physical_index()).second;
			info.depth_attachment_read_only = true;
		}

		if (pass->get_depth_stencil_output() != Resource::null_resource_idx)
		{
			const auto& resource = resources[pass->get_depth_stencil_output()];
			const auto& physical_resource = physical_resources[resource->get_physical_index()];

			info.depth_stencil_attachment = get_handles_from_physical_attachment(resource->get_physical_index()).second;
			info.render_area.width = std::max<uint32_t>(info.render_area.width, physical_resource.texture_info.width);
			info.render_area.height = std::max<uint32_t>(info.render_area.height, physical_resource.texture_info.height);
		}

		get_device()->cmd_begin_region(in_list, pass->get_name(), { 0.5f, 0.75f, 0.35f, 1 });
		get_device()->cmd_begin_render_pass(in_list, info);
		pass->execute(in_list);
		get_device()->cmd_end_render_pass(in_list);
		get_device()->cmd_end_region(in_list);
	}

	get_device()->cmd_end_region(in_list);
}

std::pair<TextureHandle, TextureViewHandle> RenderGraph::get_handles_from_physical_attachment(uint32_t in_physical_resource)
{
	const auto& physical_resource = physical_resources[in_physical_resource];
	return physical_resource_registry.get_texture_handles(physical_resource.name, TextureInfo{
		TextureCreateInfo
		{
			TextureType::Tex2D,
			MemoryUsage::GpuOnly,
			physical_resource.texture_info.format,
			physical_resource.texture_info.width == 0 ? backbuffer_width : physical_resource.texture_info.width,
			physical_resource.texture_info.height == 0 ? backbuffer_height : physical_resource.texture_info.height,
			1,
			1,
			1,
			SampleCountFlagBits::Count1,
			physical_resource.texture_usage_flags
		} }.set_debug_name(physical_resource.name));
}

}
