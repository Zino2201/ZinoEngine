#pragma once

#include <string_view>

namespace ze::hal
{

void* load_library(const std::string_view& path);
void* get_function_address(void* lib, const std::string_view& name);
void free_library(void* lib);

inline std::string_view get_dynamic_library_ext() { return "dll"; }

}