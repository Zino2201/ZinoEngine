#pragma once

#include "engine/core.hpp"
#include <imgui.h>
#include "engine/application/message_handler.hpp"
#include "engine/gfx/device.hpp"
#include "glm/vec2.hpp"
#include "engine/gfx/uniform_buffer.hpp"

namespace ze::imgui
{

struct WindowData
{
	std::variant<gfx::UniqueSwapchain, gfx::SwapchainHandle> swapchain;

	WindowData() = default;

	gfx::SwapchainHandle get_swapchain() const
	{
		if (swapchain.index() == 0)
			return std::get<0>(swapchain).get();

		return std::get<1>(swapchain);
	}

	bool does_own_swapchain() const
	{
		return swapchain.index() == 0;
	}
};

struct ViewportDrawData
{
	struct GlobalData
	{
		glm::vec2 translate;
		glm::vec2 scale;
	};

	gfx::UniqueBuffer vertex_buffer;
	gfx::UniqueBuffer index_buffer;
	gfx::UniformBuffer<GlobalData> global_data;
	size_t vertex_buffer_size;
	size_t index_buffer_size;

	ViewportDrawData() : vertex_buffer_size(0), index_buffer_size(0) {}
};

struct ViewportPlatformData
{
	std::variant<std::unique_ptr<platform::Window>, platform::Window*> window;

	[[nodiscard]] platform::Window* get_window() const
	{
		if (window.index() == 0)
			return std::get<0>(window).get();

		return std::get<1>(window);
	}
};

struct ViewportRendererData
{
	WindowData window;
	ViewportDrawData draw_data;
	gfx::UniqueSemaphore image_available_semaphore;
	gfx::UniqueSemaphore render_finished_semaphore;
	bool has_submitted_work;

	ViewportRendererData() : has_submitted_work(false)
	{
		image_available_semaphore = gfx::UniqueSemaphore(
			gfx::get_device()->create_semaphore(gfx::SemaphoreInfo().set_debug_name("ImGui Image Available Semaphore")).get_value());
		render_finished_semaphore = gfx::UniqueSemaphore(
			gfx::get_device()->create_semaphore(gfx::SemaphoreInfo().set_debug_name("ImGui Render Finished Semaphore")).get_value());
	}
};

/**
 * Initialize ImGui renderer (create default font atlas and setup pipeline & shaders)
 */
void initialize();
void initialize_main_viewport(platform::Window& in_window, gfx::SwapchainHandle in_swapchain);
void update_main_viewport(platform::Window& in_window, gfx::SwapchainHandle in_swapchain);
void new_frame(float in_delta_time, platform::Window& in_main_window);
void draw_viewport(ImGuiViewport* viewport);
void swap_buffers(ImGuiViewport* viewport);
void draw_viewports();
void present_viewports();
void update_monitors();
void destroy();

/** Events, mirrored from platform::ApplicationMessageHandler */
void on_mouse_down(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos);
void on_mouse_up(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos);
void on_mouse_wheel(platform::Window& in_window, const float in_delta, const glm::ivec2& in_mouse_pos);
void on_resized_window(platform::Window& in_window, uint32_t in_width, uint32_t in_height);

}