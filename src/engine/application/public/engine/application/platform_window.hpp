#pragma once

#include "engine/core.hpp"
#include "engine/flags.hpp"
#include "engine/multicast_delegate.hpp"

namespace ze
{

enum class PlatformWindowFlagBits
{
	Centered = 1 << 0,
	Maximized = 1 << 1,
};
ZE_ENABLE_FLAG_ENUMS(PlatformWindowFlagBits, PlatformWindowFlags);

class PlatformWindow
{
public:
	PlatformWindow(const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const PlatformWindowFlags& in_flags = PlatformWindowFlags())
	{
		UnusedParameters{ in_name, in_width, in_height, in_x, in_y, in_flags };
	}

	virtual ~PlatformWindow() = default;

	[[nodiscard]] virtual void* get_handle() const = 0;
	[[nodiscard]] virtual uint32_t get_width() const = 0;
	[[nodiscard]] virtual uint32_t get_height() const = 0;
};

}