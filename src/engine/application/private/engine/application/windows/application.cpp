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
			return windows_platform_application->wnd_proc(in_hwnd, in_msg, wparam, lparam);
		};
	wnd_class.hInstance = instance;
	wnd_class.lpszClassName = "ZinoEngine";

	ZE_ASSERTF(::RegisterClass(&wnd_class), "Failed to register Windows window class!");
}

LRESULT CALLBACK WindowsPlatformApplication::wnd_proc(HWND in_hwnd, uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam)
{
	if (message_handler)
	{
		if (const auto window = get_window_by_hwnd(in_hwnd))
		{
			window->wnd_proc(in_msg, in_wparam, in_lparam);

			switch (in_msg)
			{
			case WM_SIZING:
			{
				const RECT* rect = reinterpret_cast<const RECT*>(in_lparam);
				const uint32_t width = rect->right - rect->left;
				const uint32_t height = rect->bottom - rect->top;
				message_handler->on_resizing_window(*window, width, height);
				break;
			}
			case WM_SIZE:
			{
				const uint32_t width = LOWORD(in_lparam);
				const uint32_t height = HIWORD(in_lparam);
				message_handler->on_resized_window(*window, width, height);
				break;
			}
			}
		}
	}

	return DefWindowProc(in_hwnd, in_msg, in_wparam, in_lparam);
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