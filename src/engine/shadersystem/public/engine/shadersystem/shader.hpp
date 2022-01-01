#pragma once

#include "shader_declaration.hpp"
#include <bitset>
#include "tbb/flow_graph.h"

namespace ze::shadersystem
{

class Shader;
class ShaderInstance;
class ShaderManager;

static constexpr size_t permutation_bit_count = 32;

using ShaderPermutationId = std::bitset<permutation_bit_count>;
using ShaderMap = robin_hood::unordered_map<gfx::ShaderStageFlagBits, gfx::UniqueShader>;

enum class ShaderPermutationState
{
	/** May be compiling or has not yet been requested */
	Unavailable,

	/** Ready to be used */
	Available,
};

/**
 * A single shader permutation
 */
class ShaderPermutation
{
public:
	ShaderPermutation(Shader& in_shader, ShaderPermutationId in_id);

	ShaderPermutation(const ShaderPermutation&) = delete;
	ShaderPermutation& operator=(const ShaderPermutation&) = delete;

	void compile();

	/** Get shader map (BLOCKING !) */
	[[nodiscard]] const ShaderMap& get_shader_map()
	{
		compilation_task_graph.wait_for_all();
		return shader_map;
	}

	[[nodiscard]] ShaderPermutationState get_state() const { return state; }
	[[nodiscard]] gfx::PipelineLayoutHandle get_pipeline_layout() const { return pipeline_layout.get(); }
private:
	Shader& shader;
	ShaderPermutationId id;
	std::atomic<ShaderPermutationState> state;
	gfx::UniquePipelineLayout pipeline_layout;
	ShaderMap shader_map;
	tbb::flow::graph compilation_task_graph;
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
	[[nodiscard]] const auto& get_declaration() const { return declaration; }
private:
	[[nodiscard]] ShaderPermutation* get_permutation(const ShaderPermutationId in_id);
private:
	ShaderManager& shader_manager;
	ShaderDeclaration declaration;
	size_t total_permutation_count;
	std::vector<ShaderOption> options;
	robin_hood::unordered_map<std::string, size_t> name_to_option_idx;
	robin_hood::unordered_map<ShaderPermutationId, std::unique_ptr<ShaderPermutation>> permutations;
};

/**
 * A shader instance is a instance with its own resources of a Shader permutation
 */
class ShaderInstance
{
public:
	ShaderInstance(Shader& in_shader, ShaderPermutation& in_permutation);

	ShaderPermutation& get_permutation() { return permutation; }
private:
	Shader& shader;
	ShaderPermutation& permutation;
};

}
