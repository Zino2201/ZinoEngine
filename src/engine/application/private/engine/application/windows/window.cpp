#include "engine/application/windows/window.hpp"
#include <boost/locale.hpp>

namespace ze
{

WindowsPlatformWindow::WindowsPlatformWindow(
	WindowsPlatformApplication& in_application,
	const std::string& in_name,
	uint32_t in_width, 
	uint32_t in_height, 
	uint32_t in_x,
	uint32_t in_y, 
	const PlatformWindowFlags& in_flags) : PlatformWindow(in_name, in_width, in_height, in_x, in_y, in_flags),
	application(in_application), hwnd(nullptr), width(in_width), height(in_height), position(in_x, in_y)
{
	DWORD ex_style = 0;
	DWORD style = WS_OVERLAPPEDWINDOW;
	hwnd = ::CreateWindowEx(ex_style,
		"ZinoEngine",
		in_name.c_str(),
		style,
		static_cast<int32_t>(in_x),
		static_cast<int32_t>(in_y),
		static_cast<int32_t>(in_width),
		static_cast<int32_t>(in_height),
		nullptr,
		nullptr,
		application.get_hinstance(),
		nullptr);
	ZE_ASSERT(hwnd);

	application.register_window(this);

	if(in_flags & PlatformWindowFlagBits::Centered)
	{
		const int screen_width = GetSystemMetrics(SM_CXSCREEN);
		const int screen_height = GetSystemMetrics(SM_CYSCREEN);

		RECT client_rect;
		GetClientRect(hwnd, &client_rect);
		::AdjustWindowRectEx(&client_rect,
			style,
			FALSE,
			ex_style);

		const int client_width = client_rect.right - client_rect.left;
		const int client_height = client_rect.bottom - client_rect.top;

		SetWindowPos(hwnd,
			nullptr,
			screen_width / 2 - client_width / 2,
			screen_height / 2 - client_height / 2,
			client_width,
			client_height,
			0);

		width = client_width;
		height = client_height;
		position = { screen_width / 2 - client_width / 2, screen_height / 2 - client_height / 2 };
	}

	ShowWindow(hwnd, in_flags & PlatformWindowFlagBits::Maximized ? SW_MAXIMIZE : SW_SHOW);
}

WindowsPlatformWindow::~WindowsPlatformWindow()
{
	if(hwnd)
	{
		application.unregister_window(this);
		DestroyWindow(hwnd);
	}
}


LRESULT CALLBACK WindowsPlatformWindow::wnd_proc(uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam)
{
	application.wnd_proc(*this, in_msg, in_wparam, in_lparam);

	switch(in_msg)
	{
	case WM_SIZE:
	{
		width = LOWORD(in_lparam);
		height = HIWORD(in_lparam);
		return 0;
	}
	case WM_MOVE:
	{
		position.x = LOWORD(in_lparam);
		position.y = HIWORD(in_lparam);
		return 0;
	}
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		return TRUE;
	}

	return DefWindowProc(hwnd, in_msg, in_wparam, in_lparam);
}

}