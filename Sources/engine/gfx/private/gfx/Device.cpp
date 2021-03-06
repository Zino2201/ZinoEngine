#include "gfx/Gfx.h"

namespace ze::gfx
{

TextureViewInfo TextureViewInfo::make_2d_view(const DeviceResourceHandle& in_texture,
	const Format in_format, const TextureSubresourceRange& in_subresource)
{
	return 
	{
		TextureViewCreateInfo(Device::get().get_backend_texture(in_texture),
			gfx::TextureViewType::Tex2D,
			in_format,
			in_subresource)
	};
}

Swapchain::Swapchain(const SwapChainCreateInfo& in_info, const ResourceHandle& in_handle)
	: info(in_info), handle(in_handle) 
{
	create_texture_handles();
}

Swapchain::~Swapchain()
{
	/** For some reasons, for-each loops crashes here */
	for(size_t i = 0; i < textures.size(); ++i)
		Device::get().textures.remove(textures[i].get_index());

	for(size_t i = 0; i < texture_views.size(); ++i)
		Device::get().texture_views.remove(texture_views[i].get_index());

	textures.clear();
	texture_views.clear();

	Backend::get().swapchain_destroy(handle);
}

void Swapchain::create_texture_handles()
{
	for (size_t i = 0; i < textures.size(); ++i)
		Device::get().textures.remove(textures[i].get_index());

	for (size_t i = 0; i < texture_views.size(); ++i)
		Device::get().texture_views.remove(texture_views[i].get_index());

	textures.clear();
	texture_views.clear();

	for(const auto& texture : Backend::get().swapchain_get_backbuffer_textures(handle))
	{
		DeviceResourceHandle device_handle = DeviceResourceHandle::make(Device::get().textures.add(Texture({}, texture)),
			DeviceResourceType::Texture);
		textures.emplace_back(device_handle);
	}

	for(const auto& texture_view : Backend::get().swapchain_get_backbuffer_texture_views(handle))
	{
		DeviceResourceHandle device_handle = DeviceResourceHandle::make(Device::get().texture_views.add(TextureView({}, texture_view)),
			DeviceResourceType::TextureView);
		texture_views.emplace_back(device_handle);
	}
}

DeviceResourceHandle Swapchain::get_current_texture() const
{
	return textures[Backend::get().swapchain_get_backbuffer_index(handle)];
}

DeviceResourceHandle Swapchain::get_current_texture_view() const
{
	return texture_views[Backend::get().swapchain_get_backbuffer_index(handle)];
}

void Device::Frame::free_resources()
{
	for(const auto& buffer : expired_buffers)
	{
		Device::get().buffers.remove(buffer.get_index());
	}

	for(const auto& texture : expired_textures)
	{
		Device::get().textures.remove(texture.get_index());
	}

	for(const auto& texture_view : expired_texture_views)
	{
		Device::get().texture_views.remove(texture_view.get_index());
	}

	for(const auto& shader : expired_shaders)
	{
		Device::get().shaders.remove(shader.get_index());
	}

	for(const auto& pipeline_layout : expired_pipeline_layouts)
	{
		Device::get().pipeline_layouts.remove(pipeline_layout.get_index());
	}

	for(const auto& sampler : expired_samplers)
	{
		Device::get().samplers.remove(sampler.get_index());
	}
	
	for(const auto& swapchain : expired_swapchains)
	{
		Device::get().swapchains.remove(swapchain.get_index());
	}

	for(const auto& semaphore : expired_semaphores)
	{
		Device::get().semaphores.remove(semaphore.get_index());
	}
}

Device* device = nullptr;

Device::Device()
	: current_frame(0)
{
	device = this;
}

Device& Device::get()
{
	ZE_CHECK(device);
	return *device;
}

void Device::destroy()
{
	StaticSamplerBase::destroy_samplers();

	for(auto& frame : frames)
	{
		frame.free_resources();
		frame.reset();

		frame.gfx_command_pool.clear();
		Backend::get().fence_destroy(frame.gfx_fence);
		Backend::get().fence_destroy(frame.compute_fence);
		Backend::get().fence_destroy(frame.transfer_fence);
	}

	for(const auto& [ci, handle] : render_passes)
		Backend::get().render_pass_destroy(handle.value);
	
	for(const auto& [ci, handle] : gfx_pipelines)
		Backend::get().pipeline_destroy(handle);

	device = nullptr;

	ZE_CHECKF(buffers.is_empty(), "Some buffers have not been destroyed!");
	ZE_CHECKF(textures.is_empty(), "Some textures have not been destroyed!");
	ZE_CHECKF(texture_views.is_empty(), "Some texture views have not been destroyed!");
	ZE_CHECKF(samplers.is_empty(), "Some samplers have not been destroyed!");
	ZE_CHECKF(swapchains.is_empty(), "Some swapchains have not been destroyed!");
	ZE_CHECKF(pipeline_layouts.is_empty(), "Some pipeline layouts have not been destroyed!");
}

void Device::new_frame()
{
	Backend::get().new_frame();

	static bool first_frame = true;
	
	/** On the first frame, don't do anything. They maybe some commands list/resources queued that waits for submission */
	if(!first_frame)
	{
		current_frame = (current_frame + 1) % max_frames_in_flight;

		/** Wait for fences */
		if(!get_current_frame().wait_fences.empty())
		{
			Backend::get().fence_wait_for(get_current_frame().wait_fences);
			Backend::get().fence_reset(get_current_frame().wait_fences);
			get_current_frame().wait_fences.clear();
		}

		render_passes.new_frame();
	

		/** Destroy resources since we know they have been processed */
		get_current_frame().free_resources();

		/** 
		 * Reset command pools only if there have been submissions
		 * because they maybe submissions that have happend outside frame context (e.g game startup)
		 */
		if(get_current_frame().gfx_submitted)
		{
			get_current_frame().gfx_command_pool.new_frame();
		}

		get_current_frame().reset();
	}
	else
		first_frame = false;
}

void Device::end_frame()
{
	if(!get_current_frame().frame_cancelled)
	{
		/** Submit to queues if required */
		if(!get_current_frame().gfx_lists.empty())
		{
			submit_queue(CommandListType::Gfx);
			get_current_frame().wait_fences.emplace_back(get_current_frame().gfx_fence);
		}

	}

	/** At this point all command lists must have been submitted/dropped */
	get_current_frame().command_lists.clear();
}

void Device::cancel_frame()
{
	get_current_frame().frame_cancelled = true;
}

void Device::submit(CommandList* list, const std::vector<DeviceResourceHandle>& in_signal_semaphores)
{
	std::vector<CommandList*>* lists = nullptr;
	switch(list->get_type())
	{
	case CommandListType::Gfx:
		lists = &get_current_frame().gfx_lists;
		break;
	case CommandListType::Compute:
		lists = &get_current_frame().compute_lists;
		break;
	case CommandListType::Transfer:
		lists = &get_current_frame().transfer_lists;
		break;
	}

	list->end();
	lists->emplace_back(list);

	for(const auto& signal : in_signal_semaphores)
		get_current_frame().gfx_signal_semaphores.emplace_back(signal);
}

void Device::wait_gpu_idle()
{
	/** End current frame */
	end_frame();

	Backend::get().device_wait_idle();
	for(auto& frame : frames)
	{
		/** No fences to wait from because we are sure that the GPU does nothing, reset them */
		if(!frame.wait_fences.empty())
			Backend::get().fence_reset(frame.wait_fences);
		frame.wait_fences.clear();
		frame.trim_pools();
	}

	/** Start a new one */
	new_frame();
}

void Device::submit_queue(const CommandListType& in_type)
{
	std::vector<CommandList*>* lists = nullptr;
	ResourceHandle fence;
	std::vector<ResourceHandle> signal_semaphores;

	switch(in_type)
	{
	case CommandListType::Gfx:
		lists = &get_current_frame().gfx_lists;
		fence = get_current_frame().gfx_fence;
		get_current_frame().gfx_submitted = true;
		for(const auto& semaphore : get_current_frame().gfx_signal_semaphores)
			signal_semaphores.emplace_back(get_semaphore(semaphore)->get_handle());
		break;
	case CommandListType::Compute:
		lists = &get_current_frame().compute_lists;
		fence = get_current_frame().compute_fence;
		break;
	case CommandListType::Transfer:
		lists = &get_current_frame().transfer_lists;
		fence = get_current_frame().transfer_fence;
		break;
	}

	std::vector<ResourceHandle> handles;
	handles.reserve(lists->size());
	for(auto list : *lists)
	{
		handles.emplace_back(list->get_backend_handle());
	}

	if(!handles.empty())
	{
		// TODO: Support multiple queues
		ResourceHandle queue = Backend::get().get_gfx_queue();
		Backend::get().queue_execute(queue,
			handles,
			fence,
			{},
			{},
			signal_semaphores);
		lists->clear();
	}
}

std::pair<Result, DeviceResourceHandle> Device::create_buffer(const BufferInfo& in_info, 
	const std::span<uint8_t>& in_initial_data)
{
	ZE_CHECKF(in_info.create_info.size != 0, "Size is 0!");

	BufferInfo info = in_info;
	info.create_info.usage |= BufferUsageFlagBits::TransferSrc | BufferUsageFlagBits::TransferDst;
	auto [result, handle] = Backend::get().buffer_create(info.create_info);
	if(!handle)
		return { result, {} };
	
	DeviceResourceHandle buffer;
	{
		std::lock_guard<std::mutex> guard(resources_mutex);
		buffer = DeviceResourceHandle::make(buffers.emplace(in_info, handle),
			DeviceResourceType::Buffer);
	}

	if(!in_initial_data.empty())
	{
		if(in_info.create_info.mem_usage != MemoryUsage::CpuOnly)
		{
			auto [staging_result, staging_buf] = create_buffer(
				BufferInfo::make_staging_buffer(in_info.create_info.size),
				in_initial_data);

			CommandList* list = allocate_cmd_list(CommandListType::Gfx);
			list->copy_buffer(staging_buf, buffer);
			destroy_buffer(staging_buf);
			submit(list);
		}
		else
		{
			auto [map_result, map_data] = map_buffer(buffer);
			memcpy(map_data, in_initial_data.data(), in_initial_data.size());
			unmap_buffer(buffer);
		}
	}

	return { result, buffer };
}

std::pair<Result, void*> Device::map_buffer(const DeviceResourceHandle& in_buffer)
{
	ZE_CHECK(in_buffer && in_buffer.get_resource_type() == DeviceResourceType::Buffer);
	ZE_CHECK(buffers.is_valid(in_buffer.get_index()));

	auto& buffer = buffers[in_buffer.get_index()];
	return Backend::get().buffer_map(buffer.get_handle());
}

void Device::unmap_buffer(const DeviceResourceHandle& in_buffer)
{
	ZE_CHECK(in_buffer && in_buffer.get_resource_type() == DeviceResourceType::Buffer);
	ZE_CHECK(buffers.is_valid(in_buffer.get_index()));

	auto& buffer = buffers[in_buffer.get_index()];
	Backend::get().buffer_unmap(buffer.get_handle());
}

/** Texture */
std::pair<Result, DeviceResourceHandle> Device::create_texture(TextureInfo in_info, 
	const TextureInfo::InitialData& in_initial_data)
{
	in_info.create_info.usage_flags |= TextureUsageFlagBits::TransferSrc | TextureUsageFlagBits::TransferDst;
	auto [result, handle] = Backend::get().texture_create(in_info.create_info);
	if(!handle)
		return { result, {} };

	DeviceResourceHandle texture;
	{
		std::lock_guard<std::mutex> guard(resources_mutex);
		texture = DeviceResourceHandle::make(textures.emplace(in_info, handle),
			DeviceResourceType::Texture);
	}

	if(!in_initial_data.data.empty())
	{
		auto [staging_result, staging_buf] = create_buffer(BufferInfo::make_staging_buffer(in_initial_data.data.size()));

		auto [map_result, map_data] = map_buffer(staging_buf);
		memcpy(map_data, in_initial_data.data.data(), in_initial_data.data.size());
		unmap_buffer(staging_buf);

		/**
		 * Transition the texture to the TransferDst layout so we can copy to it and later on transfer it
		 * to a user-specified layout
		 */
		CommandList* list = allocate_cmd_list(CommandListType::Gfx);
		list->texture_barrier(texture,
			PipelineStageFlagBits::TopOfPipe,
			AccessFlags(),
			TextureLayout::Undefined,
			PipelineStageFlagBits::Transfer,
			AccessFlagBits::TransferWrite,
			TextureLayout::TransferDst);
		list->copy_buffer_to_texture(staging_buf, texture,
			BufferTextureCopyRegion(0,
				TextureSubresourceLayers(
					TextureAspectFlagBits::Color,
					0,
					0,
					1),
				Offset3D(),
				Extent3D(in_info.create_info.width,
					in_info.create_info.height,
					in_info.create_info.depth)));
		list->texture_barrier(texture,
			PipelineStageFlagBits::Transfer,
			AccessFlagBits::TransferWrite,
			TextureLayout::TransferDst,
			in_initial_data.dst_pipeline_flags,
			in_initial_data.dst_access_flags,
			in_initial_data.dst_layout);
		submit(list);
		destroy_buffer(staging_buf);
	}

	return { result, texture };
}

/** Texture view */
std::pair<Result, DeviceResourceHandle> Device::create_texture_view(const TextureViewInfo& in_info)
{
	auto [result, handle] = Backend::get().texture_view_create(in_info.create_info);
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(texture_views.emplace(in_info, handle), DeviceResourceType::TextureView) };
}

