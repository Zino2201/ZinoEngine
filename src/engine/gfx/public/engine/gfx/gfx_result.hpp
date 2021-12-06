#pragma once

namespace ze::gfx
{

/**
 * Possible resulting success/error code from gfx functions
 */
enum class GfxResult
{
	Success = 0,
	Timeout = 1,
	
	ErrorUnknown = -1,
	ErrorOutOfDeviceMemory = -2,
	ErrorOutOfHostMemory = -3,
	ErrorInvalidParameter = -4,
	ErrorInitializationFailed = -5,
};

}