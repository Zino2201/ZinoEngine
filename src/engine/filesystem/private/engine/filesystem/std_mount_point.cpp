#include "engine/filesystem/std_mount_point.hpp"
#include <fstream>

namespace ze::filesystem
{

Result<std::unique_ptr<std::streambuf>, FileSystemError> StdMountPoint::read(const std::filesystem::path& in_path, FileReadFlags in_flags)
{
	std::ios::openmode read_flags = std::ios::in;
	if (in_flags & FileReadFlagBits::Binary)
		read_flags |= std::ios::binary;

	auto sex = correct_path(in_path);

	auto file = std::make_unique<std::filebuf>();
	file->open(correct_path(in_path), read_flags);
	if(!file->is_open())
		return make_error(convert_errno(errno));

	return make_result(std::unique_ptr<std::streambuf>(file.release()));
}

Result<std::unique_ptr<std::streambuf>, FileSystemError> StdMountPoint::write(const std::filesystem::path& in_path, FileWriteFlags in_flags)
{
	std::ios::openmode read_flags = std::ios::out;
	if (in_flags & FileWriteFlagBits::Binary)
		read_flags |= std::ios::binary;

	auto file = std::make_unique<std::filebuf>();
	file->open(correct_path(in_path), read_flags);
	if (!file->is_open())
		return make_error(convert_errno(errno));

	return make_result(std::unique_ptr<std::streambuf>(file.release()));
}

bool StdMountPoint::iterate_directory(const std::filesystem::path& in_path, 
	std::function<void(const std::filesystem::path&)> in_function, 
	IterateDirectoryFlags in_flags)
{
	ZE_CHECK(in_function);
	if (!in_function)
		return false;

	const std::filesystem::path corrected_path = correct_path(in_path);

	if (in_flags & IterateDirectoryFlagBits::Recursive)
	{
		for (auto& entry : std::filesystem::recursive_directory_iterator(corrected_path))
		{
			in_function(std::filesystem::relative(entry.path(), corrected_path));
		}
	}
	else
	{
		for (auto& entry : std::filesystem::directory_iterator(corrected_path))
		{
			in_function(std::filesystem::relative(entry.path(), corrected_path));
		}
	}

	return true;
}

bool StdMountPoint::exists(const std::filesystem::path& in_path)
{
	return std::filesystem::exists(correct_path(in_path));
}

std::filesystem::path StdMountPoint::correct_path(const std::filesystem::path& in_path) const
{
	if(in_path.is_relative())
		return root / in_path;

	return root;
}

FileSystemError StdMountPoint::convert_errno(const errno_t in_errno)
{
	switch(in_errno)
	{
	case ENOENT:
		return FileSystemError::NotFound;
	case EEXIST:
		return FileSystemError::AlreadyExists;
	default:
		return FileSystemError::Unknown;
	}
}

}