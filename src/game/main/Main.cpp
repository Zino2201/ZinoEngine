#include "engine/gfx/device.hpp"
#include "engine/logger/logger.hpp"
#include "engine/logger/sinks/stdout_sink.hpp"
#include "engine/module/module_manager.hpp"
#include "boost/locale/generator.hpp"
#include "engine/filesystem/filesystem_module.hpp"
#include "engine/filesystem/filesystem.hpp"
#include "engine/filesystem/std_mount_point.hpp"
#include "engine/hal/thread.hpp"
#include "engine/jobsystem/jobsystem.hpp"
#include "engine/engine.hpp"

int main()
{
	using namespace ze;

	logger::set_pattern("[{time}] [{severity}/{thread}] ({category}) {message}");
	logger::add_sink(std::make_unique<logger::StdoutSink>());

	const boost::locale::generator generator;
	const std::locale locale = generator.generate("");
	std::locale::global(locale);

	auto& filesystem = get_module<filesystem::Module>("FileSystem")->get_filesystem();
	filesystem.mount(std::make_unique<filesystem::StdMountPoint>(std::filesystem::current_path(), "main"));

	hal::set_thread_name(std::this_thread::get_id(), "Main Thread");

	jobsystem::initialize();

	{
		Engine engine;
		engine.run();
	}

	jobsystem::shutdown();
	unload_all_modules();

	return 0;
}
