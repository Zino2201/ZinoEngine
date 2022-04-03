#pragma once

namespace ze::shadersystem
{

static constexpr size_t permutation_bit_count = 32;

using ShaderPermutationId = std::bitset<permutation_bit_count>;

struct ShaderPermutationPassIdPair
{
	std::string_view pass;
	ShaderPermutationId id;

	bool operator==(const ShaderPermutationPassIdPair& in_id) const
	{
		return pass == in_id.pass && 
			id == in_id.id;
	}
};

}

namespace std
{

template<> struct hash<ze::shadersystem::ShaderPermutationPassIdPair>
{
	uint64_t operator()(const ze::shadersystem::ShaderPermutationPassIdPair& in_id) const noexcept
	{
		uint64_t hash = std::hash<std::string_view>()(in_id.pass);
		ze::hash_combine(hash, in_id.pass);
		ze::hash_combine(hash, in_id.id);
		return hash;
	}
};

}