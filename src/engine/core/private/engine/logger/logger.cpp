#include "engine/logger/logger.hpp"
#include "engine/platform_macros.hpp"
#include "engine/logger/sink.hpp"
#include <chrono>
#include <thread>
#include <iomanip>
#include <mutex>
#include <fmt/format-inl.h>
#include "engine/hal/thread.hpp"
#if ZE_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace ze::logger
{

std::mutex log_mutex;

namespace detail
{
std::string severity_to_string(SeverityFlagBits in_severity)
{
	switch(in_severity)
	{
	default:
	case SeverityFlagBits::Verbose:
		return "verbose";
	case SeverityFlagBits::Info:
		return "info";
	case SeverityFlagBits::Warn:
		return "warn";
	case SeverityFlagBits::Error:
		return "error";
	case SeverityFlagBits::Fatal:
		return "fatal";
	}
}

std::string format_message(const std::string& in_pattern, const Message& in_message)
{
	const std::time_t time = std::chrono::system_clock::to_time_t(in_message.time);

#if ZE_PLATFORM(WINDOWS)
	std::tm localtime = {};
	::localtime_s(&localtime, &time);
#else
	const std::tm* localtime_ptr = ::localtime(&time);
	std::tm localtime = *localtime_ptr;
#endif

	char time_str[128];
	std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &localtime);

	return fmt::format(fmt::runtime(in_pattern), fmt::arg("time", time_str),
		fmt::arg("severity", severity_to_string(in_message.severity)),
		fmt::arg("thread", hal::get_thread_name(in_message.thread)),
		fmt::arg("category", in_message.category.name),
		fmt::arg("message", in_message.message));
}

}

std::vector<std::unique_ptr<Sink>> sinks;
std::string pattern;

void add_sink(std::unique_ptr<Sink>&& in_sink)
{
	in_sink->set_pattern(pattern);
	sinks.emplace_back(std::move(in_sink));
}

void set_pattern(const std::string& in_pattern)
{
	pattern = in_pattern;
	for(const auto& sink : sinks)
		sink->set_pattern(pattern);
}

void log(SeverityFlagBits in_severity, const Category& in_category, const std::string& in_message)
{
	Message message;
	message.time = std::chrono::system_clock::now();
	message.thread = std::this_thread::get_id();
	message.severity = in_severity;
	message.category = in_category;
	message.message = in_message;

	std::scoped_lock lock(log_mutex);
	for(const auto& sink : sinks)
	{
		sink->log(message);
		if(in_severity == SeverityFlagBits::Fatal)
		{
			MessageBoxA(nullptr, in_message.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
			std::terminate();
		}
	}
}
	
}