/** Shader */
std::pair<Result, DeviceResourceHandle> Device::create_shader(const ShaderCreateInfo& in_info)
{
	auto [result, handle] = Backend::get().shader_create(in_info);
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(shaders.emplace(in_info, handle), DeviceResourceType::Shader) };
}

/** Pipeline layout */
std::pair<Result, DeviceResourceHandle> Device::create_pipeline_layout(const gfx::PipelineLayoutCreateInfo& in_info)
{
	auto [result, handle] = Backend::get().pipeline_layout_create(in_info);
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(pipeline_layouts.emplace(in_info, handle), DeviceResourceType::PipelineLayout) };
}

/** Sampler */
std::pair<Result, DeviceResourceHandle> Device::create_sampler(const gfx::SamplerCreateInfo& in_info)
{
	auto [result, handle] = Backend::get().sampler_create(in_info);
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(samplers.emplace(in_info, handle), DeviceResourceType::Sampler) };
}

/** Swapchain */
std::pair<Result, DeviceResourceHandle> Device::create_swapchain(const gfx::SwapChainCreateInfo& in_info)
{
	auto [result, handle] = Backend::get().swapchain_create(in_info);
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(swapchains.emplace(in_info, handle), DeviceResourceType::SwapChain) };
}

std::pair<Result, DeviceResourceHandle> Device::create_semaphore()
{
	auto [result, handle] = Backend::get().semaphore_create();
	if(!handle)
		return { result, {} };

	std::lock_guard<std::mutex> guard(resources_mutex);
	return { result, DeviceResourceHandle::make(semaphores.emplace(handle), DeviceResourceType::Semaphore) };
}

