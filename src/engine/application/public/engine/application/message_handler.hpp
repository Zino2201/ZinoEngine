#pragma once

namespace ze
{

class PlatformWindow;

/**
 * Base class that handle platform application messages
 * This is given to the platform application at construction time
 */
class PlatformApplicationMessageHandler
{
public:
	virtual ~PlatformApplicationMessageHandler() = default;

	virtual void on_resizing_window(PlatformWindow& in_window, uint32_t in_width, uint32_t in_height) {}
	virtual void on_resized_window(PlatformWindow& in_window, uint32_t in_width, uint32_t in_height) {}
};

}