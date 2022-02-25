#include "engine/shadersystem/shader_manager.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/shadersystem/shader.hpp"
#include "engine/filesystem/filesystem_module.hpp"
#include "zeshader_compiler.hpp"

namespace ze::shadersystem
{

ShaderManager::ShaderManager(gfx::Device& in_device) : device(in_device) {}
ShaderManager::~ShaderManager() = default;

void ShaderManager::add_shader_directory(const std::string& in_name)
{
	logger::info(log_shadersystem, "Added shader search directory: \"{}\"", in_name);
	scan_directory(in_name);
	std::scoped_lock lock(shader_directories_mutex);
	shader_directories.emplace_back(in_name);
}

void ShaderManager::scan_directory(const std::string& in_directory)
{
	auto& filesystem = get_module<filesystem::Module>("FileSystem")->get_filesystem();
	if (!filesystem.iterate_directory(in_directory,
		[&](const std::filesystem::path& in_path)
		{
			if (in_path.extension() == ".zeshader")
				build_shader(in_directory / in_path);
		},
		filesystem::IterateDirectoryFlagBits::Recursive))
	{
		logger::error("Failed to scan directory {} for shaders", in_directory);
	}
}

void ShaderManager::build_shader(const std::filesystem::path& in_path)
{
	auto& filesystem = get_module<filesystem::Module>("FileSystem")->get_filesystem();

	auto file = filesystem.read(in_path);
	if (file)
	{
		auto result = compile_zeshader(std::move(file.get_value()));
		if(result)
			register_shader(result.get_value());
		else
			logger::error("Failed to parse shader {}: {}", in_path.string(), result.get_error());
	}
	else
	{
		logger::error("Failed to build shader {}: {}", in_path.string(), std::to_string(file.get_error()));
	}
}

void ShaderManager::register_shader(const ShaderDeclaration& in_declaration)
{
	std::scoped_lock lock(shader_map_mutex);
	shader_map.insert({ in_declaration.name, std::make_unique<Shader>(*this, in_declaration) });
	logger::info(log_shadersystem, "Registered shader {}", in_declaration.name);
}

Shader* ShaderManager::get_shader(const std::string_view& in_name)
{
	/**
	 * First, attempt to get the shader from our shader map
	 * If it's not found we try to search it in our search directories
	 * If it cannot be found anywhere then it is a Unknown shader
	 */
	if (Shader* shader = get_shader_from_shader_map(in_name))
		return shader;

	return nullptr;
}

Shader* ShaderManager::get_shader_from_shader_map(const std::string_view& in_name)
{
	std::scoped_lock lock(shader_map_mutex);
	auto it = shader_map.find(std::string(in_name));
	if (it != shader_map.end())
		return it->second.get();

	return nullptr;
}

}