#include "engine/core.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "engine/hal/library.hpp"

namespace ze::hal
{

void* load_library(const std::string_view& path)
{
	return ::LoadLibraryA(path.data());
}

void* get_function_address(void* lib, const std::string_view& name)
{
	return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(lib), name.data()));
}

void free_library(void* lib)
{
	::FreeLibrary(static_cast<HMODULE>(lib));
}

}