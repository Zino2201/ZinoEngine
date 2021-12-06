#pragma once

#include <cstdint>
#include <string>

namespace ze::logger
{

enum class SeverityFlagBits : uint8_t;
struct Message;

/**
 * Base class for logger sinks, that process messages coming from the logger
 */
class Sink
{
public:
	virtual ~Sink() = default;

	virtual void set_pattern(const std::string& in_pattern) = 0;
	virtual void log(const Message& in_message) = 0;
};

}