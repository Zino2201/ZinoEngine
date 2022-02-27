#pragma once

#include <memory>
#include "engine/application/message_handler.hpp"
#include "engine/gfx/device.hpp"
#include "glm/vec3.hpp"

namespace ze
{

namespace gfx { class Device; }
namespace shadersystem { class ShaderManager;  }

class Engine : public platform::ApplicationMessageHandler
{
public:
	Engine();
	~Engine();

	void run();
private:
	void create_swapchain(const gfx::UniqueSwapchain& old_swapchain);
	void on_resized_window(platform::Window& in_window, uint32_t in_width, uint32_t in_height) override;
	void on_closing_window(platform::Window& in_window) override;
	void on_mouse_down(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos) override;
	void on_mouse_up(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos) override;
	void on_mouse_wheel(platform::Window& in_window, const float in_delta, const glm::ivec2& in_mouse_pos) override;
	void on_mouse_double_click(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos) override;
	void on_cursor_set() override;
	void on_key_down(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat) override;
	void on_key_up(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat) override;
	void on_mouse_move(const glm::ivec2& in_delta) override;
private:
	bool running;
	std::unique_ptr<platform::Window> main_window;
	std::unique_ptr<gfx::Backend> backend;
	std::unique_ptr<gfx::Device> device;
	std::unique_ptr<shadersystem::ShaderManager> shader_manager;
	gfx::UniqueSwapchain swapchain;
	gfx::UniqueSemaphore image_available_semaphore;
	gfx::UniqueSemaphore render_finished_semaphore;
	glm::ivec2 mouse_delta;
};

}
