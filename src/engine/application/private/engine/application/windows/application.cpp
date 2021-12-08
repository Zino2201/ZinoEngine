#include "engine/application/windows/application.hpp"
#include "engine/application/windows/window.hpp"
#include "engine/application/message_handler.hpp"

namespace ze
{

ZE_DEFINE_LOG_CATEGORY(windows_application);

WindowsPlatformApplication* windows_platform_application = nullptr;

WindowsPlatformApplication::WindowsPlatformApplication(
	HINSTANCE in_instance)
	: message_handler(nullptr), instance(in_instance)
{
	windows_platform_application = this;

	::CoInitialize(nullptr);
	register_win_class();
}

WindowsPlatformApplication::~WindowsPlatformApplication()
{
	::CoUninitialize();
	windows_platform_application = nullptr;
}

void WindowsPlatformApplication::register_win_class()
{
	WNDCLASS wnd_class = {};
	wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wnd_class.lpfnWndProc = 
		[](HWND in_hwnd, uint32_t in_msg, WPARAM wparam, LPARAM lparam) -> LRESULT
		{
			if(auto* window = windows_platform_application->get_window_by_hwnd(in_hwnd))
				return window->wnd_proc(in_msg, wparam, lparam);

			return DefWindowProc(in_hwnd, in_msg, wparam, lparam);
		};
	wnd_class.hInstance = instance;
	wnd_class.lpszClassName = "ZinoEngine";

	ZE_ASSERTF(::RegisterClass(&wnd_class), "Failed to register Windows window class!");
}

void WindowsPlatformApplication::wnd_proc(WindowsPlatformWindow& in_window, uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam)
{
	auto convert_button = [](uint32_t in_msg) -> PlatformMouseButton
	{
		switch(in_msg)
		{
		default:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
			return PlatformMouseButton::Left;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			return PlatformMouseButton::Middle;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
			return PlatformMouseButton::Right;
		}
	};

	if (message_handler)
	{
		switch (in_msg)
		{
		case WM_SIZING:
		{
			message_handler->on_resizing_window(in_window, in_window.get_width(), in_window.get_height());
			break;
		}
		case WM_SIZE:
		{
			message_handler->on_resized_window(in_window, in_window.get_width(), in_window.get_height());
			break;
		}
		case WM_CLOSE:
		{
			message_handler->on_closing_window(in_window);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			message_handler->on_mouse_down(in_window, convert_button(in_msg), get_mouse_pos());
			break;
		}
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
			message_handler->on_mouse_up(in_window, convert_button(in_msg), get_mouse_pos());
			break;
		}
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
			message_handler->on_mouse_double_click(in_window, convert_button(in_msg), get_mouse_pos());
			break;
		}
		case WM_MOUSEWHEEL:
		{
			message_handler->on_mouse_wheel(in_window, static_cast<float>(GET_WHEEL_DELTA_WPARAM(in_wparam)) / WHEEL_DELTA, get_mouse_pos());
			break;
		}
		}
	}
}

void WindowsPlatformApplication::register_window(WindowsPlatformWindow* window)
{
	windows.emplace_back(window);

	char name[512];
	GetWindowText(window->get_hwnd(), name, sizeof(name));

	logger::verbose(log_windows_application, "Registered window \"{}\"", name);
}

void WindowsPlatformApplication::unregister_window(WindowsPlatformWindow* window)
{
	for(auto it = windows.begin(); it != windows.end(); ++it)
	{
		if(*it == window)
		{
			windows.erase(it);
			break;
		}
	}
}

void WindowsPlatformApplication::set_message_handler(PlatformApplicationMessageHandler* in_message_handler)
{
	message_handler = in_message_handler;
}

void WindowsPlatformApplication::pump_messages()
{
	MSG msg;
	while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

std::unique_ptr<PlatformWindow> WindowsPlatformApplication::create_window(const std::string& in_name,
	uint32_t in_width,
	uint32_t in_height,
	uint32_t in_x,
	uint32_t in_y,
	const PlatformWindowFlags& in_flags)
{
	return std::make_unique<WindowsPlatformWindow>(*this,
		in_name,
		in_width,
		in_height,
		in_x,
		in_y,
		in_flags);
}

glm::ivec2 WindowsPlatformApplication::get_mouse_pos() const
{
	POINT point;
	::GetCursorPos(&point);
	return { point.x, point.y };
}

WindowsPlatformWindow* WindowsPlatformApplication::get_window_by_hwnd(HWND in_hwnd) const
{
	for(const auto& window : windows)
	{
		if (window->get_hwnd() == in_hwnd)
			return window;
	}

	return nullptr;
}

}