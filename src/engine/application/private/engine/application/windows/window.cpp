#include "engine/application/windows/window.hpp"
#include <boost/locale.hpp>

namespace ze::platform
{

WindowsWindow::WindowsWindow(
	WindowsApplication& in_application,
	const std::string& in_name,
	uint32_t in_width, 
	uint32_t in_height, 
	uint32_t in_x,
	uint32_t in_y, 
	const WindowFlags& in_flags) : Window(in_name, in_width, in_height, in_x, in_y, in_flags),
	application(in_application), hwnd(nullptr), width(in_width), height(in_height), position(in_x, in_y), style(0), ex_style(0)
{
	const std::wstring wide_name = boost::locale::conv::utf_to_utf<wchar_t, char>(in_name);

	if(in_flags & WindowFlagBits::Borderless)
		style |= WS_POPUP;
	else
		style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	if (in_flags & WindowFlagBits::Resizable)
		style |= WS_THICKFRAME;

	ex_style |= WS_EX_LAYERED;

	/**
	 * We need to adjust the initial rect since CreateWindowEx
	 * width/height is the whole window size and not client area
	 * (Only in non-bordeless)
	 */
	RECT initial_area = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRectEx(&initial_area,
		style,
		FALSE,
		ex_style);

	hwnd = ::CreateWindowEx(ex_style,
		L"ZinoEngine",
		wide_name.c_str(),
		style,
		static_cast<int32_t>(in_x),
		static_cast<int32_t>(in_y),
		initial_area.right - initial_area.left,
		initial_area.bottom - initial_area.top,
		nullptr,
		nullptr,
		application.get_hinstance(),
		nullptr);
	ZE_ASSERT(hwnd);

	SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

	application.register_window(this);

	if(in_flags & WindowFlagBits::Centered)
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

	ShowWindow(hwnd, in_flags & WindowFlagBits::Maximized ? SW_MAXIMIZE : SW_SHOW);
}

WindowsWindow::~WindowsWindow()
{
	if(hwnd)
	{
		application.unregister_window(this);
		DestroyWindow(hwnd);
	}
}

void WindowsWindow::set_title(const std::string& in_name)
{
	const std::wstring wide_name = boost::locale::conv::utf_to_utf<wchar_t, char>(in_name);
	::SetWindowText(hwnd, wide_name.c_str());
}

void WindowsWindow::set_size(glm::ivec2 in_size)
{
	width = in_size.x;
	height = in_size.y;

	RECT initial_area = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRectEx(&initial_area,
		style,
		FALSE,
		ex_style);

	::SetWindowPos(hwnd,
		nullptr,
		position.x,
		position.y,
		initial_area.right - initial_area.left,
		initial_area.bottom - initial_area.top,
		0);
}

void WindowsWindow::set_position(glm::ivec2 in_position)
{
	position = in_position;
	::SetWindowPos(hwnd,
		nullptr,
		position.x,
		position.y,
		width,
		height,
		0);
}

void WindowsWindow::set_opacity(float in_alpha)
{
	SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(in_alpha * 255.f), LWA_ALPHA);
}

void WindowsWindow::show()
{
	::ShowWindow(hwnd, SW_SHOW);
}

LRESULT CALLBACK WindowsWindow::wnd_proc(uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam)
{
	const LRESULT def_result = DefWindowProc(hwnd, in_msg, in_wparam, in_lparam);

	switch(in_msg)
	{
	case WM_SIZE:
	{
		width = LOWORD(in_lparam);
		height = HIWORD(in_lparam);
		break;
	}
	case WM_MOVE:
	{
		position.x = LOWORD(in_lparam);
		position.y = HIWORD(in_lparam);
		break;
	}
	}

	application.wnd_proc(*this, in_msg, in_wparam, in_lparam);
	return def_result;
}

}