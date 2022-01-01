#include "engine/shadersystem/shader.hpp"

namespace ze::shadersystem
{

Shader::Shader(ShaderManager& in_shader_manager, const ShaderDeclaration& in_declaration)
	: shader_manager(in_shader_manager), declaration(in_declaration), total_permutation_count(1)
{
	size_t required_bits = 0;
	size_t idx = 0;
	size_t id_idx = 0;

	/** Calculate permutation count */
	for (auto& option : options)
	{
		if (option.type == ShaderOptionType::Bool)
			total_permutation_count *= 2;
		else
			total_permutation_count *= option.count;

		option.bit_width = std::bit_width(option.count);
		option.id_index = id_idx;
		id_idx += option.bit_width;
		name_to_option_idx.insert({ option.name, idx++ });
		required_bits += option.bit_width;
	}

	ZE_CHECKF(required_bits < permutation_bit_count, "Shader has too many options !");
}

std::unique_ptr<ShaderInstance> Shader::instantiate(ShaderPermutationId in_id)
{
	if (ShaderPermutation* permutation = get_permutation(in_id))
		return std::make_unique<ShaderInstance>(*this, *permutation);

	return nullptr;
}

ShaderPermutation* Shader::get_permutation(const ShaderPermutationId in_id)
{
	const auto it = permutations.find(in_id);
	if (it != permutations.end())
		return it->second.get();

	auto [new_it, unused] = permutations.insert({in_id, std::make_unique<ShaderPermutation>(*this, in_id)});
	return new_it->second.get();
}

}