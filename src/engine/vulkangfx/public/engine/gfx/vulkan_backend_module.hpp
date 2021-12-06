#pragma once

#include "engine/result.hpp"
#include "engine/module/module.hpp"
#include "engine/gfx/backend.hpp"

namespace ze::gfx
{

class Backend;

class VulkanBackendModule final : public Module
{
public:
	VulkanBackendModule();

	Result<std::unique_ptr<Backend>, std::string> create_vulkan_backend(const BackendFlags& in_flags);
};

}