#pragma once

#include "engine/core.hpp"
#include "engine/application/platform_window.hpp"
#include <Windows.h>

namespace ze
{

class WindowsPlatformApplication;

class WindowsPlatformWindow : public PlatformWindow
{
public:
	WindowsPlatformWindow(WindowsPlatformApplication& in_application,
		const std::string& in_name,
		uint32_t in_width,
		uint32_t in_height,
		uint32_t in_x,
		uint32_t in_y,
		const PlatformWindowFlags& in_flags = PlatformWindowFlags());
	~WindowsPlatformWindow() override;

	LRESULT CALLBACK wnd_proc(uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam);

	HWND get_hwnd() const { return hwnd; }
	void* get_handle() const override { return hwnd; }
	uint32_t get_width() const override { return width;  }
	uint32_t get_height() const override { return height;  }
	glm::ivec2 get_position() const override { return position; }
private:
	WindowsPlatformApplication& application;
	HWND hwnd;
	uint32_t width;
	uint32_t height;
	glm::ivec2 position;
};

}