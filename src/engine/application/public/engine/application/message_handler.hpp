#pragma once

#include "glm/vec2.hpp"
#include "engine/unused_parameters.hpp"

namespace ze::platform
{

class Window;

enum class MouseButton
{
	Left,
	Right,
	Middle,
};

enum class KeyCode
{
	None,
	Num0,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,
	Numpad0,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad4,
	Numpad5,
	Numpad6,
	Numpad7,
	Numpad8,
	Numpad9,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	Escape,
	LeftControl,
	RightControl,
	LeftAlt,
	RightAlt,
	LeftShift,
	RightShift,
	Space,
	Backspace,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
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
	virtual void on_cursor_set() {}
	virtual void on_key_down(const KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat) { UnusedParameters{ in_key_code, in_character_code, in_repeat }; }
	virtual void on_key_up(const KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat) { UnusedParameters{ in_key_code, in_character_code, in_repeat }; }
	virtual void on_key_char(const char in_char) { UnusedParameters{ in_char }; }
	virtual void on_mouse_move(const glm::ivec2& in_delta) { UnusedParameters{ in_delta }; }
};

}
