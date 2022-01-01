#include "engine/filesystem/filesystem.hpp"

namespace ze::filesystem
{

FileSystem::FileSystem()
	: write_mount_point(nullptr) {}

Result<std::unique_ptr<std::streambuf>, FileSystemError> FileSystem::read(const std::filesystem::path& in_path, FileReadFlags in_flags)
{
	if(MountPoint* mount_point = get_matching_mount_point_from_path(in_path))
		return mount_point->read(in_path, in_flags);

	return make_error(FileSystemError::NotFound);
}

Result<std::unique_ptr<std::streambuf>, FileSystemError> FileSystem::write(const std::filesystem::path& in_path, FileWriteFlags in_flags)
{
	if (write_mount_point)
		return write_mount_point->write(in_path, in_flags);

	return make_error(FileSystemError::NoWriteFilesystem);
}

bool FileSystem::iterate_directory(const std::filesystem::path& in_path,
	std::function<void(const std::filesystem::path&)> in_function,
	IterateDirectoryFlags in_flags)
{
	if (MountPoint* mount_point = get_matching_mount_point_from_path(in_path))
		return mount_point->iterate_directory(in_path, in_function, in_flags);

	return false;
}

MountPoint* FileSystem::get_matching_mount_point_from_path(const std::filesystem::path& in_path)
{
	std::scoped_lock lock(mount_point_mutex);
	for (const auto& mount_point : mount_points)
		if (mount_point->exists(in_path))
			return mount_point.get();

	return nullptr;
}

void FileSystem::sort_mount_points()
{
	std::ranges::sort(mount_points.begin(), mount_points.end(),
		[](const auto& left, const auto& right)
		{
			return left->get_priority() > right->get_priority();
		});
}

}