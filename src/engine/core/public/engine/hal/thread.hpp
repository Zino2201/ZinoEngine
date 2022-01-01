#pragma once

#include "engine/platform_macros.hpp"

#if ZE_PLATFORM(WINDOWS)
#include "windows/thread.hpp"
#else
#error "Platform not supported"
#endif