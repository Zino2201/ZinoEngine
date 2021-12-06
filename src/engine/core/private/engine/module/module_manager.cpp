#include "engine/core.hpp"
#include "engine/module/module.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/hal/library.hpp"
#include <boost/locale.hpp>

namespace ze
{

ZE_DEFINE_LOG_CATEGORY(module_manager);

using InstantiateModuleFunc = Module*(*)();

std::vector<std::unique_ptr<Module>> modules;
std::vector<std::string> loaded_module_names;

Result<Module*, ModuleLoadError> load_shared(const std::string_view& name)
{
	std::string corrected_path = fmt::format("ze-{}.{}", name, hal::get_dynamic_library_ext());
	corrected_path = boost::locale::to_lower(corrected_path);

	void* module = hal::load_library(corrected_path.c_str());
	if (!module)
		return make_error(ModuleLoadError::NotFound);

	auto func = reinterpret_cast<InstantiateModuleFunc>(hal::get_function_address(module, "ze_create_module"));
	if (!func)
		return make_error(ModuleLoadError::MissingImplementModuleMacro);

	Module* module_class = func();
	if (!module_class)
		return make_error(ModuleLoadError::InvalidModule);

	module_class->initialize(name, module);

	return make_result(module_class);
}

Result<Module*, ModuleLoadError> load_module(const std::string_view& name)
{
	/** Return module ptr if already exists */
	for (const auto& module : modules)
		if (module->get_name() == name)
			return make_result(module.get());

	auto result = load_shared(name);
	if (result)
	{
		Module* module = result.get_value();
		modules.emplace_back(module);
		logger::info(log_module_manager, "Loaded module {}", name);
		return make_result(module);
	}

	return make_error(result.get_error());
}

void unload_module(const std::string_view& name)
{
	for(auto it = modules.begin(); it != modules.end(); ++it)
	{
		const auto& module = *it;

		if(module->get_name() == name)
		{
			modules.erase(it);
		}
	}
}

void unload_all_modules()
{
	/** We remove one by one reversed since some modules may depend on another modules */
	for (auto& module : modules)
	{
		module.reset();
	}

	modules.clear();
}

}