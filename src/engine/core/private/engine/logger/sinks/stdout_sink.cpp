#include "engine/logger/sinks/stdout_sink.hpp"
#include <iostream>
#include "engine/logger/Logger.hpp"
#if ZE_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace ze::logger
{

void StdoutSink::log(const Message& in_message)
{
#if ZE_PLATFORM(WINDOWS)
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	switch(in_message.severity)
	{
	default:
	case SeverityFlagBits::Info:
		SetConsoleTextAttribute(handle, 7);
		break;
	case SeverityFlagBits::Verbose:
		SetConsoleTextAttribute(handle, 11);
		break;
	case SeverityFlagBits::Warn:
		SetConsoleTextAttribute(handle, 14);
		break;
	case SeverityFlagBits::Error:
		SetConsoleTextAttribute(handle, 12);
		break;
	case SeverityFlagBits::Fatal:
		SetConsoleTextAttribute(handle, 64);
		break;
	}

#endif
	std::cout << detail::format_message(pattern, in_message) << std::endl;
#if ZE_PLATFORM(WINDOWS)
	SetConsoleTextAttribute(handle, 7);
#endif
}

}
