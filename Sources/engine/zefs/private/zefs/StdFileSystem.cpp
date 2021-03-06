#include "zefs/StdFileSystem.h"
#include <fstream>
#if ZE_PLATFORM(WINDOWS)
#include <Windows.h>
#endif

namespace ze::filesystem
{

StdFileSystem::StdFileSystem(const std::string& in_alias,
	const uint8_t& in_priority, const std::string& in_root) : FileSystem(in_alias,
		in_priority), root(in_root) {}

OwnerPtr<std::streambuf> StdFileSystem::read(const std::filesystem::path& path, const FileReadFlags& flags)
{
	std::filesystem::path correct_path = get_correct_path(path);
	std::filebuf* file = new std::filebuf;
	
	uint32_t rflags = std::ios::in;
	if (flags & FileReadFlagBits::Binary)
		rflags |= std::ios::binary;
	
	if (flags & FileReadFlagBits::End)
		rflags |= std::ios::ate;
	
	file->open(correct_path, rflags);
	if (!file->is_open())
	{
		ze::logger::error("Failed to open file {}: {}",
			path.string(),
			strerror(errno));
		delete file;
		file = nullptr;
	}

	return file;
}

OwnerPtr<std::streambuf> StdFileSystem::write(const std::filesystem::path& path, const FileWriteFlags& flags)
{
	std::filesystem::path correct_path = get_correct_path(path);

	OwnerPtr<std::filebuf> file = new std::filebuf;

	uint32_t wflags = std::ios::out;
	if (flags & FileWriteFlagBits::Binary)
		wflags |= std::ios::binary;

	/** On hidden files for some reason it fails.. */
	FileAttributeFlags attribs = get_file_attributes(path);
	if(exists(path) && (attribs & FileAttributeFlagBits::Hidden))
	{
		attribs &= ~FileAttributeFlags(FileAttributeFlagBits::Hidden);
		set_file_attributes(path, attribs);
		file->open(correct_path, wflags);
		attribs |= FileAttributeFlagBits::Hidden;
		set_file_attributes(path, attribs);
	}
	else
	{
		file->open(correct_path, wflags);	
	}
	
	if (!file->is_open())
	{
		ze::logger::error("Failed to open file {}: {}",
			path.string(),
			strerror(errno));
		delete file;
		file = nullptr;
	}

	return file;
}

bool StdFileSystem::iterate_directories(const std::filesystem::path& path,
	const DirectoryIterator& iterator, const IterateDirectoriesFlags& flags)
{
	std::filesystem::path correct_path = get_correct_path(path);
	if (!iterator)
		return false;

	if (flags & IterateDirectoriesFlagBits::Recursive)
	{
		for (auto& entry : std::filesystem::recursive_directory_iterator(correct_path))
		{
			iterator.execute(DirectoryEntry(std::filesystem::relative(entry.path(), correct_path)));
		}
	}
	else
	{
		for (auto& Entry : std::filesystem::directory_iterator(correct_path))
		{
			iterator.execute(DirectoryEntry(std::filesystem::relative(Entry.path(), correct_path)));
		}
	}

	return true;
}

bool StdFileSystem::exists(const std::filesystem::path& path)
{
	return std::filesystem::exists(get_correct_path(path));
}

bool StdFileSystem::is_directory(const std::filesystem::path& path)
{
	return std::filesystem::is_directory(get_correct_path(path));
}

std::filesystem::path StdFileSystem::get_correct_path(const std::filesystem::path& path) const
{
	std::filesystem::path correct_path = path;
	if (correct_path.is_relative())
		correct_path = root / path;

	return correct_path;
}

FileAttributeFlags StdFileSystem::get_file_attributes(const std::filesystem::path& path) const
{
	std::filesystem::path correct_path = get_correct_path(path);
	FileAttributeFlags ze_flags;

#if ZE_PLATFORM(WINDOWS)
	DWORD win_flags = GetFileAttributesA(correct_path.string().c_str());
	if(win_flags & FILE_ATTRIBUTE_HIDDEN)
		ze_flags |= FileAttributeFlagBits::Hidden;
#endif

	return ze_flags;
}

bool StdFileSystem::set_file_attributes(const std::filesystem::path& path, const FileAttributeFlags& in_flags)
{
	std::filesystem::path correct_path = get_correct_path(path);

#if ZE_PLATFORM(WINDOWS)
	DWORD win_flags = 0;
	if (in_flags& FileAttributeFlagBits::Hidden)
		win_flags |= FILE_ATTRIBUTE_HIDDEN;
	return SetFileAttributesA(correct_path.string().c_str(), win_flags);
#endif
}

}