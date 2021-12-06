#include "engine/gfx/window.hpp"
#include <GLFW/glfw3.h>
#if ZE_PLATFORM(WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace ze
{

void on_window_size_changed(GLFWwindow* glfw_window, int width, int height)
{
	auto window = static_cast<Window*>(nullptr);// glfwGetWindowUserPointer(glfw_window));
	window->width = width;
	window->height = height;
	window->window_resized_delegate.call(static_cast<uint32_t>(width),
		static_cast<uint32_t>(height));
}

Window::Window(const uint32_t in_width,
	const uint32_t in_height,
	WindowFlags in_flags) : window(nullptr), width(in_width), height(in_height), flags(std::move(in_flags))
{
#if 0
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_MAXIMIZED , GL_TRUE);
	window = glfwCreateWindow(width, 
		height, 
		"ZinoEngine", 
		nullptr, 
		nullptr);

	int win_width;
	int win_height;
	glfwGetWindowSize(window, &win_width, &win_height);
	width = win_width;
	height = win_height;

	glfwSetWindowUserPointer(window, this);

	/** Binds callbacks */
	glfwSetWindowSizeCallback(window, &on_window_size_changed);
	
	if(flags & WindowFlagBits::Centered)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);

		const uint32_t x = (static_cast<uint32_t>(video_mode->width) / 2) - (width / 2);
		const uint32_t y = (static_cast<uint32_t>(video_mode->height) / 2) - (height / 2);
		glfwSetWindowPos(window, x, y);
	}
#endif
}

Window::~Window()
{
	//if(window)
	//	glfwDestroyWindow(window);
	window = nullptr;
}

void* Window::get_native_handle() const
{
#if ZE_PLATFORM(WINDOWS)
	//return glfwGetWin32Window(window);
#else
	ZE_ASSERTF(false, "Unsupported platform");
#endif

	return nullptr;
}

	
}