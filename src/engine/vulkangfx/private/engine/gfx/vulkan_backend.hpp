#pragma once

#include "engine/gfx/backend.hpp"

namespace ze::gfx
{

class VulkanBackend final : public Backend
{
public:
	VulkanBackend(const BackendFlags& in_flags, vkb::Instance in_instance);
	~VulkanBackend();

	Result<std::unique_ptr<BackendDevice>, std::string> create_device(ShaderModel in_requested_shader_model) override;

	[[nodiscard]] VkInstance get_instance() const { return instance.instance; }
	[[nodiscard]] bool has_debug_layers() const { return debug_layers_enabled; }
private:
	vkb::Instance instance;
	bool debug_layers_enabled;
public:
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
};

}