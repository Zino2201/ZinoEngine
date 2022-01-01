#pragma once

#include "mount_point.hpp"

namespace ze::filesystem
{

class StdMountPoint : public MountPoint
{
public:
	StdMountPoint(const std::filesystem::path& in_root, const std::string& in_id, uint8_t in_priority = 0)
		: MountPoint(in_id, in_priority), root(in_root) {}

	Result<std::unique_ptr<std::streambuf>, FileSystemError> read(const std::filesystem::path& in_path, FileReadFlags in_flags) override;
	Result<std::unique_ptr<std::streambuf>, FileSystemError> write(const std::filesystem::path& in_path, FileWriteFlags in_flags) override;
	bool iterate_directory(const std::filesystem::path& in_path, 
		std::function<void(const std::filesystem::path&)> in_function,
		IterateDirectoryFlags in_flags) override;
	bool exists(const std::filesystem::path& in_path) override;
private:
	[[nodiscard]] std::filesystem::path correct_path(const std::filesystem::path& in_path) const;
	static FileSystemError convert_errno(const errno_t in_errno);
private:
	std::filesystem::path root;
};

}