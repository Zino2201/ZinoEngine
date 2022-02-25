#include "engine/gfx/rendergraph/render_graph.hpp"

namespace ze::gfx::rendergraph
{

RenderPass::RenderPass(RenderGraph& in_graph,
	std::string in_name,
	RenderPassQueueFlagBits in_target_queue) : graph(in_graph), name(in_name), target_queue(in_target_queue) {}

ResourceHandle RenderPass::add_attachment_input(const std::string& in_name)
{
	auto& resource = graph.get_attachment_resource(in_name);
	ZE_CHECKF(!(resource.get_usage_flags() & TextureUsageFlagBits::DepthStencilAttachment), 
		"Use set_depth_stencil_input for depth/stencil inputs");

	resource.add_usage(TextureUsageFlagBits::Sampled);
	resource.add_read(this);
	resource.add_queue(target_queue);

	attachment_inputs.emplace_back(resource.get_index());

	return resource.get_index();
}

void RenderPass::add_color_input(const std::string& in_name)
{
	auto& resource = graph.get_attachment_resource(in_name);
	resource.add_read(this);
	ZE_CHECK(resource.get_usage_flags() & TextureUsageFlagBits::ColorAttachment);
	color_inputs.emplace_back(resource.get_index());
}

ResourceHandle RenderPass::add_color_output(const std::string& in_name, const AttachmentInfo& in_attachment, bool in_force_load)
{
	auto& resource = graph.get_attachment_resource(in_name);
	resource.set_info(in_attachment);
	resource.add_usage(TextureUsageFlagBits::ColorAttachment);
	resource.add_write(this);
	resource.add_queue(target_queue);

	if (in_force_load)
		force_loads.emplace_back(resource.get_index());

	color_outputs.emplace_back(resource.get_index());

	return resource.get_index();
}

ResourceHandle RenderPass::set_depth_stencil_input(const std::string& in_name)
{
	ZE_CHECK(depth_stencil_output == Resource::null_resource_idx);

	auto& resource = graph.get_attachment_resource(in_name);
	resource.add_usage(TextureUsageFlagBits::Sampled);
	resource.add_read(this);
	resource.add_queue(target_queue);

	depth_stencil_input = resource.get_index();
	return resource.get_index();
}

ResourceHandle RenderPass::set_depth_stencil_output(const std::string& in_name, const AttachmentInfo& in_attachment)
{
	ZE_CHECK(depth_stencil_input == Resource::null_resource_idx);

	auto& resource = graph.get_attachment_resource(in_name);
	resource.set_info(in_attachment);
	resource.add_usage(TextureUsageFlagBits::DepthStencilAttachment);
	resource.add_write(this);
	resource.add_queue(target_queue);

	depth_stencil_output = resource.get_index();
	return resource.get_index();
}

bool RenderPass::is_color_input(const ResourceHandle in_handle) const
{
	for (const auto& input : color_inputs)
	{
		if (input == in_handle)
			return true;
	}

	return false;
}

bool RenderPass::should_load(const ResourceHandle in_handle) const
{
	if (is_color_input(in_handle))
		return true;

	for (const auto& input : force_loads)
	{
		if (input == in_handle)
			return true;
	}

	return false;
}

}