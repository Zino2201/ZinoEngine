#pragma once

#include "engine/core.hpp"
#include <string_view>
#include <vector>
#include "engine/result.hpp"

namespace ze
{

class Module;

enum class ModuleLoadError
{
	InvalidModule,
	MissingImplementModuleMacro,
	NotFound,
};

/**
 * Load the specified module, a correspond dll/lib must be present
 */
[[nodiscard]] Result<Module*, ModuleLoadError> load_module(const std::string_view& name);

/**
 * Unload the specified module
 * \warn Will also unload the corresponding DLL!
 */
void unload_module(const std::string_view& name);

void unload_all_modules();

/**
 * Get the module class, try to load the module
 * Returns nullptr if not found, use \load if requiring load reason error
 */
template<typename T>
	requires std::derived_from<T, Module>
[[nodiscard]] T* get_module(const std::string_view& name)
{
	auto result = load_module(name);
	if (result.has_value())
		return static_cast<T*>(result.get_value());

	UnusedParameters{ result.get_error() };
	return nullptr;
}

}