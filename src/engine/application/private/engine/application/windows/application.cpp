#include "engine/application/windows/application.hpp"
#include "engine/application/windows/window.hpp"
#include "engine/application/windows/cursor.hpp"
#include "engine/application/message_handler.hpp"
#include <ShellScalingApi.h>

namespace ze::platform
{

ZE_DEFINE_LOG_CATEGORY(windows_application);

WindowsApplication* windows_platform_application = nullptr;

WindowsApplication::WindowsApplication(
	HINSTANCE in_instance)
	: message_handler(nullptr), instance(in_instance), window_locking_cursor(nullptr)
{
	windows_platform_application = this;

	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	timeBeginPeriod(1);
	register_win_class();
	update_monitors();
}

WindowsApplication::~WindowsApplication()
{
	::CoUninitialize();
	timeEndPeriod(1);
	windows_platform_application = nullptr;
}

void WindowsApplication::register_win_class()
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
	wnd_class.lpszClassName = L"ZinoEngine";
	wnd_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

	ZE_ASSERTF(::RegisterClass(&wnd_class), "Failed to register Windows window class!");
}

void WindowsApplication::update_monitors()
{
	monitor_infos.clear();

	EnumDisplayMonitors(nullptr,
		nullptr,
		[](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL
		{
			MONITORINFO win_monitor_info = { sizeof(MONITORINFO) };
			GetMonitorInfo(monitor, &win_monitor_info);

			UINT dpi_x;
			UINT dpi_y;
			GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);

			auto* app = reinterpret_cast<WindowsApplication*>(data);
			app->monitor_infos.emplace_back(
				glm::ivec4 {
					win_monitor_info.rcMonitor.left,
					win_monitor_info.rcMonitor.top,
					win_monitor_info.rcMonitor.right - win_monitor_info.rcMonitor.left,
					win_monitor_info.rcMonitor.bottom - win_monitor_info.rcMonitor.top,
				},
				glm::ivec4 {
					win_monitor_info.rcWork.left,
					win_monitor_info.rcWork.top,
					win_monitor_info.rcWork.right - win_monitor_info.rcWork.left,
					win_monitor_info.rcWork.bottom - win_monitor_info.rcWork.top,
				},
				static_cast<float>(dpi_x));

			return TRUE;
		},
		reinterpret_cast<LPARAM>(this));
}

LRESULT WindowsApplication::wnd_proc(WindowsWindow& in_window, uint32_t in_msg, WPARAM in_wparam, LPARAM in_lparam)
{
	auto convert_button = [](uint32_t in_msg) -> MouseButton
	{
		switch(in_msg)
		{
		default:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
			return MouseButton::Left;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			return MouseButton::Middle;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
			return MouseButton::Right;
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
			if (&in_window == window_locking_cursor)
				update_clip_cursor(window_locking_cursor);
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
			message_handler->on_mouse_wheel(in_window, 
				static_cast<float>(GET_WHEEL_DELTA_WPARAM(in_wparam)) / WHEEL_DELTA, get_mouse_pos());
			break;
		}
		case WM_SETCURSOR:
		{
			if (LOWORD(in_lparam) == HTCLIENT)
				message_handler->on_cursor_set();
			break;
		}
		case WM_CHAR:
		{
			return 0;
		}
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			const int32_t key_code = static_cast<int32_t>(in_wparam);
			const bool is_repeat = (in_lparam & 0x40000000) != 0;
			const uint32_t character_code = ::MapVirtualKeyW(key_code, MAPVK_VK_TO_CHAR);
			message_handler->on_key_down(convert_win_character_code(key_code), character_code, is_repeat);
			if (in_msg != WM_SYSKEYDOWN)
				return 0;
			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			const int32_t key_code = in_wparam;
			const bool is_repeat = (in_lparam & 0x40000000) != 0;
			const uint32_t character_code = ::MapVirtualKeyW(key_code, MAPVK_VK_TO_CHAR);
			message_handler->on_key_up(convert_win_character_code(key_code), character_code, is_repeat);
			return 0;
		}
		case WM_INPUT:
		{
			UINT data_size = sizeof(RAWINPUT);
			static BYTE lpb[sizeof(RAWINPUT)];
			GetRawInputData(reinterpret_cast<HRAWINPUT>(in_lparam), RID_INPUT, lpb, &data_size, sizeof(RAWINPUTHEADER));
			auto* raw = reinterpret_cast<RAWINPUT*>(lpb);

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				message_handler->on_mouse_move({ raw->data.mouse.lLastX, raw->data.mouse.lLastY });
			}

			break;
		}
		case WM_SETFOCUS:
		{
			update_clip_cursor(window_locking_cursor);
			break;
		}
		case WM_KILLFOCUS:
		{
			update_clip_cursor(nullptr);
			break;
		}
		}
	}

	return DefWindowProc(in_window.get_hwnd(), in_msg, in_wparam, in_lparam);
}

