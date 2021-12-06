#pragma once

#include "engine/core.hpp"
#include "shader_format.hpp"
#include "engine/flags.hpp"
#include "engine/result.hpp"

namespace ze::gfx
{
			
class BackendDevice;

enum class BackendFlagBits
{
	/** Enable debugging utilities like Vulkan's Validation Layers or D3D Debug Layer */
	DebugLayers = 1 << 0	
};
ZE_ENABLE_FLAG_ENUMS(BackendFlagBits, BackendFlags);
	
/**
 * A graphical backend, this is what links the engine and the GPU
 * This class is only used to generate a Device and to check for support
 */
class Backend
{
public:
	explicit Backend(const BackendFlags& in_flags) : shader_language(ShaderLanguage::DXIL) { UnusedParameters{ in_flags }; }
	virtual ~Backend() = default;
	
	Backend(const Backend&) = delete;
	Backend(Backend&&) noexcept = default;
	
	void operator=(const Backend&) = delete;
	Backend& operator=(Backend&&) noexcept = default;

	/**
	 * Try create a logical device mapping to a physical device
	 * It is the backend responsibility to ensure that the GPU supports the requested features
	 * \param in_requested_shader_model : Requested shader model, controls which specific features to enable on certain backend
	 * \return A device or null no compatible has been found
	 */
	virtual Result<std::unique_ptr<BackendDevice>, std::string> create_device(ShaderModel in_requested_shader_model) = 0;

	/**
	 * Get the supported shader models by this backend, ignoring any currently installed GPU limitation
	 */
	[[nodiscard]] const auto& get_supported_shader_models() const { return supported_shader_models;}
	[[nodiscard]] const auto& get_shader_language() const { return shader_language;}
	[[nodiscard]] std::string_view get_name() const { return name; }
protected:
	std::string name;
	ShaderLanguage shader_language;
	std::vector<ShaderModel> supported_shader_models;
};

/**
 * Get the currently used backend
 */
Backend* get_backend();

	
}