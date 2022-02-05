#pragma once

namespace ze::shadersystem
{

static constexpr size_t permutation_bit_count = 32;

struct ShaderPermutationId
{
	std::string_view pass;
	std::bitset<permutation_bit_count> options;

	bool operator==(const ShaderPermutationId& in_id) const
	{
		return pass == in_id.pass && 
			options == in_id.options;
	}
};

}

namespace std
{

template<> struct hash<ze::shadersystem::ShaderPermutationId>
{
	uint64_t operator()(const ze::shadersystem::ShaderPermutationId& in_id) const noexcept
	{
		uint64_t hash = std::hash<std::string_view>()(in_id.pass);
		ze::hash_combine(hash, in_id.options);
		return hash;
	}
};

}