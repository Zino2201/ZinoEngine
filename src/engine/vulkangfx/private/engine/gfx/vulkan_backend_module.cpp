#include "engine/gfx/vulkan_backend_module.hpp"
#include "engine/module/module.hpp"
#include "vulkan.hpp"
#include "vulkan_backend.hpp"
#include "engine/module/module_manager.hpp"

namespace ze::gfx
{

VulkanBackendModule::VulkanBackendModule()
{
	auto result = load_module("VulkanShaderCompiler");
	if (!result)
		logger::fatal(log_vulkan, "Failed to load vulkan shader compiler module! Exiting.");
}

Result<std::unique_ptr<Backend>, std::string> VulkanBackendModule::create_vulkan_backend(const BackendFlags& in_flags)
{
	vkb::InstanceBuilder builder;
	builder.set_engine_name("ZinoEngine");
	builder.require_api_version(1, 2, 0);

	/** Customize our instance based on the flags */
	if (in_flags & BackendFlagBits::DebugLayers)
	{
		builder.enable_validation_layers();
		builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
		//builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
		//builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
		builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		builder.set_debug_callback(
			[](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
				VkDebugUtilsMessageTypeFlagsEXT message_type,
				const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
				void* user_data) -> VkBool32
			{
				(void)(user_data);

				const auto type = vkb::to_string_message_type(message_type);

				switch (message_severity)
				{
				default:
					logger::verbose(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					logger::info(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					logger::warn(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					logger::error(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					ZE_DEBUGBREAK();
					break;
				}

				return VK_FALSE;
			}
		);

	}

	/** Create instance */
	{
		auto result = builder.build();
		if (!result)
		{
			return make_error(fmt::format("Failed to create Vulkan Instance: {}", result.error().message()));
		}

		return make_result(std::make_unique<VulkanBackend>(in_flags, result.value()));
	}
}

ZE_IMPLEMENT_MODULE(VulkanBackendModule, VulkanGfx);

}