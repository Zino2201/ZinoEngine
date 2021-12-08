#pragma once

#include <glm/glm.hpp>

namespace ze
{

class PlatformApplicationMessageHandler;

class PlatformApplication
{
public:
	PlatformApplication() = default;
	virtual ~PlatformApplication() = default;

	virtual void set_message_handler(PlatformApplicationMessageHandler* in_message_handler) = 0;
	virtual void pump_messages() = 0;
	[[nodiscard]] virtual std::unique_ptr<PlatformWindow> create_window(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const PlatformWindowFlags& in_flags = PlatformWindowFlags()) = 0;
	[[nodiscard]] virtual glm::ivec2 get_mouse_pos() const = 0;
};

}