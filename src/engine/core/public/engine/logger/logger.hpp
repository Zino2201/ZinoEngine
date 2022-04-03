#pragma once

#include <memory>
#include <chrono>
#include <thread>
#include <string_view>
#include <string>
#include <fmt/format.h>
#include "engine/platform_macros.hpp"

namespace ze::logger
{

class Sink;

enum class SeverityFlagBits : uint8_t
{
	Verbose,
	Info,
	Warn,
	Error,
	Fatal
};

struct Category
{
	std::string_view name;

	Category() = default;
	constexpr Category(const std::string_view& in_name) : name(in_name) {}
};

struct Message
{
	std::chrono::system_clock::time_point time;
	std::thread::id thread;
	SeverityFlagBits severity;
	Category category;
	std::string message;

	Message() : severity(SeverityFlagBits::Info) {}
};

void log(SeverityFlagBits in_severity, const Category& in_category, const std::string& in_message);
void add_sink(std::unique_ptr<Sink>&& in_sink);
void set_pattern(const std::string& in_pattern);

namespace detail
{

std::string_view severity_to_string(SeverityFlagBits in_severity);
std::string format_message(const std::string& in_pattern, const Message& in_message);

}

#define ZE_DEFINE_LOG_CATEGORY(Name) constexpr ze::logger::Category log_##Name(#Name);

ZE_DEFINE_LOG_CATEGORY(unknown);

template<typename... Args>
void logf(SeverityFlagBits in_severity, const Category& in_category,
	const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	log(in_severity, in_category, fmt::format(in_format, std::forward<Args>(in_args)...));
}

#if ZE_BUILD(IS_DEBUG)
template<typename... Args>
void verbose(const Category& in_category, const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Verbose, in_category, in_format, std::forward<Args>(in_args)...);
}
#else
template<typename... Args>
void verbose(const Category&, const fmt::format_string<Args...>&, Args&&...) {}
#endif

template<typename... Args>
void verbose(const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	verbose(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void info(const Category& in_category, const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Info, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void info(const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	info(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void warn(const Category& in_category, const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Warn, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void warn(const std::string_view& in_format, Args&&... in_args)
{
	warn(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void error(const Category& in_category, const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Error, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void error(const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	error(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void fatal(const Category& in_category, const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Fatal, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void fatal(const fmt::format_string<Args...>& in_format, Args&&... in_args)
{
	fatal(log_unknown, in_format, std::forward<Args>(in_args)...);
}

}