bool Device::acquire_swapchain_texture(const DeviceResourceHandle& in_swapchain)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	return Backend::get().swapchain_acquire_image(swapchain->get_handle());
}

void Device::resize_swapchain(const DeviceResourceHandle& in_swapchain, const uint32_t in_width,
	const uint32_t in_height)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	Backend::get().swapchain_resize(swapchain->get_handle(), in_width, in_height);
	swapchain->create_texture_handles();
}

DeviceResourceHandle Device::get_swapchain_backbuffer_texture(const DeviceResourceHandle& in_swapchain)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	return swapchain->get_current_texture();
}

DeviceResourceHandle Device::get_swapchain_backbuffer_texture_view(const DeviceResourceHandle& in_swapchain)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	return swapchain->get_current_texture_view();
}

uint32_t Device::get_swapchain_texture_count(const DeviceResourceHandle& in_swapchain)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	return swapchain->get_swapchain_texture_count();
}

uint32_t Device::get_swapchain_current_idx(const DeviceResourceHandle& in_swapchain)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);
	return swapchain->get_current_idx();
}

void Device::present(const DeviceResourceHandle& in_swapchain, const std::vector<DeviceResourceHandle>& in_wait_semaphores)
{
	Swapchain* swapchain = get_swapchain(in_swapchain);

	std::vector<ResourceHandle> handles;
	handles.reserve(in_wait_semaphores.size());
	for(const auto& semaphore : in_wait_semaphores)
		handles.emplace_back(get_semaphore(semaphore)->get_handle());

	Backend::get().swapchain_present(swapchain->get_handle(), handles);
}

