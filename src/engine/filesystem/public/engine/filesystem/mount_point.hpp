#pragma once

#include "engine/core.hpp"
#include "engine/flags.hpp"
#include "engine/result.hpp"
#include <filesystem>

namespace ze::filesystem
{

enum class FileReadFlagBits
{
	Binary = 1 << 0,
};
ZE_ENABLE_FLAG_ENUMS(FileReadFlagBits, FileReadFlags);

enum class FileWriteFlagBits
{
	ReplaceExisting = 1 << 0,
	Binary = 1 << 1,
};
ZE_ENABLE_FLAG_ENUMS(FileWriteFlagBits, FileWriteFlags);

enum class IterateDirectoryFlagBits
{
	Recursive = 1 << 0,
};
ZE_ENABLE_FLAG_ENUMS(IterateDirectoryFlagBits, IterateDirectoryFlags);

enum class FileSystemError
{
	Unknown,
	NotFound,
	AlreadyExists,
	NoWriteFilesystem
};

class MountPoint
{
public:
	MountPoint(const std::string& in_id, uint8_t in_priority = 0) : id(in_id),
		priority(in_priority) {}
	virtual ~MountPoint() = default;

	[[nodiscard]] virtual Result<std::unique_ptr<std::streambuf>, FileSystemError> read(const std::filesystem::path& in_path,
		FileReadFlags in_flags = FileReadFlags()) = 0;

	[[nodiscard]] virtual Result<std::unique_ptr<std::streambuf>, FileSystemError> write(const std::filesystem::path& in_path,
		FileWriteFlags in_flags = FileWriteFlags()) = 0;

	[[nodiscard]] virtual bool iterate_directory(const std::filesystem::path& in_path,
		std::function<void(const std::filesystem::path&)> in_function,
		IterateDirectoryFlags in_flags = IterateDirectoryFlags()) = 0;

	[[nodiscard]] virtual bool exists(const std::filesystem::path& in_path) = 0;

	[[nodiscard]] const auto& get_id() const { return id;  }
	[[nodiscard]] const auto& get_priority() const { return priority;  }
private:
	std::string id;
	uint8_t priority;
};

}

namespace std
{

inline std::string to_string(ze::filesystem::FileSystemError in_filesystem)
{
	switch(in_filesystem)
	{
	default:
		return "Unknown";
	case ze::filesystem::FileSystemError::NotFound:
		return "File or directory not found";
	case ze::filesystem::FileSystemError::AlreadyExists:
		return "File or directory already exists";
	case ze::filesystem::FileSystemError::NoWriteFilesystem:
		return "No write filesystem available";
	}
}

}