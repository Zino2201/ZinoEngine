#include "engine/application/application_module.hpp"
#if ZE_PLATFORM(WINDOWS)
#include "engine/application/windows/application.hpp"
#include "engine/application/windows/window.hpp"
#endif

namespace ze::platform
{

ApplicationModule::ApplicationModule()
{
#if ZE_PLATFORM(WINDOWS)
	application = std::make_unique<WindowsApplication>(GetModuleHandle(nullptr));
#endif
}

ZE_IMPLEMENT_MODULE(ApplicationModule, Application);

}