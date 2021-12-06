#pragma once

#include "engine/core.hpp"
#include "engine/module/module.hpp"
#include "engine/application/platform_window.hpp"

namespace ze
{

class PlatformApplication;

class ApplicationModule : public Module
{
public:
	ApplicationModule();

	PlatformApplication& get_application() const { return *application.get(); }
private:
	std::unique_ptr<PlatformApplication> application;
};

}