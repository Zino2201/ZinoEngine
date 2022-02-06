#pragma once

#include <glm/glm.hpp>
#include "platform_window.hpp"
#include "cursor.hpp"

namespace ze::platform
{

class ApplicationMessageHandler;

struct MonitorInfo
{
	glm::ivec4 bounds;
	glm::ivec4 work_bounds;
	float dpi;

	MonitorInfo(glm::ivec4 in_bounds,
		glm::ivec4 in_work_bounds,
		float in_dpi) : bounds(in_bounds), work_bounds(in_work_bounds), dpi(in_dpi) {}
};

class Application
{
public:
	Application() = default;
	virtual ~Application() = default;

	virtual void set_message_handler(ApplicationMessageHandler* in_message_handler) = 0;
	virtual void pump_messages() = 0;
	[[nodiscard]] virtual std::unique_ptr<Window> create_window(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const WindowFlags& in_flags = WindowFlags()) = 0;
	virtual void set_mouse_pos(const glm::ivec2& in_pos) = 0;
	[[nodiscard]] virtual glm::ivec2 get_mouse_pos() const = 0;
	[[nodiscard]] virtual void set_capture(const Window& in_window) = 0;
	[[nodiscard]] virtual void release_capture() = 0;

	/** Monitor API */
	[[nodiscard]] virtual size_t get_num_monitors() const = 0;
	[[nodiscard]] virtual const MonitorInfo& get_monitor_info(uint32_t in_monitor) const = 0;

	/** Cursor API */
	[[nodiscard]] virtual std::unique_ptr<Cursor> create_system_cursor(SystemCursor in_cursor) = 0;
	virtual void set_cursor(Cursor* in_cursor) = 0;
	virtual void set_show_cursor(bool in_show) = 0;
	virtual void lock_cursor(Window* in_window) = 0;
	virtual void unlock_cursor() = 0;
};

}
