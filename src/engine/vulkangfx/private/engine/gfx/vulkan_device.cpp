#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_pipeline_layout.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_command_list.hpp"
#include "vulkan_texture_view.hpp"
#include "vulkan_sync.hpp"
#include "vulkan_sampler.hpp"

namespace ze::gfx
{

VulkanDevice::VmaAllocatorWrapper::VmaAllocatorWrapper(VulkanDevice& in_device) : device(in_device)
{
	VmaAllocatorCreateInfo create_info = {};
	create_info.instance = device.get_backend().get_instance();
	create_info.device = device.get_device();
	create_info.physicalDevice = device.get_physical_device();

	VkResult result = vmaCreateAllocator(&create_info, &allocator);
	if (result != VK_SUCCESS)
		logger::fatal(log_vulkan, "Failed to create memory allocator");
}

VulkanDevice::VmaAllocatorWrapper::~VmaAllocatorWrapper()
{
	vmaDestroyAllocator(allocator);
}

/** FramebufferManager */
void VulkanDevice::FramebufferManager::new_frame()
{
	std::vector<Framebuffer> expired_framebuffers;
	for(auto& it : framebuffers)
	{
		it.second.unpolled_frames++;
		if(it.second.unpolled_frames == framebuffer_expiration_frames)
			expired_framebuffers.push_back(it.first);
	}

	for(const auto& framebuffer : expired_framebuffers)
		framebuffers.erase(framebuffer);
}

VkFramebuffer VulkanDevice::FramebufferManager::get_or_create(VkRenderPass in_render_pass, const Framebuffer& in_framebuffer)
{
	auto it = framebuffers.find(in_framebuffer);
	if(it != framebuffers.end())
	{
		it->second.unpolled_frames = 0;
		return it->second.framebuffer;
	}
	
	VkFramebufferCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.renderPass = in_render_pass;
	create_info.width = in_framebuffer.width;
	create_info.height = in_framebuffer.height;
	create_info.layers = 1;

	std::vector<VkImageView> views;
	views.reserve(in_framebuffer.attachments.size());

	for(const auto& attachment : in_framebuffer.attachments)
	{
		views.emplace_back(get_resource<VulkanTextureView>(attachment)->get_image_view());
	}

	create_info.attachmentCount = static_cast<uint32_t>(views.size());
	create_info.pAttachments = views.data();

	VkFramebuffer framebuffer;
	vkCreateFramebuffer(device.get_device(),
		&create_info,
		nullptr,
		&framebuffer);
	framebuffers.insert({ in_framebuffer, Entry(device, framebuffer) });
	
	return framebuffer;
}

VulkanDevice::VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device) :
	backend(in_backend),
	device_wrapper(DeviceWrapper(std::move(in_device))),
	vma_allocator(*this),
	surface_manager(*this),
	framebuffer_manager(*this),
	descriptor_manager(*this)
{
	vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
		vkGetDeviceProcAddr(get_device(), "vkCmdBeginDebugUtilsLabelEXT"));
	vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
		vkGetDeviceProcAddr(get_device(), "vkCmdEndDebugUtilsLabelEXT"));
}
	
VulkanDevice::~VulkanDevice() = default;

void VulkanDevice::new_frame()
{
	framebuffer_manager.new_frame();
}

