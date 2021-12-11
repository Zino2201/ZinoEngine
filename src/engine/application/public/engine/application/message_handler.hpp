#pragma once

#include "glm/vec2.hpp"

namespace ze::platform
{

class Window;

enum class MouseButton
{
	Left,
	Right,
	Middle,
};

/**
 * Base class that handle platform application messages
 * This is given to the platform application at construction time
 */
class ApplicationMessageHandler
{
public:
	virtual ~ApplicationMessageHandler() = default;

	virtual void on_resizing_window(Window& in_window, uint32_t in_width, uint32_t in_height) { UnusedParameters{in_window, in_width, in_height}; }
	virtual void on_resized_window(Window& in_window, uint32_t in_width, uint32_t in_height) { UnusedParameters{ in_window, in_width, in_height }; }
	virtual void on_closing_window(Window& in_window) { UnusedParameters{ in_window }; }
	virtual void on_mouse_down(Window& in_window, MouseButton in_button, const glm::ivec2& in_mouse_pos) { UnusedParameters{ in_window, in_button, in_mouse_pos }; }
	virtual void on_mouse_up(Window& in_window, MouseButton in_button, const glm::ivec2& in_mouse_pos) { UnusedParameters{ in_window, in_button, in_mouse_pos }; }
	virtual void on_mouse_double_click(Window& in_window, MouseButton in_button, const glm::ivec2& in_mouse_pos) { UnusedParameters{ in_window, in_button, in_mouse_pos }; }
	virtual void on_mouse_wheel(Window& in_window, const float in_delta, const glm::ivec2& in_mouse_pos) { UnusedParameters{ in_window, in_delta, in_mouse_pos }; }
};

}
