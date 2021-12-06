#pragma once

#include <string_view>

namespace ze
{

/**
 * Base class for modules
 */
class Module
{
public:
	Module();
	virtual ~Module() = default;

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;

	Module(Module&&) = default;
	Module& operator=(Module&&) = default;

	void initialize(std::string_view in_name, void* in_handle)
	{
		name = in_name;
		handle = in_handle;
	}

	[[nodiscard]] const std::string& get_name() const { return name; }
	[[nodiscard]] void* get_handle() const { return handle; }
private:
	std::string name;
	void* handle;
};

#define ZE_IMPLEMENT_MODULE(ModuleClass, ModuleName) \
	extern "C" ze::Module* ze_create_module() \
	{ \
		return new ModuleClass; \
	} 
	
}