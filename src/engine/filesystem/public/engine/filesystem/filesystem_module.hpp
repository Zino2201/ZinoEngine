#pragma once

#include "engine/module/module.hpp"
#include "engine/filesystem/filesystem.hpp"

namespace ze::filesystem
{

class Module : public ze::Module
{
public:
	[[nodiscard]] auto& get_filesystem() { return filesystem;  }
private:
	FileSystem filesystem;
};

}