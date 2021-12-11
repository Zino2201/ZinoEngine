#pragma once

#include "engine/core.hpp"
#include "engine/flags.hpp"
#include <glm/glm.hpp>

namespace ze::platform
{

enum class WindowFlagBits
{
	Centered = 1 << 0,
	Maximized = 1 << 1,
	Borderless = 1 << 2,
};
ZE_ENABLE_FLAG_ENUMS(WindowFlagBits, WindowFlags);

class Window
{
public:
	Window(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const WindowFlags& in_flags = WindowFlags())
	{
		UnusedParameters{ in_name, in_width, in_height, in_x, in_y, in_flags };
	}

	virtual ~Window() = default;

	virtual void set_title(const std::string& in_name) = 0;
	virtual void set_size(glm::ivec2 in_size) = 0;
	virtual void set_position(glm::ivec2 in_position) = 0;
	virtual void show() = 0;

	[[nodiscard]] virtual void* get_handle() const = 0;
	[[nodiscard]] virtual uint32_t get_width() const = 0;
	[[nodiscard]] virtual uint32_t get_height() const = 0;
	[[nodiscard]] virtual glm::ivec2 get_position() const = 0;
};

}