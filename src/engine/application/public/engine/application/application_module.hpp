#pragma once

#include "engine/core.hpp"
#include "engine/module/module.hpp"

namespace ze::platform
{

class Application;

class ApplicationModule : public Module
{
public:
	ApplicationModule();

	Application& get_application() const { return *application.get(); }
private:
	std::unique_ptr<Application> application;
};

}