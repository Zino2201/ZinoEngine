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
	ErrorSurfaceLost = 6,
};

}

namespace std
{

inline std::string to_string(const ze::gfx::GfxResult& in_result)
{
	switch(in_result)
	{
	case ze::gfx::GfxResult::Success:
		return "Success";
	case ze::gfx::GfxResult::Timeout:
		return "Timeout";
	case ze::gfx::GfxResult::ErrorUnknown:
		return "ErrorUnknown";
	case ze::gfx::GfxResult::ErrorOutOfDeviceMemory:
		return "ErrorOutOfDeviceMemory";
	case ze::gfx::GfxResult::ErrorOutOfHostMemory:
		return "ErrorOutOfHostMemory";
	case ze::gfx::GfxResult::ErrorInvalidParameter:
		return "ErrorInvalidParameter";
	case ze::gfx::GfxResult::ErrorInitializationFailed:
		return "ErrorInitializationFailed";
	case ze::gfx::GfxResult::ErrorSurfaceLost:
		return "ErrorSurfaceLost";
	}
}

}