/** Cmd */
CommandList* Device::allocate_cmd_list(const CommandListType& in_type)
{
	CommandList* list = get_current_frame().gfx_command_pool.allocate_cmd_list();
	list->begin();
	return list;
}

ResourceHandle Device::create_or_find_render_pass(const RenderPassCreateInfo& in_create_info)
{
	auto render_pass = render_passes.find(in_create_info);
	if(render_pass)
		return ResourceHandle(render_pass->handle);

	std::lock_guard<std::mutex> guard(resources_mutex);
	auto [result, handle] = Backend::get().render_pass_create(in_create_info);
	render_passes.insert(in_create_info, { handle });
	return handle;
}

ResourceHandle Device::create_or_find_gfx_pipeline(const GfxPipelineRenderPassState& in_render_pass_state,
	const GfxPipelineInstanceState& in_instance_state, ResourceHandle in_render_pass, ResourceHandle in_pipeline_layout)
{
	GfxPipelineCreateInfo create_info;
	create_info.multisampling_state = in_render_pass_state.multisampling;
	create_info.depth_stencil_state = in_render_pass_state.depth_stencil;
	create_info.color_blend_state = in_render_pass_state.color_blend;
	create_info.render_pass = in_render_pass;

	for(const auto& stage : in_instance_state.shaders) 
		create_info.shader_stages.emplace_back(stage.stage, 
			get_shader(stage.shader)->get_handle(), stage.entry_point);
	create_info.vertex_input_state = in_instance_state.vertex_input;
	create_info.input_assembly_state = in_instance_state.input_assembly;
	create_info.rasterization_state = in_instance_state.rasterization;
	create_info.pipeline_layout = in_pipeline_layout;

	auto pipeline = gfx_pipelines.find(create_info);
	if(pipeline != gfx_pipelines.end())
		return pipeline->second;

	std::lock_guard<std::mutex> guard(resources_mutex);
	auto [result, handle] = Backend::get().gfx_pipeline_create(create_info);
	gfx_pipelines.insert({ create_info, handle });
	return handle;
}

