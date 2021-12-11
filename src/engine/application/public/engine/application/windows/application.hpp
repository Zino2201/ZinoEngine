#pragma once

#include "engine/core.hpp"
#include "engine/application/platform_application.hpp"
#include <Windows.h>

namespace ze::platform
{

class WindowsWindow;

class WindowsApplication : public Application
{
public:
	WindowsApplication(HINSTANCE in_instance);
	~WindowsApplication();

	/** Called by each WindowsPlatformWindow::wnd_proc */
	void wnd_proc(WindowsWindow& window, uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam);

	void register_window(WindowsWindow* window);
	void unregister_window(WindowsWindow* window);

	void set_message_handler(ApplicationMessageHandler* in_message_handler) override;
	void pump_messages() override;
	std::unique_ptr<Window> create_window(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const WindowFlags& in_flags = WindowFlags()) override;
	glm::ivec2 get_mouse_pos() const override;

	/** Monitor API */
	uint32_t get_num_monitors() const override { return monitor_infos.size(); }
	const MonitorInfo& get_monitor_info(uint32_t in_monitor) const override;

	/** Cursor API */
	std::unique_ptr<Cursor> create_system_cursor(SystemCursor in_cursor) override;
	void set_cursor(Cursor& in_cursor) override;
	void set_show_cursor(bool in_show) override;

	[[nodiscard]] HINSTANCE get_hinstance() const { return instance; }
private:
	void register_win_class();
	void update_monitors();
	[[nodiscard]] WindowsWindow* get_window_by_hwnd(HWND in_hwnd) const;
private:
	ApplicationMessageHandler* message_handler;
	HINSTANCE instance;
	std::vector<WindowsWindow*> windows;
	std::vector<MonitorInfo> monitor_infos;
};

}