void VulkanDevice::set_resource_name(const std::string_view& in_name, 
	const DeviceResourceType in_type, 
	const BackendDeviceResource in_handle) 
{
	if(!backend.has_debug_layers())
		return;

	VkDebugUtilsObjectNameInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info.pNext = nullptr;
	info.objectType = convert_object_type(in_type);
	info.pObjectName = in_name.data();

	switch(in_type)
	{
	case DeviceResourceType::Buffer:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanBuffer>(in_handle)->get_buffer());
		break;
	case DeviceResourceType::Texture:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanTexture>(in_handle)->get_texture());
		break;
	case DeviceResourceType::TextureView:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanTextureView>(in_handle)->get_image_view());
		break;
	case DeviceResourceType::Sampler:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanSampler>(in_handle)->get_sampler());
		break;
	case DeviceResourceType::Swapchain:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanSwapChain>(in_handle)->get_swapchain().swapchain);
		break;
	case DeviceResourceType::Pipeline:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanPipeline>(in_handle)->get_pipeline());
		break;
	case DeviceResourceType::PipelineLayout:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanPipelineLayout>(in_handle)->get_pipeline_layout());
		break;
	case DeviceResourceType::Shader:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanShader>(in_handle)->shader_module);
		break;
	case DeviceResourceType::CommandList:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanCommandList>(in_handle)->get_command_buffer());
		break;
	case DeviceResourceType::Fence:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanFence>(in_handle)->get_fence());
		break;
	case DeviceResourceType::Semaphore:
		info.objectHandle = reinterpret_cast<uint64_t>(get_resource<VulkanSemaphore>(in_handle)->get_semaphore());
		break;
	}

	backend.vkSetDebugUtilsObjectNameEXT(get_device(), &info);	
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_buffer(const BufferCreateInfo& in_create_info)
{
	ZE_CHECK(in_create_info.size != 0 && in_create_info.usage_flags != BufferUsageFlags());

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = nullptr;
	buffer_create_info.size = in_create_info.size;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.usage = 0;
	buffer_create_info.flags = 0;

	/** Usage flags */
	if(in_create_info.usage_flags & BufferUsageFlagBits::VertexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::IndexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::StorageBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::TransferSrc)
		buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	if(in_create_info.usage_flags & BufferUsageFlagBits::TransferDst)
		buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if(in_create_info.usage_flags & BufferUsageFlagBits::IndirectBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.flags = 0;
	alloc_create_info.usage = convert_memory_usage(in_create_info.mem_usage);

	VmaAllocation allocation;
	VkBuffer handle;
	VmaAllocationInfo alloc_info;

	VkResult result = vmaCreateBuffer(get_allocator(),
		&buffer_create_info, 
		&alloc_create_info,
		&handle,
		&allocation,
		&alloc_info);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	VulkanDescriptorManager::DescriptorIndexHandle srv_index;
	VulkanDescriptorManager::DescriptorIndexHandle uav_index;

	if (in_create_info.usage_flags & BufferUsageFlagBits::StorageBuffer)
	{
		srv_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::StorageBuffer, false);
		uav_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::StorageBuffer, true);
		descriptor_manager.update_descriptor(srv_index, handle, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		descriptor_manager.update_descriptor(uav_index, handle, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	auto buffer = new_resource<VulkanBuffer>(*this, handle, allocation, alloc_info, srv_index, uav_index);
	return make_result(buffer.get());
}
	
Result<BackendDeviceResource, GfxResult> VulkanDevice::create_swap_chain(const SwapChainCreateInfo& in_create_info)
{
	ZE_CHECK(in_create_info.os_handle && in_create_info.width != 0 && in_create_info.height != 0);

	auto surface = surface_manager.get_or_create(in_create_info.os_handle);
	if(!surface)
	{
		return make_error(surface.get_error());
	}

	/** Create swapchain */
	VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
	if(in_create_info.old_swapchain != null_backend_resource)
	{
		old_swapchain = get_resource<VulkanSwapChain>(in_create_info.old_swapchain)->get_swapchain().swapchain;
	}

	vkb::SwapchainBuilder swapchain_builder(get_physical_device(),
		get_device(),
		surface.get_value());

	VkSurfaceFormatKHR format = {};
	format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	format.format = VK_FORMAT_B8G8R8A8_UNORM;
	
	auto result = 
		swapchain_builder.set_old_swapchain(old_swapchain)
		.set_desired_extent(in_create_info.width, in_create_info.height)
		.set_desired_format(format)
		.build();
	if(!result)
	{
		logger::error("Failed to create Vulkan swapchain: {}", result.error().message());
		return make_error(convert_result(result.vk_result()));
	}

	auto swapchain = new_resource<VulkanSwapChain>(*this, result.value());
	return static_cast<BackendDeviceResource>(swapchain);
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_shader(const ShaderCreateInfo& in_create_info)
{
	ZE_CHECK(!in_create_info.bytecode.empty());

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.codeSize = in_create_info.bytecode.size();
	create_info.pCode = in_create_info.bytecode.data();
	create_info.flags = 0;

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(get_device(),
		&create_info,
		nullptr,
		&shader_module);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto shader = new_resource<VulkanShader>(*this, shader_module);
	return make_result(shader.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info)
{
	VkGraphicsPipelineCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = nullptr;

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
	shader_stages.reserve(in_create_info.shader_stages.size());
	for(const auto& stage : in_create_info.shader_stages)
	{
		VkPipelineShaderStageCreateInfo stage_create_info = {};
		stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info.pNext = nullptr;
		stage_create_info.stage = convert_shader_stage_bits(stage.shader_stage);
		stage_create_info.module = get_resource<VulkanShader>(stage.shader)->shader_module;
		stage_create_info.pName = stage.entry_point;
		stage_create_info.flags = 0;
		stage_create_info.pSpecializationInfo = nullptr;
		shader_stages.push_back(stage_create_info);
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = nullptr;

	std::vector<VkVertexInputBindingDescription> bindings;
	bindings.reserve(in_create_info.vertex_input_state.input_binding_descriptions.size());
	for(const auto& input_binding_desc : in_create_info.vertex_input_state.input_binding_descriptions)
	{
		VkVertexInputBindingDescription binding_desc = {};
		binding_desc.binding = input_binding_desc.binding;
		binding_desc.stride = input_binding_desc.stride;
		binding_desc.inputRate = convert_vertex_input_rate(input_binding_desc.input_rate);
		bindings.push_back(binding_desc);
	}

	std::vector<VkVertexInputAttributeDescription> attributes = {};
	attributes.reserve(in_create_info.vertex_input_state.input_attribute_descriptions.size());
	for(const auto& input_attribute_desc : in_create_info.vertex_input_state.input_attribute_descriptions)
	{
		VkVertexInputAttributeDescription attribute_desc = {};
		attribute_desc.binding = input_attribute_desc.binding;
		attribute_desc.location = input_attribute_desc.location;
		attribute_desc.offset = input_attribute_desc.offset;
		attribute_desc.format = convert_format(input_attribute_desc.format);
		attributes.push_back(attribute_desc);
	}

	vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
	vertex_input_state.pVertexBindingDescriptions = bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertex_input_state.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = nullptr;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = convert_primitive_topology(in_create_info.input_assembly_state.primitive_topology);
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = nullptr;
	rasterization_state.polygonMode = convert_polygon_mode(in_create_info.rasterization_state.polygon_mode);
	rasterization_state.cullMode = convert_cull_mode(in_create_info.rasterization_state.cull_mode);
	rasterization_state.frontFace = convert_front_face(in_create_info.rasterization_state.front_face);
	rasterization_state.depthClampEnable = in_create_info.rasterization_state.enable_depth_clamp;
	rasterization_state.depthBiasEnable = in_create_info.rasterization_state.enable_depth_bias;
	rasterization_state.depthBiasConstantFactor = in_create_info.rasterization_state.depth_bias_constant_factor;
	rasterization_state.depthBiasClamp = in_create_info.rasterization_state.depth_bias_clamp;
	rasterization_state.depthBiasSlopeFactor = in_create_info.rasterization_state.depth_bias_slope_factor;
	rasterization_state.lineWidth = 1.f;
	
	VkPipelineMultisampleStateCreateInfo multisampling_state = {};
	multisampling_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_state.pNext = nullptr;
	multisampling_state.rasterizationSamples = convert_sample_count_bit(in_create_info.multisampling_state.samples);
	multisampling_state.sampleShadingEnable = VK_FALSE;
	multisampling_state.minSampleShading = 0.f;
	multisampling_state.pSampleMask = nullptr;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = nullptr;
	depth_stencil_state.depthTestEnable = in_create_info.depth_stencil_state.enable_depth_test;
	depth_stencil_state.depthWriteEnable = in_create_info.depth_stencil_state.enable_depth_write;
	depth_stencil_state.depthCompareOp = convert_compare_op(in_create_info.depth_stencil_state.depth_compare_op);
	depth_stencil_state.depthBoundsTestEnable = in_create_info.depth_stencil_state.enable_depth_bounds_test;
	depth_stencil_state.stencilTestEnable = in_create_info.depth_stencil_state.enable_stencil_test;
	depth_stencil_state.front = convert_stencil_op_state(in_create_info.depth_stencil_state.front_face);
	depth_stencil_state.back = convert_stencil_op_state(in_create_info.depth_stencil_state.back_face);
	depth_stencil_state.minDepthBounds = 0.f;
	depth_stencil_state.maxDepthBounds = 1.f;

	VkPipelineColorBlendStateCreateInfo color_blend_state = {};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOpEnable = in_create_info.color_blend_state.enable_logic_op;
	color_blend_state.logicOp = convert_logic_op(in_create_info.color_blend_state.logic_op);

	std::vector<VkPipelineColorBlendAttachmentState> color_attachments;
	color_attachments.reserve(in_create_info.color_blend_state.attachments.size());
	for(const auto& color_attachment : in_create_info.color_blend_state.attachments)
	{
		VkPipelineColorBlendAttachmentState state = {};
		state.blendEnable = color_attachment.enable_blend;
		state.srcColorBlendFactor = convert_blend_factor(color_attachment.src_color_blend_factor);
		state.dstColorBlendFactor = convert_blend_factor(color_attachment.dst_color_blend_factor);
		state.colorBlendOp = convert_blend_op(color_attachment.color_blend_op);
		state.srcAlphaBlendFactor = convert_blend_factor(color_attachment.src_alpha_blend_factor);
		state.dstAlphaBlendFactor = convert_blend_factor(color_attachment.dst_alpha_blend_factor);
		state.alphaBlendOp = convert_blend_op(color_attachment.alpha_blend_op);
		state.colorWriteMask = convert_color_component_flags(color_attachment.color_write_flags);
		color_attachments.push_back(state);
	}
	color_blend_state.attachmentCount = static_cast<uint32_t>(color_attachments.size());
	color_blend_state.pAttachments = color_attachments.data();
	color_blend_state.blendConstants[0] = 0.f;
	color_blend_state.blendConstants[1] = 0.f;
	color_blend_state.blendConstants[2] = 0.f;
	color_blend_state.blendConstants[3] = 0.f;

	std::array dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.pNext = nullptr;
	dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state_create_info.pDynamicStates = dynamic_states.data();

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;
	viewport_state.flags = 0;

	VkViewport viewport = {};
	VkRect2D scissor = {};

	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;
	
	create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	create_info.pStages = shader_stages.data();
	create_info.pVertexInputState = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pTessellationState = nullptr;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState = &multisampling_state;
	create_info.pDepthStencilState = &depth_stencil_state;
	create_info.pColorBlendState = &color_blend_state;
	create_info.pDynamicState = &dynamic_state_create_info;
	create_info.layout = get_resource<VulkanPipelineLayout>(in_create_info.pipeline_layout)->get_pipeline_layout();
	create_info.renderPass = get_resource<VulkanRenderPass>(in_create_info.render_pass)->get_render_pass();
	create_info.subpass = in_create_info.subpass;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;
	
	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(get_device(),
		VK_NULL_HANDLE,
		1,
		&create_info,
		nullptr,
		&pipeline);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanPipeline>(*this, pipeline);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_compute_pipeline(const ComputePipelineCreateInfo& in_create_info)
{
	VkComputePipelineCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	create_info.stage.stage = convert_shader_stage_bits(in_create_info.shader_stage.shader_stage);
	create_info.stage.module = get_resource<VulkanShader>(in_create_info.shader_stage.shader)->shader_module;
	create_info.stage.pName = in_create_info.shader_stage.entry_point;
	create_info.layout = get_resource<VulkanPipelineLayout>(in_create_info.pipeline_layout)->get_pipeline_layout();
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkResult result = vkCreateComputePipelines(get_device(),
		VK_NULL_HANDLE,
		1,
		&create_info,
		nullptr,
		&pipeline);
	if (result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanPipeline>(*this, pipeline);
	return make_result(ret.get());
}
	
Result<BackendDeviceResource, GfxResult> VulkanDevice::create_render_pass(const RenderPassCreateInfo& in_create_info)
{
	VkRenderPassCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	std::vector<VkAttachmentDescription> attachments;
	attachments.reserve(in_create_info.attachments.size());
	for(const auto& attachment : in_create_info.attachments)
	{
		VkAttachmentDescription desc = {};
		desc.format = convert_format(attachment.format);
		desc.samples = convert_sample_count_bit(attachment.samples);
		desc.loadOp = convert_load_op(attachment.load_op);
		desc.storeOp = convert_store_op(attachment.store_op);
		desc.stencilLoadOp = convert_load_op(attachment.stencil_load_op);
		desc.stencilStoreOp = convert_store_op(attachment.stencil_store_op);
		desc.initialLayout = convert_texture_layout(attachment.initial_layout);
		desc.finalLayout = convert_texture_layout(attachment.final_layout);
		attachments.push_back(desc);
	}

	create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	create_info.pAttachments = attachments.data();

	/**
	 * Struct to keep attachments descriptions alive
	 */
	struct Subpass
	{
		std::vector<VkAttachmentReference> input_attachments;
		std::vector<VkAttachmentReference> color_attachments;
		std::vector<VkAttachmentReference> resolve_attachments;
		VkAttachmentReference depth_stencil_attachment = {};
	};

	std::vector<Subpass> subpass_holders;
	std::vector<VkSubpassDescription> subpasses;
	subpasses.reserve(in_create_info.subpasses.size());
	subpass_holders.reserve(in_create_info.subpasses.size());
	for(const auto& subpass : in_create_info.subpasses)
	{
		ZE_CHECK(subpass.resolve_attachments.empty()
			|| subpass.resolve_attachments.size() == subpass.color_attachments.size());
		
		VkSubpassDescription desc = {};
		desc.flags = 0;
		desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		auto process_attachment_reference_single = [](const AttachmentReference& in_src,
			VkAttachmentReference& in_dst)
		{
			in_dst.attachment = in_src.attachment;
			in_dst.layout = convert_texture_layout(in_src.layout);
		};
		
		auto process_attachment_reference_multiple = [](const std::vector<AttachmentReference>& in_src,
			std::vector<VkAttachmentReference>& in_dst)
		{
			in_dst.reserve(in_src.size());
			for(const auto& src_ref : in_src)
			{
				VkAttachmentReference ref;
				ref.attachment = src_ref.attachment;
				ref.layout = convert_texture_layout(src_ref.layout);
				in_dst.push_back(ref);
			}
		};

		auto& holder = subpass_holders.emplace_back();
		
		process_attachment_reference_multiple(subpass.input_attachments, holder.input_attachments);
		process_attachment_reference_multiple(subpass.color_attachments, holder.color_attachments);
		process_attachment_reference_multiple(subpass.resolve_attachments, holder.resolve_attachments);
		process_attachment_reference_single(subpass.depth_stencil_attachment, holder.depth_stencil_attachment);

		desc.inputAttachmentCount = static_cast<uint32_t>(holder.input_attachments.size());
		desc.pInputAttachments = holder.input_attachments.data();
		desc.colorAttachmentCount = static_cast<uint32_t>(holder.color_attachments.size());
		desc.pColorAttachments = holder.color_attachments.data();
		desc.pResolveAttachments = holder.resolve_attachments.data();
		desc.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserve_attachments.size());
		desc.pPreserveAttachments = subpass.preserve_attachments.data();
		desc.pDepthStencilAttachment = &holder.depth_stencil_attachment;
		
		subpasses.push_back(desc);
	}
	
	create_info.subpassCount = static_cast<uint32_t>(subpasses.size());
	create_info.pSubpasses = subpasses.data();

	create_info.dependencyCount = 0;
	create_info.pDependencies = nullptr;

	VkRenderPass render_pass;
	VkResult result = vkCreateRenderPass(get_device(),
		&create_info,
		nullptr,
		&render_pass);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanRenderPass>(*this, render_pass);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_command_pool(const CommandPoolCreateInfo& in_create_info)
{
	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	auto index = device_wrapper.device.get_queue_index(convert_queue_type(in_create_info.queue_type));
	if(!index)
	{
		logger::error("Failed to create Vulkan command pool: {}", index.error().message());
		return make_error(GfxResult::ErrorInitializationFailed);
	}
	create_info.queueFamilyIndex = index.value();
	VkCommandPool command_pool;
	VkResult result = vkCreateCommandPool(get_device(), 
		&create_info,
		nullptr,
		&command_pool);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanCommandPool>(*this, command_pool);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_texture(const TextureCreateInfo& in_create_info)
{
	VkImageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.imageType = convert_texture_type(in_create_info.type);
	create_info.format = convert_format(in_create_info.format);
	create_info.extent = { in_create_info.width, in_create_info.height, in_create_info.depth };
	create_info.arrayLayers = in_create_info.array_layers;
	create_info.mipLevels = in_create_info.mip_levels;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = convert_sample_count_bit(in_create_info.sample_count);
	create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	create_info.usage = 0;

	if(in_create_info.usage_flags & TextureUsageFlagBits::ColorAttachment)
		create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if(in_create_info.usage_flags & TextureUsageFlagBits::DepthStencilAttachment)
		create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	if(in_create_info.usage_flags & TextureUsageFlagBits::Sampled)
		create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	if (in_create_info.usage_flags & TextureUsageFlagBits::UAV)
		create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if(in_create_info.usage_flags & TextureUsageFlagBits::TransferSrc)
		create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if(in_create_info.usage_flags & TextureUsageFlagBits::TransferDst)
		create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (in_create_info.usage_flags & TextureUsageFlagBits::Cube)
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.flags = 0;
	alloc_create_info.usage = convert_memory_usage(in_create_info.mem_usage);	

	VkImage image;
	VmaAllocation allocation;
	VkResult result = vmaCreateImage(get_allocator(),
		&create_info,
		&alloc_create_info,
		&image,
		&allocation,
		nullptr);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanTexture>(*this, image, allocation, create_info.usage);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_texture_view(const TextureViewCreateInfo& in_create_info)
{
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.viewType = convert_view_type(in_create_info.type);
	create_info.image = get_resource<VulkanTexture>(in_create_info.texture)->get_texture();
	create_info.format = convert_format(in_create_info.format);
	create_info.subresourceRange = convert_subresource_range(in_create_info.subresource_range);
	create_info.components = VkComponentMapping { VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G ,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A };

	VkImageView view = VK_NULL_HANDLE;
	VkResult result = vkCreateImageView(get_device(),
		&create_info,
		nullptr,
		&view);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	VulkanDescriptorManager::DescriptorIndexHandle srv_index;
	VulkanDescriptorManager::DescriptorIndexHandle uav_index;

	const auto* texture = get_resource<VulkanTexture>(in_create_info.texture);
	if (texture->get_image_usage_flags() & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		if (in_create_info.type == TextureViewType::Tex2D)
		{
			srv_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::Texture2D, false);
		}
		else if (in_create_info.type == TextureViewType::TexCube)
		{
			srv_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::TextureCube, false);
		}

		descriptor_manager.update_descriptor(srv_index, create_info.viewType, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	if (texture->get_image_usage_flags() & VK_IMAGE_USAGE_STORAGE_BIT)
	{
		if (in_create_info.type == TextureViewType::Tex2D)
		{
			uav_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::Texture2D, true);
		}
		else if (in_create_info.type == TextureViewType::TexCube)
		{
			uav_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::TextureCube, true);
		}

		descriptor_manager.update_descriptor(uav_index, create_info.viewType, view, VK_IMAGE_LAYOUT_GENERAL);
	}

	auto ret = new_resource<VulkanTextureView>(*this, view, srv_index, uav_index);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_sampler(const SamplerCreateInfo& in_create_info)
{
	VkSamplerCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.minFilter = convert_filter(in_create_info.min_filter);
	create_info.magFilter = convert_filter(in_create_info.mag_filter);
	create_info.mipmapMode = convert_filter_mipmap_mode(in_create_info.mip_map_mode);
	create_info.addressModeU = convert_address_mode(in_create_info.address_mode_u);
	create_info.addressModeV = convert_address_mode(in_create_info.address_mode_v);
	create_info.addressModeW = convert_address_mode(in_create_info.address_mode_w);
	create_info.mipLodBias = in_create_info.mip_lod_bias;
	create_info.compareEnable = in_create_info.compare_op != CompareOp::Never;
	create_info.compareOp = convert_compare_op(in_create_info.compare_op);
	create_info.anisotropyEnable = in_create_info.enable_anisotropy;
	create_info.maxAnisotropy = in_create_info.max_anisotropy;
	create_info.minLod = in_create_info.min_lod;
	create_info.maxLod = in_create_info.max_lod;

	VkSampler sampler;
	VkResult result = vkCreateSampler(get_device(),
		&create_info,
		nullptr,
		&sampler);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	VulkanDescriptorManager::DescriptorIndexHandle srv_index = descriptor_manager.allocate_index(VulkanDescriptorManager::DescriptorType::Sampler, 
		false);

	descriptor_manager.update_descriptor(srv_index, sampler);

	auto ret = new_resource<VulkanSampler>(*this, sampler, srv_index);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_semaphore(const SemaphoreCreateInfo& in_create_info)
{
	(void)(in_create_info);
	
	VkSemaphoreCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(get_device(),
		&create_info,
		nullptr,
		&semaphore);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanSemaphore>(*this, semaphore);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_fence(const FenceCreateInfo& in_create_info)
{
	VkFenceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	if(in_create_info.signaled)
		create_info.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence fence;
	VkResult result = vkCreateFence(get_device(),
		&create_info,
		nullptr,
		&fence);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanFence>(*this, fence);
	return make_result(ret.get());
}

Result<BackendDeviceResource, GfxResult> VulkanDevice::create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info)
{
	std::vector<VkPushConstantRange> push_constant_ranges;
	for(const auto& push_constant : in_create_info.push_constant_ranges)
	{
		VkPushConstantRange range = {};
		range.stageFlags = convert_shader_stage_flags(push_constant.stage);
		range.offset = push_constant.offset;
		range.size = push_constant.size;
		push_constant_ranges.emplace_back(range);
	}
	
	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_manager.get_global_descriptor_set_layout();
	create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
	create_info.pPushConstantRanges = push_constant_ranges.data();

	VkPipelineLayout layout;
	VkResult result = vkCreatePipelineLayout(get_device(),
		&create_info,
		nullptr,
		&layout);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanPipelineLayout>(*this, layout);
	return make_result(ret.get());
}

Result<std::vector<BackendDeviceResource>, GfxResult> VulkanDevice::allocate_command_lists(const BackendDeviceResource& in_pool, 
	const uint32_t in_count)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.commandPool = get_resource<VulkanCommandPool>(in_pool)->get_command_pool();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = in_count;

	std::vector<VkCommandBuffer> buffers;
	buffers.resize(in_count);

	VkResult result = vkAllocateCommandBuffers(get_device(),
		&alloc_info,
		buffers.data());
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));
	
	std::vector<BackendDeviceResource> resources;
	resources.reserve(in_count);
	for(const auto& buffer : buffers)
	{
		resources.emplace_back(new_resource<VulkanCommandList>(*this, buffer));	
	}

	return make_result(resources);
}

void VulkanDevice::free_command_lists(const BackendDeviceResource& in_pool, const std::vector<BackendDeviceResource>& in_lists)
{
	std::vector<VkCommandBuffer> buffers;
	buffers.reserve(in_lists.size());
	for(const auto& list : in_lists)
	{
		buffers.emplace_back(get_resource<VulkanCommandList>(list)->get_command_buffer());
		free_resource<VulkanCommandList>(list);
	}

	vkFreeCommandBuffers(get_device(), 
		get_resource<VulkanCommandPool>(in_pool)->get_command_pool(),
		static_cast<uint32_t>(buffers.size()),
		buffers.data());
}

void VulkanDevice::reset_command_pool(const BackendDeviceResource& in_pool)
{
	vkResetCommandPool(get_device(),
		get_resource<VulkanCommandPool>(in_pool)->get_command_pool(),
		0);	
}

void VulkanDevice::destroy_buffer(const BackendDeviceResource& in_buffer)
{
	free_resource<VulkanBuffer>(in_buffer);
}

void VulkanDevice::destroy_swap_chain(const BackendDeviceResource& in_swap_chain)
{
	free_resource<VulkanSwapChain>(in_swap_chain);
}

void VulkanDevice::destroy_shader(const BackendDeviceResource& in_shader)
{
	free_resource<VulkanShader>(in_shader);
}

void VulkanDevice::destroy_pipeline(const BackendDeviceResource& in_pipeline)
{
	free_resource<VulkanPipeline>(in_pipeline);		
}
	
void VulkanDevice::destroy_render_pass(const BackendDeviceResource& in_render_pass)
{
	free_resource<VulkanRenderPass>(in_render_pass);		
}

void VulkanDevice::destroy_command_pool(const BackendDeviceResource& in_command_pool)
{
	free_resource<VulkanCommandPool>(in_command_pool);
}

void VulkanDevice::destroy_texture(const BackendDeviceResource& in_texture)
{
	free_resource<VulkanTexture>(in_texture);	
}

void VulkanDevice::destroy_texture_view(const BackendDeviceResource& in_texture_view)
{
	free_resource<VulkanTextureView>(in_texture_view);	
}

void VulkanDevice::destroy_sampler(const BackendDeviceResource& in_sampler)
{
	free_resource<VulkanSampler>(in_sampler);	
}

void VulkanDevice::destroy_semaphore(const BackendDeviceResource& in_semaphore)
{
	free_resource<VulkanSemaphore>(in_semaphore);		
}

void VulkanDevice::destroy_fence(const BackendDeviceResource& in_fence)
{
	free_resource<VulkanFence>(in_fence);
}

void VulkanDevice::destroy_pipeline_layout(const BackendDeviceResource& in_pipeline_layout)
{
	free_resource<VulkanPipelineLayout>(in_pipeline_layout);				
}

uint32_t VulkanDevice::get_buffer_srv_descriptor_index(const BackendDeviceResource& in_handle)
{
	return get_resource<VulkanBuffer>(in_handle)->get_srv_index();
}

uint32_t VulkanDevice::get_buffer_uav_descriptor_index(const BackendDeviceResource& in_handle)
{
	return get_resource<VulkanBuffer>(in_handle)->get_uav_index();
}

uint32_t VulkanDevice::get_texture_view_srv_descriptor_index(const BackendDeviceResource& in_handle)
{
	return get_resource<VulkanTextureView>(in_handle)->get_srv_index();
}

uint32_t VulkanDevice::get_texture_view_uav_descriptor_index(const BackendDeviceResource& in_handle)
{
	return get_resource<VulkanTextureView>(in_handle)->get_uav_index();
}

uint32_t VulkanDevice::get_sampler_srv_descriptor_index(const BackendDeviceResource& in_handle)
{
	return get_resource<VulkanSampler>(in_handle)->get_srv_index();
}

/** Buffers */
Result<void*, GfxResult> VulkanDevice::map_buffer(const BackendDeviceResource& in_buffer)
{
	void* data = nullptr;

	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	VkResult result = vmaMapMemory(get_allocator(),
		buffer->get_allocation(),
		&data);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	return make_result(data);
}

void VulkanDevice::unmap_buffer(const BackendDeviceResource& in_buffer)
{
	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	vmaUnmapMemory(get_allocator(), buffer->get_allocation());
}

/** Swapchain */
std::pair<GfxResult, uint32_t> VulkanDevice::acquire_swapchain_image(const BackendDeviceResource& in_swapchain,
	const BackendDeviceResource& in_signal_semaphore)
{
	VkSemaphore signal_semaphore = VK_NULL_HANDLE;
	if(in_signal_semaphore != null_backend_resource)
		signal_semaphore = get_resource<VulkanSemaphore>(in_signal_semaphore)->get_semaphore();

	auto swapchain = get_resource<VulkanSwapChain>(in_swapchain);
	return { convert_result(swapchain->acquire_image(signal_semaphore)), swapchain->get_current_image_idx() };
}

void VulkanDevice::present(const BackendDeviceResource& in_swapchain,
	const std::span<BackendDeviceResource>& in_wait_semaphores)
{
	std::vector<VkSemaphore> wait_semaphores;
	wait_semaphores.reserve(in_wait_semaphores.size());

	for(const auto& semaphore : in_wait_semaphores)
		wait_semaphores.emplace_back(get_resource<VulkanSemaphore>(semaphore)->get_semaphore());
	
	get_resource<VulkanSwapChain>(in_swapchain)->present(wait_semaphores);
}

BackendDeviceResource VulkanDevice::get_swapchain_backbuffer_view(const BackendDeviceResource& in_swap_chain)
{
	return get_resource<VulkanSwapChain>(in_swap_chain)->get_texture_view();
}

const std::vector<BackendDeviceResource>& VulkanDevice::get_swapchain_backbuffer_views(const BackendDeviceResource& in_swapchain)
{
	return get_resource<VulkanSwapChain>(in_swapchain)->get_texture_views();
}

const std::vector<BackendDeviceResource>& VulkanDevice::get_swapchain_backbuffers(const BackendDeviceResource& in_swapchain)
{
	return get_resource<VulkanSwapChain>(in_swapchain)->get_textures();
}

Format VulkanDevice::get_swapchain_format(const BackendDeviceResource& in_swapchain)
{
	return get_resource<VulkanSwapChain>(in_swapchain)->get_format();
}

/** Fences */
GfxResult VulkanDevice::wait_for_fences(const std::span<BackendDeviceResource>& in_fences, 
	const bool in_wait_for_all, 
	const uint64_t in_timeout)
{
	std::vector<VkFence> fences;
	fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		fences.emplace_back(get_resource<VulkanFence>(fence)->get_fence());
	
	return convert_result(vkWaitForFences(get_device(),
		static_cast<uint32_t>(fences.size()),
		fences.data(),
		in_wait_for_all,
		in_timeout));
}

GfxResult VulkanDevice::get_fence_status(const BackendDeviceResource in_fence)
{
	return convert_result(vkGetFenceStatus(get_device(),
		get_resource<VulkanFence>(in_fence)->get_fence()));
}

void VulkanDevice::reset_fences(const std::span<BackendDeviceResource>& in_fences)
{
	std::vector<VkFence> fences;
	fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		fences.emplace_back(get_resource<VulkanFence>(fence)->get_fence());

	vkResetFences(get_device(),
		static_cast<uint32_t>(fences.size()),
		fences.data());
}

/** Commands */

void VulkanDevice::cmd_begin_region(const BackendDeviceResource& in_list, const std::string_view& in_name, const glm::vec4& in_color)
{
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = in_name.data();
	label.color[0] = in_color.r;
	label.color[1] = in_color.g;
	label.color[2] = in_color.b;
	label.color[3] = in_color.a;

	vkCmdBeginDebugUtilsLabelEXT(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		&label);
}

void VulkanDevice::cmd_end_region(const BackendDeviceResource& in_list)
{
	vkCmdEndDebugUtilsLabelEXT(get_resource<VulkanCommandList>(in_list)->get_command_buffer());
}

void VulkanDevice::begin_cmd_list(const BackendDeviceResource& in_list)
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = nullptr;
	
	vkBeginCommandBuffer(
		get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		&begin_info);
}

void VulkanDevice::cmd_begin_render_pass(const BackendDeviceResource& in_list,
	const BackendDeviceResource& in_render_pass,
	const Framebuffer& in_framebuffer,
	Rect2D in_render_area,
	std::span<ClearValue> in_clear_values)
{
	VkRenderPass render_pass = get_resource<VulkanRenderPass>(in_render_pass)->get_render_pass();
	
	VkRenderPassBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.framebuffer = framebuffer_manager.get_or_create(render_pass, in_framebuffer);
	begin_info.clearValueCount = static_cast<uint32_t>(in_clear_values.size());
	begin_info.pClearValues = reinterpret_cast<VkClearValue*>(in_clear_values.data());
	begin_info.renderArea = *reinterpret_cast<VkRect2D*>(&in_render_area);
	begin_info.renderPass = render_pass;
	
	vkCmdBeginRenderPass(
		get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		&begin_info,
		VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanDevice::cmd_bind_pipeline(const BackendDeviceResource& in_list, 
	const PipelineBindPoint in_bind_point, 
	const BackendDeviceResource& in_pipeline)
{
	vkCmdBindPipeline(
		get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		convert_pipeline_bind_point(in_bind_point),
		get_resource<VulkanPipeline>(in_pipeline)->get_pipeline());
}

void VulkanDevice::cmd_dispatch(const BackendDeviceResource& in_list, const uint32_t in_x, const uint32_t in_y, const uint32_t in_z)
{
	vkCmdDispatch(
		get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_x,
		in_y,
		in_z);
}

void VulkanDevice::cmd_draw(const BackendDeviceResource& in_list,
	const uint32_t in_vertex_count,
	const uint32_t in_instance_count,
	const uint32_t in_first_vertex,
	const uint32_t in_first_instance)
{
	vkCmdDraw(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_vertex_count,
		in_instance_count,
		in_first_vertex,
		in_first_instance);
}

void VulkanDevice::cmd_draw_indexed(const BackendDeviceResource& in_list, 
	const uint32_t in_index_count, 
	const uint32_t in_instance_count, 
	const uint32_t in_first_index, 
	const int32_t in_vertex_offset, 
	const uint32_t in_first_instance)
{
	vkCmdDrawIndexed(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_index_count,
		in_instance_count,
		in_first_index,
		in_vertex_offset,
		in_first_instance);
}

void VulkanDevice::cmd_end_render_pass(const BackendDeviceResource& in_list)
{
	vkCmdEndRenderPass(get_resource<VulkanCommandList>(in_list)->get_command_buffer());
}

void VulkanDevice::cmd_bind_descriptors(const BackendDeviceResource in_list, 
	const PipelineBindPoint in_bind_point, 
	const BackendDeviceResource in_pipeline_layout)
{
	descriptor_manager.flush_updates();

	descriptor_manager.bind_descriptors(
		get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		convert_pipeline_bind_point(in_bind_point),
		get_resource<VulkanPipelineLayout>(in_pipeline_layout)->get_pipeline_layout());
}

void VulkanDevice::cmd_bind_vertex_buffers(const BackendDeviceResource& in_list, 
	const uint32_t in_first_binding, 
	const std::span<BackendDeviceResource> in_buffers, 
	const std::span<uint64_t> in_offsets)
{
	std::vector<VkBuffer> benjas;
	benjas.reserve(in_buffers.size());

	for(const auto& buffer : in_buffers)
		benjas.emplace_back(get_resource<VulkanBuffer>(buffer)->get_buffer());

	vkCmdBindVertexBuffers(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_first_binding,
		static_cast<uint32_t>(benjas.size()),
		benjas.data(),
		in_offsets.data());
}

void VulkanDevice::cmd_bind_index_buffer(const BackendDeviceResource in_list, 
	const BackendDeviceResource in_index_buffer,
	const uint64_t in_offset,
	const IndexType in_index_type)
{
	vkCmdBindIndexBuffer(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		get_resource<VulkanBuffer>(in_index_buffer)->get_buffer(),
		in_offset,
		in_index_type == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void VulkanDevice::cmd_set_viewports(const BackendDeviceResource& in_list, 
	const uint32_t in_first_viewport, 
	const std::span<Viewport>& in_viewports)
{
	vkCmdSetViewport(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_first_viewport,
		static_cast<uint32_t>(in_viewports.size()),
		reinterpret_cast<VkViewport*>(in_viewports.data()));	
}

void VulkanDevice::cmd_set_scissors(const BackendDeviceResource& in_list, 
	const uint32_t in_first_scissor, 
	const std::span<Rect2D>& in_scissors)
{
	vkCmdSetScissor(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		in_first_scissor,
		static_cast<uint32_t>(in_scissors.size()),
		reinterpret_cast<VkRect2D*>(in_scissors.data()));
}

void VulkanDevice::cmd_pipeline_barrier(const BackendDeviceResource in_list, 
	const PipelineStageFlags in_src_flags, 
	const PipelineStageFlags in_dst_flags, 
	const std::span<TextureMemoryBarrier>& in_texture_memory_barriers)
{
	std::vector<VkImageMemoryBarrier> image_barriers;
	image_barriers.reserve(in_texture_memory_barriers.size());

	for(const auto& barrier : in_texture_memory_barriers)
		image_barriers.push_back(VkImageMemoryBarrier {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			convert_access_flags(barrier.src_access_flags),
			convert_access_flags(barrier.dst_access_flags),
			convert_texture_layout(barrier.old_layout),
			convert_texture_layout(barrier.new_layout),
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			get_resource<VulkanTexture>(barrier.texture)->get_texture(),
			convert_subresource_range(barrier.subresource_range) });
	
	vkCmdPipelineBarrier(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		convert_pipeline_stage_flags(in_src_flags),
		convert_pipeline_stage_flags(in_dst_flags),
		0,
		0,
		nullptr,
		0,
		nullptr,
		static_cast<uint32_t>(image_barriers.size()),
		image_barriers.data());
}

void VulkanDevice::cmd_copy_buffer(const BackendDeviceResource& in_cmd_list, 
	const BackendDeviceResource& in_src_buffer, 
	const BackendDeviceResource& in_dst_buffer, 
	const std::span<BufferCopyRegion>& in_regions)
{
	VkBuffer src_buffer = get_resource<VulkanBuffer>(in_src_buffer)->get_buffer();
	VkBuffer dst_buffer = get_resource<VulkanBuffer>(in_dst_buffer)->get_buffer();

	std::vector<VkBufferCopy> regions;
	regions.reserve(in_regions.size());
	for(const auto& region : in_regions)
	{
		regions.push_back({ region.src_offset, region.dst_offset, region.size} );
	}
	
	vkCmdCopyBuffer(get_resource<VulkanCommandList>(in_cmd_list)->get_command_buffer(),
		src_buffer,
		dst_buffer,
		static_cast<uint32_t>(regions.size()),
		regions.data());
}

void VulkanDevice::cmd_copy_buffer_to_texture(const BackendDeviceResource in_list, 
	const BackendDeviceResource in_src_buffer, 
	const BackendDeviceResource in_dst_texture, 
	const TextureLayout in_dst_layout, 
	const std::span<BufferTextureCopyRegion>& in_copy_regions)
{
	std::vector<VkBufferImageCopy> regions;
	regions.reserve(in_copy_regions.size());

	ZE_CHECK((std::is_same_v<decltype(Offset3D::x), decltype(VkOffset3D::x)>));
	ZE_CHECK((std::is_same_v<decltype(Extent3D::width), decltype(VkExtent3D::width)>));

	for(const auto& region : in_copy_regions)
		regions.push_back(VkBufferImageCopy {
			region.buffer_offset,
			0,
			0,
			convert_subresource_layers(region.texture_subresource),
			*reinterpret_cast<const VkOffset3D*>(&region.texture_offset),
			*reinterpret_cast<const VkExtent3D*>(&region.texture_extent)});

	vkCmdCopyBufferToImage(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		get_resource<VulkanBuffer>(in_src_buffer)->get_buffer(),
		get_resource<VulkanTexture>(in_dst_texture)->get_texture(),
		convert_texture_layout(in_dst_layout),
		static_cast<uint32_t>(regions.size()),
		regions.data());
}

void VulkanDevice::cmd_push_constants(const BackendDeviceResource in_list, 
	const BackendDeviceResource in_pipeline_layout, 
	ShaderStageFlags in_stage_flags, 
	const uint32_t in_offset, 
	const uint32_t in_size, 
	const void* in_data)
{
	vkCmdPushConstants(get_resource<VulkanCommandList>(in_list)->get_command_buffer(),
		get_resource<VulkanPipelineLayout>(in_pipeline_layout)->get_pipeline_layout(),
		convert_shader_stage_flags(in_stage_flags),
		in_offset,
		in_size,
		in_data);
}

void VulkanDevice::end_cmd_list(const BackendDeviceResource& in_list)
{
	vkEndCommandBuffer(get_resource<VulkanCommandList>(in_list)->get_command_buffer());
}

void VulkanDevice::queue_submit(const QueueType& in_type, 
	const std::span<BackendDeviceResource>& in_command_lists, 
	const std::span<BackendDeviceResource>& in_wait_semaphores, 
	const std::span<PipelineStageFlags>& in_wait_pipeline_stages, 
	const std::span<BackendDeviceResource>& in_signal_semaphores, 
	const BackendDeviceResource& in_fence)
{
	vkb::QueueType type;
	switch(in_type)
	{
	default:
	case QueueType::Gfx:
		type = vkb::QueueType::graphics;
		break;
	case QueueType::Compute:
		type = vkb::QueueType::compute;
		break;
	case QueueType::Transfer:
		type = vkb::QueueType::transfer;
		break;
	case QueueType::Present:
		type = vkb::QueueType::present;
		break;
	}
	
	VkQueue queue = device_wrapper.device.get_queue(type).value();

	std::vector<VkCommandBuffer> command_buffers;
	command_buffers.reserve(in_command_lists.size());
	for(const auto& list : in_command_lists)
		command_buffers.emplace_back(get_resource<VulkanCommandList>(list)->get_command_buffer());
	
	std::vector<VkSemaphore> wait_semaphores;
	wait_semaphores.reserve(in_wait_semaphores.size());
	for(const auto& semaphore : in_wait_semaphores)
		wait_semaphores.emplace_back(get_resource<VulkanSemaphore>(semaphore)->get_semaphore());

	std::vector<VkSemaphore> signal_semaphores;
	signal_semaphores.reserve(in_signal_semaphores.size());
	for(const auto& semaphore : in_signal_semaphores)
		signal_semaphores.emplace_back(get_resource<VulkanSemaphore>(semaphore)->get_semaphore());

	std::vector<VkPipelineStageFlags> wait_stage;
	wait_stage.reserve(in_wait_semaphores.size());
	for(const auto& stage : in_wait_pipeline_stages)
		wait_stage.emplace_back(convert_pipeline_stage_flags(stage));
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr; 
	submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
	submit_info.pCommandBuffers = command_buffers.data();
	submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()); 
	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.pWaitDstStageMask = wait_stage.data();
	submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
	submit_info.pSignalSemaphores = signal_semaphores.data();

	VkFence fence = VK_NULL_HANDLE;
	if(in_fence != null_backend_resource)
		fence = get_resource<VulkanFence>(in_fence)->get_fence();
	
	vkQueueSubmit(queue, 1, &submit_info, fence);
}

}