void Device::destroy_buffer(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_buffers.emplace_back(in_handle);
}

void Device::destroy_texture(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_textures.emplace_back(in_handle);
}

void Device::destroy_texture_view(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_texture_views.emplace_back(in_handle);
}

void Device::destroy_shader(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_shaders.emplace_back(in_handle);
}

void Device::destroy_pipeline_layout(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_pipeline_layouts.emplace_back(in_handle);
}

void Device::destroy_sampler(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_samplers.emplace_back(in_handle);
}

void Device::destroy_swapchain(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_swapchains.emplace_back(in_handle);
}

void Device::destroy_semaphore(const DeviceResourceHandle& in_handle)
{
	std::lock_guard<std::mutex> guard(get_current_frame().frame_lock);
	get_current_frame().expired_semaphores.emplace_back(in_handle);
}

/** Useful getters */
ResourceHandle Device::get_backend_texture(const DeviceResourceHandle& in_texture) const
{
	ZE_CHECK(in_texture && in_texture.get_resource_type() == DeviceResourceType::Texture);
	ZE_CHECK(textures.is_valid(in_texture.get_index()));

	auto& texture = textures[in_texture.get_index()];
	return texture.get_handle();
}

ResourceHandle Device::get_backend_texture_view(const DeviceResourceHandle& in_texture_view) const
{
	ZE_CHECK(in_texture_view && in_texture_view.get_resource_type() == DeviceResourceType::TextureView);
	ZE_CHECK(texture_views.is_valid(in_texture_view.get_index()));

	auto& texture_view = texture_views[in_texture_view.get_index()];
	return texture_view.get_handle();
}

Buffer* Device::get_buffer(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::Buffer && buffers.is_valid(in_handle.get_index()));
	Buffer& buffer = buffers[in_handle.get_index()];
	return &buffer;
}

PipelineLayout* Device::get_pipeline_layout(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::PipelineLayout && pipeline_layouts.is_valid(in_handle.get_index()));
	PipelineLayout& pipeline_layout = pipeline_layouts[in_handle.get_index()];
	return &pipeline_layout;
}

Sampler* Device::get_sampler(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::Sampler && samplers.is_valid(in_handle.get_index()));
	Sampler& sampler = samplers[in_handle.get_index()];
	return &sampler;
}

Texture* Device::get_texture(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::Texture && textures.is_valid(in_handle.get_index()));
	Texture& texture = textures[in_handle.get_index()];
	return &texture;
}

TextureView* Device::get_texture_view(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::TextureView && texture_views.is_valid(in_handle.get_index()));
	TextureView& view = texture_views[in_handle.get_index()];
	return &view;
}

Swapchain* Device::get_swapchain(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::SwapChain && swapchains.is_valid(in_handle.get_index()));
	Swapchain& swapchain = swapchains[in_handle.get_index()];
	return &swapchain;
}

Shader* Device::get_shader(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::Shader && shaders.is_valid(in_handle.get_index()));
	Shader& shader = shaders[in_handle.get_index()];
	return &shader;
}

Semaphore* Device::get_semaphore(const DeviceResourceHandle& in_handle)
{
	ZE_CHECK(in_handle.get_resource_type() == DeviceResourceType::Semaphore && semaphores.is_valid(in_handle.get_index()));
	Semaphore& semaphore = semaphores[in_handle.get_index()];
	return &semaphore;
}

}