#include "engine/application/application_module.hpp"
#if ZE_PLATFORM(WINDOWS)
#include "engine/application/windows/application.hpp"
#include "engine/application/windows/window.hpp"
#endif

namespace ze
{

ApplicationModule::ApplicationModule()
{
#if ZE_PLATFORM(WINDOWS)
	application = std::make_unique<WindowsPlatformApplication>(GetModuleHandle(nullptr));
#endif
}

ZE_IMPLEMENT_MODULE(ApplicationModule, Application);

}