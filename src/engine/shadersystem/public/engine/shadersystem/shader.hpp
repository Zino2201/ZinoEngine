#pragma once

#include "shader_declaration.hpp"
#include <bitset>
#include "engine/jobsystem/job.hpp"
#include "shader_permutation_id.hpp"
#include "glm/vec2.hpp"

namespace ze::shadersystem
{

ZE_DEFINE_LOG_CATEGORY(shadersystem);

class Shader;
class ShaderInstance;
class ShaderManager;

using ShaderMap = robin_hood::unordered_map<gfx::ShaderStageFlagBits, gfx::UniqueShader>;

enum class ShaderPermutationState
{
	/** Not available */
	Unavailable,

	Compiling,

	/** Ready to be used */
	Available,
};

/**
 * A single shader permutation
 */
class ShaderPermutation
{
public:
	struct ParameterInfo
	{
		size_t offset;
		size_t size;
		bool is_uav;
	};

	ShaderPermutation(Shader& in_shader, ShaderPermutationPassIdPair in_id);

	ShaderPermutation(const ShaderPermutation&) = delete;
	ShaderPermutation& operator=(const ShaderPermutation&) = delete;

	/** Don't allow move as this may invalidate some this ptrs in lambdas for compiling */
	ShaderPermutation(ShaderPermutation&&) = delete;
	ShaderPermutation& operator=(ShaderPermutation&&) = delete;

	void compile();

	/** Get shader map (BLOCKING !) */
	[[nodiscard]] const ShaderMap& get_shader_map()
	{
		while(root_compilation_job->is_running()) {}
		return shader_map;
	}

	const ParameterInfo* get_parameter_info(const std::string& in_name) const
	{
		auto it = parameter_infos.find(in_name);
		if (it != parameter_infos.end())
			return &it->second;

		return nullptr;
	}

	Shader& get_shader() const { return shader; }
	ShaderPermutationState get_state() const { return state; }
	gfx::PipelineLayoutHandle get_pipeline_layout() const { return pipeline_layout.get(); }
	bool is_compiling() const { return state == ShaderPermutationState::Compiling; }
	bool is_available() const { return state == ShaderPermutationState::Available; }
	auto get_shader_stage_flags() const { return shader_stage_flags; }
	auto get_parameters_size() const { return parameters_size; }
private:
	Shader& shader;
	ShaderPermutationPassIdPair pass_id_pair;
	std::atomic<ShaderPermutationState> state;
	gfx::UniquePipelineLayout pipeline_layout;
	ShaderMap shader_map;
	jobsystem::Job* root_compilation_job;
	robin_hood::unordered_map<std::string, ParameterInfo> parameter_infos;
	gfx::ShaderStageFlags shader_stage_flags;
	size_t parameters_size;
};

enum class ShaderOptionType
{
	Bool,
	Int
};

struct ShaderOption
{
	std::string name;
	ShaderOptionType type;
	uint32_t count;
	size_t bit_width;

	/** Index of the option in permutation id */
	size_t id_index;

	ShaderOption(const std::string& in_name,
		const ShaderOptionType in_type,
		const int32_t in_count = -1)
			: name(in_name), type(in_type), bit_width(0), id_index(0)
	{
		if (type == ShaderOptionType::Int)
			count = in_count;
		else
			count = 1;
	}
};

/*
 * Utility class to build permutation ids from options
 */
class ShaderPermutationBuilder
{
public:
	ShaderPermutationBuilder(Shader& in_shader);

	void add_option(std::string in_name, int32_t value);

	[[nodiscard]] ShaderPermutationId get_id() const { return id; }
private:
	Shader& shader;
	ShaderPermutationId id;
};

/**
 * A shader that can be instancied later on to a ShaderInstance
 */
class Shader
{
public:
	Shader(ShaderManager& in_shader_manager, const ShaderDeclaration& in_declaration);

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	[[nodiscard]] std::unique_ptr<ShaderInstance> instantiate(ShaderPermutationPassIdPair in_id);

	[[nodiscard]] ShaderManager& get_shader_manager() { return shader_manager; }
	[[nodiscard]] const ShaderManager& get_shader_manager() const { return shader_manager; }
	[[nodiscard]] const auto& get_declaration() const { return declaration; }
	[[nodiscard]] const auto& get_options() const { return options; }

	[[nodiscard]] ShaderPermutation* get_permutation(const ShaderPermutationPassIdPair in_id);
private:
	ShaderManager& shader_manager;
	ShaderDeclaration declaration;
	size_t total_permutation_count;
	std::vector<ShaderOption> options;
	robin_hood::unordered_map<std::string, size_t> name_to_option_idx;
	robin_hood::unordered_map<ShaderPermutationPassIdPair, std::unique_ptr<ShaderPermutation>> permutations;
	std::mutex permutations_lock;
};

/**
 * A shader instance is a instance with its own resources of a Shader permutation
 */
class ShaderInstance
{
public:
	ShaderInstance(ShaderPermutation& in_permutation);

	void bind(gfx::CommandListHandle handle);

	bool set_parameter(const std::string& in_name, gfx::BufferHandle in_buffer);
	bool set_parameter(const std::string& in_name, gfx::TextureViewHandle in_buffer);
	bool set_parameter(const std::string& in_name, gfx::SamplerHandle in_buffer);
	bool set_parameter_uav(const std::string& in_name, const std::span<gfx::TextureViewHandle>& in_textures);
	bool set_parameter_uav(const std::string& in_name, const std::span<gfx::UniqueTextureView>& in_textures);

	template<typename T>
		requires std::is_standard_layout_v<T>
	bool set_parameter(const std::string& in_name, T in_value)
	{
		if (const auto* parameter_info = permutation.get_parameter_info(in_name))
		{
			memcpy(push_constant_data.data() + parameter_info->offset, &in_value, sizeof(T));
			return true;
		}

		return false;
	}

	bool set_parameter(const std::string& in_name, glm::vec2 in_value)
	{
		if (const auto* parameter_info = permutation.get_parameter_info(in_name))
		{
			memcpy(push_constant_data.data() + parameter_info->offset, &in_value, sizeof(glm::ivec2));
			return true;
		}

		return false;
	}

	ShaderPermutation& get_permutation() { return permutation; }
private:
	bool build_parameters_cache();
private:
	ShaderPermutation& permutation;
	std::array<uint8_t, gfx::max_push_constant_size> push_constant_data;
};

}
