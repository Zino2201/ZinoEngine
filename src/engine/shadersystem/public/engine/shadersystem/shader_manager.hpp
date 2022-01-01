#pragma once

#include "engine/result.hpp"
#include "shader.hpp"
#include "engine/gfx/shader_format.hpp"
#include <filesystem>

namespace ze::shadersystem
{

class ShaderManager
{
public:
	ShaderManager(gfx::Device& in_device);
	~ShaderManager();

	/**
	 * Change the shader format
	 * \warn This will NOT recompile all shaders, it MUST be set once before any shader loaded!
	 */
	[[deprecated("TODO: Set directly from device")]] void set_shader_format(const gfx::ShaderFormat in_shader_format) { shader_format = in_shader_format; }

	/**
	 * [THREAD SAFE] Add a directory (not recursive!) containing potential valid shader files
	 * This will scan all shaders inside
	 */
	void add_shader_directory(const std::string& in_name);

	/**
	 * [THREAD SAFE] Request a shader
	 */
	[[nodiscard]] Shader* get_shader(const std::string_view& in_name);
	[[nodiscard]] gfx::ShaderFormat get_shader_format() const { return shader_format; }
	[[nodiscard]] gfx::Device& get_device() { return device; }
private:
	void scan_directory(const std::string& in_directory);
	void build_shader(const std::filesystem::path& in_path);
	void register_shader(const ShaderDeclaration& in_declaration);
	[[nodiscard]] Shader* get_shader_from_shader_map(const std::string_view& in_name);
private:
	gfx::Device& device;
	robin_hood::unordered_map<std::string, std::unique_ptr<Shader>> shader_map;
	std::vector<std::string> shader_directories;
	std::mutex shader_map_mutex;
	std::mutex shader_directories_mutex;
	gfx::ShaderFormat shader_format;
};


}
