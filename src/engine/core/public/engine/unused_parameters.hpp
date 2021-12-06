#pragma once

namespace ze
{

/**
 * Utility structure to remove warnings about unused parameters
 */
struct UnusedParameters
{
	template<typename... Args>
	UnusedParameters(const Args&...) {}
};

}