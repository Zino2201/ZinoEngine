#include "engine/shadersystem/shader.hpp"

namespace ze::shadersystem
{

ShaderInstance::ShaderInstance(Shader& in_shader, ShaderPermutation& in_permutation)
	: shader(in_shader), permutation(in_permutation)
{
	if(permutation.get_state() != ShaderPermutationState::Available)
		permutation.compile();
}

}