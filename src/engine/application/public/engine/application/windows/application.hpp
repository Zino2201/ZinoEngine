#pragma once

#include "engine/core.hpp"
#include "engine/application/platform_application.hpp"
#include <Windows.h>

namespace ze
{

class WindowsPlatformWindow;

class WindowsPlatformApplication : public PlatformApplication
{
public:
	WindowsPlatformApplication(HINSTANCE in_instance);
	~WindowsPlatformApplication();

	/** Called by each WindowsPlatformWindow::wnd_proc */
	void wnd_proc(WindowsPlatformWindow& window, uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam);

	void register_window(WindowsPlatformWindow* window);
	void unregister_window(WindowsPlatformWindow* window);

	void set_message_handler(PlatformApplicationMessageHandler* in_message_handler) override;
	void pump_messages() override;
	std::unique_ptr<PlatformWindow> create_window(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const PlatformWindowFlags& in_flags = PlatformWindowFlags()) override;
	glm::ivec2 get_mouse_pos() const override;

	[[nodiscard]] HINSTANCE get_hinstance() const { return instance; }
private:
	void register_win_class();
	[[nodiscard]] WindowsPlatformWindow* get_window_by_hwnd(HWND in_hwnd) const;
private:
	PlatformApplicationMessageHandler* message_handler;
	HINSTANCE instance;
	std::vector<WindowsPlatformWindow*> windows;
};

}