void WindowsApplication::register_window(WindowsWindow* window)
{
	windows.emplace_back(window);
}

void WindowsApplication::unregister_window(WindowsWindow* window)
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

void WindowsApplication::set_message_handler(ApplicationMessageHandler* in_message_handler)
{
	message_handler = in_message_handler;
}

void WindowsApplication::pump_messages()
{
	MSG msg;
	while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

std::unique_ptr<Window> WindowsApplication::create_window(const std::string& in_name,
	uint32_t in_width,
	uint32_t in_height,
	uint32_t in_x,
	uint32_t in_y,
	const WindowFlags& in_flags)
{
	return std::make_unique<WindowsWindow>(*this,
		in_name,
		in_width,
		in_height,
		in_x,
		in_y,
		in_flags);
}

void WindowsApplication::set_mouse_pos(const glm::ivec2& in_pos)
{
	::SetCursorPos(in_pos.x, in_pos.y);
}

glm::ivec2 WindowsApplication::get_mouse_pos() const
{
	POINT point;
	::GetCursorPos(&point);
	return { point.x, point.y };
}

void WindowsApplication::set_capture(const Window& in_window)
{
	::SetCapture(static_cast<HWND>(in_window.get_handle()));
}

void WindowsApplication::release_capture()
{
	::ReleaseCapture();
}

const MonitorInfo& WindowsApplication::get_monitor_info(uint32_t in_monitor) const
{
	return monitor_infos[in_monitor];
}

std::unique_ptr<Cursor> WindowsApplication::create_system_cursor(SystemCursor in_cursor)
{
	LPCTSTR name = IDC_ARROW;
	switch (in_cursor)
	{
	case SystemCursor::No:
		name = IDC_NO;
		break;
	case SystemCursor::Crosshair:
		name = IDC_CROSS;
		break;
	case SystemCursor::Ibeam:
		name = IDC_IBEAM;
		break;
	case SystemCursor::Arrow:
		name = IDC_ARROW;
		break;
	case SystemCursor::Hand:
		name = IDC_HAND;
		break;
	case SystemCursor::SizeAll:
		name = IDC_SIZEALL;
		break;
	case SystemCursor::SizeNorthEastSouthWest:
		name = IDC_SIZENESW;
		break;
	case SystemCursor::SizeNorthSouth:
		name = IDC_SIZENS;
		break;
	case SystemCursor::SizeNorthWestSouthEast:
		name = IDC_SIZENWSE;
		break;
	case SystemCursor::SizeWestEast:
		name = IDC_SIZEWE;
		break;
	case SystemCursor::Wait:
	case SystemCursor::WaitArrow:
		name = IDC_WAIT;
		break;
	}

	return std::make_unique<WindowsCursor>(LoadCursor(nullptr, name));
}

void WindowsApplication::set_cursor(Cursor* in_cursor)
{
	if (in_cursor)
		SetCursor(static_cast<WindowsCursor*>(in_cursor)->get_cursor());
	else
		SetCursor(nullptr);
}

void WindowsApplication::set_show_cursor(bool in_show)
{
	ShowCursor(in_show);
}

void WindowsApplication::lock_cursor(Window* in_window)
{
	window_locking_cursor = static_cast<WindowsWindow*>(in_window);
	update_clip_cursor(window_locking_cursor);
}

void WindowsApplication::unlock_cursor()
{
	window_locking_cursor = nullptr;
	update_clip_cursor(nullptr);
}

void WindowsApplication::update_clip_cursor(WindowsWindow* window)
{
	if(window)
	{
		RECT clip_rect;
		GetClientRect(window->get_hwnd(), &clip_rect);
		ClientToScreen(window->get_hwnd(), reinterpret_cast<POINT*>(&clip_rect.left));
		ClientToScreen(window->get_hwnd(), reinterpret_cast<POINT*>(&clip_rect.right));
		ClipCursor(&clip_rect);
	}
	else
	{
		ClipCursor(nullptr);
	}
}

KeyCode WindowsApplication::convert_win_character_code(int32_t in_character_code) const
{
	switch (in_character_code)
	{
	case '0': return KeyCode::Num0;
	case '1': return KeyCode::Num1;
	case '2': return KeyCode::Num2;
	case '3': return KeyCode::Num3;
	case '4': return KeyCode::Num4;
	case '5': return KeyCode::Num5;
	case '6': return KeyCode::Num6;
	case '7': return KeyCode::Num7;
	case '8': return KeyCode::Num8;
	case '9': return KeyCode::Num9;
	case 'A': return KeyCode::A;
	case 'B': return KeyCode::B;
	case 'C': return KeyCode::C;
	case 'D': return KeyCode::D;
	case 'E': return KeyCode::E;
	case 'F': return KeyCode::F;
	case 'G': return KeyCode::G;
	case 'H': return KeyCode::H;
	case 'I': return KeyCode::I;
	case 'J': return KeyCode::J;
	case 'K': return KeyCode::K;
	case 'L': return KeyCode::L;
	case 'M': return KeyCode::M;
	case 'N': return KeyCode::N;
	case 'O': return KeyCode::O;
	case 'P': return KeyCode::P;
	case 'Q': return KeyCode::Q;
	case 'R': return KeyCode::R;
	case 'S': return KeyCode::S;
	case 'T': return KeyCode::T;
	case 'U': return KeyCode::U;
	case 'V': return KeyCode::V;
	case 'W': return KeyCode::W;
	case 'X': return KeyCode::X;
	case 'Y': return KeyCode::Y;
	case 'Z': return KeyCode::Z;
	case VK_NUMPAD0: return KeyCode::Numpad0;
	case VK_NUMPAD1: return KeyCode::Numpad1;
	case VK_NUMPAD2: return KeyCode::Numpad2;
	case VK_NUMPAD3: return KeyCode::Numpad3;
	case VK_NUMPAD4: return KeyCode::Numpad4;
	case VK_NUMPAD5: return KeyCode::Numpad5;
	case VK_NUMPAD6: return KeyCode::Numpad6;
	case VK_NUMPAD7: return KeyCode::Numpad7;
	case VK_NUMPAD8: return KeyCode::Numpad8;
	case VK_NUMPAD9: return KeyCode::Numpad9;
	case VK_ESCAPE: return KeyCode::Escape;
	case VK_LCONTROL: return KeyCode::LeftControl;
	case VK_RCONTROL: return KeyCode::RightControl;
	case VK_LMENU: return KeyCode::LeftAlt;
	case VK_RMENU: return KeyCode::RightAlt;
	case VK_SHIFT: return KeyCode::LeftShift;
	case VK_RSHIFT: return KeyCode::RightShift;
	default: return KeyCode::None;
	}
}

WindowsWindow* WindowsApplication::get_window_by_hwnd(HWND in_hwnd) const
{
	for(const auto& window : windows)
	{
		if (window->get_hwnd() == in_hwnd)
			return window;
	}

	return nullptr;
}

}