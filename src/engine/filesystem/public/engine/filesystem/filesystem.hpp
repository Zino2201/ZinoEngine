#pragma once

#include <mutex>
#include "engine/core.hpp"
#include "engine/flags.hpp"
#include "engine/result.hpp"
#include "mount_point.hpp"

namespace ze::filesystem
{

class MountPoint;

class FileSystem
{
public:
	FileSystem();

	/**
	 * Thread-safe way to mount a new mount point to the filesystem
	 */
	template<typename T>
		requires std::derived_from<T, MountPoint>
	T* mount(std::unique_ptr<T>&& in_mount_point)
	{
		std::scoped_lock lock(mount_point_mutex);
		mount_points.emplace_back(std::move(in_mount_point));
		sort_mount_points();
		return static_cast<T*>(mount_points.back().get());
	}
	
	[[nodiscard]] Result<std::unique_ptr<std::streambuf>, FileSystemError> read(const std::filesystem::path& in_path,
		FileReadFlags in_flags = FileReadFlags());

	[[nodiscard]] Result<std::unique_ptr<std::streambuf>, FileSystemError> write(const std::filesystem::path& in_path, 
		FileWriteFlags in_flags = FileWriteFlags());

	[[nodiscard]] bool iterate_directory(const std::filesystem::path& in_path,
		std::function<void(const std::filesystem::path&)> in_function,
		IterateDirectoryFlags in_flags = IterateDirectoryFlags());
private:
	void sort_mount_points();
	[[nodiscard]] MountPoint* get_matching_mount_point_from_path(const std::filesystem::path& in_path);
private:
	std::vector<std::unique_ptr<MountPoint>> mount_points;
	std::mutex mount_point_mutex;
	MountPoint* write_mount_point;
};

}