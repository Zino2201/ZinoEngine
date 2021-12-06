#pragma once

#include "engine/core.hpp"

namespace ze::gfx
{

struct SwapChainCreateInfo
{
	void* os_handle;
	uint32_t width;
	uint32_t height;
	BackendDeviceResource old_swapchain;

	SwapChainCreateInfo(void* in_window_handle = nullptr,
		const uint32_t& in_width = 0,
		const uint32_t& in_height = 0,
		const BackendDeviceResource in_old_swapchain = {}) : os_handle(in_window_handle),
		width(in_width), height(in_height), old_swapchain(in_old_swapchain) {}
};
	
}