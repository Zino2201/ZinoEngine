#include "engine/gfx/vulkan_backend.hpp"
#include "engine/gfx/device.hpp"
#include "vulkan_device.hpp"

namespace ze::gfx
{

VulkanBackend::VulkanBackend(const BackendFlags& in_flags, vkb::Instance in_instance)
	: Backend(in_flags), instance(in_instance), debug_layers_enabled(instance.debug_messenger != VK_NULL_HANDLE)
{
	name = "Vulkan";
	shader_language = ShaderLanguage::VK_SPIRV;
	supported_shader_models = { ShaderModel::SM6_0, ShaderModel::SM6_5 };

	if (has_debug_layers())
	{
		vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(
			instance.instance, "vkSetDebugUtilsObjectNameEXT"));
		logger::warn(log_vulkan, "Validation layers enabled, except slowdowns");
	}
}

VulkanBackend::~VulkanBackend()
{
	logger::verbose(log_vulkan, "~VulkanBackend()");

	vkb::destroy_instance(instance);
}
	
Result<std::unique_ptr<BackendDevice>, std::string> VulkanBackend::create_device(ShaderModel in_requested_shader_model)
{
	(void)(in_requested_shader_model);

	vkb::PhysicalDevice physical_device;
	
	/** At this point we have an instance, let's try finding a suitable physical device */
	{
		vkb::PhysicalDeviceSelector phys_device_selector(instance);

		VkPhysicalDeviceVulkan12Features features = {};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features.descriptorIndexing = VK_TRUE;
		features.runtimeDescriptorArray = VK_TRUE;
		features.descriptorBindingPartiallyBound = VK_TRUE;
		features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;

		phys_device_selector.set_required_features_12(features);
		phys_device_selector.defer_surface_initialization();
		phys_device_selector.require_present();

		VkPhysicalDeviceFeatures required_features = {};
		required_features.fillModeNonSolid = VK_TRUE;
		phys_device_selector.set_required_features(required_features);

		auto result = phys_device_selector.select();
		if(!result)
		{
			return fmt::format("Failed to select a physical device: {}", result.error().message());
		}

		physical_device = result.value();

		logger::info(log_vulkan, "Found suitable GPU \"{}\"", physical_device.properties.deviceName);
	}

	vkb::DeviceBuilder device_builder(physical_device);
	auto device = device_builder.build();
	if(!device)
	{
		return fmt::format("Failed to create logical device: {}", device.error().message());
	}
	
	return make_result(std::make_unique<VulkanDevice>(*this, std::move(device.value())));	
}

}