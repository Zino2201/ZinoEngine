#pragma once

#include "glm/vec2.hpp"

namespace ze
{

class PlatformWindow;

enum class PlatformMouseButton
{
	Left,
	Right,
	Middle,
};

/**
 * Base class that handle platform application messages
 * This is given to the platform application at construction time
 */
class PlatformApplicationMessageHandler
{
public:
	virtual ~PlatformApplicationMessageHandler() = default;

	virtual void on_resizing_window(PlatformWindow& in_window, uint32_t in_width, uint32_t in_height) { UnusedParameters{in_window, in_width, in_height}; }
	virtual void on_resized_window(PlatformWindow& in_window, uint32_t in_width, uint32_t in_height) { UnusedParameters{ in_window, in_width, in_height }; }
	virtual void on_closing_window(PlatformWindow& in_window) { UnusedParameters{ in_window }; }
	virtual void on_mouse_down(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos) {}
	virtual void on_mouse_up(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos) {}
	virtual void on_mouse_double_click(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos) {}
	virtual void on_mouse_wheel(PlatformWindow& in_window, const float in_delta, const glm::ivec2& in_mouse_pos) {}
};

}
