#pragma once

#include "shader_declaration.hpp"
#include <bitset>
#include "engine/jobsystem/job.hpp"
#include "shader_permutation_id.hpp"

namespace ze::shadersystem
{

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
		uint32_t set;
		uint32_t binding;
		size_t offset;
	};

	struct ResourceInfo
	{
		gfx::DescriptorType type;
		size_t binding;
		size_t size;
	};

	ShaderPermutation(Shader& in_shader, ShaderPermutationId in_id);

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
	const auto& get_sets() const { return sets; }
private:
	Shader& shader;
	ShaderPermutationId id;
	std::atomic<ShaderPermutationState> state;
	gfx::UniquePipelineLayout pipeline_layout;
	ShaderMap shader_map;
	jobsystem::Job* root_compilation_job;
	std::vector<std::vector<ResourceInfo>> sets;
	robin_hood::unordered_map<std::string, ParameterInfo> parameter_infos;
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
	ShaderPermutationBuilder(Shader& in_shader, const std::string_view& in_pass = "");

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

	[[nodiscard]] std::unique_ptr<ShaderInstance> instantiate(ShaderPermutationId in_id);

	[[nodiscard]] ShaderManager& get_shader_manager() { return shader_manager; }
	[[nodiscard]] const ShaderManager& get_shader_manager() const { return shader_manager; }
	[[nodiscard]] const auto& get_declaration() const { return declaration; }
	[[nodiscard]] const auto& get_options() const { return options; }

	[[nodiscard]] ShaderPermutation* get_permutation(const ShaderPermutationId in_id);
private:
	ShaderManager& shader_manager;
	ShaderDeclaration declaration;
	size_t total_permutation_count;
	std::vector<ShaderOption> options;
	robin_hood::unordered_map<std::string, size_t> name_to_option_idx;
	robin_hood::unordered_map<ShaderPermutationId, std::unique_ptr<ShaderPermutation>> permutations;
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

	bool set_uint32_parameter(const std::string& in_name, uint32_t data)
	{
		if (!build_parameters_cache())
			return false;

		if(const auto* parameter_info = permutation.get_parameter_info(in_name))
		{
			for(const auto& ubo : ubos)
			{
				if(ubo.set == parameter_info->set &&
					ubo.binding == parameter_info->binding)
				{
					memcpy(static_cast<uint8_t*>(ubo.mapped_data) + parameter_info->offset, &data, sizeof(uint32_t));
				}
			}
		}

		return false;
	}

	bool set_ubo_parameter(const std::string& in_name, gfx::BufferHandle in_buffer);
	bool set_ssbo_parameter(const std::string& in_name, gfx::BufferHandle in_buffer);

	ShaderPermutation& get_permutation() { return permutation; }
private:
	bool build_parameters_cache();
private:
	ShaderPermutation& permutation;
	bool parameter_cache_built;

	struct BufferParameter
	{
		size_t set;
		size_t binding;
		std::variant<gfx::UniqueBuffer, gfx::BufferHandle> buffer;
		void* mapped_data;

		BufferParameter() : set(0), binding(0), mapped_data(nullptr) {}
		BufferParameter(const size_t in_set, const size_t in_binding, gfx::UniqueBuffer&& in_buffer, void* in_mapped_data)
			: set(in_set), binding(in_binding), buffer(std::move(in_buffer)), mapped_data(in_mapped_data) {}
		BufferParameter(const size_t in_set, const size_t in_binding, gfx::BufferHandle in_buffer, void* in_mapped_data)
			: set(in_set), binding(in_binding), buffer(in_buffer), mapped_data(in_mapped_data) {}

		BufferParameter(const BufferParameter&) = delete;
		BufferParameter& operator=(const BufferParameter&) = delete;

		BufferParameter(BufferParameter&&) = default;
		BufferParameter& operator=(BufferParameter&&) = default;

		~BufferParameter()
		{
			if(mapped_data)
			{
				gfx::get_device()->unmap_buffer(std::get<gfx::UniqueBuffer>(buffer).get());
			}
		}
	};

	std::vector<BufferParameter> ubos;
	std::vector<BufferParameter> ssbos;
};

}