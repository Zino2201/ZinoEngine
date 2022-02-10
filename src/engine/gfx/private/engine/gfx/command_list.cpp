#include "engine/gfx/device.hpp"

namespace ze::gfx::detail
{

CommandList::CommandList(Device& in_device,
	const BackendDeviceResource& in_list,
	const QueueType& in_type,
	const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_list, in_debug_name),
	type(in_type), render_pass(null_backend_resource), pipeline_state_dirty(false), has_compute_stage(false), dirty_sets_mask(0)
{
	bound_descriptor_sets.reserve(max_descriptor_sets);
	reset();
}

void CommandList::reset()
{
	for (auto& set : descriptors)
		for (auto& descriptor : set)
			descriptor = Descriptor();

	stages.clear();
	has_compute_stage = false;
	pipeline_layout = PipelineLayoutHandle();
	render_pass = null_backend_resource;

	static std::array default_blend_attachment_states = { PipelineColorBlendAttachmentState() };
	color_blend_state = { false, LogicOp::NoOp, default_blend_attachment_states };
	depth_stencil_state = {};
	multisampling_state = {};
	vertex_input_state = {};
	input_assembly_state = {};
	rasterizer_state = {};
}

void CommandList::prepare_draw()
{
	if (pipeline_state_dirty)
		update_pipeline_state();

	update_descriptors();
}

void CommandList::set_render_pass(const BackendDeviceResource& in_handle)
{
	render_pass = in_handle;
}

void CommandList::set_pipeline_layout(const PipelineLayoutHandle& in_handle)
{
	pipeline_layout = in_handle;
	pipeline_state_dirty = true;
}

void CommandList::update_pipeline_state()
{
	ZE_CHECKF(pipeline_layout, "No pipeline layout was bound!");

	if (has_compute_stage)
	{
		ComputePipelineCreateInfo create_info(stages[0],
			Device::cast_handle<PipelineLayout>(pipeline_layout)->get_resource());

		auto pipeline = device.get_or_create_compute_pipeline(create_info);
		device.get_backend_device()->cmd_bind_pipeline(
			resource,
			PipelineBindPoint::Compute,
			pipeline);
	}
	else
	{
		GfxPipelineCreateInfo create_info(stages,
			vertex_input_state,
			input_assembly_state,
			rasterizer_state,
			multisampling_state,
			depth_stencil_state,
			color_blend_state,
			Device::cast_handle<PipelineLayout>(pipeline_layout)->get_resource(),
			render_pass,
			0);

		ZE_CHECKF(render_pass, "No render pass set!");

		auto pipeline = device.get_or_create_gfx_pipeline(create_info);

		device.get_backend_device()->cmd_bind_pipeline(
			resource,
			PipelineBindPoint::Gfx,
			pipeline);
	}

	pipeline_state_dirty = false;
}

void CommandList::update_descriptors()
{
	if (dirty_sets_mask == 0)
		return;

	auto layout = Device::cast_handle<PipelineLayout>(pipeline_layout);
	bound_descriptor_sets.clear();

	for (uint32_t i = 0; i < max_descriptor_sets; ++i)
	{
		if (dirty_sets_mask & (1 << i))
		{
			auto result = device.get_backend_device()->allocate_descriptor_set(layout->get_resource(),
				i,
				descriptors[i]);
			bound_descriptor_sets.emplace_back(result.get_value());
		}
	}

	device.get_backend_device()->cmd_bind_descriptor_sets(resource,
		has_compute_stage ? PipelineBindPoint::Compute : PipelineBindPoint::Gfx,
		layout->get_resource(),
		bound_descriptor_sets);
}

void CommandList::set_descriptor(uint32_t in_set, uint32_t in_binding, const Descriptor& in_descriptor)
{
	descriptors[in_set][in_binding] = in_descriptor;
	dirty_sets_mask = 1 << in_set;
}

void CommandList::set_depth_stencil_state(const PipelineDepthStencilStateCreateInfo& in_state)
{
	depth_stencil_state = in_state;
	pipeline_state_dirty = true;
}

void CommandList::set_multisampling_state(const PipelineMultisamplingStateCreateInfo& in_info)
{
	multisampling_state = in_info;
	pipeline_state_dirty = true;
}

void CommandList::set_color_blend_state(const PipelineColorBlendStateCreateInfo& in_info)
{
	color_blend_state = in_info;
	pipeline_state_dirty = true;
}

void CommandList::set_vertex_input_state(const PipelineVertexInputStateCreateInfo& in_info)
{
	vertex_input_state = in_info;
	pipeline_state_dirty = true;
}

void CommandList::set_input_assembly_state(const PipelineInputAssemblyStateCreateInfo& in_info)
{
	input_assembly_state = in_info;
	pipeline_state_dirty = true;
}

void CommandList::set_rasterization_state(const PipelineRasterizationStateCreateInfo& in_info)
{
	rasterizer_state = in_info;
	pipeline_state_dirty = true;
}

void CommandList::bind_shader(const PipelineShaderStage& in_stage)
{
	if(in_stage.shader_stage == ShaderStageFlagBits::Compute)
	{
		stages.clear();
		stages.emplace_back(in_stage);
		has_compute_stage = true;
	}
	else
	{
		if(has_compute_stage)
			stages.clear();
		has_compute_stage = false;

		bool found_stage = false;
		for (auto& stage : stages)
		{
			if (stage.shader_stage == in_stage.shader_stage)
			{
				stage = in_stage;
				found_stage = true;
				break;
			}
		}

		if (!found_stage)
			stages.emplace_back(in_stage);
	}
}

}