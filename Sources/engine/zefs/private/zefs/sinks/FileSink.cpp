#include "EngineCore.h"
#include "zefs/sinks/FileSink.h"

namespace ze::filesystem
{

FileSink::FileSink(const std::string& in_name,
	const std::string& in_filename) : 
	Sink(in_name), stream(in_filename, FileWriteFlagBits::ReplaceExisting) {}

void FileSink::log(const logger::Message& message)
{
	std::string msg = format(message);

	stream << msg;
	stream.flush();
}

}