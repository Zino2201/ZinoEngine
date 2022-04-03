#pragma once

#include "vulkan.hpp"

namespace ze::gfx
{

struct VulkanShader
{
	VulkanDevice& device;
	VkShaderModule shader_module;

	VulkanShader(VulkanDevice& in_device, VkShaderModule in_shader_module)
		: device(in_device), shader_module(in_shader_module) {}
	
	~VulkanShader()
	{
		vkDestroyShaderModule(device.get_device(), shader_module, nullptr);
	}

	VulkanShader(const VulkanShader&) = delete;
	void operator=(const VulkanShader&) = delete;
	
	VulkanShader(VulkanShader&& in_other) noexcept : device(in_other.device),
		shader_module(std::exchange(shader_module, VK_NULL_HANDLE)) {}
};

inline VkShaderStageFlagBits convert_shader_stage_bits(ShaderStageFlagBits in_stage)
{
	switch (in_stage)
	{
	default:
	case ShaderStageFlagBits::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderStageFlagBits::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderStageFlagBits::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderStageFlagBits::TessellationControl:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderStageFlagBits::TessellationEvaluation:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case ShaderStageFlagBits::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}
}

inline VkShaderStageFlags convert_shader_stage_flags(ShaderStageFlags in_flags)
{
	VkShaderStageFlags flags = 0;

	if (in_flags == all_shader_stages)
		return VK_SHADER_STAGE_ALL;

	if(in_flags & ShaderStageFlagBits::Vertex)
		flags |= VK_SHADER_STAGE_VERTEX_BIT;
	
	if(in_flags & ShaderStageFlagBits::Fragment)
		flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	
	if(in_flags & ShaderStageFlagBits::Geometry)
		flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	
	if(in_flags & ShaderStageFlagBits::TessellationEvaluation)
		flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	
	if(in_flags & ShaderStageFlagBits::TessellationControl)
		flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	
	if(in_flags & ShaderStageFlagBits::Compute)
		flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	
	return flags;
}
	
}